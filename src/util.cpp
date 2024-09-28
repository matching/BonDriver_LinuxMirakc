// SPDX-License-Identifier: MIT
/*
 * Useful functions (util.cpp)
 *
 * Copyright (c) 2021 nns779
 * modified by matching
 */

#include "util.hpp"

namespace util {

void Trim(char **str, std::size_t *len)
{
	char *p = *str;
	std::size_t n = *len;

	while (n) {
		if (*p == ' ' || *p == '\t') {
			p++;
			n--;
		} else {
			break;
		}
	}

	*str = p;
	*len = n;

	return;
}

void RTrim(char **str, std::size_t *len)
{
	char *p = *str - 1;
	std::size_t n = *len;

	while (n) {
		if (*p == ' ' || *p == '\t') {
			p--;
			n--;
		} else {
			*(p + 1) = '\0';
			break;
		}
	}

	*str = p + 1;
	*len = n;

	return;
}

void Separate(const std::string& str, std::vector<std::string>& res)
{
	auto separator = std::string(", \t");
	auto ofs = std::string::size_type(0);
	auto tail = str.length();

	while (ofs <= tail) {
		auto h = str.find_first_not_of(separator, ofs);
		if (h != std::string::npos)
			ofs = h;

		auto t = str.find_first_of(separator, ofs);
		if (t == std::string::npos)
			t = tail;

		if (ofs == t)
			break;

		res.emplace_back(str.substr(ofs, t - ofs));
		ofs = t + 1;
	}

	return;
}

} // namespace util

