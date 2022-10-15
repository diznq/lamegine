#pragma once
#include <iostream>
#include <time.h>

class Stopwatch {
	clock_t t;
public:
	Stopwatch() {}
	void start();
	void snap(const char *what);
	void stop(const char *what);
};