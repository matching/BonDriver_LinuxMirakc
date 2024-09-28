//------------------------------------------------------------------------------
// File: GrabTsData.h
//   Header file of GrabTsData
//   originaled by tkmsst
//   modified by matching
//------------------------------------------------------------------------------

//#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

//#include <atomic>
#include <pthread.h>

#define MAX_PATH 256

#define CEIL(a,b) (((a)+(b)-1)/(b))

// TS data buffer size
//
#define DATA_BUF_SIZE (188 * 256)
#define RING_BUF_SIZE (DATA_BUF_SIZE * 512) // > 24Mbps / 8bit * 5sec

// GrabTsData class
//
class GrabTsData
{
public:
	// Constructor
//	GrabTsData(HANDLE *phOnStreamEvent)
	GrabTsData()
	{
//		m_phOnStreamEvent = phOnStreamEvent;
		m_nAccumData = 0;
		m_bPurge = FALSE;
		m_nPush = 0;
		m_nPull = 0;
		m_pDst = (BYTE *)malloc(DATA_BUF_SIZE);
		m_pBuf = (BYTE *)malloc(RING_BUF_SIZE);

		::pthread_cond_init( &m_phOnStreamEvent, NULL );
		::pthread_cond_init( &m_phOnStreamGetEvent, NULL );
//		::pthread_mutex_init( &m_pRingMutex, NULL );

		pthread_mutexattr_t attr;
		::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		::pthread_mutex_init( &m_pRingMutex, &attr );
		
	}
	// Destructor
	~GrabTsData()
	{
		if (m_pBuf) {
			::free(m_pBuf);
		}
		if (m_pDst) {
			::free(m_pDst);
		}
	}
	// Interfaces
	BOOL put_TsStream(BYTE *pSrc, DWORD dwSize);
	BOOL get_TsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain);
	BOOL purge_TsStream(void);
	BOOL get_ReadyCount(DWORD *pdwRemain);
	BOOL get_Bitrate(float *pfBitrate);

	DWORD Wait_TsStream(DWORD waitMs);

private:
	uint64_t GetTickCount64();
	// Stream event
//	HANDLE *m_phOnStreamEvent;
	pthread_cond_t m_phOnStreamEvent;
	pthread_cond_t m_phOnStreamGetEvent;
	pthread_mutex_t m_pRingMutex;
	
	// Bitrate calculation
#if 0
	std::atomic_long m_nAccumData;
	// Purge flag
	std::atomic_bool m_bPurge;
	// TS data buffer (simple ring buffer)
	std::atomic_ulong m_nPush;
	std::atomic_ulong m_nPull;
#endif
	long m_nAccumData;
	// Purge flag
	bool m_bPurge;
	// TS data buffer (simple ring buffer)
	unsigned long m_nPush;
	unsigned long m_nPull;
	
	BYTE *m_pDst;
	BYTE *m_pBuf;
};
