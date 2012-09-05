#include "nasu.hpp"

#ifdef WINDOWS
    #define srandom srand
    #define random rand
    #define strcasecmp stricmp
    #define strncasecmp strnicmp
#endif

// Author's comment: This is a special rewrite for SFML 2.0 (^^;

unsigned long int score = 0;

using namespace std;
using namespace sf;

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
    srandom(time(NULL));

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

    float ScreenMult = 2;
    bool bIsFullscreen = false;
    if ( argc > 1 )
    {
        if ( strcasecmp(argv[1],"-fullscreen") == 0 )
        {
            bIsFullscreen = true;
            if ( argc > 2 )
            {
                sscanf(argv[1],"-%f",&ScreenMult);
                if ( ScreenMult < 1.f )
                    ScreenMult = 1.f;
                ScreenMult = truncf(ScreenMult);
            }
        }
        else
        {
            sscanf(argv[1],"-%f",&ScreenMult);
            if ( ScreenMult < 1.f )
                ScreenMult = 1.f;
            ScreenMult = truncf(ScreenMult);
            if ( argc > 2 )
                if ( strcasecmp(argv[2],"-fullscreen") == 0 )
                    bIsFullscreen = true;
        }
    }

    atexit(savescore);
    RenderWindow App;
    if ( bIsFullscreen && VideoMode(320*ScreenMult,240*ScreenMult,32).IsValid() )
        App.Create(VideoMode(320*ScreenMult,240*ScreenMult,32),"NASU",Style::Fullscreen);
    else
    {
        bIsFullscreen = false;
        App.Create(VideoMode(320*ScreenMult,240*ScreenMult,32),"NASU",Style::Titlebar|Style::Close);
    }
    App.EnableVerticalSync(true);
    Image Icon;
    Icon.Create(32,32,Color(0,0,0));
    Icon.LoadFromFile("nasuicon.png");
    App.SetIcon(32,32,Icon.GetPixelsPtr());
    Image PlayerImg;
    PlayerImg.Create(96,24,Color(255,255,255));
    PlayerImg.LoadFromFile("player.png");
    Texture PlayerTex;
    PlayerTex.LoadFromImage(PlayerImg);
    PlayerTex.SetSmooth(false);
    Sprite Player;
    Player.SetColor(Color(255,255,255,255));
    Player.SetPosition(160.f*ScreenMult,149.f*ScreenMult);
    Player.SetScale(ScreenMult,ScreenMult);
    Player.SetOrigin(12.f,12.f);
    Player.SetTexture(PlayerTex,false);
    Player.SetSubRect(IntRect(0,0,24,24));

    Image MainFrameImg;
    MainFrameImg.Create(320,240,Color(0,0,0));
    MainFrameImg.LoadFromFile("mainscreen.png");
    Texture MainFrameTex;
    MainFrameTex.LoadFromImage(MainFrameImg);
    MainFrameTex.SetSmooth(false);
    Sprite MainFrame;
    MainFrame.SetColor(Color(255,255,255,255));
    MainFrame.SetPosition(0.f,0.f);
    MainFrame.SetScale(ScreenMult,ScreenMult);
    MainFrame.SetOrigin(0.f,0.f);
    MainFrame.SetTexture(MainFrameTex);

    Image GameScrImg;
    GameScrImg.Create(368,260,Color(64,0,0));
    GameScrImg.LoadFromFile("gamescreen.png");
    Texture GameScrTex;
    GameScrTex.LoadFromImage(GameScrImg);
    GameScrTex.SetSmooth(false);
    Sprite GameScr;
    GameScr.SetColor(Color(255,255,255,255));
    GameScr.SetPosition(68.f*ScreenMult,47.f*ScreenMult);
    GameScr.SetScale(ScreenMult,ScreenMult);
    GameScr.SetOrigin(0.f,0.f);
    GameScr.SetTexture(GameScrTex);
    GameScr.SetSubRect(IntRect(0,0,184,130));

    Image NasuImg;
    NasuImg.Create(16,16,Color(64,0,96));
    NasuImg.LoadFromFile("nasu.png");
    Texture NasuTex;
    NasuTex.LoadFromImage(NasuImg);
    NasuTex.SetSmooth(false);
    Sprite Nasu;
    Nasu.SetColor(Color(255,255,255,255));
    Nasu.SetPosition(-100.f,-100.f);
    Nasu.SetScale(ScreenMult,ScreenMult);
    Nasu.SetOrigin(4.f,4.f);
    Nasu.SetTexture(NasuTex);
    Nasu.SetSubRect(IntRect(0,0,8,8));
    Sprite NasuB;
    NasuB.SetColor(Color(255,255,255,255));
    NasuB.SetPosition(-100.f,-100.f);
    NasuB.SetScale(ScreenMult,ScreenMult);
    NasuB.SetOrigin(4.f,4.f);
    NasuB.SetTexture(NasuTex);
    NasuB.SetSubRect(IntRect(8,0,8,8));

    Text ScoreTxt;
    Text ScoreNumTxt;
    char scorenum[256];
    Font TehFont;
    TehFont.LoadFromFile("04B.TTF");
    ScoreTxt.SetFont(TehFont);
    ScoreTxt.SetOrigin(0.f,0.f);
    ScoreTxt.SetCharacterSize(8.f*ScreenMult);
    ScoreTxt.SetStyle(sf::Text::Regular);
    ScoreTxt.SetPosition(192.f*ScreenMult,164.f*ScreenMult);
    ScoreNumTxt.SetFont(TehFont);
    ScoreNumTxt.SetOrigin(0.f,0.f);
    ScoreNumTxt.SetCharacterSize(8.f*ScreenMult);
    ScoreNumTxt.SetStyle(sf::Text::Regular);
    ScoreNumTxt.SetPosition(240.f*ScreenMult,164.f*ScreenMult);

    Image TextyImg;
    TextyImg.Create(72,40,Color(0,0,0));
    TextyImg.LoadFromFile("texty.png");
    Texture TextyTex;
    TextyTex.LoadFromImage(TextyImg);
    TextyTex.SetSmooth(false);
    Sprite Texty;
    Texty.SetColor(Color(255,255,255,255));
    Texty.SetPosition(160.f*ScreenMult,120.f*ScreenMult);
    Texty.SetScale(ScreenMult,ScreenMult);
    Texty.SetOrigin(36.f,4.f);
    Texty.SetTexture(TextyTex);
    Texty.SetSubRect(IntRect(0,24,72,8));

    SoundBuffer NasuTitleBuf;
    NasuTitleBuf.LoadFromFile("nasutitle.wav");
    Sound NasuTitle;
    NasuTitle.SetBuffer(NasuTitleBuf);
    NasuTitle.SetLoop(true);
    NasuTitle.SetVolume(35.f);

    SoundBuffer NasuGameBuf;
    NasuGameBuf.LoadFromFile("nasugame.wav");
    Sound NasuGame;
    NasuGame.SetBuffer(NasuGameBuf);
    NasuGame.SetLoop(true);
    NasuGame.SetVolume(35.f);

    SoundBuffer NasuOverBuf;
    NasuOverBuf.LoadFromFile("nasuover.wav");
    Sound NasuOver;
    NasuOver.SetBuffer(NasuOverBuf);
    NasuOver.SetVolume(35.f);

    SoundBuffer NasuStepBuf;
    NasuStepBuf.LoadFromFile("nasustep.wav");
    Sound NasuStep1;
    NasuStep1.SetBuffer(NasuStepBuf);
    NasuStep1.SetPitch(1.1f);
    NasuStep1.SetVolume(75.f);
    Sound NasuStep2;
    NasuStep2.SetBuffer(NasuStepBuf);
    NasuStep2.SetPitch(0.9f);
    NasuStep2.SetVolume(75.f);
    Sound NasuStep3;
    NasuStep3.SetBuffer(NasuStepBuf);
    NasuStep3.SetPitch(1.5f);

    SoundBuffer NasuGetBuf;
    NasuGetBuf.LoadFromFile("nasuget.wav");
    Sound NasuGet;
    NasuGet.SetBuffer(NasuGetBuf);
    Sound NasuBGet;
    NasuBGet.SetBuffer(NasuGetBuf);
    NasuBGet.SetPitch(1.1f);
    Sound NasuXGet;
    NasuXGet.SetBuffer(NasuGetBuf);
    NasuXGet.SetPitch(1.5f);

    SoundBuffer NasuLoseBuf;
    NasuLoseBuf.LoadFromFile("nasulose.wav");
    Sound NasuLose;
    NasuLose.SetBuffer(NasuLoseBuf);

    Image PointsImg;
    PointsImg.Create(40,36,Color(0,0,0));
    PointsImg.LoadFromFile("scores.png");
    Texture PointsTex;
    PointsTex.LoadFromImage(PointsImg);
    PointsTex.SetSmooth(false);
    Sprite Points1, Points2, Points3;
    Points1.SetColor(Color(255,255,255,255));
    Points1.SetPosition(-100.f,-100.f);
    Points1.SetScale(ScreenMult,ScreenMult);
    Points1.SetOrigin(20.f,6.f);
    Points1.SetTexture(PointsTex);
    Points1.SetSubRect(IntRect(0,0,40,12));
    Points2.SetColor(Color(255,255,255,255));
    Points2.SetPosition(-100.f,-100.f);
    Points2.SetScale(ScreenMult,ScreenMult);
    Points2.SetOrigin(20.f,6.f);
    Points2.SetTexture(PointsTex);
    Points2.SetSubRect(IntRect(0,12,40,12));
    Points3.SetColor(Color(255,255,255,255));
    Points3.SetPosition(-100.f,-100.f);
    Points3.SetScale(ScreenMult,ScreenMult);
    Points3.SetOrigin(20.f,6.f);
    Points3.SetTexture(PointsTex);
    Points3.SetSubRect(IntRect(0,24,40,12));

    float pts1time = 0.f, pts2time = 0.f, pts3time = 0.f;
    unsigned long int cscore = 0;
    unsigned short int GameState = 0;
    bool bJumping = false;
    float VelX = 0.f, VelY = 0.f, NasuB_X = 0.f, NasuB_Y = 0.f;
    float TimeUntilNextNasu = 1.0f, TimeUntilNextNasuB = 35.f;
    bool bLostGame = false;
    bool bDerp = false;
    float blink = 0.f;
    Vector2f Nasupos, Nasubpos, Playerpos;
    unsigned short int blinkcounter = 0;
    NasuTitle.Play();
    while (App.IsOpened())
    {
        Event Event;
        while (App.PollEvent(Event))
        {
            if ( (Event.Type == Event::Closed) || ((Event.Type == Event::KeyPressed) && (Event.Key.Code == Keyboard::Escape)) )
                App.Close();
            if ( Event.Type == Event::KeyPressed )
            {
                if ( Event.Key.Code == Keyboard::F12 )
                {
                    Image Screen = App.Capture();
#ifdef WINDOWS
                    Screen.SaveToFile("screenshot.png");
#else
                    char scrname[256];
                    snprintf(scrname,256,"%s/nasu_screenshot.png",getenv("HOME"));
                    Screen.SaveToFile(scrname);
#endif
                }
                if ( Event.Key.Code == Keyboard::F10 )
                {
                    bIsFullscreen = !bIsFullscreen;
                    if ( bIsFullscreen && VideoMode(320*ScreenMult,240*ScreenMult,32).IsValid() )
                        App.Create(VideoMode(320*ScreenMult,240*ScreenMult,32),"NASU",Style::Fullscreen);
                    else
                    {
                        bIsFullscreen = false;
                        App.Create(VideoMode(320*ScreenMult,240*ScreenMult,32),"NASU",Style::Titlebar|Style::Close);
                    }
                }
                if ( (Event.Key.Code == Keyboard::Return) && (GameState == 0) )
                {
                    NasuTitle.Stop();
                    GameState = 3;
                    Player.SetPosition(160.f*ScreenMult,149.f*ScreenMult);
                    TimeUntilNextNasu = 1.0f;
                    TimeUntilNextNasuB = 35.f;
                    Nasu.SetPosition(-100.f,-100.f);
                    NasuB.SetPosition(-100.f,-100.f);
                    bLostGame = false;
                    bDerp = false;
                    blinkcounter = 0;
                    blink = 0.f;
                    bJumping = false;
                    cscore = 0;
                    VelX = 0.f;
                    VelY = 0.f;
                    Texty.SetSubRect(IntRect(0,24,72,8));
                    pts1time = 0.f;
                    pts2time = 0.f;
                    pts3time = 0.f;
                }
                if ( Event.Key.Code == Keyboard::P )
                {
                    if ( (GameState == 1) && !bLostGame )
                    {
                        GameState = 4;
                        NasuGame.Pause();
                        Texty.SetSubRect(IntRect(0,32,72,8));
                    }
                    else if ( GameState == 4 )
                    {
                        GameState = 1;
                        NasuGame.Play();
                        Texty.SetSubRect(IntRect(0,24,72,8));
                    }
                }
                if ( !bLostGame && (GameState == 1) )
                {
                    if ( ((Event.Key.Code == Keyboard::Left) || (Event.Key.Code == Keyboard::A)) && ((Playerpos.x-12.f*ScreenMult) > (68.f*ScreenMult)) )
                    {
                        Player.FlipX(true);
                        VelX = -60.f*ScreenMult;
                    }
                    if ( ((Event.Key.Code == Keyboard::Right) || (Event.Key.Code == Keyboard::D)) && ((Playerpos.x+12.f*ScreenMult) < (252.f*ScreenMult)) )
                    {
                        Player.FlipX(false);
                        VelX = +60.f*ScreenMult;
                    }
                    if ( (Event.Key.Code == Keyboard::Up) || (Event.Key.Code == Keyboard::W) || (Event.Key.Code == Keyboard::Z) || (Event.Key.Code == Keyboard::LShift) )
                    {
                        if ( !bJumping )
                        {
                            VelY = -80.f*ScreenMult;
                            NasuStep3.Play();
                            bJumping = true;
                        }
                    }
                }
            }
            if ( Event.Type == Event::KeyReleased )
            {
                if ( !bLostGame && (GameState == 1) )
                {
                    if ( (Event.Key.Code == Keyboard::Left) || (Event.Key.Code == Keyboard::A) )
                    {
                        VelX = 0.f;
                    }
                    if ( (Event.Key.Code == Keyboard::Right) || (Event.Key.Code == Keyboard::D) )
                    {
                        VelX = 0.f;
                    }
                }
            }
        }
        App.Clear(Color(0,0,0));

        if ( GameState == 0 )
        {
            GameScr.SetSubRect(IntRect(0,130,184,130));
            App.Draw(GameScr);
            snprintf(scorenum,256,"%9lu",score);
            ScoreTxt.SetPosition(112.f*ScreenMult,144.f*ScreenMult);
            ScoreTxt.SetString("Hi Score: ");
            ScoreNumTxt.SetPosition(160.f*ScreenMult,144.f*ScreenMult);
            ScoreNumTxt.SetString(scorenum);
            App.Draw(ScoreTxt);
            App.Draw(ScoreNumTxt);
        }
        else if ( GameState == 2 )
        {
            GameScr.SetSubRect(IntRect(184,130,184,130));
            Texty.SetSubRect(IntRect(0,16,72,8));
            blink += (App.GetFrameTime()/1000.f);
            if ( blink >= 7.f )
            {
                NasuOver.Stop();
                NasuTitle.Play();
                GameState = 0;
            }
            App.Draw(GameScr);
            App.Draw(Texty);
        }
        else if ( GameState == 3 )
        {
            GameScr.SetSubRect(IntRect(0,0,184,130));
            blink += (App.GetFrameTime()/1000.f);
            if ( blink > 0.25f )
            {
                blinkcounter++;
                blink = 0.f;
                bDerp = !bDerp;
                if ( !bDerp )
                    Texty.SetSubRect(IntRect(0,24,72,8));
                else
                {
                    if ( blinkcounter < 7 )
                        Texty.SetSubRect(IntRect(0,0,72,8));
                    else
                        Texty.SetSubRect(IntRect(0,8,72,8));
                }
            }
            if ( blinkcounter >= 8 )
            {
                blinkcounter = 0;
                GameScr.SetSubRect(IntRect(0,0,184,130));
                GameState = 1;
                NasuGame.Play();
            }

            App.Draw(GameScr);
            Player.SetSubRect(IntRect(0,0,24,24));
            App.Draw(Nasu);
            App.Draw(NasuB);
            App.Draw(Player);
            snprintf(scorenum,256,"%9lu",cscore);
            ScoreTxt.SetPosition(168.f*ScreenMult,164.f*ScreenMult);
            ScoreTxt.SetString("Score:");
            ScoreNumTxt.SetPosition(200.f*ScreenMult,164.f*ScreenMult);
            ScoreNumTxt.SetString(scorenum);
            App.Draw(ScoreTxt);
            App.Draw(ScoreNumTxt);
            App.Draw(Texty);
        }
        else if ( GameState == 4 )
        {
            App.Draw(GameScr);
            App.Draw(Nasu);
            App.Draw(NasuB);
            App.Draw(Player);
            snprintf(scorenum,256,"%9lu",cscore);
            ScoreTxt.SetPosition(168.f*ScreenMult,164.f*ScreenMult);
            ScoreTxt.SetString("Score:");
            ScoreNumTxt.SetPosition(200.f*ScreenMult,164.f*ScreenMult);
            ScoreNumTxt.SetString(scorenum);
            App.Draw(ScoreTxt);
            App.Draw(ScoreNumTxt);
            App.Draw(Texty);
        }
        else
        {
            if ( bLostGame )
            {
                VelX = 0.f;
                VelY = 0.f;
                blink += (App.GetFrameTime()/1000.f);
                if ( blink > 0.125f )
                {
                    blinkcounter++;
                    blink = 0.f;
                    bDerp = !bDerp;
                    if ( bDerp )
                        GameScr.SetSubRect(IntRect(0,0,184,130));
                    else
                        GameScr.SetSubRect(IntRect(184,0,184,130));
                }
                if ( blinkcounter >= 12 )
                {
                    NasuOver.Play();
                    blink = 0.f;
                    GameState = 2;
                }
            }
            else
            {
                blink += (App.GetFrameTime()/1000.f);
                if ( blink > 0.125f )
                {
                    bDerp = !bDerp;
                    blink = 0.f;
                    if ( (VelX != 0.f) && !bJumping )
                    {
                        if ( bDerp )
                            NasuStep2.Play();
                        else
                            NasuStep1.Play();
                    }
                }

                if ( TimeUntilNextNasu > 0.f )
                {
                    TimeUntilNextNasu -= (App.GetFrameTime()/1000.f);
                    Nasu.SetPosition(-100.f,-100.f);
                    if ( TimeUntilNextNasu <= 0.f )
                    {
                        TimeUntilNextNasu = 0.f;
                        float pose = float(((random()%1600)+800)*ScreenMult)/10.f;
                        Nasu.SetPosition(pose,51.f*ScreenMult);
                    }
                }
                else
                {
                    Nasu.Move(0.f,+60.f*(App.GetFrameTime()/1000.f)*ScreenMult);
                    Nasupos = Nasu.GetPosition();
                    if ( ((Nasupos.x-(3.f*ScreenMult)) >= (Playerpos.x-(12.f*ScreenMult))) && ((Nasupos.x+(3.f*ScreenMult)) <= (Playerpos.x+(12.f*ScreenMult))) && ((Nasupos.y-(3.f*ScreenMult)) >= (Playerpos.y-(4.f*ScreenMult))) && ((Nasupos.y+(3.f*ScreenMult)) <= (Playerpos.y+(12.f*ScreenMult))) && bJumping )
                    {
                        if ( random()%36 == 0 )
                        {
                            cscore += 1000;
                            if ( cscore > score )
                                score = cscore;
                            TimeUntilNextNasu = 1.0f;
                            Nasu.SetPosition(-100.f,-100.f);
                            NasuXGet.Play();
                            pts3time += 0.75f;
                            Points3.SetPosition(Nasupos);
                        }
                        else
                        {
                            cscore += 10;
                            if ( cscore > score )
                                score = cscore;
                            TimeUntilNextNasu = 1.0f;
                            Nasu.SetPosition(-100.f,-100.f);
                            NasuGet.Play();
                            pts1time += 0.75f;
                            Points1.SetPosition(Nasupos);
                        }
                    }
                    else if ( Nasupos.y >= (161.f*ScreenMult) )
                    {
                        NasuGame.Stop();
                        NasuLose.Play();
                        bLostGame = true;
                    }
                }

                if ( TimeUntilNextNasuB > 0.f )
                {
                    TimeUntilNextNasuB -= (App.GetFrameTime()/1000.f);
                    NasuB.SetPosition(-100.f,-100.f);
                    if ( TimeUntilNextNasuB <= 0.f )
                    {
                        TimeUntilNextNasuB = 0.f;
                        int decideposb = random()%2;
                        switch(decideposb)
                        {
                        case 0:
                            NasuB_X = +60.f*ScreenMult;
                            NasuB.SetPosition(0.f,157.f*ScreenMult);
                            NasuB.SetSubRect(IntRect(8,0,8,8));
                            break;
                        default:
                            NasuB_X = -60.f*ScreenMult;
                            NasuB.SetPosition(320.f*ScreenMult,157.f*ScreenMult);
                            NasuB.SetSubRect(IntRect(0,8,8,8));
                            break;
                        }
                    }
                }
                else
                {
                    NasuB_Y += 125.f*(App.GetFrameTime()/1000.f)*ScreenMult;
                    NasuB.Move(NasuB_X*(App.GetFrameTime()/1000.f),NasuB_Y*(App.GetFrameTime()/1000.f));
                    Nasubpos = NasuB.GetPosition();
                    if ( ((Nasubpos.x-(3.f*ScreenMult)) >= (Playerpos.x-(12.f*ScreenMult))) && ((Nasubpos.x+(3.f*ScreenMult)) <= (Playerpos.x+(12.f*ScreenMult))) && ((Nasubpos.y-(3.f*ScreenMult)) >= (Playerpos.y-(4.f*ScreenMult))) && ((Nasubpos.y+(3.f*ScreenMult)) <= (Playerpos.y+(12.f*ScreenMult))) && bJumping )
                    {
                        cscore += 300;
                        if ( cscore > score )
                            score = cscore;
                        TimeUntilNextNasuB = 35.f;
                        NasuB.SetPosition(-100.f,-100.f);
                        NasuBGet.Play();
                        pts2time += 0.75f;
                        Points2.SetPosition(Nasubpos);
                    }
                    if ( (NasuB_X > 0.f) && (Nasubpos.x-(4.f*ScreenMult)) > (252.f*ScreenMult) )
                    {
                        TimeUntilNextNasuB = 35.f;
                        NasuB.SetPosition(-100.f,-100.f);
                    }
                    if ( (NasuB_X < 0.f) && (Nasubpos.x+(4.f*ScreenMult)) < (68.f*ScreenMult) )
                    {
                        TimeUntilNextNasuB = 35.f;
                        NasuB.SetPosition(-100.f,-100.f);
                    }
                    if ( Nasubpos.y >= (157.f*ScreenMult) )
                        NasuB_Y = -80.f*ScreenMult;
                }

                Player.Move(VelX*(App.GetFrameTime()/1000.f),VelY*(App.GetFrameTime()/1000.f));
                VelY += 480.f*(App.GetFrameTime()/1000.f)*ScreenMult;
                Playerpos = Player.GetPosition();
                if ( Playerpos.y >= 149.f*ScreenMult )
                {
                    Player.SetPosition(Playerpos.x,149.f*ScreenMult);
                    bJumping = false;
                }
            }
            App.Draw(GameScr);
            Playerpos = Player.GetPosition();
            if ( ((Playerpos.x-(12.f*ScreenMult)) <= (68.f*ScreenMult)) || ((Playerpos.x+(12.f*ScreenMult)) >= (252.f*ScreenMult)) )
            {
                if ( (Playerpos.x-(12.f*ScreenMult)) <= (68.f*ScreenMult) )
                    Player.SetPosition((68.f+12.f)*ScreenMult,Playerpos.y);
                if ( (Playerpos.x+(12.f*ScreenMult)) >= (252.f*ScreenMult) )
                    Player.SetPosition((252.f-12.f)*ScreenMult,Playerpos.y);
                VelX = 0.f;
            }
            if ( bJumping )
                Player.SetSubRect(IntRect(72,0,24,24));
            else if ( VelX != 0.f )
            {
                if ( bDerp )
                    Player.SetSubRect(IntRect(48,0,24,24));
                else
                    Player.SetSubRect(IntRect(24,0,24,24));
            }
            else
                Player.SetSubRect(IntRect(0,0,24,24));
            App.Draw(Nasu);
            App.Draw(NasuB);
            App.Draw(Player);

            if ( pts1time > 0.f )
            {
                pts1time -= (App.GetFrameTime()/1000.f);
                Points1.Move(0,-50.f*(App.GetFrameTime()/1000.f));
                App.Draw(Points1);
            }
            if ( pts2time > 0.f )
            {
                pts2time -= (App.GetFrameTime()/1000.f);
                Points2.Move(0,-50.f*(App.GetFrameTime()/1000.f));
                App.Draw(Points2);
            }
            if ( pts3time > 0.f )
            {
                pts3time -= (App.GetFrameTime()/1000.f);
                Points3.Move(0,-50.f*(App.GetFrameTime()/1000.f));
                App.Draw(Points3);
            }

            snprintf(scorenum,256,"%9lu",cscore);
            ScoreTxt.SetPosition(168.f*ScreenMult,164.f*ScreenMult);
            ScoreTxt.SetString("Score:");
            ScoreNumTxt.SetPosition(200.f*ScreenMult,164.f*ScreenMult);
            ScoreNumTxt.SetString(scorenum);
            App.Draw(ScoreTxt);
            App.Draw(ScoreNumTxt);
        }
        App.Draw(MainFrame);
        App.Display();
    }

    return EXIT_SUCCESS;
}
