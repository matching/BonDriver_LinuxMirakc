//   originaled by tkmsst
//   modified by matching

#include "BonDriver_LinuxMirakc.hpp"
#include "config.hpp"
#include "logoutput.hpp"

#include <string.h>
#include <dlfcn.h>

static void Init_set_default_value(void);

static int Init()
{

	int ret;
	Dl_info info;
	char ini_filename[ MAX_PATH ];

	ret = dladdr( (void *)Init, &info );
	if( ret == 0 ) {
		ERROR_OUTPUT("dladdr %d", ret);
		return -1;
	}

	sprintf( ini_filename, "%s.ini", info.dli_fname );

	Config config;
	Config::Section sec_global;

	if( config.Load( ini_filename ) == false ) {
		DEBUG_OUTPUT("ini file(%s) not found. load default value", ini_filename);
		Init_set_default_value();
		return 0;
	}

	if( config.Exists( "GLOBAL" ) == false ) {
		DEBUG_OUTPUT1("in ini file, GLOBAL section not found. load default value");
		Init_set_default_value();
		return 0;
	}

	sec_global = config.Get( "GLOBAL" );

	strcpy( g_ServerHost, sec_global.Get("SERVER_HOST", "127.0.0.1" ).c_str() );
	g_ServerPort = sec_global.Get("SERVER_PORT", 40772 );
	g_DecodeB25 = sec_global.Get("DECODE_B25", 0 );
	g_Priority = sec_global.Get("PRIORITY", 1 );
	g_Service_Split = sec_global.Get("SERVICE_SPLIT", 0 );

	strncpy( g_ServerSockpath, sec_global.Get("SERVER_SOCKPATH", "").c_str(), sizeof( g_ServerSockpath ) - 1 );
	strncpy( g_ServerType, sec_global.Get("SERVER_TYPE", "http").c_str(), sizeof( g_ServerType ) - 1 );

	return 0;
}

static void Init_set_default_value(void)
{
	strcpy( g_ServerHost, "127.0.0.1" );
	g_ServerPort = 40772;
	g_DecodeB25 = 0;
	g_Priority = 1;
	g_Service_Split = 0;
}


//////////////////////////////////////////////////////////////////////
// インスタンス生成メソッド
//////////////////////////////////////////////////////////////////////

extern "C" IBonDriver * CreateBonDriver()
{
	if( CBonTuner::m_pThis != NULL ) {
		return CBonTuner::m_pThis;
	}
	
	int ret;
	
	ret = Init();
	if( ret < 0 ) {
		return NULL;
	}

	DEBUG_OUTPUT("SERVER_HOST:%s", g_ServerHost);
	DEBUG_OUTPUT("SERVER_PORT:%d", g_ServerPort);
	DEBUG_OUTPUT("SERVER_SOCKPATH:%s", g_ServerSockpath);

	DEBUG_OUTPUT("SERVER_TYPE:%s", g_ServerType);

	DEBUG_OUTPUT("DECODE_B25:%d", g_DecodeB25);
	DEBUG_OUTPUT("PRIORITY:%d", g_Priority);
	DEBUG_OUTPUT("SERVICE_SPLIT:%d", g_Service_Split);

	return (IBonDriver *) new CBonTuner;
}

// 静的メンバ初期化
CBonTuner * CBonTuner::m_pThis = NULL;

CBonTuner::CBonTuner()
{
	DEBUG_CALL("");

	m_pThis = this;
	m_dwCurSpace = 0xffffffff;
	m_dwCurChannel = 0xffffffff;

	m_hRecvThread = 0;

	// GrabTsDataインスタンス作成
	m_pGrabTsData = new GrabTsData();

}

CBonTuner::~CBonTuner()
{
	DEBUG_CALL("");

	// 開かれてる場合は閉じる
	CloseTuner();

	// GrabTsDataインスタンス開放
	if (m_pGrabTsData) {
		delete m_pGrabTsData;
	}

	m_pThis = NULL;
}

const BOOL CBonTuner::OpenTuner()
{
	DEBUG_CALL("");

	while (1) {
		conn = NULL;
		
		if( strcasecmp( g_ServerType, "http" ) == 0 ) {
			conn = new MirakcConnectHttp( g_ServerHost, g_ServerPort );
		}
		else if( strcasecmp( g_ServerType, "unix" ) == 0 ) {
			conn = new MirakcConnectUnix( g_ServerSockpath );
		}
		
		if( conn == NULL ) {
			ERROR_OUTPUT( "%s: Server Type Invalid (%s)", g_TunerName, g_ServerType );
			break;
		}
		
		if( conn->connect() < 0 ) {
			ERROR_OUTPUT( "%s: Connect error (%s)(%s)(%d)(%s)", g_TunerName, g_ServerType, g_ServerHost, g_ServerPort, g_ServerSockpath );
			break;
		}

		// todo error handling

		conn->disconnect(); // 要求時に接続するので切断

		//Initialize channel
		if (!InitChannel()) {
			break;
		}

		return TRUE;
	}

	CloseTuner();

	return FALSE;
}

void CBonTuner::CloseTuner()
{
	DEBUG_CALL("");
	
	// チャンネル初期化
	m_dwCurSpace = 0xffffffff;
	m_dwCurChannel = 0xffffffff;

	// スレッド終了
	if (m_hRecvThread) {
		conn->shutdown();
		pthread_join(m_hRecvThread, NULL);
		m_hRecvThread = 0;
	}

	// チューニング空間解放
	for (int i = 0; i <= g_Max_Type; i++) {
		if (g_pType[i]) {
			free(g_pType[i]);
		}
	}
	g_Max_Type = -1;

	conn->disconnect();
}

const DWORD CBonTuner::WaitTsStream(const DWORD dwTimeOut)
{
	DEBUG_CALL("");

	DWORD ret;

	ret = m_pGrabTsData->Wait_TsStream( dwTimeOut );

	return ret;
}

const DWORD CBonTuner::GetReadyCount()
{
	DEBUG_CALL("");

	DWORD dwCount = 0;
	if (m_pGrabTsData) {
		m_pGrabTsData->get_ReadyCount(&dwCount);
	}

	return dwCount;
}

const BOOL CBonTuner::GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	DEBUG_CALL("");


	BYTE *pSrc = NULL;

	// TSデータをバッファから取り出す
	if (GetTsStream(&pSrc, pdwSize, pdwRemain)) {
		if (*pdwSize) {
			::memcpy( pDst, pSrc, *pdwSize );
		}

		return TRUE;
	}

	return FALSE;
}

const BOOL CBonTuner::GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	DEBUG_CALL("");

	if (!m_pGrabTsData || m_dwCurChannel == 0xffffffff) {
		return FALSE;
	}

	BOOL ret;

	ret = m_pGrabTsData->get_TsStream(ppDst, pdwSize, pdwRemain);

	return ret;
}

void CBonTuner::PurgeTsStream()
{
	DEBUG_CALL("");

	if (m_pGrabTsData) {
		m_pGrabTsData->purge_TsStream();
	}
}

void CBonTuner::Release()
{
	DEBUG_CALL("");

	// インスタンス開放
	delete this;
}

LPCTSTR CBonTuner::GetTunerName(void)
{
	DEBUG_CALL("");

	// チューナ名を返す

	static WCHAR buf[ 64 ];
	m_cv.Utf8ToUtf16(TUNER_NAME, buf );
	
	return buf;
}

const BOOL CBonTuner::IsTunerOpening(void)
{
	DEBUG_CALL("");

	return FALSE; // todo?
}

LPCTSTR CBonTuner::EnumTuningSpace(const DWORD dwSpace)
{
	DEBUG_CALL("");

	if ((int32_t)dwSpace > g_Max_Type) {
		return NULL;
	}

	// 使用可能なチューニング空間を返す
	const int len = 8;

	static WCHAR buf[len];
	m_cv.Utf8ToUtf16( g_pType[dwSpace], buf );

	return buf;
}

LPCTSTR CBonTuner::EnumChannelName(const DWORD dwSpace, const DWORD dwChannel)
{
	DEBUG_CALL("");

	if ((int32_t)dwSpace > g_Max_Type) {
		return NULL;
	}
	if ((int32_t)dwSpace < g_Max_Type) {
		if (dwChannel >= g_Channel_Base[dwSpace + 1] - g_Channel_Base[dwSpace]) {
			return NULL;
		}
	}

	DWORD Bon_Channel = dwChannel + g_Channel_Base[dwSpace];
	if (!g_Channel_JSON.contains(Bon_Channel)) {
		return NULL;
	}

	picojson::object& channel_obj =
		g_Channel_JSON.get(Bon_Channel).get<picojson::object>();

	// 使用可能なチャンネル名を返す
	const int len = 128;

	static WCHAR buf[len];
	m_cv.Utf8ToUtf16( channel_obj["name"].get<std::string>().c_str(), buf );

	return buf;
}



const DWORD CBonTuner::GetCurSpace(void)
{
	DEBUG_CALL("");

	// 現在のチューニング空間を返す
	return m_dwCurSpace;
}

const DWORD CBonTuner::GetCurChannel(void)
{
	DEBUG_CALL("");

	// 現在のチャンネルを返す
	return m_dwCurChannel;
}

// チャンネル設定
const BOOL CBonTuner::SetChannel(const BYTE bCh)
{
	DEBUG_CALL("");

	return SetChannel((DWORD)0,(DWORD)bCh - 13);
}

// チャンネル設定
const BOOL CBonTuner::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	DEBUG_CALL("");

	conn->shutdown();

	if ((int32_t)dwSpace > g_Max_Type) {
		return FALSE;
	}

	DWORD Bon_Channel = dwChannel + g_Channel_Base[dwSpace];
	if (!g_Channel_JSON.contains(Bon_Channel)) {
		return FALSE;
	}

	picojson::object& channel_obj =
		g_Channel_JSON.get(Bon_Channel).get<picojson::object>();

	// Server request
	const int len = 128;
	char url[len];
	if (g_Service_Split == 1) {
		const int64_t id = (int64_t)channel_obj["id"].get<double>();
		sprintf(url, "/api/services/%lld/stream?decode=%d", (long long int)id, g_DecodeB25);

	}
	else {
		const char *type = channel_obj["type"].get<std::string>().c_str();
		const char *channel = channel_obj["channel"].get<std::string>().c_str();
		sprintf(url, "/api/channels/%s/%s/stream?decode=%d", type, channel, g_DecodeB25);

	}
	DEBUG_OUTPUT( "request:url:%s", url);

	waitForRecvThreadFinish();

	char szHeader[ 64 ];
	sprintf(szHeader, "Connection: close\r\nX-Mirakurun-Priority: %d", g_Priority);
	char respHeader[ 512 ]; // todo
	int respCode;
	int rc;
	rc = conn->sendGetRequest_WaitHeader( url, szHeader, respHeader, &respCode );
	if( rc != 0 || respCode != 200 ) {
		ERROR_OUTPUT( "%s: Tuner unavailable (rc:%d, resp:%d)", g_TunerName, rc, respCode );
		conn->disconnect();
		return FALSE;
	}

	// チャンネル情報更新
	m_dwCurSpace = dwSpace;
	m_dwCurChannel = dwChannel;

	// TSデータパージ
	PurgeTsStream();

	// 受信スレッド起動
	int ret;
	ret = pthread_create( &m_hRecvThread, NULL, CBonTuner::RecvThread, (void *)this ); 
	if( ret < 0 ) {
		ERROR_OUTPUT( "pthread_create error %d", ret);
		return FALSE;
	}

	return TRUE;
}

// 信号レベル(ビットレート)取得
const float CBonTuner::GetSignalLevel(void)
{
	DEBUG_CALL("");

	// チャンネル番号不明時は0を返す
	float fSignalLevel = 0;
	if (m_dwCurChannel != 0xffffffff && m_pGrabTsData)
		m_pGrabTsData->get_Bitrate(&fSignalLevel);

	return fSignalLevel;
}


/* private */
BOOL CBonTuner::InitChannel()
{
	// mirakc APIよりchannel取得
	if (!GetApiChannels(&g_Channel_JSON, g_Service_Split)) {
		return FALSE;
	}
	if (g_Channel_JSON.is<picojson::null>()) {
		return FALSE;
	}
	if (!g_Channel_JSON.contains(0)) {
		return FALSE;
	}

	// チューニング空間取得
	int i = 0;
	int j = -1;
	while (j < SPACE_NUM - 1) {
		if (!g_Channel_JSON.contains(i)) {
			break;
		}
		picojson::object& channel_obj =
			g_Channel_JSON.get(i).get<picojson::object>();
		const char *type;
		if (g_Service_Split == 1) {
			picojson::object& channel_detail =
				channel_obj["channel"].get<picojson::object>();
			type = channel_detail["type"].get<std::string>().c_str();
		}
		else {
			type = channel_obj["type"].get<std::string>().c_str();
		}
		if (j < 0 || strcmp(g_pType[j], type)) {
			j++;
			int len = (int)strlen(type) + 1;
			g_pType[j] = (char *)malloc(len);
			if (!g_pType[j]) {
				j--;
				break;
			}
			strcpy(g_pType[j], type);
			g_Channel_Base[j] = i;
		}
		i++;
	}
	if (j < 0) {
		return FALSE;
	}
	g_Max_Type = j;

	return TRUE;
}

BOOL CBonTuner::GetApiChannels(picojson::value *channel_json, int service_split)
{
	const int len = 14;
	char url[len];

	::strcpy(url, "/api/");
	if (service_split == 1) {
		::strcat(url, "services");
	}
	else {
		::strcat(url, "channels");
	}

	char *data = NULL;
	int dwTotalSize = 0;

	if (!SendRequest(url, &data, &dwTotalSize)) {
		return FALSE;
	}

	*(data + dwTotalSize) = '\0';

	picojson::value v;
	std::string err = picojson::parse(v, data);
	if (!err.empty()) {
		return FALSE;
	}
	*channel_json = v;

	free(data);

	return TRUE;
}

BOOL CBonTuner::SendRequest(char *url, char **body, int *bodysize)
{
	BOOL ret = FALSE;

	conn->shutdown();
	
	while (1) {

		int rc;

		const int len = 64;
		char szHeader[len];
		sprintf(szHeader, "Connection: close\r\nX-Mirakurun-Priority: %d", g_Priority);

		int respCode;
		char respHeader[ 512 ]; // todo
		*body = (char *)malloc( 128 * 1024 ); // todo 128k 

		DEBUG_OUTPUT( "request:url:%s", url);

		waitForRecvThreadFinish();

		rc = conn->sendGetRequest_WaitBody( url, szHeader, respHeader, &respCode, *body, bodysize );
		if( rc != 0 || respCode != 200 ) {
			ERROR_OUTPUT( "%s: Tuner unavailable (rc:%d, resp:%d)", g_TunerName, rc, respCode );
			break;
		}

		return TRUE;
	}

	return ret;
}


void *CBonTuner::RecvThread( void *pParam )
{
	CBonTuner *pThis = (CBonTuner *)pParam;

	//char buf[ 4096 ];
	#define BUF_SIZE (188 * 256)
	char *buf = new char[ BUF_SIZE ];

	int ret;
	
	for(;;) {
	
		ret = pThis->conn->recvBody( buf, BUF_SIZE );
		if( ret == 0 ) { // disconnect
			pThis->conn->disconnect();
			break;
		}
		else if( ret < 0 ) { // error
			ERROR_OUTPUT("recv error (%d)\n\n", ret );
			pThis->conn->disconnect();
			break;
		}
		
		pThis->m_pGrabTsData->put_TsStream( (BYTE *)buf, ret );

	}
	
	delete buf;

	return 0;
}

void CBonTuner::waitForRecvThreadFinish(void)
{
	if( m_hRecvThread > 0 ) {
		pthread_join(m_hRecvThread, NULL); // todo
		m_hRecvThread = 0;
	}
}

