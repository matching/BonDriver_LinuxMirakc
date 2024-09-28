// util.hpp
// originaled by nns779
// modified by matching

#pragma once

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

namespace util {

void Trim(char **str, std::size_t *len);
void RTrim(char **str, std::size_t *len);
void Separate(const std::string& str, std::vector<std::string>& res);

} // namespace util

