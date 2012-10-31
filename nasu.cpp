#include "nasu.h"
#include "nasudef.h"

// some redefinitions needed because Windows is weird
#ifdef WINDOWS
	#define srandom srand
	#define random rand
	#define strcasecmp stricmp
	#define strncasecmp strnicmp
#endif

uint32_t score = 0;

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

int main(int argc, char **argv)
{
	srandom(time(NULL));	// a quick 'n dirty way of initializing random seed

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

	Uint32 last_tick = SDL_GetTicks();
	
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
	FPSmanager *frameman = new FPSmanager;
	SDL_initFramerate(frameman);
	SDL_setFramerate(frameman,60);
	
	// load up player
	SDL_Surface *_playersprites = IMG_Load("player.png");
	SDL_Surface *playersprites = SDL_DisplayFormatAlpha(_playersprites);
	SDL_FreeSurface(_playersprites);
	NASU_Player player;
	player.graph = playersprites;
	player.res = NASU_GfxRes{24, 24};
	player.pivot = NASU_Vect{12.f, 12.f};
	player.pos = NASU_Vect{160.f, 149.f};
	player.vel = NASU_Vect{0.f, 0.f};
	player.tbox = NASU_BoundingBox{-12.f, -4.f, 12.f, 12.f};
	player.dim = NASU_Dim{24.f, 24.f};
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
	mainframe.res = NASU_GfxRes{320, 240};
	mainframe.pivot = NASU_Vect{0.f, 0.f};
	mainframe.pos = NASU_Vect{0.f, 0.f};
	mainframe.vel = NASU_Vect{0.f, 0.f};
	mainframe.tbox = NASU_BoundingBox{0.f, 0.f, 320.f, 240.f};
	mainframe.dim = NASU_Dim{320.f, 240.f};
	mainframe.scale = 1;
	mainframe.animframe = 0;
	
	// load up game screen
	SDL_Surface *_gamescreenimg = IMG_Load("gamescreen.png");
	SDL_Surface *gamescreenimg = SDL_DisplayFormatAlpha(_gamescreenimg);
	SDL_FreeSurface(_gamescreenimg);
	NASU_Actor gamescreen;
	gamescreen.graph = gamescreenimg;
	gamescreen.res = NASU_GfxRes{184, 130};
	gamescreen.pivot = NASU_Vect{0.f, 0.f};
	gamescreen.pos = NASU_Vect{68.f, 47.f};
	gamescreen.vel = NASU_Vect{0.f, 0.f};
	gamescreen.tbox = NASU_BoundingBox{0.f, 0.f, 320.f, 240.f};
	gamescreen.dim = NASU_Dim{184.f, 130.f};
	gamescreen.scale = 1;
	gamescreen.animframe = 0;

	// load up eggplants
	SDL_Surface *_nasuimg = IMG_Load("nasu.png");
	SDL_Surface *nasuimg = SDL_DisplayFormatAlpha(_nasuimg);
	SDL_FreeSurface(_nasuimg);
	NASU_Actor nasu;
	nasu.graph = nasuimg;
	nasu.res = NASU_GfxRes{8, 8};
	nasu.pivot = NASU_Vect{4.f, 4.f};
	nasu.pos = NASU_Vect{4.f, 4.f};
	nasu.vel = NASU_Vect{0.f, 0.f};
	nasu.tbox = NASU_BoundingBox{-3.f, -3.f, 3.f, 3.f};
	nasu.dim = NASU_Dim{8.f, 8.f};
	nasu.scale = 1;
	nasu.animframe = 0;
	NASU_Actor nasu_b;
	nasu_b.graph = nasuimg;
	nasu_b.res = NASU_GfxRes{8, 8};
	nasu_b.pivot = NASU_Vect{4.f, 4.f};
	nasu_b.pos = NASU_Vect{12.f, 4.f};
	nasu_b.vel = NASU_Vect{0.f, 0.f};
	nasu_b.tbox = NASU_BoundingBox{-3.f, -3.f, 3.f, 3.f};
	nasu_b.dim = NASU_Dim{8.f, 8.f};
	nasu_b.scale = 1;
	nasu_b.animframe = 1;

	TTF_Init();

	// load up score text
	TTF_Font *basefont = TTF_OpenFont("04B.TTF",8);
	NASU_ScrnText scoretxt;
	scoretxt.font = basefont;
	memset(&scoretxt.text,0,256);
	scoretxt.pos = NASU_Vect{192.f,164.f};
	scoretxt.colr = SDL_Color{255,255,255,255};
	NASU_ScrnText scorenumtxt;
	scorenumtxt.font = basefont;
	memset(&scorenumtxt.text,0,256);
	scorenumtxt.pos = NASU_Vect{240.f,164.f};
	scorenumtxt.colr = SDL_Color{255,255,255,255};
	
	NASU_ScrnText fpsnum;
	fpsnum.font = basefont;
	memset(&fpsnum.text,0,256);
	fpsnum.pos = NASU_Vect{4.f,4.f};
	fpsnum.colr = SDL_Color{255,255,255,255};
	
	// load up the start/game over/paused text
	SDL_Surface *_textyimg = IMG_Load("texty.png");
	SDL_Surface *textyimg = SDL_DisplayFormatAlpha(_textyimg);
	SDL_FreeSurface(_textyimg);
	NASU_Actor texty;
	texty.graph = textyimg;
	texty.res = NASU_GfxRes{72, 8};
	texty.pivot = NASU_Vect{36.f, 4.f};
	texty.pos = NASU_Vect{160.f, 120.f};
	texty.vel = NASU_Vect{0.f, 0.f};
	texty.tbox = NASU_BoundingBox{0.f, 0.f, 0.f, 0.f};
	texty.dim = NASU_Dim{72.f, 8.f};
	texty.scale = 1;
	texty.animframe = 2;
	
	// load up score sprites
	
	SDL_Surface *_pointsimg = IMG_Load("scores.png");
	SDL_Surface *pointsimg = SDL_DisplayFormatAlpha(_pointsimg);
	SDL_FreeSurface(_pointsimg);
	NASU_Actor points1;
	points1.graph = pointsimg;
	points1.res = NASU_GfxRes{40, 12};
	points1.pivot = NASU_Vect{20.f, 6.f};
	points1.pos = NASU_Vect{16.f, 16.f};
	points1.vel = NASU_Vect{0.f, 0.f};
	points1.tbox = NASU_BoundingBox{0.f, 0.f, 0.f, 0.f};
	points1.dim = NASU_Dim{40.f, 12.f};
	points1.scale = 1;
	points1.animframe = 0;
	NASU_Actor points2;
	points2.graph = pointsimg;
	points2.res = NASU_GfxRes{40, 12};
	points2.pivot = NASU_Vect{20.f, 6.f};
	points2.pos = NASU_Vect{16.f, 32.f};
	points2.vel = NASU_Vect{0.f, 0.f};
	points2.tbox = NASU_BoundingBox{0.f, 0.f, 0.f, 0.f};
	points2.dim = NASU_Dim{40.f, 12.f};
	points2.scale = 1;
	points2.animframe = 1;
	NASU_Actor points3;
	points3.graph = pointsimg;
	points3.res = NASU_GfxRes{40, 12};
	points3.pivot = NASU_Vect{20.f, 6.f};
	points3.pos = NASU_Vect{16.f, 48.f};
	points3.vel = NASU_Vect{0.f, 0.f};
	points3.tbox = NASU_BoundingBox{0.f, 0.f, 0.f, 0.f};
	points3.dim = NASU_Dim{40.f, 12.f};
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
		getsnd[0] = Mix_LoadWAV("nasuget.ogg");
		Mix_VolumeChunk(getsnd[0], MIX_MAX_VOLUME);
		getsnd[1] = Mix_LoadWAV("nasuget.ogg");
		Mix_VolumeChunk(getsnd[1], MIX_MAX_VOLUME);
		getsnd[2] = Mix_LoadWAV("nasuget.ogg");
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
							char scrname[256];
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
					}
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
				snprintf(scorenumtxt.text,256,"%9lu",score);
				scorenumtxt.pos = NASU_Vect{320-(112+CalcTextW(&scorenumtxt)),144.f};
				scoretxt.pos = NASU_Vect{112.f,144.f};
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
				scorenumtxt.pos = NASU_Vect{320-(80+CalcTextW(&scorenumtxt)),164.f};
				scoretxt.pos = NASU_Vect{168.f,164.f};
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
				player.pos = NASU_Vect{160.f, 149.f};
				nasu.pos = NASU_Vect{4.f,4.f};
				nasu_b.pos = NASU_Vect{12.f,4.f};
				points1.pos = NASU_Vect{16.f, 16.f};
				points2.pos = NASU_Vect{16.f, 24.f};
				points3.pos = NASU_Vect{16.f, 32.f};
				RenderPlayer(realscreen,&player);
				snprintf(scorenumtxt.text,256,"%9lu",cscore);
				scorenumtxt.pos = NASU_Vect{320-(80+CalcTextW(&scorenumtxt)),164.f};
				scoretxt.pos = NASU_Vect{168.f,164.f};
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
				scorenumtxt.pos = NASU_Vect{320-(80+CalcTextW(&scorenumtxt)),164.f};
				scoretxt.pos = NASU_Vect{168.f,164.f};
				snprintf(scoretxt.text,256,"Score:");
				RenderScrnText(realscreen,&scoretxt);
				RenderScrnText(realscreen,&scorenumtxt);
				RenderActor(realscreen,&texty);
				break;
			default:	// Main game
				if ( bLostGame )
				{
					player.vel = NASU_Vect{0.f,0.f};
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
						nasu.pos = NASU_Vect{4.f,4.f};
						if ( TimeUntilNextNasu <= 0.f )
						{
							TimeUntilNextNasu = 0.f;
							float pose = float(random()%160+80);
							nasu.pos = NASU_Vect{pose,0.f};
							nasu.vel.y = 50.f+difficulty*0.3f;
						}
					}
					else
					{
						MoveActor(&nasu,deltatime);
						Nasupos = nasu.pos;
						if ( CollidePlayer(&nasu,&player) && player.bIsJumping )
						{
							if ( (random()%(30+int(difficulty*0.35f))) == 0 )
							{
								cscore += 1000;
								if ( cscore > score )
									score = cscore;
								TimeUntilNextNasu = 2.f-(difficulty*0.03f);
								if ( TimeUntilNextNasu <= 0.1f )
									TimeUntilNextNasu = 0.1f;
								nasu.pos = NASU_Vect{4.f,4.f};
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
								nasu.pos = NASU_Vect{4.f,4.f};
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
						nasu_b.pos = NASU_Vect{12.f,4.f};
						if ( TimeUntilNextNasuB <= 0.f )
						{
							TimeUntilNextNasuB = 0.f;
							int decideposb = random()%2;
							switch (decideposb)
							{
							case 0:
								nasu_b.vel.x = (50.f+difficulty*0.15f);
								nasu_b.vel.y = 0.f;
								nasu_b.pos = NASU_Vect{0.f,160.f};
								nasu_b.animframe = 2;
								break;
							default:
								nasu_b.vel.x = -(50.f+difficulty*0.15f);
								nasu_b.vel.y = 0.f;
								nasu_b.pos = NASU_Vect{320.f,160.f};
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
							nasu_b.pos = NASU_Vect{12.f,4.f};
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
							nasu_b.pos = NASU_Vect{12.f,4.f};
						}
						if ( (nasu_b.vel.x < 0.f) && (nasu_b.pos.x < gamescreen.pos.x) )
						{
							TimeUntilNextNasuB = 25.f+(difficulty*0.15f);
							nasu_b.pos = NASU_Vect{12.f,4.f};
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
							points1.pos = NASU_Vect{16.f, 16.f};
						}
					}
					if ( pts2time > 0.f )
					{
						pts2time -= deltatime;
						MoveActor(&points2,deltatime);
						if ( pts2time <= 0 )
						{
							points2.vel.y = 0.f;
							points2.pos = NASU_Vect{16.f, 32.f};
						}
					}
					if ( pts3time > 0.f )
					{
						pts3time -= deltatime;
						MoveActor(&points3,deltatime);
						if ( pts3time <= 0 )
						{
							points3.vel.y = 0.f;
							points3.pos = NASU_Vect{16.f, 48.f};
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
				scorenumtxt.pos = NASU_Vect{320-(80+CalcTextW(&scorenumtxt)),164.f};
				scoretxt.pos = NASU_Vect{168.f,164.f};
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
		fpsnum.pos = NASU_Vect{resx-(4+CalcTextW(&fpsnum)),4.f};
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
