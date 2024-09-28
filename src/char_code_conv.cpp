// SPDX-License-Identifier: MIT
/*
 * Character code converter (char_code_conv.cpp)
 *
 * Copyright (c) 2021 nns779
 * modified by matching
 */

#include "char_code_conv.hpp"

#include <stdexcept>
#include <string.h>

CharCodeConv::CharCodeConv()
{
	cd_ = ::iconv_open("UTF-16LE", "UTF-8");
	if (cd_ == reinterpret_cast<::iconv_t>(-1))
		throw std::runtime_error("CharCodeConv::CharCodeConv: ::iconv_open() failed");
}

CharCodeConv::~CharCodeConv()
{
	if (cd_ != reinterpret_cast<::iconv_t>(-1))
		::iconv_close(cd_);
}

bool CharCodeConv::Utf8ToUtf16(const char *src, WCHAR *dst)
{
	char *s = (char *)src;
	size_t s_len = ::strlen( src );

	size_t d_len = s_len * sizeof(WCHAR);
	memset( dst, 0, d_len + sizeof(WCHAR) );

	size_t cr = ::iconv(cd_, &s, &s_len, (char **)&dst, &d_len);
	if (cr == (size_t)-1)
		return false;

	return true;
}

