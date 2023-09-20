#define NANOSVG_IMPLEMENTATION
#include <nanosvg/src/nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvg/src/nanosvgrast.h>
#include <ext_lib.h>
#include <ext_vector.h>

typedef struct Svg {
	void*      data;
	NSVGimage* img;
} Svg;

Svg* Svg_New(const void* mem, size_t size) {
	Svg* this = new(Svg);
	
	this->data = memdup(mem, size);
	osAssert(this->data);
	this->img = nsvgParse(this->data, "px", 96);
	osAssert(this->img);
	
	return this;
}

void* Svg_Rasterize(Svg* this, f32 scale, Rect* ovrr) {
	Rect r = { .w = this->img->width, .h = this->img->height };
	
	if (ovrr && ovrr->w && ovrr->h) r = *ovrr;
	
	r.x *= scale;
	r.y *= scale;
	r.w *= scale;
	r.h *= scale;
	
	int channels = 4;
	int size = r.w * r.h * channels;
	u8* data = new(u8[size]);
	
	osAssert(data);
	
	NSVGrasterizer* ras = nsvgCreateRasterizer();
	nsvgRasterize(ras, this->img, -r.x, -r.y, scale, data, r.w, r.h, r.w * channels);
	nsvgDeleteRasterizer(ras);
	
	if (ovrr) *ovrr = r;
	
	return data;
}

void Svg_Delete(Svg* this) {
	nsvgDelete(this->img);
	delete(this->data, this);
}
