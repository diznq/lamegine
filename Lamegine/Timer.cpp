#include "shared.h"
#include "Timer.h"

void Stopwatch::start() {
	t = clock();
}

void Stopwatch::snap(const char *what) {
	std::cout << what << " finished in " << (clock() - t) / (1.0f * CLOCKS_PER_SEC) << " s" << std::endl;
}

void Stopwatch::stop(const char *what) {
	std::cout << what << " finished in " << (clock() - t) / (1.0f * CLOCKS_PER_SEC) << " s" << std::endl;
	t = clock();
}