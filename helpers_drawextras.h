#ifndef HELPERS_DRAWEXTRAS_H
#define HELPERS_DRAWEXTRAS_H
#ifdef __cplusplus
extern "C" {
#endif
int BlitAt(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dest, int x, int y, bool bcenterx, bool bcentery);
SDL_Surface *TransferAlpha(SDL_Surface *base, SDL_Surface *alpha);
void RenderText(SDL_Surface *dest, TTF_Font *font, const char *text, SDL_Rect *rc, SDL_Color colr, int offx, int offy, int *outw, int *outh);
SDL_Surface* Upscale(SDL_Surface *base, uint16_t factor);
SDL_Surface* Crop(SDL_Surface *base, long int ox, long int oy, long int nw, long int nh);
SDL_Surface* MirrorSprite(SDL_Surface *base);
#ifdef __cplusplus
}
#endif
#endif
