#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include "helpers_pixelfunc.h"

// blit one surface into another at a specific location
extern int BlitAt( SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dest, int x, int y, bool bcenterx, bool bcentery )
{
	SDL_Rect *offsetrect = malloc(sizeof(SDL_Rect));
	offsetrect->x = bcenterx ? (x-src->w/2) : x;
	offsetrect->y = bcentery ? (y-src->h/2) : y;
	offsetrect->w = src->w;
	offsetrect->h = src->h;
	int result = SDL_BlitSurface(src,srcrect,dest,offsetrect);
	free(offsetrect);
	return result;
}

// Create a surface which has the RGB values of the base surface and the Alpha values made from an average between the RGB values of the alpha mask surface
// Both input surfaces must have the same dimensions
extern SDL_Surface *TransferAlpha( SDL_Surface *base, SDL_Surface *alpha )
{
	SDL_Surface *_outsurf = SDL_CreateRGBSurface(SDL_SWSURFACE,base->w,base->h,base->format->BitsPerPixel,base->format->Rmask,base->format->Gmask,base->format->Bmask,base->format->Amask);
	SDL_Surface *outsurf = SDL_DisplayFormatAlpha(_outsurf);
	SDL_FreeSurface(_outsurf);
	long int pix, piy;
	Uint32 pxx, pxx2, pxx3;
	Uint8 R,G,B,A;
	Uint8 A1,A2,A3,A4,A5;
	SDL_LockSurface(outsurf);
	SDL_LockSurface(base);
	SDL_LockSurface(alpha);
	pix = 0;
	piy = 0;
	while ( (pix < outsurf->w) && (piy < outsurf->h) )
	{
		pxx = getpixel(base,pix,piy);
		pxx2 = getpixel(alpha,pix,piy);
		SDL_GetRGBA(pxx,base->format,&R,&G,&B,&A5);
		SDL_GetRGBA(pxx2,alpha->format,&A1,&A2,&A3,&A4);
		A = (A1/3)+(A2/3)+(A3/3);
		A = (int)((((float)A/255.0*(float)A4)/255.0)*(float)A5);
		pxx3 = SDL_MapRGBA(outsurf->format,R,G,B,A);
		putpixel(outsurf,pix,piy,pxx3);
		pix++;
		if ( pix < outsurf->w )
			continue;
		pix = 0;
		piy++;
	}
	SDL_UnlockSurface(outsurf);
	SDL_UnlockSurface(base);
	SDL_UnlockSurface(alpha);
	return outsurf;
}

// easy text rendering
extern void RenderText( SDL_Surface *dest, TTF_Font *font, const char *text, SDL_Rect *rc, SDL_Color colr, int offx, int offy, int *outw, int *outh )
{
	int width = 12, height = 16;
	const SDL_Color whitecol = {255,255,255,255};
	if ( (text == NULL) || (strlen(text) == 0) )
	{
		fprintf(stderr,"line is empty\n");
		return;
	}
	TTF_SizeText(font,text,&width,&height);
	SDL_Surface *stext = SDL_CreateRGBSurface(SDL_SWSURFACE,width,height,dest->format->BitsPerPixel,dest->format->Rmask,dest->format->Gmask,dest->format->Bmask,dest->format->Amask);
	if ( stext == NULL )
	{
		fprintf(stderr,"couldn't create text box surface (%s)\n", SDL_GetError());
		return;
	}
	SDL_Rect *temprect = malloc(sizeof(SDL_Rect));
	temprect->x = 0;
	temprect->y = 0;
	temprect->w = width;
	temprect->h = height;
	SDL_FillRect(stext,temprect,SDL_MapRGB(stext->format,0,0,0));
	SDL_Surface *stemp = NULL;
	stemp = TTF_RenderUTF8_Blended(font, text, whitecol);
	if ( SDL_BlitSurface(stemp,NULL,stext,temprect) == -1 )
	{
		fprintf(stderr,"error blitting line (%s)\n", SDL_GetError());
		SDL_FreeSurface(stext);
		SDL_FreeSurface(stemp);
		free(temprect);
		return;
	}
	free(temprect);
	temprect = NULL;
	SDL_FreeSurface(stemp);
	SDL_Surface *stext2 = NULL;
	SDL_Surface *fbox = SDL_CreateRGBSurface(SDL_SWSURFACE,width,height,dest->format->BitsPerPixel,dest->format->Rmask,dest->format->Gmask,dest->format->Bmask,dest->format->Amask);
	SDL_FillRect(fbox,temprect,SDL_MapRGB(fbox->format,colr.r,colr.g,colr.b));
	stext2 = TransferAlpha(fbox,stext);
	SDL_FreeSurface(fbox);
	temprect = malloc(sizeof(SDL_Rect));
	if ( rc == NULL )
	{
		temprect->x = offx;
		temprect->y = offy;
	}
	else
	{
		temprect->x = rc->x+offx;
		temprect->y = rc->y+offy;
	}
	temprect->w = width;
	temprect->h = height;
	if ( SDL_BlitSurface(stext2,NULL,dest,temprect) == -1 )
	{
		fprintf(stderr,"error blitting text box (%s)\n", SDL_GetError());
		SDL_FreeSurface(stext2);
		free(temprect);
		return;
	}
	SDL_FreeSurface(stext2);
	free(temprect);
	if ( outw != NULL )
		*outw = width;
	if ( outh != NULL )
		*outh = height;
}

// quickly upscale a SDL_Surface with nearest-neighbor filtering
extern SDL_Surface* Upscale(SDL_Surface *base, uint16_t factor)
{
	if ( factor == 0 )
		factor = 1;
	SDL_Surface *temp = SDL_CreateRGBSurface(SDL_SWSURFACE,(base->w)*factor,(base->h)*factor,base->format->BitsPerPixel,base->format->Rmask,base->format->Gmask,base->format->Bmask,base->format->Amask);
	SDL_Surface *final = SDL_DisplayFormatAlpha(temp);
	SDL_FreeSurface(temp);
	SDL_LockSurface(base);
	SDL_LockSurface(final);
	long int px, py, nx, ny, i, j;
	uint32_t pux, pnx;
	uint8_t R,G,B,A;
	px = 0;
	py = 0;
	nx = 0;
	ny = 0;
	do
	{
		pux = getpixel(base,px,py);
		SDL_GetRGBA(pux,base->format,&R,&G,&B,&A);
		pnx = SDL_MapRGBA(final->format,R,G,B,A);
		for ( i=0;i<factor;i++ )
			for ( j=0;j<factor;j++ )
				putpixel(final,nx+j,ny+i,pnx);
		nx+=factor;
		px++;
		if ( px >= base->w )
		{
			px = 0;
			nx = 0;
			ny+=factor;
			py++;
		}
	}
	while ( (px < base->w) && (py < base->h) );
	SDL_UnlockSurface(final);
	SDL_UnlockSurface(base);
	return final;
}

// crop one surface to a new one, with tiling when needed
extern SDL_Surface* Crop(SDL_Surface *base, long int ox, long int oy, long int nw, long int nh)
{
	SDL_Surface *tform = NULL;
	SDL_Surface *ttmp = SDL_CreateRGBSurface(SDL_SWSURFACE,nw,nh,base->format->BitsPerPixel,base->format->Rmask,base->format->Gmask,base->format->Bmask,base->format->Amask);
	tform = SDL_DisplayFormatAlpha(ttmp);
	SDL_FreeSurface(ttmp);
	while ( ox < 0 )
		ox += base->w;
	while ( oy < 0 )
		oy += base->h;
	while ( ox >= base->w )
		ox -= base->w;
	while ( oy >= base->h )
		oy -= base->h;
	if ( nw <= 0 || nh <= 0 )
		return tform;
	long int px, py, nx, ny;
	uint32_t pux, pnx;
	uint8_t R,G,B,A;
	px = ox;
	py = oy;
	nx = 0;
	ny = 0;
	SDL_LockSurface(base);
	SDL_LockSurface(tform);
	do
	{
		pux = getpixel(base,px,py);
		SDL_GetRGBA(pux,base->format,&R,&G,&B,&A);
		pnx = SDL_MapRGBA(tform->format,R,G,B,A);
		putpixel(tform,nx,ny,pnx);
		nx++;
		px++;
		if ( px >= base->w )
			px = ox;
		if ( nx >= nw )
		{
			nx = 0;
			px = ox;
			ny++;
			py++;
			if ( py >= base->h )
				py = oy;
		}
	}
	while ( (nx < nw) && (ny < nh) );
	SDL_UnlockSurface(tform);
	SDL_UnlockSurface(base);
	return tform;
}

// Mirror a sprite horizontally
extern SDL_Surface* MirrorSprite(SDL_Surface *base)
{
	SDL_Surface *temp = SDL_CreateRGBSurface(SDL_SWSURFACE,base->w,base->h,base->format->BitsPerPixel,base->format->Rmask,base->format->Gmask,base->format->Bmask,base->format->Amask);
	SDL_Surface *final = SDL_DisplayFormatAlpha(temp);
	SDL_FreeSurface(temp);
	SDL_LockSurface(base);
	SDL_LockSurface(final);
	long int px, py, nx, ny;
	uint32_t pux, pnx;
	uint8_t R,G,B,A;
	px = 0;
	py = 0;
	nx = base->w-1;
	ny = 0;
	do
	{
		pux = getpixel(base,px,py);
		SDL_GetRGBA(pux,base->format,&R,&G,&B,&A);
		pnx = SDL_MapRGBA(final->format,R,G,B,A);
		putpixel(final,nx,ny,pnx);
		nx--;
		px++;
		if ( px >= base->w )
		{
			px = 0;
			nx = base->w-1;
			ny++;
			py++;
		}
	}
	while ( (px < base->w) && (py < base->h) );
	SDL_UnlockSurface(final);
	SDL_UnlockSurface(base);
	return final;
}
