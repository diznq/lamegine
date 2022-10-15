#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

// Really simple BMP reader implementation
class BMP {
private:
    int w=0, h=0;
    char *data=0;
public:
    BMP(int w, int h) {
        this->w = w;
        this->h = h;
        if (data) {
            free(data);
            data = 0;
        }
        data = (char*)malloc(w * h * 3);
    }

    BMP(const char *path) {
        w = 0;
        h = 0;
        data = 0;
        FILE *f = fopen(path, "rb");
        unsigned char header[54];
        fread(header, 54, 1, f);
        w = (*(int*)(header + 18)) & 0xFFFFFFFF;
        h = (*(int*)(header + 22)) & 0xFFFFFFFF;
		char reversed = 0;
        if (w < 0) { w = -w; reversed = 1; }
        if (h < 0) { h = -h; reversed = 1; }
        data = (char*)malloc(w*h * 3 + 24);
        int padlen = (4 - ((w * 3) % 4)) & 3;
        char padd[4] = { 0,0,0,0 };
        int i = 0;
        int begin = reversed ? 0 : (h - 1);
        int end = reversed ? h : 0;
        int step = reversed ? 1 : -1;
		fseek(f, int(header[10]),SEEK_SET);
        for (i = begin; reversed ? i < end : i >= end; i += step) {
            fread(data + (w*i * 3), w * 3, 1, f);
            if (padlen > 0) fread(padd, padlen, 1, f);
        }
        fclose(f);
    }
    
    void set(int x, int y, int color) const {
        if(x < 0 || x>=w || y < 0 || y>= h) return;
        char rem = data[y*w * 3 + x * 3 + 3];
        color |= ((int)rem) << 24;
        *(int*)&data[y * w * 3 + x * 3] = color;
    }

    int get(int x, int y) const {
        if(x < 0 || x>=w || y < 0 || y>= h) return 0;
        return (*(int*)&data[y*w * 3 + x * 3]) & 0xFFFFFF;
    }

    int get_direct(int ptr) const {
        return (*(int*)&data[ptr * 3]) & 0xFFFFFF;
    }

    void set_direct(int ptr, int color) const {
        char rem = data[ptr * 3 + 3];
        color |= ((int)rem) << 24;
        *(int*)&data[ptr * 3] = color;
    }

    void write(const char *path) {
        unsigned char header[54] = {
            0x42, 0x4d,
            0x00, 0x00, 0x00, 0x00,  //size
            0x00, 0x00,
            0x00, 0x00,
            0x36, 0x00, 0x00, 0x00,

            0x28, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,  //w
            0x00, 0x00, 0x00, 0x00,  //h
            0x01, 0x00,
            0x18, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x10, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00
        };
        FILE *f = fopen(path, "wb+");
        fwrite(header, 54, 1, f);
        char padd[4] = { 0,0,0,0 };
        int padlen = (4 - ((w * 3) % 4)) & 3;
        for (int y = h - 1; y >= 0; y--) {
            fwrite(data + y * w * 3, w * 3, 1, f);
            fwrite(padd, padlen, 1, f);
        }
        int size = ftell(f);
        fseek(f, 2, SEEK_SET);
        fwrite(&size, 1, 4, f);
        int raw = size - 54;
        fseek(f, 0x22, SEEK_SET);
        fwrite(&raw, 1, 4, f);
        fseek(f, 18, SEEK_SET);
        fwrite(&w, 1, 4, f);
        fwrite(&h, 1, 4, f);
        fclose(f);
    }

    int getWidth() const {
        return w;
    }

    int getHeight() const {
        return h;
    }
};