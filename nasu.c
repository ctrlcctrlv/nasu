#include "nasu.h"
#include "nasudef.h"

// some redefinitions needed because Windows is weird
#ifdef WINDOWS
	#define strcasecmp stricmp
	#define strncasecmp strnicmp
#endif

uint32_t score = 0;

////// helper functions start

/*
 * Return the pixel value at (x, y)
 * NOTE: The surface must be locked before calling this!
 */
Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
        return *(Uint32 *)p;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}

// blit one surface into another at a specific location
int BlitAt(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dest, int x, int y, bool bcenterx, bool bcentery)
{
    SDL_Rect *offsetrect = (SDL_Rect*)malloc(sizeof(SDL_Rect));
    offsetrect->x = x;
    offsetrect->y = y;
    offsetrect->w = src->w;
    offsetrect->h = src->h;
    if ( bcenterx )
        offsetrect->x -= src->w / 2;
    if ( bcentery )
        offsetrect->y -= src->h / 2;
    return SDL_BlitSurface(src,srcrect,dest,offsetrect);
}

// Create a surface which has the RGB values of the base surface and the Alpha values made from an average between the RGB values of the alpha mask surface
// Both input surfaces must have the same dimensions
SDL_Surface *TransferAlpha(SDL_Surface *base, SDL_Surface *alpha)
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
    do
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
        if ( pix >= outsurf->w )
        {
            pix = 0;
            piy++;
        }
    } while ( (pix < outsurf->w) && (piy < outsurf->h) );
    SDL_UnlockSurface(outsurf);
    SDL_UnlockSurface(base);
    SDL_UnlockSurface(alpha);
    return outsurf;
}

// easy text rendering
void RenderText(SDL_Surface *dest, TTF_Font *font, const char *text, SDL_Rect *rc, SDL_Color colr, int offx, int offy, int *outw, int *outh)
{
    int width = 12, height = 16;
    const SDL_Color whitecol = {255,255,255,255};

    if ( (text == NULL) || (strlen(text) == 0) )
    {
        fprintf(stderr,"line is empty\n");
        fflush(stderr);
        return;
    }

    TTF_SizeText(font,text,&width,&height);
    SDL_Surface *stext = SDL_CreateRGBSurface(SDL_SWSURFACE,width,height,dest->format->BitsPerPixel,dest->format->Rmask,dest->format->Gmask,dest->format->Bmask,dest->format->Amask);
    if ( stext == NULL )
    {
        fprintf(stderr,"couldn't create text box surface (%s)\n", SDL_GetError());
        fflush(stderr);
        return;
    }
    SDL_Rect *temprect = (SDL_Rect*)malloc(sizeof(SDL_Rect));
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
        fflush(stderr);
        return;
    }
    SDL_FreeSurface(stemp);
    SDL_Surface *stext2 = NULL;
    SDL_Surface *fbox = SDL_CreateRGBSurface(SDL_SWSURFACE,width,height,dest->format->BitsPerPixel,dest->format->Rmask,dest->format->Gmask,dest->format->Bmask,dest->format->Amask);
    SDL_FillRect(fbox,temprect,SDL_MapRGB(fbox->format,colr.r,colr.g,colr.b));
    stext2 = TransferAlpha(fbox,stext);
    SDL_FreeSurface(fbox);
    temprect = (SDL_Rect*)malloc(sizeof(SDL_Rect));
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
        fflush(stderr);
        return;
    }
    SDL_FreeSurface(stext2);
    if ( outw != NULL )
        *outw = width;
    if ( outh != NULL )
        *outh = height;
}

// quickly upscale a SDL_Surface with nearest-neighbor filtering
SDL_Surface* Upscale(SDL_Surface *base, uint16_t factor)
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
	} while ( (px < base->w) && (py < base->h) );
	SDL_UnlockSurface(final);
	SDL_UnlockSurface(base);
	
	return final;
}

// crop one surface to a new one, with tiling when needed
SDL_Surface* Crop(SDL_Surface *base, long int ox, long int oy, long int nw, long int nh)
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
	} while ( (nx < nw) && (ny < nh) );
	SDL_UnlockSurface(tform);
	SDL_UnlockSurface(base);
	
	return tform;
}

// Mirror a sprite horizontally
SDL_Surface* MirrorSprite(SDL_Surface *base)
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
	} while ( (px < base->w) && (py < base->h) );
	SDL_UnlockSurface(final);
	SDL_UnlockSurface(base);
	
	return final;
}

// Process actor and put it on screen
void RenderActor(SDL_Surface *screen, NASU_Actor *a)
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
void RenderPlayer(SDL_Surface *screen, NASU_Player *p)
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

float CalcTextW(NASU_ScrnText *t)
{
	int w = 0, h = 0;
	if ( (t->text == NULL) || (strlen(t->text) == 0) )
		return 0.f;
	TTF_SizeText(t->font,t->text,&w,&h);
	return (float)w;
}
float CalcTextH(NASU_ScrnText *t)
{
	int w = 0, h = 0;
	if ( (t->text == NULL) || (strlen(t->text) == 0) )
		return 0.f;
	TTF_SizeText(t->font,t->text,&w,&h);
	return (float)h;
}

// Process on-screen text and put it on screen
void RenderScrnText(SDL_Surface *screen, NASU_ScrnText *t)
{
	RenderText(screen,t->font,t->text,NULL,t->colr,t->pos.x,t->pos.y,NULL,NULL);
}

bool CollidePlayer(NASU_Actor *a, NASU_Player *p)
{
	return ((a->pos.x+a->tbox.x1 >= p->pos.x+p->tbox.x1) && (a->pos.x+a->tbox.x2 <= p->pos.x+p->tbox.x2) && (a->pos.y+a->tbox.y1 >= p->pos.y+p->tbox.y1) && (a->pos.y+a->tbox.y2 <= p->pos.y+p->tbox.y2));
}

void MoveActor(NASU_Actor *a, float deltatime)
{
	a->pos.x += a->vel.x*deltatime;
	a->pos.y += a->vel.y*deltatime;
}


void MovePlayer(NASU_Player *p, float deltatime)
{
	p->pos.x += p->vel.x*deltatime;
	p->pos.y += p->vel.y*deltatime;
}
/*
#define lerp(a,b,c) (c*b+(1-c)*a)

// TODO finish this thingy so I don't have to stretch the sounds externally
// an attempt to make some simple sound stretching with linear interpolation
// expects audio in signed 16-bit stereo
Mix_Chunk* SndStretch(Mix_Chunk *in, float factor)
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

// write scores to disk
// Windows: Own directory
// Linux: Running user's home directory
void savescore()
{
#ifdef WINDOWS
	FILE *scorefil = fopen("score.dat","wb");
#else
	char filen[256];
	snprintf(filen,256,"%s/.nasuscore",getenv("HOME"));
	FILE *scorefil = fopen(filen,"wb");
#endif
	fwrite((void *)&score,sizeof(unsigned long int),1,scorefil);
	fclose(scorefil);
}

////// helper functions end

int main(int argc, char **argv)
{
	const SDL_Color whitecol = {255,255,255,255};

	srand(time(NULL));	// a quick 'n dirty way of initializing random seed

	// load scores
#ifdef WINDOWS
	FILE *scorefil = fopen("score.dat","rb");
#else
	char filen[256];
	snprintf(filen,256,"%s/.nasuscore",getenv("HOME"));
	FILE *scorefil = fopen(filen,"rb");
	chdir("/opt/nasu");
#endif
	if ( scorefil == NULL )
	{
		score = 0;
	}
	else
	{
		fread((void *)&score,sizeof(unsigned long int),1,scorefil);
		fclose(scorefil);
	}

	// parameter parsing
	// -fullscreen specifies that the game is fullscreen
	// -res WxH specifies resolution
	// -noscale does not scale virtual screen
	float ScreenMult = 2;
	int resx = 640, resy = 480;
	bool bIsFullscreen = false;
	bool bNoScale = false;
	bool bNoSound = false;
	for ( int i=1; i<argc; i++ )
	{
		if ( strcasecmp(argv[i],"-fullscreen") == 0 )
			bIsFullscreen = true;
		else if ( strcasecmp(argv[i],"-res") == 0 )
		{
			// expect next parameter to be a resolution in format WxH
			if ( argc <= i )
			{
				fprintf(stderr,"Expected video resolution for parameter -res in format WxH\n");
				return 1;
			}
			else
			{
				sscanf(argv[i+1],"%dx%d",&resx,&resy);
				if ( resx < 320 )
					resx = 320;
				if ( resy < 240 )
					resy = 240;
				
			}
		}
		else if ( strcasecmp(argv[i],"-noscale") == 0 )
			bNoScale = true;
		else if ( strcasecmp(argv[i],"-nosound") == 0 )
			bNoSound = true;
	}

	// calculate screen mult based on specified resolution
	int sm[2];
	sm[0] = floor(resx/320);
	sm[1] = floor(resy/240);
	ScreenMult = (sm[0]>sm[1])?sm[1]:sm[0];
	
	if ( bNoScale )
		ScreenMult = 1;

	atexit(savescore);
	if ( SDL_Init(SDL_INIT_EVERYTHING) == -1 )
 	{
		fprintf(stderr,"couldn't initialize SDL (%s)\n", SDL_GetError());
		return 1;
	}

	Uint8 blankcursor = 0;
	Uint8 blankcursormask = 0;
	SDL_Cursor *blankcur = SDL_CreateCursor(&blankcursor,&blankcursormask,1,1,0,0);
	SDL_SetCursor(blankcur);
	
	// load program icon
	SDL_Surface *icone = IMG_Load("nasuicon.png");
	if ( icone != NULL )
		SDL_WM_SetIcon(icone,NULL);
	SDL_WM_SetCaption("NASU","NASU");	// I don't really know why the two parameters but oh well~
	SDL_Surface *mainscreen = NULL;
	// Create window. If SDL can't set a fullscreen mode it'll fall back to windowed
	if ( bIsFullscreen && (SDL_VideoModeOK(resx,resy,32,SDL_SWSURFACE|SDL_FULLSCREEN) != 0) )
	{
		mainscreen = SDL_SetVideoMode(resx,resy,32,SDL_SWSURFACE|SDL_FULLSCREEN);
	}
	else
	{
		bIsFullscreen = false;
		mainscreen = SDL_SetVideoMode(resx,resy,32,SDL_SWSURFACE);
	}

	if ( mainscreen == NULL )
	{
		fprintf(stderr,"couldn't create window (%s)\n", SDL_GetError());
		fflush(stderr);
		return 1;
	}
	SDL_Rect *scrrect = NULL;
	SDL_GetClipRect(mainscreen,scrrect);
	
	// real screen that will be resized to fit
	SDL_Surface *_realscreen = SDL_CreateRGBSurface(SDL_SWSURFACE,320,240,mainscreen->format->BitsPerPixel,mainscreen->format->Rmask,mainscreen->format->Gmask,mainscreen->format->Bmask,mainscreen->format->Amask);
	SDL_Surface *realscreen = SDL_DisplayFormatAlpha(_realscreen);
	SDL_FreeSurface(_realscreen);
	
	SDL_Rect *rscrrect = NULL;
	SDL_GetClipRect(realscreen,rscrrect);

	// I love this little thing from SDL_gfx
	// Try to keep framerate at 60FPS most
	FPSmanager *frameman = (FPSmanager*)malloc(sizeof(FPSmanager));
	SDL_initFramerate(frameman);
	SDL_setFramerate(frameman,60);
	
	// load up player
	SDL_Surface *_playersprites = IMG_Load("player.png");
	SDL_Surface *playersprites = SDL_DisplayFormatAlpha(_playersprites);
	SDL_FreeSurface(_playersprites);
	NASU_Player player;
	player.graph = playersprites;
	player.res.w = 24;
	player.res.h = 24;
	player.pivot.x = 12.f;
	player.pivot.y = 12.f;
	player.pos.x = 160.f;
	player.pos.y = 149.f;
	player.vel.x = 0.f;
	player.vel.y = 0.f;
	player.tbox.x1 = -12.f;
	player.tbox.y1 = -4.f;
	player.tbox.x2 = 12.f;
	player.tbox.y2 = 12.f;
	player.dim.w = 24.f;
	player.dim.h = 24.f;
	player.scale = 1;
	player.bIsJumping = false;
	player.animframe = 0;
	player.bLeft = false;
	
	// load up main frame
	SDL_Surface *_mainframeimg = IMG_Load("mainscreen.png");
	SDL_Surface *mainframeimg = SDL_DisplayFormatAlpha(_mainframeimg);
	SDL_FreeSurface(_mainframeimg);
	NASU_Actor mainframe;
	mainframe.graph = mainframeimg;
	mainframe.res.w = 320;
	mainframe.res.h = 240;
	mainframe.pivot.x = 0.f;
	mainframe.pivot.y = 0.f;
	mainframe.pos.x = 0.f;
	mainframe.pos.y = 0.f;
	mainframe.vel.x = 0.f;
	mainframe.vel.y = 0.f;
	mainframe.tbox.x1 = 0.f;
	mainframe.tbox.y1 = 0.f;
	mainframe.tbox.x2 = 320.f;
	mainframe.tbox.y2 = 240.f;
	mainframe.dim.w = 320.f;
	mainframe.dim.h = 240.f;
	mainframe.scale = 1;
	mainframe.animframe = 0;
	
	// load up game screen
	SDL_Surface *_gamescreenimg = IMG_Load("gamescreen.png");
	SDL_Surface *gamescreenimg = SDL_DisplayFormatAlpha(_gamescreenimg);
	SDL_FreeSurface(_gamescreenimg);
	NASU_Actor gamescreen;
	gamescreen.graph = gamescreenimg;
	gamescreen.res.w = 184;
	gamescreen.res.h = 130;
	gamescreen.pivot.x = 0.f;
	gamescreen.pivot.y = 0.f;
	gamescreen.pos.x = 68.f;
	gamescreen.pos.y = 47.f;
	gamescreen.vel.x = 0.f;
	gamescreen.vel.y = 0.f;
	gamescreen.tbox.x1 = 0.f;
	gamescreen.tbox.y1 = 0.f;
	gamescreen.tbox.x2 = 320.f;
	gamescreen.tbox.y2 = 240.f;
	gamescreen.dim.w = 184.f;
	gamescreen.dim.h = 130.f;
	gamescreen.scale = 1;
	gamescreen.animframe = 0;

	// load up eggplants
	SDL_Surface *_nasuimg = IMG_Load("nasu.png");
	SDL_Surface *nasuimg = SDL_DisplayFormatAlpha(_nasuimg);
	SDL_FreeSurface(_nasuimg);
	NASU_Actor nasu;
	nasu.graph = nasuimg;
	nasu.res.w = 8;
	nasu.res.h = 8;
	nasu.pivot.x = 4.f;
	nasu.pivot.y = 4.f;
	nasu.pos.x = 4.f;
	nasu.pos.y = 4.f;
	nasu.vel.x = 0.f;
	nasu.vel.y = 0.f;
	nasu.tbox.x1 = -3.f;
	nasu.tbox.y1 = -3.f;
	nasu.tbox.x2 = 3.f;
	nasu.tbox.y2 = 3.f;
	nasu.dim.w = 8.f;
	nasu.dim.h = 8.f;
	nasu.scale = 1;
	nasu.animframe = 0;
	NASU_Actor nasu_b;
	nasu_b.graph = nasuimg;
	nasu_b.res.w = 8;
	nasu_b.res.h = 8;
	nasu_b.pivot.x = 4.f;
	nasu_b.pivot.y = 4.f;
	nasu_b.pos.x = 12.f;
	nasu_b.pos.y = 4.f;
	nasu_b.vel.x = 0.f;
	nasu_b.vel.y = 0.f;
	nasu_b.tbox.x1 = -3.f;
	nasu_b.tbox.y1 = -3.f;
	nasu_b.tbox.x2 = 3.f;
	nasu_b.tbox.y2 = 3.f;
	nasu_b.dim.w = 8.f;
	nasu_b.dim.h = 8.f;
	nasu_b.scale = 1;
	nasu_b.animframe = 1;

	TTF_Init();

	// load up score text
	TTF_Font *basefont = TTF_OpenFont("04B.TTF",8);
	NASU_ScrnText scoretxt;
	scoretxt.font = basefont;
	memset(&scoretxt.text,0,256);
	scoretxt.pos.x = 192.f;
	scoretxt.pos.y = 164.f;
	scoretxt.colr = whitecol;
	NASU_ScrnText scorenumtxt;
	scorenumtxt.font = basefont;
	memset(&scorenumtxt.text,0,256);
	scorenumtxt.pos.x = 240.f;
	scorenumtxt.pos.y = 164.f;
	scorenumtxt.colr = whitecol;
	
	NASU_ScrnText fpsnum;
	fpsnum.font = basefont;
	memset(&fpsnum.text,0,256);
	fpsnum.pos.x = 4.f;
	fpsnum.pos.y = 4.f;
	fpsnum.colr = whitecol;
	
	// load up the start/game over/paused text
	SDL_Surface *_textyimg = IMG_Load("texty.png");
	SDL_Surface *textyimg = SDL_DisplayFormatAlpha(_textyimg);
	SDL_FreeSurface(_textyimg);
	NASU_Actor texty;
	texty.graph = textyimg;
	texty.res.w = 72;
	texty.res.h = 8;
	texty.pivot.x = 36.f;
	texty.pivot.y = 4.f;
	texty.pos.x = 160.f;
	texty.pos.y = 120.f;
	texty.vel.x = 0.f;
	texty.vel.y = 0.f;
	texty.tbox.x1 = 0.f;
	texty.tbox.y1 = 0.f;
	texty.tbox.x2 = 0.f;
	texty.tbox.y2 = 0.f;
	texty.dim.w = 72.f;
	texty.dim.h = 8.f;
	texty.scale = 1;
	texty.animframe = 2;
	
	// load up score sprites
	
	SDL_Surface *_pointsimg = IMG_Load("scores.png");
	SDL_Surface *pointsimg = SDL_DisplayFormatAlpha(_pointsimg);
	SDL_FreeSurface(_pointsimg);
	NASU_Actor points1;
	points1.graph = pointsimg;
	points1.res.w = 40;
	points1.res.h = 12;
	points1.pivot.x = 20.f;
	points1.pivot.y = 6.f;
	points1.pos.x = 16.f;
	points1.pos.y = 16.f;
	points1.vel.x = 0.f;
	points1.vel.y = 0.f;
	points1.tbox.x1 = 0.f;
	points1.tbox.y1 = 0.f;
	points1.tbox.x2 = 0.f;
	points1.tbox.y2 = 0.f;
	points1.dim.w = 40.f;
	points1.dim.h = 12.f;
	points1.scale = 1;
	points1.animframe = 0;
	NASU_Actor points2;
	points2.graph = pointsimg;
	points2.res.w = 40;
	points2.res.h = 12;
	points2.pivot.x = 20.f;
	points2.pivot.y = 6.f;
	points2.pos.x = 16.f;
	points2.pos.y = 32.f;
	points2.vel.x = 0.f;
	points2.vel.y = 0.f;
	points2.tbox.x1 = 0.f;
	points2.tbox.y1 = 0.f;
	points2.tbox.x2 = 0.f;
	points2.tbox.y2 = 0.f;
	points2.dim.w = 40.f;
	points2.dim.h = 12.f;
	points2.scale = 1;
	points2.animframe = 1;
	NASU_Actor points3;
	points3.graph = pointsimg;
	points3.res.w = 40;
	points3.res.h = 12;
	points3.pivot.x = 20.f;
	points3.pivot.y = 6.f;
	points3.pos.x = 16.f;
	points3.pos.y = 48.f;
	points3.vel.x = 0.f;
	points3.vel.y = 0.f;
	points3.tbox.x1 = 0.f;
	points3.tbox.y1 = 0.f;
	points3.tbox.x2 = 0.f;
	points3.tbox.y2 = 0.f;
	points3.dim.w = 40.f;
	points3.dim.h = 12.f;
	points3.scale = 1;
	points3.animframe = 2;
	
	// load up sounds

	// TODO Pitch modification
	// step sounds: 1.1, 0.9, 1.5
	// nasu get sounds: 1.0, 1.1, 1.5

	Mix_Chunk *stepsnd[3];
	Mix_Chunk *getsnd[3];
	Mix_Chunk *losesnd;
	Mix_Music *titlemus;
	Mix_Music *gamemus;
	Mix_Music *losemus;
	if ( !bNoSound )
	{
		Mix_Init(MIX_INIT_OGG);
		Mix_OpenAudio(44100,AUDIO_S16SYS,2,1024);
		Mix_AllocateChannels(8);
	
		stepsnd[0] = Mix_LoadWAV("nasustep1.ogg");
		Mix_VolumeChunk(stepsnd[0], MIX_MAX_VOLUME*0.75f);
		stepsnd[1] = Mix_LoadWAV("nasustep2.ogg");
		Mix_VolumeChunk(stepsnd[1], MIX_MAX_VOLUME*0.75f);
		stepsnd[2] = Mix_LoadWAV("nasustep3.ogg");
		Mix_VolumeChunk(stepsnd[2], MIX_MAX_VOLUME*0.75f);
		getsnd[0] = Mix_LoadWAV("nasuget1.ogg");
		Mix_VolumeChunk(getsnd[0], MIX_MAX_VOLUME);
		getsnd[1] = Mix_LoadWAV("nasuget2.ogg");
		Mix_VolumeChunk(getsnd[1], MIX_MAX_VOLUME);
		getsnd[2] = Mix_LoadWAV("nasuget3.ogg");
		Mix_VolumeChunk(getsnd[2], MIX_MAX_VOLUME);
		losesnd = Mix_LoadWAV("nasulose.ogg");
		Mix_VolumeChunk(losesnd, MIX_MAX_VOLUME);
	
		titlemus = Mix_LoadMUS("nasutitle.ogg");
		gamemus = Mix_LoadMUS("nasugame.ogg");
		losemus = Mix_LoadMUS("nasuover.ogg");
		Mix_VolumeMusic(MIX_MAX_VOLUME*0.35f);
	}

	// game variables

	float pts1time = 0.f, pts2time = 0.f, pts3time = 0.f;
	unsigned long int cscore = 0;
	unsigned short int GameState = 0;
	float TimeUntilNextNasu = 2.0f, TimeUntilNextNasuB = 25.f;
	bool bLostGame = false;
	float blink = 0.f;
	unsigned short int blinkcounter = 0;
	bool bDerp = false;
	NASU_Vect Nasupos, Nasubpos;
	float deltatime = 0.f, calfps = 0.f;
	bool bShowFPS = false;
	
	float difficulty = 0.f;
	uint32_t lastframe = 0;
	char scrname[256];

	// start main loop!
	
	if ( !bNoSound )
		Mix_PlayMusic(titlemus,-1);
	bool bQueriedQuit = false;
	lastframe = SDL_GetTicks();
	while (!bQueriedQuit)
	{
		// input event polling
		SDL_Event Event;
		while ( SDL_PollEvent(&Event) )
		{
			switch (Event.type)
			{
				case SDL_QUIT:
					bQueriedQuit = true;
					break;
				case SDL_KEYDOWN:
					switch (Event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							bQueriedQuit = true;
							break;
						case SDLK_F12:
							#ifdef WINDOWS
							SDL_SaveBMP(realscreen,"screenshot.bmp");
							#else
							snprintf(scrname,256,"%s/nasu_screenshot.bmp",getenv("HOME"));
							SDL_SaveBMP(realscreen,scrname);
							#endif
							break;
						case SDLK_F10:
							bIsFullscreen = !bIsFullscreen;
							SDL_FreeSurface(mainscreen);
							mainscreen = NULL;
							if ( bIsFullscreen && (SDL_VideoModeOK(resx,resy,32,SDL_SWSURFACE|SDL_FULLSCREEN) != 0) )
							{
								mainscreen = SDL_SetVideoMode(resx,resy,32,SDL_SWSURFACE|SDL_FULLSCREEN);
							}
							else
							{
								bIsFullscreen = false;
								mainscreen = SDL_SetVideoMode(resx,resy,32,SDL_SWSURFACE);
							}
							break;
						case SDLK_F11:
							bShowFPS = !bShowFPS;
							break;
						case SDLK_RETURN:
							if (GameState == 0)
							{
								if ( !bNoSound )
									Mix_HaltMusic();
								GameState = 3;
								texty.animframe = 3;
							}
							break;
						case SDLK_p:
							if ( (GameState == 1) && !bLostGame )
							{
								GameState = 4;
								if ( !bNoSound )
									Mix_PauseMusic();
								texty.animframe = 4;
							}
							else if ( GameState == 4 )
							{
								GameState = 1;
								if ( !bNoSound )
									Mix_ResumeMusic();
								texty.animframe = 3;
							}
							break;
						case SDLK_LEFT:
						case SDLK_a:
							if ( !bLostGame && (GameState == 1) && (player.pos.x+player.tbox.x1 > gamescreen.pos.x) )
							{
								player.bLeft = true;
								player.vel.x = -60.f;
							}
							break;
						case SDLK_RIGHT:
						case SDLK_d:
							if ( !bLostGame && (GameState == 1) && (player.pos.x+player.tbox.x2 < gamescreen.pos.x+gamescreen.dim.w) )
							{
								player.bLeft = false;
								player.vel.x = 60.f;
							}
							break;
						case SDLK_UP:
						case SDLK_w:
						case SDLK_z:
						case SDLK_LSHIFT:
							if ( !bLostGame && (GameState == 1) && !player.bIsJumping )
							{
								player.bIsJumping = true;
								player.vel.y = -80.f;
								if ( !bNoSound )
									Mix_PlayChannel(-1,stepsnd[2],0);
							}
							break;
						default:
							break;
					}
					break;
				case SDL_KEYUP:
					switch (Event.key.keysym.sym)
					{
						case SDLK_LEFT:
						case SDLK_a:
							if ( !bLostGame && (GameState == 1) )
							{
								player.vel.x = 0.f;
							}
							break;
						case SDLK_RIGHT:
						case SDLK_d:
							if ( !bLostGame && (GameState == 1) )
							{
								player.vel.x = 0.f;
							}
							break;
						default:
							break;
					}
					break;
				default:
					break;
			}
		}

		// clear screen
		SDL_FillRect(mainscreen,scrrect,SDL_MapRGB(mainscreen->format,0,0,0));
		SDL_FillRect(realscreen,rscrrect,SDL_MapRGB(realscreen->format,0,0,0));

		// process game logic

		switch (GameState)
		{
			case 0:		// Title screen
				gamescreen.animframe = 2;
				RenderActor(realscreen,&gamescreen);
				snprintf(scorenumtxt.text,256,"%9u",score);
				scorenumtxt.pos.x = 320.f-(112.f+CalcTextW(&scorenumtxt));
				scorenumtxt.pos.y = 144.f;
				scoretxt.pos.x = 112.f;
				scoretxt.pos.y = 144.f;
				snprintf(scoretxt.text,256,"Hi Score:");
				RenderScrnText(realscreen,&scoretxt);
				RenderScrnText(realscreen,&scorenumtxt);
				break;
			case 2:		// Lose screen
				gamescreen.animframe = 3;
				RenderActor(realscreen,&gamescreen);
				blink += deltatime;
				if ( blink >= 7.f )
				{
					if ( !bNoSound )
						Mix_PlayMusic(titlemus,-1);
					GameState = 0;
				}
				texty.animframe = 2;
				snprintf(scorenumtxt.text,256,"%9lu",cscore);
				scorenumtxt.pos.x = 320.f-(80.f+CalcTextW(&scorenumtxt));
				scorenumtxt.pos.y = 164.f;
				scoretxt.pos.x = 168.f;
				scoretxt.pos.y = 164.f;
				snprintf(scoretxt.text,256,"Score:");
				RenderScrnText(realscreen,&scoretxt);
				RenderScrnText(realscreen,&scorenumtxt);
				RenderActor(realscreen,&texty);
				break;
			case 3:		// Game preparation
				gamescreen.animframe = 0;
				RenderActor(realscreen,&gamescreen);
				blink += deltatime;
				if ( blink > 0.25f )
				{
					blinkcounter++;
					blink = 0.f;
					bDerp = !bDerp;
					if ( !bDerp )
						texty.animframe = 3;
					else
					{
						if ( blinkcounter < 7 )
							texty.animframe = 0;
						else
							texty.animframe = 1;
					}
				}
				if ( blinkcounter >= 8 )
				{
					bDerp = false;
					blinkcounter = 0;
					GameState = 1;
					if ( !bNoSound )
						Mix_PlayMusic(gamemus,-1);
				}
				difficulty = 0.f;
				cscore = 0;
				TimeUntilNextNasu = 2.0f;
				TimeUntilNextNasuB = 25.f;
				bLostGame = false;
				pts1time = 0.f;
				pts2time = 0.f;
				pts3time = 0.f;
				player.animframe = 0;
				player.bLeft = 0;
				player.bIsJumping = 0;
				player.pos.x = 160.f;
				player.pos.y = 149.f;
				nasu.pos.x = 4.f;
				nasu.pos.y = 4.f;
				nasu_b.pos.x = 12.f;
				nasu_b.pos.y = 4.f;
				points1.pos.x = 16.f;
				points1.pos.y = 16.f;
				points2.pos.x = 16.f;
				points2.pos.y = 32.f;
				points3.pos.x = 16.f;
				points3.pos.y = 48.f;
				RenderPlayer(realscreen,&player);
				snprintf(scorenumtxt.text,256,"%9lu",cscore);
				scorenumtxt.pos.x = 320.f-(80.f+CalcTextW(&scorenumtxt));
				scorenumtxt.pos.y = 164.f;
				scoretxt.pos.x = 168.f;
				scoretxt.pos.y = 164.f;
				snprintf(scoretxt.text,256,"Score:");
				RenderScrnText(realscreen,&scoretxt);
				RenderScrnText(realscreen,&scorenumtxt);
				RenderActor(realscreen,&texty);
				break;
			case 4:		// Paused
				RenderActor(realscreen,&gamescreen);
				RenderPlayer(realscreen,&player);
				RenderActor(realscreen,&nasu);
				RenderActor(realscreen,&nasu_b);
				snprintf(scorenumtxt.text,256,"%9lu",cscore);
				scorenumtxt.pos.x = 320.f-(80.f+CalcTextW(&scorenumtxt));
				scorenumtxt.pos.y = 164.f;
				scoretxt.pos.x = 168.f;
				scoretxt.pos.y = 164.f;
				snprintf(scoretxt.text,256,"Score:");
				RenderScrnText(realscreen,&scoretxt);
				RenderScrnText(realscreen,&scorenumtxt);
				RenderActor(realscreen,&texty);
				break;
			default:	// Main game
				if ( bLostGame )
				{
					player.vel.x = 0.f;
					player.vel.y = 0.f;
					blink += deltatime;
					if ( blink > 0.125f )
					{
						blinkcounter++;
						blink = 0.f;
						bDerp = !bDerp;
						if ( bDerp )
							gamescreen.animframe = 0;
						else
							gamescreen.animframe = 1;
					}
					if ( blinkcounter >= 12 )
					{
						blinkcounter = 0;
						if ( !bNoSound )
							Mix_PlayMusic(losemus,0);
						blink = 0.f;
						GameState = 2;
						bDerp = false;
					}
				}
				else
				{
					gamescreen.animframe = 0;
					blink += deltatime;
					if ( blink > 0.125f )
					{
						bDerp = !bDerp;
						blink = 0.f;
						if ( !bNoSound )
						{
							if ( player.vel.x != 0.f && !player.bIsJumping )
							{
								if ( bDerp )
									Mix_PlayChannel(-1,stepsnd[0],0);
								else
									Mix_PlayChannel(-1,stepsnd[1],0);
							}
						}
					}
					
					if ( TimeUntilNextNasu > 0.f )
					{
						TimeUntilNextNasu -= deltatime;
						nasu.pos.x = 4.f;
						nasu.pos.y = 4.f;
						
						if ( TimeUntilNextNasu <= 0.f )
						{
							TimeUntilNextNasu = 0.f;
							nasu.pos.x = (float)(rand()%160+80);
							nasu.pos.y = 0.f;
							nasu.vel.y = 50.f+difficulty*0.3f;
						}
					}
					else
					{
						MoveActor(&nasu,deltatime);
						Nasupos = nasu.pos;
						if ( CollidePlayer(&nasu,&player) && player.bIsJumping )
						{
							if ( (rand()%(30+(int)(difficulty*0.35f))) == 0 )
							{
								cscore += 1000;
								if ( cscore > score )
									score = cscore;
								TimeUntilNextNasu = 2.f-(difficulty*0.03f);
								if ( TimeUntilNextNasu <= 0.1f )
									TimeUntilNextNasu = 0.1f;
								nasu.pos.x = 4.f;
								nasu.pos.y = 4.f;
								if ( !bNoSound )
									Mix_PlayChannel(-1,getsnd[2],0);
								pts3time += 0.75f;
								points3.pos = Nasupos;
								points3.vel.y = -50.f;
								difficulty += 7.5f;
							}
							else
							{
								cscore += 10;
								if ( cscore > score )
									score = cscore;
								TimeUntilNextNasu = 2.f-(difficulty*0.03f);
								if ( TimeUntilNextNasu <= 0.1f )
									TimeUntilNextNasu = 0.1f;
								nasu.pos.x = 4.f;
								nasu.pos.y = 4.f;
								if ( !bNoSound )
									Mix_PlayChannel(-1,getsnd[0],0);
								pts1time += 0.75f;
								points1.pos = Nasupos;
								points1.vel.y = -50.f;
								difficulty += 1.f;
							}
						}
						else if ( nasu.pos.y >= 160.f )
						{
							if ( !bNoSound )
							{
								Mix_HaltMusic();
								Mix_PlayChannel(-1,losesnd,0);
							}
							bLostGame = true;
						}
					}
					
					if ( TimeUntilNextNasuB > 0.f )
					{
						TimeUntilNextNasuB -= deltatime;
						nasu_b.pos.x = 12.f;
						nasu_b.pos.y = 4.f;
						if ( TimeUntilNextNasuB <= 0.f )
						{
							TimeUntilNextNasuB = 0.f;
							int decideposb = rand()%2;
							switch (decideposb)
							{
							case 0:
								nasu_b.vel.x = (50.f+difficulty*0.15f);
								nasu_b.vel.y = 0.f;
								nasu_b.pos.x = 0.f;
								nasu_b.pos.y = 160.f;
								nasu_b.animframe = 2;
								break;
							default:
								nasu_b.vel.x = -(50.f+difficulty*0.15f);
								nasu_b.vel.y = 0.f;
								nasu_b.pos.x = 320.f;
								nasu_b.pos.y = 160.f;
								nasu_b.animframe = 1;
								break;
							}
						}
					}
					else
					{
						nasu_b.vel.y += (120.f+difficulty*0.75f)*deltatime;
						MoveActor(&nasu_b,deltatime);
						Nasubpos = nasu_b.pos;
						if ( CollidePlayer(&nasu_b,&player) && player.bIsJumping )
						{
							cscore += 300;
							if ( cscore > score )
								score = cscore;
							TimeUntilNextNasuB = 25.f+(difficulty*0.1f);
							nasu_b.pos.x = 12.f;
							nasu_b.pos.y = 4.f;
							if ( !bNoSound )
								Mix_PlayChannel(-1,getsnd[1],0);
							pts2time += 0.75f;
							points2.pos = Nasubpos;
							points2.vel.y = -50.f;
							difficulty += 2.5f;
						}
						if ( (nasu_b.vel.x > 0.f) && (nasu_b.pos.x > gamescreen.pos.x+gamescreen.dim.w) )
						{
							TimeUntilNextNasuB = 25.f+(difficulty*0.15f);
							nasu_b.pos.x = 12.f;
							nasu_b.pos.y = 4.f;
						}
						if ( (nasu_b.vel.x < 0.f) && (nasu_b.pos.x < gamescreen.pos.x) )
						{
							TimeUntilNextNasuB = 25.f+(difficulty*0.15f);
							nasu_b.pos.x = 12.f;
							nasu_b.pos.y = 4.f;
						}
						if ( nasu_b.pos.y >= 160.f )
							nasu_b.vel.y = -(80.f+difficulty*0.65f);
					}
					
					MovePlayer(&player,deltatime);
					if ( player.bIsJumping )
						player.vel.y += 480.f*deltatime;
					if ( player.pos.y >= 149.f )
					{
						player.pos.y = 149.f;
						player.bIsJumping = false;
					}
					if ( player.pos.x+player.tbox.x1 <= gamescreen.pos.x )
					{
						player.pos.x = gamescreen.pos.x-player.tbox.x1;
						player.vel.x = 0;
					}
					if ( player.pos.x+player.tbox.x2 >= gamescreen.pos.x+gamescreen.dim.w )
					{
						
						player.pos.x = (gamescreen.pos.x+gamescreen.dim.w)+player.tbox.x1;
						player.vel.x = 0;
					}
					if ( player.bIsJumping )
						player.animframe = 3;
					else if ( player.vel.x != 0.f )
					{
						if ( bDerp )
							player.animframe = 2;
						else
							player.animframe = 1;
					}
					else
						player.animframe = 0;
					
					if ( pts1time > 0.f )
					{
						pts1time -= deltatime;
						MoveActor(&points1,deltatime);
						if ( pts1time <= 0 )
						{
							points1.vel.y = 0.f;
							points1.pos.x = 16.f;
							points1.pos.y = 16.f;
						}
					}
					if ( pts2time > 0.f )
					{
						pts2time -= deltatime;
						MoveActor(&points2,deltatime);
						if ( pts2time <= 0 )
						{
							points2.vel.y = 0.f;
							points2.pos.x = 16.f;
							points2.pos.y = 32.f;
						}
					}
					if ( pts3time > 0.f )
					{
						pts3time -= deltatime;
						MoveActor(&points3,deltatime);
						if ( pts3time <= 0 )
						{
							points3.vel.y = 0.f;
							points3.pos.x = 16.f;
							points3.pos.y = 48.f;
						}
					}
				}
				RenderActor(realscreen,&gamescreen);
				RenderPlayer(realscreen,&player);
				RenderActor(realscreen,&nasu);
				RenderActor(realscreen,&nasu_b);
				RenderActor(realscreen,&points1);
				RenderActor(realscreen,&points2);
				RenderActor(realscreen,&points3);
				snprintf(scorenumtxt.text,256,"%9lu",cscore);
				scorenumtxt.pos.x = 320.f-(80.f+CalcTextW(&scorenumtxt));
				scorenumtxt.pos.y = 164.f;
				scoretxt.pos.x = 168.f;
				scoretxt.pos.y = 164.f;
				snprintf(scoretxt.text,256,"Score:");
				RenderScrnText(realscreen,&scoretxt);
				RenderScrnText(realscreen,&scorenumtxt);
				break;
		}

		// render frame
		RenderActor(realscreen,&mainframe);
		// blit frame to screen
		SDL_Surface *resizedframe = Upscale(realscreen,ScreenMult);
		BlitAt(resizedframe,NULL,mainscreen,resx/2,resy/2,true,true);
		SDL_FreeSurface(resizedframe);
		// flip screen and calculate fps
		calfps = 1000.f/(SDL_GetTicks()-lastframe);
		deltatime = 1.f/calfps;
		snprintf(fpsnum.text,256,"%.2f",calfps);
		fpsnum.pos.x = resx-(4+CalcTextW(&fpsnum));
		fpsnum.pos.y = 4.f;
		if ( bShowFPS )
			RenderScrnText(mainscreen,&fpsnum);
		if (SDL_Flip(mainscreen) == -1)
		{
			fprintf(stderr,"error updating screen (%s)\n", SDL_GetError());
			return 1;
		}
		lastframe = SDL_GetTicks();
	}
	if ( !bNoSound )
	{
		// Free audio
		Mix_FreeChunk(stepsnd[0]);
		Mix_FreeChunk(stepsnd[1]);
		Mix_FreeChunk(stepsnd[2]);
		Mix_FreeChunk(getsnd[0]);
		Mix_FreeChunk(getsnd[1]);
		Mix_FreeChunk(getsnd[2]);
		Mix_FreeChunk(losesnd);
		Mix_FreeMusic(titlemus);
		Mix_FreeMusic(gamemus);
		Mix_FreeMusic(losemus);
		Mix_AllocateChannels(0);
		Mix_CloseAudio();
	}
	// Free surfaces
	SDL_FreeSurface(playersprites);
	SDL_FreeSurface(mainframeimg);
	SDL_FreeSurface(gamescreenimg);
	SDL_FreeSurface(nasuimg);
	SDL_FreeSurface(textyimg);
	// Free font
	TTF_CloseFont(basefont);
	// Quit stuff
	Mix_Quit();
	TTF_Quit();
	SDL_Quit();

	return EXIT_SUCCESS;
}
