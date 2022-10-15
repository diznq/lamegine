#include "bmplib.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int bmp_create(struct bmp *self, int w, int h) {
	self->w = w;
	self->h = h;
	if (self->data) {
		free(self->data);
		self->data = 0;
	}
	self->data = (char*)malloc(w*h * 3);
	return self->data ? 1 : 0;
}
void bmp_destroy(struct bmp *self) {
	if (self && self->data) {
		free(self->data);
		self->data = 0;
	}
}
void bmp_set(struct bmp *self, int x, int y, int color) {
	if (!self || !self->data) return;
	if (x >= 0 && x < self->w && y >= 0 && y < self->h) {
		char rem = self->data[y*self->w * 3 + x * 3 + 3];
		color |= ((int)rem) << 24;
		*(int*)&self->data[y*self->w * 3 + x * 3] = color;
	}
}
int bmp_get(struct bmp *self, int x, int y) {
	if (!self || !self->data) return 0;
	if (x >= 0 && x < self->w && y >= 0 && y < self->h) {
		return (*(int*)&self->data[y*self->w * 3 + x * 3]) & 0xFFFFFF;
	}
	return 0;
}
int bmp_get_direct(struct bmp *self, int ptr) {
	if (!self || !self->data) return 0;
	if (ptr >= 0 && ptr < (self->w * self->h)) {
		return (*(int*)&self->data[ptr * 3]) & 0xFFFFFF;
	}
	return 0;
}
void bmp_set_direct(struct bmp *self, int ptr, int color) {
	if (!self || !self->data) return;
	if (ptr >= 0 && ptr < (self->w * self->h)) {
		char rem = self->data[ptr * 3 + 3];
		color |= ((int)rem) << 24;
		*(int*)&self->data[ptr * 3] = color;
	}
}
int bmp_write(struct bmp *self, const char *path) {
	if (!self || !self->data) {
		printf("error: unknown self: %p, %p", self, self ? self->data : 0);
		return 0;
	}
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
	if (!f) {
		printf("error: %s\n", strerror(errno));
		return 0;
	}
	fwrite(header, 54, 1, f);
	char padd[4] = { 0,0,0,0 };
	int padlen = (4 - ((self->w * 3) % 4)) & 3;
	for (int y = self->h - 1; y >= 0; y--) {
		fwrite(self->data + y * self->w * 3, self->w * 3, 1, f);
		fwrite(padd, padlen, 1, f);
	}
	int size = ftell(f);
	fseek(f, 2, SEEK_SET);
	fwrite(&size, 1, 4, f);
	int raw = size - 54;
	fseek(f, 0x22, SEEK_SET);
	fwrite(&raw, 1, 4, f);
	fseek(f, 18, SEEK_SET);
	fwrite(&self->w, 1, 4, f);
	fwrite(&self->h, 1, 4, f);
	fclose(f);
	return 1;
}
int bmp_read(struct bmp *self, const char *path) {
	if (!self) return 0;
	self->w = 0;
	self->h = 0;
	self->data = 0;
	FILE *f = fopen(path, "rb");
	if (!f) {
		printf("error: %s\n", strerror(errno));
		return 0;
	}
	unsigned char header[54];
	fread(header, 54, 1, f);
	self->w = (*(int*)(header + 18)) & 0xFFFFFFFF;
	self->h = (*(int*)(header + 22)) & 0xFFFFFFFF;
	char reversed = 0;
	if (self->w < 0) { self->w = -self->w; reversed = 1; }
	if (self->h < 0) { self->h = -self->h; reversed = 1; }
	printf("width: %d, height: %d\n", self->w, self->h);
	self->data = (char*)malloc(self->w*self->h * 3 + 24);
	int padlen = (4 - ((self->w * 3) % 4)) & 3;
	char padd[4] = { 0,0,0,0 };
	int i = 0;
	int begin = reversed ? 0 : (self->h - 1);
	int end = reversed ? self->h : 0;
	int step = reversed ? 1 : -1;
	for (i = begin; reversed ? i < end : i >= end; i += step) {
		fread(self->data + (self->w*i * 3), self->w * 3, 1, f);
		if (padlen > 0) fread(padd, padlen, 1, f);
	}
	printf("file read: %s\n", path);
	fclose(f);
	return 1;
}