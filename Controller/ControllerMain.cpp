#include "CarInterface.h"
#include "NN.h"
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>

void CarImpl::onUpdate(float dt) {
	int left = 0;
	int right = 0;
	float buff[256];
	readCam(buff);
	float limit = 0.25f;
	bool found = false;
	static Net* nn = new Net({ 3, 6, 2 });
	static std::vector<std::pair<std::vector<double>, std::vector<double>>> dataset;
	static float LastGood = 0.0f;
	static int LastDir = -1, LastGoodDir = -1;
	static float I = 0.0f;
	static float T = 0.0f;
	static float LastError = -1.0f;

	T += dt;

	for (right = 64; right <= 127; right++) {
		if (buff[right] < limit) {
			found = true; break;
		}
	}
	for (left = 64; left > 0; left--) {
		if (buff[left] < limit) {
			found = true; break;
		}
	}

	int Dir = (left + right) / 2 - 64; 
	
	char msg[256];

	float P = Dir / 64.0f;
	float D = ((Dir - LastDir) / 64.0f) * dt;
	I += D;

#if 1
	int state = GetAsyncKeyState(VK_SPACE) & 0x8000;
	static int wasX = 0;
	static int wasC = 0;
	static bool activelyLearning = false;
	int isC = GetAsyncKeyState('C') & 0x8000;
	int isX = GetAsyncKeyState('X') & 0x8000;
	if (!isX && wasX) {
		controlled = !controlled;
	}
	if (!isC && wasC) {
		activelyLearning = !activelyLearning;
	}
	wasC = isC;
	wasX = isX;

	P = (P + 1.0f) / 2.0f;
	D = (D + 1.0f) / 2.0f;
	float St = getSteering();

	if (!controlled) {
		if (activelyLearning) {
			std::vector<double> data = { (double)P, (double)D, (double)I };
			double Ldir = -St;// St < -0.1f ? 1.0 : 0.0;
			double Rdir = St;// > 0.1f ? 1.0 : 0.0;
			std::vector<double> out = { Ldir, Rdir };

			dataset.push_back(std::make_pair(data, out));
		}
		for (int i = 0; i < 10; i++) {
			int sz = dataset.size();
			if (sz > 250) sz = 250;
			for (int j = 0; j < sz; j++) {
				int id = rand() % dataset.size();
				nn->feedForward(dataset[id].first);
				nn->backProp(dataset[id].second);
			}
		}
	} else {
		std::vector<double> data = { (double)P, (double)D, (double)I };
		nn->feedForward(data);
		std::vector<double> out;
		nn->getResults(out);
		if (state) {
			dataset.push_back(std::make_pair(data, out));
		}
		St = (float)(-out[0] + out[1]);
		setSteering(St);
		setSpeed(0.2f);
	}
	sprintf(msg, "$00FF00 Mode: $008080%s $00FF00 P: %.2f I: %.2f D: %.2f", (controlled ? "driving" : (activelyLearning) ? "actively learning" : "passively learning"), P, I, D);
	writeLine(msg);
#else
	float mult = 1.0f;
	float Final = 1.5f * P + 10 * D + 5 * I;
	if (!found || (((abs(P) > 0.4f) && LastGoodDir != -1) || (T - LastError) < 0.5f)) {
		sprintf(msg, "$FF0000 Error, T: %.2f, Timeout: %.2f, Dt: %.4f", T, T - LastError, dt);
		writeLine(msg);
		Final = LastGood;
		if (T - LastError > 1.5f) {
			LastError = T;
		}
	} else {
		LastGood = Final;
		LastGoodDir = Dir;
	}
	sprintf(msg, "$FF0000 Final: %.4f, P: %.2f, I: %.4f, D: %.4f", Final, P, I, D);
	writeLine(msg);
	if (Final < -0.25f || Final > 0.25f) mult /= 3.0F;
	setSteering(Final);
	setSpeed(0.52f * mult);
#endif
	LastDir = Dir;
}

extern "C" __declspec(dllexport) CarInterface* getController() {
	return new CarImpl;
}