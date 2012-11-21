#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include "helpers_drawextras.h"
#include "helpers_nasudef.h"

// Process actor and put it on screen
extern void RenderActor(SDL_Surface *screen, NASU_Actor *a)
{
	SDL_Surface *pass1 = Crop(a->graph,0,a->animframe*a->res.h,a->res.w,a->res.h);
	SDL_Surface *final = Upscale(pass1,a->scale);
	SDL_FreeSurface(pass1);
	SDL_Rect *finalrect = NULL;
	SDL_GetClipRect(final,finalrect);
	BlitAt(final,finalrect,screen,a->pos.x-(a->pivot.x*a->scale),a->pos.y-(a->pivot.y*a->scale),false,false);
	SDL_FreeSurface(final);
}

// Process player and put it on screen
extern void RenderPlayer(SDL_Surface *screen, NASU_Player *p)
{
	SDL_Surface *pass1 = Crop(p->graph,0,p->animframe*p->res.h,p->res.w,p->res.h);
	SDL_Surface *final = NULL;
	if ( p->bLeft )
	{
		SDL_Surface *pass2 = MirrorSprite(pass1);
		SDL_FreeSurface(pass1);
		final = Upscale(pass2,p->scale);
		SDL_FreeSurface(pass2);
	}
	else
	{
		final = Upscale(pass1,p->scale);
		SDL_FreeSurface(pass1);
	}
	SDL_Rect *finalrect = NULL;
	SDL_GetClipRect(final,finalrect);
	BlitAt(final,finalrect,screen,p->pos.x-(p->pivot.x*p->scale),p->pos.y-(p->pivot.y*p->scale),false,false);
	SDL_FreeSurface(final);
}

extern float CalcTextW(NASU_ScrnText *t)
{
	int w = 0, h = 0;
	if ( (t->text == NULL) || (strlen(t->text) == 0) )
		return 0.f;
	TTF_SizeText(t->font,t->text,&w,&h);
	return (float)w;
}

extern float CalcTextH(NASU_ScrnText *t)
{
	int w = 0, h = 0;
	if ( (t->text == NULL) || (strlen(t->text) == 0) )
		return 0.f;
	TTF_SizeText(t->font,t->text,&w,&h);
	return (float)h;
}

// Process on-screen text and put it on screen
extern void RenderScrnText(SDL_Surface *screen, NASU_ScrnText *t)
{
	RenderText(screen,t->font,t->text,NULL,t->colr,t->pos.x,t->pos.y,NULL,NULL);
}

extern bool CollidePlayer(NASU_Actor *a, NASU_Player *p)
{
	return ((a->pos.x+a->tbox.x1 >= p->pos.x+p->tbox.x1) && (a->pos.x+a->tbox.x2 <= p->pos.x+p->tbox.x2) && (a->pos.y+a->tbox.y1 >= p->pos.y+p->tbox.y1) && (a->pos.y+a->tbox.y2 <= p->pos.y+p->tbox.y2));
}

extern void MoveActor(NASU_Actor *a, float deltatime)
{
	a->pos.x += a->vel.x*deltatime;
	a->pos.y += a->vel.y*deltatime;
}


extern void MovePlayer(NASU_Player *p, float deltatime)
{
	p->pos.x += p->vel.x*deltatime;
	p->pos.y += p->vel.y*deltatime;
}
/*
#define lerp(a,b,c) (c*b+(1-c)*a)

// TODO finish this thingy so I don't have to stretch the sounds externally
// an attempt to make some simple sound stretching with linear interpolation
// expects audio in signed 16-bit stereo
extern Mix_Chunk* SndStretch(Mix_Chunk *in, float factor)
{
	int i = 0, j = 0;
	Uint16 *inchan[2] = {NULL,NULL};
	Uint16 *outchan[2] = {NULL,NULL};
	Uint32 inlen = 0, outlen = 0;
	
	// separate sample data in two channels
	inchan[0] = (Uint16*)malloc(size_t(ceil(in->alen/2)));
	inchan[1] = (Uint16*)malloc(size_t(ceil(in->alen/2)));
	inlen = (Uint32)ceil(in->alen/2);
	// allocate needed memory for output sample data
	outchan[0] = (Uint16*)malloc(size_t(ceil((in->alen/2)/factor)));
	outchan[0] = (Uint16*)malloc(size_t(ceil((in->alen/2)/factor)));
	outlen = (Uint32)ceil((in->alen/2)/factor);
	j = 0;
	for ( i=0; i<in->alen; i+=4 )	// copy input
	{
		inchan[0][j] = (Uint16)in->abuf[i];
		inchan[1][j] = (Uint16)in->abuf[i+2];
		j++;
	}
	
	Mix_Chunk *out = NULL;
	float step = 0.f;
	Uint16 isample_l[2] = {0,0};
	Uint16 osample_l = 0;
	Uint16 isample_r[2] = {0,0};
	Uint16 osample_r = 0;
	out = (Mix_Chunk*)malloc(sizeof(Mix_Chunk));
	out->allocated = in->allocated;
	out->volume = in->volume;
	if ( factor > 0.0 )	// stretch in normal direction
	{
		while (ceil(step) < inlen)
		{
			isample_l[0] = inchan[0][int(floor(step))];
			isample_l[1] = inchan[0][int(ceil(step))];
			isample_r[0] = inchan[1][int(floor(step))];
			isample_r[1] = inchan[1][int(ceil(step))];
			osample_l = lerp(isample_l[0],isample_l[1],(step-floor(step)));
			osample_r = lerp(isample_r[0],isample_r[1],(step-floor(step)));
			step += factor;
		}
		out->abuf = (Uint8*)malloc(4);
		memset(out->abuf, 0, 4);
		out->alen = 4;
		for ( i=0; i<outlen; i++ )
		{
			
		}
	}
	else	// just send a blank sample
	{
		out->abuf = (Uint8*)malloc(4);
		memset(out->abuf, 0, 4);
		out->alen = 4;
	}
}*/
