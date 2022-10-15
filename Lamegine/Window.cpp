#include "shared.h"
#include "Window.h"
#include "Engine.h"
#include <windowsx.h>

std::map<HWND, Window*> Window::windows;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

Window::Window(Engine *engine, const char *title, int width, int height, bool fullscreen, bool silent) {
	for (int i = 0; i < 256; i++) keys[i] = KEY_UP;
	
	mouseStates[MOUSE_LEFT] = MOUSE_UP;
	mouseStates[MOUSE_RIGHT] = MOUSE_UP;
	
	cursorShown = true;
	
	pEngine = engine;
	w = width;
	h = height;
	fs = fullscreen;
	appClose = false;
	
	HINSTANCE hInst = GetModuleHandle(0);
	internal_handle = 0;
	
	WNDCLASSEX wcex;
	
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = (HICON)LoadIconW(hInst, IDI_APPLICATION);
	wcex.hCursor = (HCURSOR)LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"Lamegine";
	wcex.hIconSm = LoadIconW(wcex.hInstance, IDI_APPLICATION);
	
	if (!RegisterClassEx(&wcex))
	{
		Engine::Terminate(L"Call to RegisterClassEx failed!");
		Close();
		return;
	}
	
	RECT rect;
	rect.left = 0;
	rect.right = w;
	rect.top = 0;
	rect.bottom = h;

	uint style = WS_OVERLAPPEDWINDOW;
	if (silent) style = WS_POPUP;// | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;

	if(!silent) AdjustWindowRect(&rect, style, false);
	
	internal_handle = CreateWindow( L"Lamegine", L"Lamegine 1.0", style, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInst, NULL );
	if (!internal_handle)
	{
		Engine::Terminate(L"Call to CreateWindow failed!");
		return;
	}

	ShowWindow(internal_handle, SW_SHOW);
	UpdateWindow(internal_handle);
	
	windows[internal_handle] = this;
}

bool Window::Poll() {
	static MSG msg;
	bool state = true;
	if (PeekMessage(&msg, internal_handle, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_QUIT) Close();
	} else {
		state = false;
	}
	return state;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Window *window = Window::GetWindow(hWnd);
	if (!window) return DefWindowProc(hWnd, message, wParam, lParam);
	Engine *eng = Engine::getInstance();
	int *mouseStates = window->GetMouseStates();
	int *keys = window->GetKeys();
	switch (message)
	{
		case WM_MOUSEWHEEL:
		if (eng) eng->onScroll(GET_WHEEL_DELTA_WPARAM(wParam));
		break;
		case WM_LBUTTONDOWN:
		mouseStates[MOUSE_LEFT] = MOUSE_DOWN;
		if (eng) eng->onMouseDown(MOUSE_LEFT);
		break;
		case WM_LBUTTONUP:
		mouseStates[MOUSE_LEFT] = MOUSE_PRESS;
		if (eng) eng->onMouseUp(MOUSE_LEFT);
		break;
		case WM_RBUTTONDOWN:
		mouseStates[MOUSE_RIGHT] = MOUSE_DOWN;
		if (eng) eng->onMouseDown(MOUSE_RIGHT);
		break;
		case WM_RBUTTONUP:
		mouseStates[MOUSE_RIGHT] = MOUSE_PRESS;
		if (eng) eng->onMouseUp(MOUSE_RIGHT);
		break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		if (keys[wParam & 0xFF] != KEY_PRESS)
		keys[wParam & 0xFF] = KEY_DOWN;
		if (eng) eng->onKeyDown(wParam & 0xFF);
		//printf("key down: %d\n", wParam);
		break;
		case WM_KEYUP:
		case WM_SYSKEYUP:
		keys[wParam & 0xFF] = KEY_UP;
		if (eng) eng->onKeyUp(wParam & 0xFF);
		//printf("key up: %d\n", wParam);
		break;
		case WM_CLOSE:
		case WM_DESTROY:
		window->Close();
		PostQuitMessage(0);
		break;
		default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
	return 0;
}

bool Window::IsKeyDown(int key) {
	return keys[key & 0xFF] != KEY_UP;
}

void Window::Close() {
	appClose = true;
}

void* Window::GetHandle() { return internal_handle; }

void Window::SetTitle(const char *text) {
	SetWindowTextA(internal_handle, text);
}

bool Window::IsMouseDown(int mouseBtn) {
	return mouseStates[mouseBtn] == MOUSE_DOWN;
}

int Window::GetKeyState(int key) {
	return keys[key & 0xFF]?KEY_PRESS:0;
}

void Window::SwapBuffers() {
	//...
}

void Window::ShowCursor() {
	if (!cursorShown) {
		cursorShown = true;
		::ShowCursor(true);
	}
}

bool Window::ShouldClose() {
	return appClose;
}

void Window::GetCursorPos(double* x, double *y) {
	POINT pnt;
	::GetCursorPos(&pnt);
	ScreenToClient(internal_handle, &pnt);
	*x = (double)pnt.x;
	*y = (double)pnt.y;
}

void Window::HideCursor() {
	if (cursorShown) {
		cursorShown = false;
		::ShowCursor(false);
	}
}

bool Window::IsActive() {
	return GetActiveWindow() == internal_handle;
}

bool Window::IsKeyUp(int key) {
	return keys[key & 0xFF] == KEY_UP;
}

bool Window::IsKeyPressed(int key) {
	key &= 0xFF;
	if (keys[key] == KEY_DOWN) {
		keys[key] = KEY_PRESS;
		return true;
	}
	return false;
}

bool Window::IsMovementKeys(float& zAngle) {
	bool isW = IsKeyDown('W');
	bool isS = IsKeyDown('S');
	bool isA = IsKeyDown('A');
	bool isD = IsKeyDown('D');
	int fwd = 0;
	int side = 0;
	if (isW) fwd += 1;
	if (isS) fwd -= 1;
	if (isA) side -= 1;
	if (isD) side += 1;
	if (!fwd && !side) return false;
	
	switch (fwd) {
		case 1:
		switch (side) {
			case -1:
			zAngle = 45.0f;
			break;
			case 0:
			zAngle = 0.0f;
			break;
			case 1:
			zAngle = -45.0f;
			break;
		}
		break;
		case 0:
		switch (side) {
			case 1:
			zAngle = -90.0f;
			break;
			case -1:
			zAngle = 90.0f;
			break;
		}
		break;
		case -1:
		switch (side) {
			case -1:
			zAngle = 135.0f;
			break;
			case 0:
			zAngle = 180.0f;
			break;
			case 1:
			zAngle = 225.0f;
			break;
		}
		break;
	}
	
	return true;
}

Window* Window::GetWindow(HWND wnd) {
	auto it = windows.find(wnd);
	if (it == windows.end()) return 0;
	return it->second;
}

void Window::KeepMouseDown(int btn) {
	mouseStates[btn] = MOUSE_PRESS;
}

int Window::GetMouseState(int btn) {
	if (mouseStates[btn] == MOUSE_PRESS) {
		mouseStates[btn] = MOUSE_UP;
		return MOUSE_PRESS;
	}
	return mouseStates[btn];
}

void Window::SetCursorPos(double x, double y) {
	POINT pnt;
	pnt.x = (int)x;
	pnt.y = (int)y;
	ClientToScreen(internal_handle, &pnt);
	::SetCursorPos(pnt.x, pnt.y);
}

void Window::SetTitle(const wchar_t *text) {
	SetWindowTextW(internal_handle, text);
}

