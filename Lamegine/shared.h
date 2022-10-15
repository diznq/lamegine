#pragma once
#ifndef __SHARED_H__
#define __SHARED_H__
#define _CRT_SECURE_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN 
#define WIN32_LEAN_AND_MEAN 
#endif
#ifndef _ENABLE_EXTENDED_ALIGNED_STORAGE
#define _ENABLE_EXTENDED_ALIGNED_STORAGE
#endif
#include "Constants.h"
#include "Interfaces.h"

#include <Windows.h>
#include <D3D11.h>

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <list>
#include <map>
#include <deque>
#include <string>
#include <typeinfo>
#include <memory>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <wrl.h>
#include <ppltasks.h>

#ifdef _MSC_VER
#pragma comment(lib, "gdi32")
#pragma comment(lib, "user32")
#pragma comment(lib, "kernel32")
#pragma comment(lib, "D3D11")
#pragma comment(lib, "DirectXTK")
#pragma comment(lib, "ScriptingAPI")
#endif

#define GLFN(...) 1

typedef Ptr<IObject3D> IObject3DPtr;

using std::shared_ptr;
using std::cout;
using std::endl;
using std::hex;
using std::dec;
using std::make_shared;

#define CullFront 1
#define CullBack 2
#define CullBoth (CullFront|CullBack)

using Math::Vec2;
using Math::Vec3;
using Math::Vec4;
using Math::Mat4;

template <class T> static void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

#define INDEX_FLAG DXGI_FORMAT_R32_UINT

const char *toHuman(float memUsage);
std::string fixPath(std::string path);
void fixSzPath(char *path);
void generateTangents(std::vector<Vertex>& vertices, std::vector<Index_t>& indices);

void ltrim(std::string &s);
void rtrim(std::string &s);
void trim(std::string &s);

#endif