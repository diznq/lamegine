#include "shared.h"

#include "String.h"

StringStream::StringStream(const char *text) : idx(0) {
	static char buffer[32768];
	strncpy(buffer, text, 32768);
	char *ptr = strtok(buffer, " ");
	if (ptr) {
		tokens.push_back(ptr);
		while (ptr = strtok(NULL, " ")) tokens.push_back(ptr);
	}
	ttl = tokens.size();
}

bool StringStream::eof() const {
	return idx >= ttl;
}

const char* StringStream::readString() {
	if (idx >= ttl) return 0;
	return tokens[idx++];
}

bool StringStream::readFloat(float& n) {
	if (idx >= ttl) return false;
	sscanf(tokens[idx++], "%f", &n);
	return true;
}

bool StringStream::readInt(int& n) {
	if (idx >= ttl) return false;
	sscanf(tokens[idx++], "%d", &n);
	return true;
}