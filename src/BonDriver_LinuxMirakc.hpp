//   originaled by tkmsst
//   modified by matching

#include <limits.h>

#include "type_compat.h"
#include "IBonDriver2.h"
#include "GrabTsData.hpp"
#include "picojson/picojson.h"

#include "char_code_conv.hpp"
#include "http_proc.hpp"


#if !defined(_BONTUNER_H_)
#define _BONTUNER_H_

#define TUNER_NAME "LinuxMirakc"

static const char g_TunerName[] = TUNER_NAME;

#define MAX_HOST_LEN 256
static char g_ServerHost[MAX_HOST_LEN];

static uint32_t g_ServerPort;
static int g_DecodeB25;
static int g_Priority;
static int g_Service_Split;

#define SPACE_NUM 8
static char *g_pType[SPACE_NUM];
static int g_Max_Type = -1;
static DWORD g_Channel_Base[SPACE_NUM];
picojson::value g_Channel_JSON;

static char g_ServerSockpath[ PATH_MAX + 1 ];
static char g_ServerType[ 4 + 1 ];

class CBonTuner : public IBonDriver2
{
public:
	CBonTuner();
	virtual ~CBonTuner();

	// IBonDriver
	const BOOL OpenTuner(void);
	void CloseTuner(void);

	const BOOL SetChannel(const BYTE bCh);
	const float GetSignalLevel(void);

	const DWORD WaitTsStream(const DWORD dwTimeOut = 0);
	const DWORD GetReadyCount(void);

	const BOOL GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain);
	const BOOL GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain);

	void PurgeTsStream(void);

	// IBonDriver2(暫定)
	LPCTSTR GetTunerName(void);

	const BOOL IsTunerOpening(void);

	LPCTSTR EnumTuningSpace(const DWORD dwSpace);
	LPCTSTR EnumChannelName(const DWORD dwSpace, const DWORD dwChannel);

	const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel);

	const DWORD GetCurSpace(void);
	const DWORD GetCurChannel(void);

	void Release(void);

	static CBonTuner *m_pThis;

protected:
	GrabTsData *m_pGrabTsData;

	DWORD m_dwCurSpace;
	DWORD m_dwCurChannel;

	BOOL InitChannel(void);
	BOOL GetApiChannels(picojson::value *json_array, int service_split);

	BOOL SendRequest(char *url, char **body, int *bodysize);

	static void *RecvThread( void *pParam );
	
	CharCodeConv m_cv;
	MirakcConnectBase *conn;
	pthread_t m_hRecvThread;

	void waitForRecvThreadFinish(void);
};

#endif // !defined(_BONTUNER_H_)
