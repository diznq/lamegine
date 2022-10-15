#pragma once
#include "shared.h"

class Engine;

class Window : public IWindow {
private:
	int w;
	int h;
	bool fs;
	bool appClose;
	bool cursorShown;
	void* handle;
	HWND internal_handle;
	Engine *pEngine;
	int keys[256];
	int mouseStates[4];
	static std::map<HWND, Window*> windows;
public:
	virtual int *GetKeys() { return keys; }
	virtual int *GetMouseStates() { return mouseStates; }
	Window(Engine *pEngine, const char *title, int width, int height, bool fullscreen=false, bool silent=false);
	static Window* GetWindow(HWND wnd);
	virtual void* GetHandle();
	virtual void SetTitle(const char *text);
	virtual void SetTitle(const wchar_t *text);
	virtual int GetKeyState(int key);
	virtual bool IsMouseDown(int mouseBtn);
	virtual bool IsKeyPressed(int key);
	virtual bool IsKeyUp(int key);
	virtual bool IsKeyDown(int key);
	virtual bool IsMovementKeys(float& zAngle);
	virtual int GetMouseState(int btn);
	virtual void KeepMouseDown(int btn);
	virtual bool ShouldClose();
	virtual void Close();
	virtual bool IsActive();
	virtual void GetCursorPos(double* x, double *y);
	virtual void ShowCursor();
	virtual void HideCursor();
	virtual void SetCursorPos(double x, double y);
	virtual void SwapBuffers();
	virtual bool Poll();
};