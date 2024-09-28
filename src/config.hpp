// config.hpp
// originaled by nns779
// modified by matching

#pragma once

#include <string>
#include <functional>
#include <unordered_map>

class Config final {
public:
	class Section final {
	public:
		Section() {}
		~Section() {}

		bool Exists(const std::string& key) const noexcept;
		bool Set(const std::string& key, const std::string& value);
		const std::string& Get(const std::string& key) const;
		std::string Get(const std::string& key, const std::string& default_value) const;
		int Get(const std::string& key, int default_value) const;

	private:
		std::unordered_map<std::string, std::string> data_;
	};

	Config() {}
	~Config() {}

	bool Load(const std::string& path);
	bool Exists(const std::string& sct) const noexcept;
	const Section& Get(const std::string& sct) const;

private:
	std::unordered_map<std::string, Section> sections_;
};


