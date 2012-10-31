// graphic rectangle resolution
typedef struct NASU_GfxRes
{
	uint16_t w, h;
} NASU_GfxRes;

// 2D vector
typedef struct NASU_Vect
{
	float x, y;
} NASU_Vect;

// logical dimensions
typedef struct NASU_Dim
{
	float w, h;
} NASU_Dim;

// a bounding box
typedef struct NASU_BoundingBox
{
	float x1, y1, x2, y2;
} NASU_BoundingBox;

// a standard object
typedef struct NASU_Actor
{
	SDL_Surface *graph;
	NASU_GfxRes res;
	NASU_Vect pivot, pos, vel;
	NASU_BoundingBox tbox;
	NASU_Dim dim;
	uint16_t scale;
	uint16_t animframe;
} NASU_Actor;

// a player
typedef struct NASU_Player
{
	SDL_Surface *graph;	// graphic sheet
	NASU_GfxRes res;	// physical resolution of sprite (clip rect)
	NASU_Vect pivot, pos, vel;	// pivot point of sprite, position and velocity of player
	NASU_BoundingBox tbox;	// "touch" box
	NASU_Dim dim;	// logical size of sprite, used for world collision
	uint16_t scale;	// scale to multiply other properties
	bool bIsJumping;	// is player jumping?
	uint16_t animframe;	// current animation frame
	bool bLeft;	// player looking left? (mirror sprite)
	
} NASU_Player;

// on-screen text object
typedef struct NASU_ScrnText
{
	TTF_Font *font;	// associated font
	char text[256];	// contained text
	NASU_Vect pos;	// screen position
	SDL_Color colr;	// text color
} NASU_ScrnText;

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
    SDL_Rect *offsetrect = new SDL_Rect;
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
        A = int(((float(A)/255.0*float(A4))/255.0)*float(A5));
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
    SDL_Rect *temprect = new SDL_Rect;
    temprect->x = 0;
    temprect->y = 0;
    temprect->w = width;
    temprect->h = height;
    SDL_FillRect(stext,temprect,SDL_MapRGB(stext->format,0,0,0));
    SDL_Surface *stemp = NULL;
    SDL_Surface *grad = NULL;
    stemp = TTF_RenderUTF8_Blended(font, text, SDL_Color{255,255,255,255});
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
    temprect = new SDL_Rect;
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
	long int px, py, nx, ny, col, line, i, j;
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
	long int px, py, nx, ny, col, line;
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
	return float(w);
}
float CalcTextH(NASU_ScrnText *t)
{
	int w = 0, h = 0;
	if ( (t->text == NULL) || (strlen(t->text) == 0) )
		return 0.f;
	TTF_SizeText(t->font,t->text,&w,&h);
	return float(h);
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
	out = new Mix_Chunk;
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
