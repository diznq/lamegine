#pragma once
#include <stdio.h>
#include <string.h>
#include <vector>
class StringStream {
	size_t idx;
	size_t ttl;
	std::vector<char*> tokens;
public:
	StringStream(const char *text);
	~StringStream() {

	}
	bool eof() const;
	bool readInt(int& n);
	bool readFloat(float& n);
	const char* readString();
};