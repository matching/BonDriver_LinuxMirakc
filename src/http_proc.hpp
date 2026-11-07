/*
 * Connecting to Mirac
 * Copyright 2024 matching
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>

#include <algorithm>

#include "logoutput.hpp"

#define HTTP_BUFFER_SIZE (1024 * 1024 * 1)

class MirakcConnectBase
{
protected:
	int s; // socket

	char buf[ HTTP_BUFFER_SIZE + 1 ];
	int bufsize;

	char hostHeader[ 32 ];

public:
	MirakcConnectBase()
	{
		s = -1;
		bufsize = 0;
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
	void shutdownBoth();
};

class MirakcConnectHttp : public MirakcConnectBase
{
private:
	struct sockaddr_in s_addr;
	
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
	s_addr.sin_addr.s_addr = inet_addr( host );
	s_addr.sin_port        = htons( port );
	s_addr.sin_family      = AF_INET;

	snprintf( hostHeader, sizeof(hostHeader), "Host: %s:%d", host, port );
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

	/* Unix Domain Socket は固定設定*/
	strcpy( hostHeader, "Host: localhost:40772" );
}


int MirakcConnectUnix::connect()
{
	int ret;
	
	s = socket(AF_UNIX, SOCK_STREAM, 0);

	ret = ::connect( s, (sockaddr *)&addr, sizeof( addr ) );

	return ret;
}


/*************************************************/

void MirakcConnectBase::shutdown()
{
	if( s >= 0 ) {
		::shutdown(s, SHUT_WR);
	}
}

void MirakcConnectBase::shutdownBoth()
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

	ret = connect();
	if( ret < 0 ) {
		return -errno - 2000;
	}

	char send_string[ 2024 ];
	::sprintf( send_string, "GET %s HTTP/1.0\r\n%s\r\n%s\r\n\r\n", url, hostHeader, requestHeader );

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

	ret = connect();
	if( ret < 0 ) {
		return -errno - 2000;
	}
	char send_string[ 2024 ];
	::sprintf( send_string, "GET %s HTTP/1.0\r\n%s\r\n%s\r\n\r\n", url, hostHeader, requestHeader );

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
		if( bufsize > 0 ) {
			::memmove( buf, buf + len, bufsize );
		}

		return len;
	}

	int ret;
	
	ret = ::recv( s, body, size, 0 );

	if( ret < 0 ) {
		return -errno - 10000;
	}

	return ret;
}


