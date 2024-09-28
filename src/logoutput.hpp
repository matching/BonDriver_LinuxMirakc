/*
 * Copyright 2024 matching
 */

#ifndef LOGOUTPUT_HPP
#define LOGOUTPUT_HPP


#define ERROR_OUTPUT(FMT, ...) {fprintf(stderr, "Bondriver_LinuxMirakc:%s:%d:" FMT "\n", __func__, __LINE__, __VA_ARGS__ ); }

 #if 0
 // debug
#include <stdio.h>

#define DEBUG_CALL(msg) {fprintf(stderr, "Bondriver_LinuxMirakc:◎%s:%d called (%s)\n", __func__, __LINE__, msg ); }

#define DEBUG_OUTPUT(FMT, ...) {fprintf(stderr, "Bondriver_LinuxMirakc:%s:%d:(DEBUG)" FMT "\n", __func__, __LINE__, __VA_ARGS__ ); }

 #else

#define DEBUG_CALL(msg)
#define DEBUG_OUTPUT(FMT, ...)

 #endif

#define DEBUG_OUTPUT1(MSG) DEBUG_OUTPUT("%s",MSG)


#endif

