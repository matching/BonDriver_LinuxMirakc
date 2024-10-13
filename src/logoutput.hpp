/*
 * Copyright 2024 matching
 */

#ifndef LOGOUTPUT_HPP
#define LOGOUTPUT_HPP

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

#define LOG_OUTPUT_TIME { \
	time_t t; \
	struct tm *d; \
	char ddd[64]; \
	t = time(NULL); \
    d = localtime( &t ); \
    strftime( ddd, sizeof(ddd), "%Y/%m/%d %H:%M:%S", d ); \
	fprintf(stderr, "%s ", ddd ); }
	 // todo スレッドに割り込まれる

#define GETPID getpid()
#define GETTID (int)syscall(SYS_gettid)

#define ERROR_OUTPUT(FMT, ...) {LOG_OUTPUT_TIME; fprintf(stderr, "%s(%d,%d):%s:%d:" FMT "\n", g_TunerName, GETPID, GETTID, __func__, __LINE__, __VA_ARGS__ ); }

#ifdef DEBUG
#define DEBUG_OUTPUT(FMT, ...) {LOG_OUTPUT_TIME; fprintf(stderr, "%s(%d,%d):%s:%d:(DEBUG)" FMT "\n", g_TunerName, GETPID, GETTID, __func__, __LINE__, __VA_ARGS__ ); }
#else
#define DEBUG_OUTPUT(FMT, ...)
#endif

#ifdef DEBUG_VERVOSE
#define DEBUG_CALL(msg) {LOG_OUTPUT_TIME; fprintf(stderr, "%s(%d,%d):◎%s:%d called (%s)\n", g_TunerName, GETPID, GETTID, __func__, __LINE__, msg ); }
#else
#define DEBUG_CALL(msg)
#endif

#define DEBUG_OUTPUT1(MSG) DEBUG_OUTPUT("%s",MSG)
#define ERROR_OUTPUT1(MSG) ERROR_OUTPUT("%s",MSG)


extern char g_TunerName[];

#endif

