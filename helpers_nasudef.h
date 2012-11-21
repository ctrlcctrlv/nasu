#ifndef HELPERS_NASUDEF_H
#define HELPERS_NASUDEF_H
#ifdef __cplusplus
extern "C" {
#endif
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

void RenderActor(SDL_Surface *screen, NASU_Actor *a);
void RenderPlayer(SDL_Surface *screen, NASU_Player *p);
float CalcTextW(NASU_ScrnText *t);
float CalcTextH(NASU_ScrnText *t);
void RenderScrnText(SDL_Surface *screen, NASU_ScrnText *t);
bool CollidePlayer(NASU_Actor *a, NASU_Player *p);
void MoveActor(NASU_Actor *a, float deltatime);
void MovePlayer(NASU_Player *p, float deltatime);
//Mix_Chunk* SndStretch(Mix_Chunk *in, float factor)
#ifdef __cplusplus
}
#endif
#endif
