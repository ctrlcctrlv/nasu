#ifndef HELPERS_PIXELFUNC_H
#define HELPERS_PIXELFUNC_H
#ifdef __cplusplus
extern "C" {
#endif
Uint32 getpixel(SDL_Surface *surface, int x, int y);
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
#ifdef __cplusplus
}
#endif
#endif
