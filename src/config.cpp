// SPDX-License-Identifier: MIT
/*
 * INI file loader (config.cpp)
 *
 * Copyright (c) 2021 nns779
 * modified by matching
 */

#include "config.hpp"

#include <cstring>
#include <algorithm>
#include <fstream>

#include "util.hpp"

bool Config::Section::Exists(const std::string& key) const noexcept
{
	return !!data_.count(key);
}

bool Config::Section::Set(const std::string& key, const std::string& value)
{
	return data_.emplace(key, value).second;
}

const std::string& Config::Section::Get(const std::string& key) const
{
	return data_.at(key);
}

std::string Config::Section::Get(const std::string& key, const std::string& default_value) const
{
	try {
		return Get(key);
	} catch (const std::out_of_range&) {
		return default_value;
	}
}

int Config::Section::Get(const std::string& key, int default_value) const
{
	try {
		return std::stoi(Get(key), nullptr, 0);
	} catch (const std::out_of_range&) {
		return default_value;
	} catch (const std::invalid_argument&) {
		return default_value;
	}
}

bool Config::Load(const std::string& path)
{
	std::ifstream ifs(path);
	Config::Section *sct = nullptr;

	if (!ifs.is_open())
		return false;

	while (true) {
		char line[256], *p = line;
		std::size_t len;

		if (!ifs.getline(line, 256))
			break;

		len = std::strlen(line);

		util::Trim(&p, &len);
		if (!len) {
			// blank line
			continue;
		}

		if (p[len] == '\r') {
			// CRLF
			p[len--] = '\0';
		}

		switch (*p) {
		case ';':
			// comment
			continue;

		case '[':
		{
			// section name

			p++;
			len--;

			util::Trim(&p, &len);
			if (!len) {
				// invalid section name
				sct = nullptr;
				continue;
			}

			char *term = std::strchr(p + 1, ']');
			if (!term)
				term = p + len;

			util::RTrim(&term, &len);
			if (!len) {
				// invalid section name
				sct = nullptr;
				continue;
			}

			auto s = sections_.emplace(p, Config::Section());
			sct = (s.second) ? &s.first->second : nullptr;

			break;
		}

		default:
		{
			// key-value pair

			if (!sct)
				break;

			char *key = p, *key_term = std::strchr(p, '='), *val;
			if (key_term) {
				*key_term = '\0';
				val = key_term + 1;
			} else {
				key_term = val = key + len;
			}

			std::size_t key_len = std::strlen(key), val_len = std::strlen(val);
			char *val_term = val + val_len;

			util::Trim(&key, &key_len);
			util::RTrim(&key_term, &key_len);

			util::Trim(&val, &val_len);
			util::RTrim(&val_term, &val_len);

			if ((val[0] == '\"' && val[val_len - 1] == '\"') ||
			    (val[0] == '\'' && val[val_len - 1] == '\'')) {
				val[val_len - 1] = L'\0';
				val++;
				val_len--;
			}

			sct->Set(key, val);
			break;
		}

		}
	}

	return true;
}

bool Config::Exists(const std::string& sct) const noexcept
{
	return !!sections_.count(sct);
}

const Config::Section& Config::Get(const std::string& sct) const
{
	return sections_.at(sct);
}
