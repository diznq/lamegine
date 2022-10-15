struct bmp {
	char *data;
	int w;
	int h;
};
int bmp_create(struct bmp *self, int w, int h);
void bmp_destroy(struct bmp *self);
void bmp_set(struct bmp *self, int x, int y, int color);
int bmp_get(struct bmp *self, int x, int y);
void bmp_set_direct(struct bmp *self, int ptr, int color);
int bmp_get_direct(struct bmp *self, int ptr);
int bmp_write(struct bmp *self, const char *path);
int bmp_read(struct bmp *self, const char *path);