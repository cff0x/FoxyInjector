#pragma once
#include <foxy.h>

class StringUtils
{
public:

	static const std::vector<std::string> splitString(const std::string& text, char delimiter)
	{
		// setup variables
		std::string part;
		std::stringstream input(text);
		std::vector<std::string> result;

		// split by delimiter and add to the result vector
		while (getline(input, part, delimiter)) {
			result.push_back(part);
		}

		return result;
	}

	static const std::string toUpper(const std::string& text)
	{
		// transform input string to uppercase and return it by value
		std::string result{ text };
		std::transform(result.begin(), result.end(), result.begin(),
			[](unsigned char c) {
				return std::toupper(c);
			}
		);

		return result;
	}

	static const std::string toLower(const std::string& text)
	{
		// transform input string to lowercase and return it by value
		std::string result{ text };
		std::transform(result.begin(), result.end(), result.begin(),
			[](unsigned char c) {
				return std::tolower(c);
			}
		);

		return result;
	}

	static const bool isNumber(const std::string& s)
	{
		// check each character and return true if all of them are digits
		std::string::const_iterator it = s.begin();
		while (it != s.end() && std::isdigit(*it)) ++it;
		return !s.empty() && it == s.end();
	}

};