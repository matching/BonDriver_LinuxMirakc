//------------------------------------------------------------------------------
// File: GrabTsData.cpp
//   Implementation of GrabTsData
//
//   originaled by tkmsst
//   modified by matching
//------------------------------------------------------------------------------
#include "type_compat.h"
#include "GrabTsData.hpp"

#include "logoutput.hpp"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <algorithm>

// Put TS data into the ring buffer
//
BOOL GrabTsData::put_TsStream(BYTE *pSrc, DWORD dwSize)
{
	if (dwSize < 1)
		return FALSE;

	// accumulate number of TS data received
//	std::atomic_fetch_add(&m_nAccumData, dwSize);

	// lock push and pull positions
	DWORD nPush, nPull;
	DWORD nTail;
	DWORD nData;
	
	::pthread_mutex_lock( &m_pRingMutex );
	{
		m_nAccumData += dwSize;

		for(;;) {

			nPush = m_nPush;
			nPull = m_nPull;

			// swSize が入るか確認
			nTail = RING_BUF_SIZE - nPush; // size between the current position and the buffer end
			nData = (RING_BUF_SIZE + nPush - nPull) % RING_BUF_SIZE; // size of TS data stored
			nData = RING_BUF_SIZE - nData - 1; // size of available buffer (empty buffer - 1)
			
			if( nData >= dwSize ) {
				break;
			}
			::pthread_cond_wait( &m_phOnStreamGetEvent, &m_pRingMutex );
		}
	}
	::pthread_mutex_unlock( &m_pRingMutex );

	// copy TS data to the ring buffer
//	DWORD nTail = RING_BUF_SIZE - nPush; // size between the current position and the buffer end
//	DWORD nData = (RING_BUF_SIZE + nPush - nPull) % RING_BUF_SIZE; // size of TS data stored
//	nData = RING_BUF_SIZE - nData - 1; // size of available buffer (empty buffer - 1)
	nData = std::min(nData, dwSize);
	if (nData < nTail) {
		//CopyMemory(m_pBuf + nPush, pSrc, nData);
		::memcpy(m_pBuf + nPush, pSrc, nData);
		nPush += nData;
	}
	else {
		//CopyMemory(m_pBuf + m_nPush, pSrc, nTail);
		//CopyMemory(m_pBuf, pSrc + nTail, nData - nTail);
		::memcpy(m_pBuf + m_nPush, pSrc, nTail);
		::memcpy(m_pBuf, pSrc + nTail, nData - nTail);
		nPush = nData - nTail;
	}
	// update the push position
	::pthread_mutex_lock( &m_pRingMutex );
	{
		m_nPush = nPush;
		::pthread_cond_signal( &m_phOnStreamEvent );
	}
    ::pthread_mutex_unlock( &m_pRingMutex );

	// set WaitTsStream to the signaled state
#if 0
	if (m_phOnStreamEvent)
		SetEvent(*m_phOnStreamEvent);
#endif

	return TRUE;
}

// Get TS data from the ring buffer
//
BOOL GrabTsData::get_TsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	DWORD nPush, nPull;

	::pthread_mutex_lock( &m_pRingMutex );
	{
		// purge the pull buffer
		if( m_bPurge ) {
			m_nPull = m_nPush = 0;
			m_nAccumData = 0; // reset bitrate
			m_bPurge = FALSE;
			::pthread_cond_signal( &m_phOnStreamGetEvent );
			::pthread_mutex_unlock( &m_pRingMutex );
			return FALSE;
		}

		// lock push and pull positions
		nPush = m_nPush;
		nPull = m_nPull;
	}
	::pthread_mutex_unlock( &m_pRingMutex );
#if 0
	// purge the pull buffer
	if (std::atomic_load(&m_bPurge)) {
		std::atomic_store(&m_nPull, std::atomic_load(&m_nPush));
		std::atomic_store(&m_nAccumData, 0); // reset bitrate
		std::atomic_store(&m_bPurge, FALSE);
		return FALSE;
	}
#endif

	// copy TS data to the destination buffer
	DWORD nTail = RING_BUF_SIZE - nPull; // size between the current position and the buffer end
	DWORD nData = (RING_BUF_SIZE + nPush - nPull) % RING_BUF_SIZE; // size of TS data stored
	DWORD nRemain = nData;
	if (nData > 0) {
		nData = std::min(nData, (DWORD)DATA_BUF_SIZE);
		if (nData < nTail) {
			//CopyMemory(m_pDst, m_pBuf + nPull, nData);
			memcpy(m_pDst, m_pBuf + nPull, nData);
			nPull += nData;
		} else {
			//CopyMemory(m_pDst, m_pBuf + nPull, nTail);
			//CopyMemory(m_pDst + nTail, m_pBuf, nData - nTail);
			memcpy(m_pDst, m_pBuf + nPull, nTail);
			memcpy(m_pDst + nTail, m_pBuf, nData - nTail);
			nPull = nData - nTail;
		}
		nRemain -= nData;
		// update the pull position

		::pthread_mutex_lock( &m_pRingMutex );
		{
		//		std::atomic_store(&m_nPull, nPull);
			m_nPull = nPull;
			::pthread_cond_signal( &m_phOnStreamGetEvent );
		}
		::pthread_mutex_unlock( &m_pRingMutex );
	}

	// set destination variables
	*ppDst = m_pDst;
	*pdwSize = nData;
	if (pdwRemain) {
		*pdwRemain = CEIL(nRemain, DATA_BUF_SIZE);
	}

	return TRUE;
}

// Purge TS data
//
BOOL GrabTsData::purge_TsStream(void)
{
//	std::atomic_store(&m_bPurge, TRUE);

	::pthread_mutex_lock( &m_pRingMutex );
	{
		m_bPurge = TRUE;
	}
	::pthread_mutex_unlock( &m_pRingMutex );

	return TRUE;
}

// Get the number of TS data blocks in the ring buffer
//
BOOL GrabTsData::get_ReadyCount(DWORD *pdwRemain)
{
	if (pdwRemain) {
		DWORD nPush, nPull;
		::pthread_mutex_lock( &m_pRingMutex );
		{
			nPush = m_nPush;
			nPull = m_nPull;
		}
		::pthread_mutex_unlock( &m_pRingMutex );
	
		*pdwRemain = CEIL((RING_BUF_SIZE + nPush - nPull) % RING_BUF_SIZE, DATA_BUF_SIZE);
	}

	return TRUE;
}

uint64_t GrabTsData::GetTickCount64()
{
	
    struct timespec ts;
    uint64_t theTick = 0U;

	clock_gettime( CLOCK_REALTIME, &ts );
    theTick  = ts.tv_nsec / 1000000;
    theTick += ts.tv_sec * 1000;
	
    return theTick;
}

// Calculate bitrate
//
BOOL GrabTsData::get_Bitrate(float *pfBitrate)
{
	static double dBitrate = 0;
	static uint64_t ui64LastTime = GetTickCount64();
	uint64_t ui64Now = GetTickCount64(); // ms
	uint64_t ui64Duration = ui64Now - ui64LastTime;

	if (ui64Duration >= 1000) {
		::pthread_mutex_lock( &m_pRingMutex );
		{
	//		dBitrate = std::atomic_exchange(&m_nAccumData, 0) / ui64Duration * 8 * 1000 / 1024 / 1024.0; // Mbps
			dBitrate = m_nAccumData / ui64Duration * 8 * 1000 / 1024 / 1024.0; // Mbps
			m_nAccumData = 0;
		}
		::pthread_mutex_unlock( &m_pRingMutex );
		ui64LastTime = ui64Now;
	}
	*pfBitrate = (float)std::min(dBitrate, (double)100);

	return TRUE;
}


DWORD GrabTsData::Wait_TsStream(DWORD waitMs)
{
	DWORD cnt;
	DWORD ret;
	int r;

	struct timespec to;

	to.tv_sec = time(NULL) + waitMs / 1000;
	to.tv_nsec = (waitMs % 1000) * 1000 * 1000;
	
    ::pthread_mutex_lock( &m_pRingMutex );
	{
		get_ReadyCount( &cnt );
		if( cnt == 0 ){
			r = ::pthread_cond_timedwait( &m_phOnStreamEvent, &m_pRingMutex, &to );
			r=0;
			if( r == 0 ) { // success
				ret = WAIT_OBJECT_0;
			}
			else if( errno == ETIMEDOUT ) {
				ret = WAIT_TIMEOUT;
			}
			else {
				ret = WAIT_TIMEOUT;
			}
		}
	}
	::pthread_mutex_unlock( &m_pRingMutex );

	return ret;
}

