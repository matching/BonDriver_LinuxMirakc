/*
 * Connecting to Mirac
 * Copyright 2024 matching
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>

#include <algorithm>

#include "logoutput.hpp"

class MirakcConnectBase
{
protected:
	int s; // socket

	char buf[ 188 * 256 + 1 ];
	int bufsize;

public:
	MirakcConnectBase()
	{
	}
	virtual ~MirakcConnectBase()
	{
		if(s >= 0) {
			close(s);
		}
	}
	
	virtual int connect() = 0;

	int sendGetRequest_WaitBody( char *url, char *requestHeader, char *responceHeader, int *responceCode, char *body, int *bodysize );
	int sendGetRequest_WaitHeader( char *url, char *requestHeader, char *responceHeader, int *responceCode );

	int recvBody( char *body, size_t size );

	void disconnect();
	void shutdown();
};

class MirakcConnectHttp : public MirakcConnectBase
{
private:
#if 1
	struct sockaddr_in s_addr;
#else
	struct addrinfo hints;
	struct addrinfo *addr;
#endif
	
public:
	MirakcConnectHttp( char *host, short port );
	int connect();
};

class MirakcConnectUnix : public MirakcConnectBase
{
private:
	struct sockaddr_un addr;

public:
	MirakcConnectUnix( char *path );
	int connect();
};



/*************************************************/

MirakcConnectHttp::MirakcConnectHttp( char *host, short port )
{
#if 1
	s_addr.sin_addr.s_addr = inet_addr( host );
	s_addr.sin_port        = htons( port );
	s_addr.sin_family      = AF_INET;
#endif

#if 0
	memset( &hints, 0, sizeof(hints) );
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	int ret; 
	
	ret = getaddrinfo( host, NULL, &hints, &addr );
	if( ret != 0 ) {
		// error
	}
#endif
}


int MirakcConnectHttp::connect()
{
	int ret;
	
	s = socket(AF_INET, SOCK_STREAM, 0);

	ret = ::connect( s, (sockaddr *)&s_addr, sizeof( s_addr ) );
	
	return ret;
}

/*************************************************/


MirakcConnectUnix::MirakcConnectUnix( char *path )
{
	addr.sun_family = AF_UNIX;
	
	if( strlen( path ) <= sizeof( addr.sun_path ) - 1 ) {
		strcpy( addr.sun_path, path );
	}
	else {
		addr.sun_path[0] = 0;
	}
}


int MirakcConnectUnix::connect()
{
	int ret;
	
	s = socket(AF_UNIX, SOCK_STREAM, 0);

	//ret = ::connect( s, addr->ai_addr, addr->ai_addrlen );
	ret = ::connect( s, (sockaddr *)&addr, sizeof( addr ) );

	return ret;
}


/*************************************************/

void MirakcConnectBase::shutdown()
{
	if( s >= 0 ) {
	
		::shutdown(s, SHUT_RDWR);
	}

}

void MirakcConnectBase::disconnect()
{
	if( s >= 0 ) {
	
		::close(s);
	}
	s = -1;
}

int MirakcConnectBase::sendGetRequest_WaitBody( char *url, char *requestHeader, char *responceHeader, int *responceCode, char *body, int *bodysize )
{
	int ret;
	int state = 0;
	char *p_startbody = body;

	if( s < 0 ) {
		connect();
	}

	char send_string[ 2024 ];
	::sprintf( send_string, "GET %s HTTP/1.0\r\n%s\r\n\r\n", url, requestHeader );

	ret = ::send( s, send_string, strlen( send_string ), 0 );
	if( ret < 0 ) {
		
		return ret - 1000;
	}

	
	
	for(;;) {
		ret = ::recv( s, buf, sizeof(buf) - 1, 0 );
		if( ret < 0 ){
			return -1;
		}
		if( ret == 0 ){
			*body = 0;
			*bodysize = body - p_startbody;

			disconnect();
			return 0;
		}
		buf[ ret ] = 0;

		if( state == 0 ) { // state reading
			char *p;
			p = ::strchr( buf, ' ' );
			*responceCode = ::atoi( p + 1 );
			state = 1;
		}
		if( state == 1 ){ // break through
			char *p;
			
			p = ::strstr( buf, "\r\n\r\n" );
			if( p != NULL ) {
				p += 4;
				::memcpy( responceHeader, buf, p - buf );
				responceHeader += p - buf;
				*responceHeader = 0;

				::memmove( buf, p, ret - ( p - buf ) );
				ret = ret - ( p - buf );

				state = 2;
			
			}
			else {
				::memcpy( responceHeader, buf, ret );
				responceHeader += ret;
			}
		}
		
		if( state == 2 ) { // ここはelseif
			::memcpy( body, buf, ret );
			body += ret;
		}
	}
	// not reached
}


int MirakcConnectBase::sendGetRequest_WaitHeader( char *url, char *requestHeader, char *responceHeader, int *responceCode )
{
	int ret;
	int state = 0;

	if( s < 0 ) {
		connect();
	}

	char send_string[ 2024 ];
	::sprintf( send_string, "GET %s HTTP/1.0\r\n%s\r\n\r\n", url, requestHeader );

	ret = ::send( s, send_string, strlen( send_string ), 0 );
	if( ret < 0 ) {
		return -errno - 1000;
	}
	
	for(;;) {
		ret = ::recv( s, buf, sizeof(buf) - 1, 0 );
		if( ret < 0 ){
			return -1;
		}
		if( ret == 0 ){
			close(s);
			s=-1;
			return 0;
		}
		buf[ ret ] = 0;

		if( state == 0 ) { // state reading
			char *p;
			p = ::strchr( buf, ' ' );
			*responceCode = ::atoi( p + 1 );
			state = 1;
		}
		if( state == 1 ){ // break through
			char *p;
			
			p = ::strstr( buf, "\r\n\r\n" );
			if( p != NULL ) {
				p += 4;
				::memcpy( responceHeader, buf, p - buf );
				responceHeader += p - buf;
				*responceHeader = 0;

				::memmove( buf, p, ret - ( p - buf ) );
				ret = ret - ( p - buf );

				state = 2;
			
			}
			else {
				::memcpy( responceHeader, buf, ret );
				responceHeader += ret;
			}
		}
		if( state == 2 ) { // 
			bufsize = ret;
			
			return 0; // !
		}
	}
	// not reached
}

int MirakcConnectBase::recvBody( char *body, size_t size )
{
	if( bufsize > 0 ){
		int len;

		len = std::min( (size_t)bufsize, size );

		::memcpy( body, buf, len );
		bufsize -= len;

		return len;
	}

	int ret;
	
	ret = ::recv( s, body, size, 0 );

	return ret;
}


