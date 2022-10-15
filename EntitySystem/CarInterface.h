#pragma once
#include <string.h>

struct CarInterface {
	virtual float getSteering() const = 0;
	virtual void setSteering(float a) = 0;
	virtual void setSpeed(float f) = 0;
	virtual float getSpeed() const = 0;
	virtual void clearLine() = 0;
	virtual void writeLine(const char *text) = 0;
	virtual const char* getLine() = 0;
	virtual void onButtonPressed(int btn, int dur) = 0;
	virtual void onUpdate(float dt) = 0;
	virtual void run() = 0;
	virtual int readCam(float* out) = 0;
	virtual void writeCam(float* data) = 0;
	virtual bool hasControl() = 0;
};

class CarImpl : public CarInterface {
	float steering;
	float speed;
	long _time;
	float camData[128];
	char text[256000];
public:
	CarImpl() : steering(0.0f), speed(0.0f), _time(0) { text[0] = 0; }
	virtual float getSteering() const { return steering; }
	virtual float getSpeed() const { return speed; }
	virtual void setSteering(float a) { steering = a; }
	virtual void setSpeed(float f) { speed = f; }
	virtual void clearLine() { text[0] = 0; }
	virtual void writeLine(const char *text) {
		strcat(this->text, text); strcat(this->text, "\n");
	}
	virtual const char *getLine() { return text; }
	virtual void onButtonPressed(int btn, int dur) { }
	virtual void onUpdate(float dt);
	virtual void run() { }
	virtual int readCam(float *out) {
		memcpy(out, camData, sizeof(camData));
		return 128;
	}
	virtual void writeCam(float *data) {
		memcpy(camData, data, sizeof(camData));
	}
	virtual bool hasControl() { return true; }
};