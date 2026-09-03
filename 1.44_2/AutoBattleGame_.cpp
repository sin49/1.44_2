// ==============================================================================
// AutoBattleGame.cpp  -  144MB AutoChess Survival  (Pacing & UI Overhaul)
// ==============================================================================
#include <windows.h>
#include <mmsystem.h>
#include <math.h>
#include <vector>
#include <string>
#include <algorithm>
#include <gdiplus.h>
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"gdiplus.lib")

#define MAX_UNITS  8000
#define MAX_PARTS  3000
#define CELL_SIZE  150
#define COLS (15000/CELL_SIZE)
#define ROWS (15000/CELL_SIZE)
#ifndef M_PI
#define M_PI 3.14159265f
#endif

// ??????????????????????????????????????????????
enum class UnitType : unsigned char {
    Red,Green,Blue,Purple,
    Warrior,Sniper,Healer,Warlock,Assassin,Paladin,Necromancer,Elementalist,
    Hero,Demigod,Vanguard,Ranger,Archmage,DeathKnight,BloodPriest,Berserker,Templar,IceMage,
    Supreme,Titan,Phantom,Lich,Saint,DragonKnight,Valkyrie,StormBringer,
    GodOfWar,Asura,Overlord,Seraphim,Cosmic,Chronos,Abaddon,
    Skeleton,Spirit,Coin,Enemy,Boss,Projectile,
    Tree,Rock,Ruins,Windmill,Car,Obelisk,Statue
};
enum class GameState { Title,Playing,Paused,GameOver,Settings };

struct GameUnit {
    float x,y,z,vx,vy,vz;
    UnitType type; bool active; unsigned char team;
    float hp,maxHp,attackCooldown,attackAnimTimer;
    float facingYaw,walkTimer;
    bool selected,hasTarget;
    float targetX,targetY,damage,scale,speed;
    bool isRanged; int targetId;
    float patrolX,patrolY,hitFlashTimer;
};
struct Recipe { UnitType res,a,b,c; std::wstring name; Gdiplus::Color col; };
struct RenderItem { int id; float distSq; bool operator<(const RenderItem&o)const{return distSq>o.distSq;} };
struct MergeSuggestion { int recipeIdx; int unitA,unitB,unitC; };
struct Particle { float x,y,z,vx,vy,vz,life,maxLife,size; unsigned char r,g,b,a,type; bool active; };

// ??????????????????????????????????????????????
class CGameScene {
public:
    HWND mainHwnd=nullptr;
    GameUnit units[MAX_UNITS];
    int gridHead[COLS*ROWS], gridNext[MAX_UNITS];
    Particle parts[MAX_PARTS];

    HMIDIOUT midiHandle=nullptr;
    int  currentBeat=-1;
    float audioTime=0,gameTime=0,deltaTime=0.016f,fps=60;
    float enemySpawnTimer=0,treeSpawnTimer=0;
    bool  bossSpawned=false;

    float camX=7500,camY=6500,camZ=800;
    float pitch=0.785f,fov=1000, cx=640,cy=360, shakeAmt=0;
    float mouseWX=7500,mouseWY=7200;
    POINT lastRmbPos={0,0}; bool rmbDown=false, lmbDown=false; POINT lmbStart={0,0}, dragEnd={0,0}; bool isDragging=false;
    float threatX=0, threatY=-1.f;
    float playerCenterX=7500, playerCenterY=7200;

    int money=100;
    GameState state=GameState::Title, prevState=GameState::Title;
    bool showRecipe=false; float recipeScrollY=0;

    std::vector<float> highScores;
    std::vector<Recipe> recipes;
    std::vector<MergeSuggestion> mergeSuggestions;
    float mergeCheckTimer=0, btnPauseX=0,btnPauseY=0,btnPauseW=170,btnPauseH=44;

    float volMaster=1.f, volBgm=0.6f, volSfx=1.f;
    bool isFullscreen=false; WINDOWPLACEMENT wpc;

    CGameScene(){
        memset(units,0,sizeof(units)); memset(parts,0,sizeof(parts));
        using namespace Gdiplus;
        recipes={
            {UnitType::Warrior,   UnitType::Red,   UnitType::Red,   UnitType::Red,   L"Warrior",    Color(200,180,40,40)},
            {UnitType::Sniper,    UnitType::Blue,  UnitType::Blue,  UnitType::Blue,  L"Sniper",     Color(200,60,160,220)},
            {UnitType::Healer,    UnitType::Green, UnitType::Green, UnitType::Green, L"Healer",     Color(200,60,200,120)},
            {UnitType::Warlock,   UnitType::Purple,UnitType::Purple,UnitType::Purple,L"Warlock",    Color(200,160,40,200)},
            {UnitType::Assassin,  UnitType::Red,   UnitType::Red,   UnitType::Blue,  L"Assassin",   Color(200,220,50,80)},
            {UnitType::Paladin,   UnitType::Red,   UnitType::Green, UnitType::Green, L"Paladin",    Color(200,218,165,32)},
            {UnitType::Necromancer,UnitType::Purple,UnitType::Purple,UnitType::Blue, L"Necromancer",Color(200,120,80,200)},
            {UnitType::Elementalist,UnitType::Red, UnitType::Blue,  UnitType::Green, L"Elementalist",Color(200,0,200,200)},
            // Tier 3
            {UnitType::Hero,      UnitType::Warrior,UnitType::Paladin,UnitType::Healer,L"Hero",     Color(200,255,215,0)},
            {UnitType::Demigod,   UnitType::Elementalist,UnitType::Warlock,UnitType::Necromancer,L"Demigod",Color(200,200,80,255)},
            {UnitType::Vanguard,  UnitType::Warrior,UnitType::Paladin,UnitType::Assassin,L"Vanguard",Color(200,255,100,50)},
            {UnitType::Ranger,    UnitType::Sniper,UnitType::Sniper,UnitType::Assassin,L"Ranger",Color(200,100,200,50)},
            {UnitType::Archmage,  UnitType::Elementalist,UnitType::Warlock,UnitType::Healer,L"Archmage",Color(200,50,150,255)},
            {UnitType::DeathKnight,UnitType::Paladin,UnitType::Necromancer,UnitType::Warrior,L"DeathKnight",Color(200,50,0,50)},
            {UnitType::BloodPriest,UnitType::Healer,UnitType::Assassin,UnitType::Warlock,L"BloodPriest",Color(200,255,0,0)},
            {UnitType::Berserker, UnitType::Warrior,UnitType::Assassin,UnitType::Red,L"Berserker",Color(200,200,20,20)},
            {UnitType::Templar,   UnitType::Paladin,UnitType::Healer,UnitType::Green,L"Templar",Color(200,100,255,100)},
            {UnitType::IceMage,   UnitType::Elementalist,UnitType::Sniper,UnitType::Blue,L"IceMage",Color(200,100,200,255)},
            // Tier 4
            {UnitType::Supreme,   UnitType::Hero,  UnitType::Demigod,UnitType::Assassin,L"Supreme", Color(200,255,255,255)},
            {UnitType::Titan,     UnitType::Vanguard,UnitType::Hero,UnitType::Paladin,L"Titan", Color(200,180,180,180)},
            {UnitType::Phantom,   UnitType::Ranger,UnitType::Assassin,UnitType::DeathKnight,L"Phantom", Color(200,80,80,100)},
            {UnitType::Lich,      UnitType::Archmage,UnitType::Demigod,UnitType::Necromancer,L"Lich", Color(200,0,255,150)},
            {UnitType::Saint,     UnitType::Hero,UnitType::BloodPriest,UnitType::Archmage,L"Saint", Color(200,255,240,200)},
            {UnitType::DragonKnight,UnitType::Vanguard,UnitType::Ranger,UnitType::Elementalist,L"DragonKnt", Color(200,255,100,0)},
            {UnitType::Valkyrie,  UnitType::Templar,UnitType::Hero,UnitType::Ranger,L"Valkyrie", Color(200,255,200,255)},
            {UnitType::StormBringer,UnitType::IceMage,UnitType::Archmage,UnitType::Vanguard,L"StormBrngr", Color(200,50,255,255)},
            // Tier 5
            {UnitType::GodOfWar,  UnitType::Titan,UnitType::Supreme,UnitType::Vanguard,L"GodOfWar", Color(200,255,50,50)},
            {UnitType::Asura,     UnitType::Phantom,UnitType::Supreme,UnitType::Ranger,L"Asura", Color(200,100,0,0)},
            {UnitType::Overlord,  UnitType::Lich,UnitType::Titan,UnitType::DeathKnight,L"Overlord", Color(200,50,0,100)},
            {UnitType::Seraphim,  UnitType::Saint,UnitType::Supreme,UnitType::Hero,L"Seraphim", Color(200,255,255,255)},
            {UnitType::Cosmic,    UnitType::DragonKnight,UnitType::Lich,UnitType::Archmage,L"Cosmic", Color(200,50,50,255)},
            {UnitType::Chronos,   UnitType::StormBringer,UnitType::Lich,UnitType::Supreme,L"Chronos", Color(200,200,0,255)},
            {UnitType::Abaddon,   UnitType::DeathKnight,UnitType::Phantom,UnitType::Berserker,L"Abaddon", Color(200,30,30,30)},
        };
    }

    void Initialize(HWND hwnd){ mainHwnd=hwnd; GoToTitle(); midiOutOpen(&midiHandle,0,0,0,0); midiOutShortMsg(midiHandle,0x007F00C0); midiOutShortMsg(midiHandle,0x003A01C0); }
    void Release(){ if(midiHandle)midiOutClose(midiHandle); }
    void ToggleFullscreen(){
        if(!mainHwnd) return;
        DWORD style = GetWindowLong(mainHwnd, GWL_STYLE);
        if(!isFullscreen) {
            GetWindowPlacement(mainHwnd, &wpc); HMONITOR hmon = MonitorFromWindow(mainHwnd, MONITOR_DEFAULTTONEAREST); MONITORINFO mi = { sizeof(mi) }; GetMonitorInfo(hmon, &mi);
            SetWindowLong(mainHwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(mainHwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            isFullscreen = true;
        } else {
            SetWindowLong(mainHwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW); SetWindowPlacement(mainHwnd, &wpc);
            SetWindowPos(mainHwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            isFullscreen = false;
        }
    }

    int GetRole(UnitType t){
        switch(t){
            case UnitType::Warrior: case UnitType::Paladin: case UnitType::Hero: case UnitType::Skeleton: case UnitType::Vanguard: case UnitType::DeathKnight: case UnitType::Titan: case UnitType::DragonKnight: case UnitType::GodOfWar: case UnitType::Overlord: case UnitType::Berserker: case UnitType::Templar: case UnitType::Valkyrie: case UnitType::Abaddon: return 0;
            case UnitType::Red: case UnitType::Green: case UnitType::Assassin: case UnitType::Phantom: case UnitType::Asura: return 1;
            default: return 2;
        }
    }

    int GetTier(UnitType t){
        switch(t){
            case UnitType::Red: case UnitType::Green: case UnitType::Blue: case UnitType::Purple: return 1;
            case UnitType::Warrior: case UnitType::Sniper: case UnitType::Healer: case UnitType::Warlock: case UnitType::Assassin: case UnitType::Paladin: case UnitType::Necromancer: case UnitType::Elementalist: return 2;
            case UnitType::Hero: case UnitType::Demigod: case UnitType::Vanguard: case UnitType::Ranger: case UnitType::Archmage: case UnitType::DeathKnight: case UnitType::BloodPriest: case UnitType::Berserker: case UnitType::Templar: case UnitType::IceMage: return 3;
            case UnitType::Supreme: case UnitType::Titan: case UnitType::Phantom: case UnitType::Lich: case UnitType::Saint: case UnitType::DragonKnight: case UnitType::Valkyrie: case UnitType::StormBringer: return 4;
            case UnitType::GodOfWar: case UnitType::Asura: case UnitType::Overlord: case UnitType::Seraphim: case UnitType::Cosmic: case UnitType::Chronos: case UnitType::Abaddon: return 5;
            default: return 0;
        }
    }

    const wchar_t* TypeName(UnitType t){
        switch(t){
            case UnitType::Red: return L"Red"; case UnitType::Green: return L"Green"; case UnitType::Blue: return L"Blue"; case UnitType::Purple: return L"Purple";
            case UnitType::Warrior:return L"Warrior"; case UnitType::Assassin:return L"Assassin"; case UnitType::Paladin:return L"Paladin"; case UnitType::Sniper: return L"Sniper";
            case UnitType::Healer: return L"Healer"; case UnitType::Necromancer:return L"Necro"; case UnitType::Elementalist:return L"Elemntlst"; case UnitType::Warlock:return L"Warlock";
            case UnitType::Hero: return L"Hero"; case UnitType::Demigod:return L"Demigod"; case UnitType::Vanguard:return L"Vanguard"; case UnitType::Ranger:return L"Ranger";
            case UnitType::Archmage:return L"Archmage"; case UnitType::DeathKnight:return L"DeathKnt"; case UnitType::BloodPriest:return L"B.Priest";
            case UnitType::Berserker:return L"Berserker"; case UnitType::Templar:return L"Templar"; case UnitType::IceMage:return L"IceMage";
            case UnitType::Supreme:return L"Supreme"; case UnitType::Titan:return L"Titan"; case UnitType::Phantom:return L"Phantom"; case UnitType::Lich:return L"Lich";
            case UnitType::Saint:return L"Saint"; case UnitType::DragonKnight:return L"DragonKnt"; case UnitType::Valkyrie:return L"Valkyrie"; case UnitType::StormBringer:return L"StormBgr";
            case UnitType::GodOfWar:return L"GodOfWar"; case UnitType::Asura:return L"Asura"; case UnitType::Overlord:return L"Overlord"; case UnitType::Seraphim:return L"Seraphim"; case UnitType::Cosmic:return L"Cosmic";
            case UnitType::Chronos:return L"Chronos"; case UnitType::Abaddon:return L"Abaddon";
            default: return L"???";
        }
    }

    const wchar_t* StarStr(int tier){ switch(tier){ case 1:return L"[*]"; case 2:return L"[**]"; case 3:return L"[***]"; case 4:return L"[****]"; case 5:return L"[*****]"; default:return L""; } }

    void ProcessKey(int key){
        if(key==VK_ESCAPE){
            if(state==GameState::Title) PostQuitMessage(0);
            else if(state==GameState::Settings) state=prevState;
            else { prevState=state; state=GameState::Settings; }
            return;
        }
        if(state==GameState::Title){ if(key==VK_RETURN||key==VK_SPACE) RestartGame(); return; }
        if(state==GameState::GameOver){ if(key=='R') RestartGame(); if(key=='T') GoToTitle(); return; }
        if(state==GameState::Settings) return;
        if(key==VK_TAB){ showRecipe=!showRecipe; state=showRecipe?GameState::Paused:GameState::Playing; recipeScrollY=0; }
        if(key=='P'){ if(state==GameState::Playing)state=GameState::Paused; else if(state==GameState::Paused&&!showRecipe)state=GameState::Playing; }
        if(key==VK_SPACE&&money>=10&&state==GameState::Playing){ money-=10; SpawnUnit(playerCenterX+(rand()%200-100),playerCenterY+(rand()%200-100),0,(UnitType)(rand()%4)); }
        int mIdx=key>='1'&&key<='8'?key-'1':-1;
        if(mIdx>=0&&mIdx<(int)mergeSuggestions.size()){
            auto&s=mergeSuggestions[mIdx];
            if(s.unitA!=-1&&units[s.unitA].active&&units[s.unitB].active&&units[s.unitC].active){
                auto&r=recipes[s.recipeIdx]; SpawnDeathParticles(units[s.unitA].x,units[s.unitA].y,units[s.unitA].z,r.col,20);
                units[s.unitA].type=r.res; SetupStats(units[s.unitA]); units[s.unitB].active=false; units[s.unitC].active=false; BuildMergeSuggestions();
                PlayMidiNote(9, 49, 120, true); // crash cymbal on merge
            }
        }
    }

    void ProcessMouse(UINT msg,int mx,int my,int wheel){
        if(state==GameState::Settings){
            if(msg==WM_LBUTTONUP){
                float bx=cx-150;
                if(mx>=bx&&mx<=bx+300&&my>=cy-180&&my<=cy-130) state=prevState;
                if(mx>=bx&&mx<=bx+300&&my>=cy-110&&my<=cy-60) ToggleFullscreen();
                if(my>=cy-30&&my<=cy+10){ if(mx>=bx&&mx<=bx+50)volMaster=max(0.f,volMaster-0.1f); if(mx>=bx+250&&mx<=bx+300)volMaster=min(1.f,volMaster+0.1f); }
                if(my>=cy+40&&my<=cy+80){ if(mx>=bx&&mx<=bx+50)volBgm=max(0.f,volBgm-0.1f); if(mx>=bx+250&&mx<=bx+300)volBgm=min(1.f,volBgm+0.1f); }
                if(my>=cy+110&&my<=cy+150){ if(mx>=bx&&mx<=bx+50)volSfx=max(0.f,volSfx-0.1f); if(mx>=bx+250&&mx<=bx+300)volSfx=min(1.f,volSfx+0.1f); }
                if(mx>=bx&&mx<=bx+300&&my>=cy+180&&my<=cy+230) GoToTitle();
            }
            return;
        }
        if(state==GameState::Playing||state==GameState::Paused){ Gdiplus::PointF w=ScreenToWorld((float)mx,(float)my); mouseWX=w.X; mouseWY=w.Y; }
        if(state==GameState::Title){ if(msg==WM_LBUTTONUP){ float bx=cx-150,by=cy+100; if(mx>=bx&&mx<=bx+300&&my>=by&&my<=by+70) RestartGame(); } return; }
        if(state==GameState::GameOver){
            if(msg==WM_LBUTTONUP){ if(mx>=cx-160&&mx<=cx-20&&my>=cy+80&&my<=cy+130) RestartGame(); if(mx>=cx+20&&mx<=cx+160&&my>=cy+80&&my<=cy+130) GoToTitle(); }
            return;
        }
        if(msg==WM_LBUTTONUP){ if(mx>=btnPauseX&&mx<=btnPauseX+btnPauseW&&my>=btnPauseY&&my<=btnPauseY+btnPauseH){ if(state==GameState::Playing)state=GameState::Paused; else if(state==GameState::Paused&&!showRecipe)state=GameState::Playing; return; } }
        if(msg==WM_MOUSEWHEEL){
            if(showRecipe){ recipeScrollY+=wheel*100.f; if(recipeScrollY>0)recipeScrollY=0; }
            else { camZ-=wheel*150.f; if(camZ<250)camZ=250; if(camZ>3500)camZ=3500; }
        }
        if(msg==WM_RBUTTONDOWN){rmbDown=true;lastRmbPos={mx,my};} if(msg==WM_RBUTTONUP){rmbDown=false;}
        if(msg==WM_MOUSEMOVE&&rmbDown){ float dx=(mx-lastRmbPos.x)*(camZ/fov)*2.f, dy=(my-lastRmbPos.y)*(camZ/fov)*2.f; camX-=dx; camY+=dy; lastRmbPos={mx,my}; }
        if(msg==WM_LBUTTONDOWN){lmbDown=true;isDragging=true;lmbStart={mx,my};dragEnd={mx,my};} if(msg==WM_MOUSEMOVE&&lmbDown){dragEnd={mx,my};}
        if(msg==WM_LBUTTONUP){ lmbDown=false; isDragging=false; }
    }

    Gdiplus::PointF ScreenToWorld(float sx,float sy){
        float S=(cy-sy)/fov, cosP=cosf(pitch),sinP=sinf(pitch), denom=cosP-S*sinP;
        if(fabsf(denom)<0.0001f) return Gdiplus::PointF(camX,camY);
        float ty=camZ*(sinP+S*cosP)/denom, wx=(sx-cx)*(ty*sinP+camZ*cosP)/fov+camX, wy=ty+camY;
        return Gdiplus::PointF(wx,wy);
    }

    void BuildMergeSuggestions(){
        mergeSuggestions.clear();
        for(int ri=0;ri<(int)recipes.size();ri++){
            auto&r=recipes[ri]; int ua=-1,ub=-1,uc=-1;
            for(int i=0;i<MAX_UNITS;i++) if(units[i].active&&units[i].team==0&&units[i].type==r.a){ua=i;break;} if(ua==-1) continue;
            for(int i=0;i<MAX_UNITS;i++) if(i!=ua&&units[i].active&&units[i].team==0&&units[i].type==r.b){ub=i;break;} if(ub==-1) continue;
            for(int i=0;i<MAX_UNITS;i++) if(i!=ua&&i!=ub&&units[i].active&&units[i].team==0&&units[i].type==r.c){uc=i;break;} if(uc==-1) continue;
            mergeSuggestions.push_back({ri,ua,ub,uc});
        }
    }

    int AllocParticle(){ for(int i=0;i<MAX_PARTS;i++) if(!parts[i].active) return i; return -1; }
    void SpawnBlood(float x,float y,float z,int n=6){
        for(int k=0;k<n;k++){
            int i=AllocParticle(); if(i<0)break;
            float ang=((float)rand()/RAND_MAX)*M_PI*2.f; float spd=100.f+rand()%300;
            parts[i]={x,y,z+(rand()%20), cosf(ang)*spd,sinf(ang)*spd*0.5f,50.f+rand()%150, 0.4f+((float)rand()/RAND_MAX)*0.3f,0.5f,3.f+rand()%5, (unsigned char)(180+rand()%75),10,10,255,0,true};
        }
    }
    void SpawnSpark(float x,float y,float z,Gdiplus::Color col,int n=5){
        for(int k=0;k<n;k++){
            int i=AllocParticle(); if(i<0)break;
            float ang=((float)rand()/RAND_MAX)*M_PI*2.f; float spd=200.f+rand()%400;
            parts[i]={x,y,z+(rand()%15),cosf(ang)*spd,sinf(ang)*spd*0.4f,80.f+rand()%200, 0.15f+((float)rand()/RAND_MAX)*0.15f,0.25f,2.f+rand()%3, col.GetR(),col.GetG(),col.GetB(),255,2,true};
        }
    }
    void SpawnFire(float x,float y,float z,int n=4){
        for(int k=0;k<n;k++){
            int i=AllocParticle(); if(i<0)break;
            float ang=((float)rand()/RAND_MAX)*M_PI*2.f; float spd=30.f+rand()%80;
            parts[i]={x,y,z,cosf(ang)*spd,sinf(ang)*spd*0.3f,60.f+rand()%120, 0.5f+((float)rand()/RAND_MAX)*0.5f,1.f,5.f+rand()%8, 255,(unsigned char)(80+rand()%100),0,200,1,true};
        }
    }
    void SpawnDeathParticles(float x,float y,float z,Gdiplus::Color col,int n=15){
        for(int k=0;k<n;k++){
            int i=AllocParticle(); if(i<0)break;
            float ang=((float)rand()/RAND_MAX)*M_PI*2.f; float spd=80.f+rand()%400; float frac=(float)rand()/RAND_MAX;
            parts[i]={x,y,z+(rand()%30),cosf(ang)*spd,sinf(ang)*spd*0.5f,80.f+rand()%200, 0.6f+frac*0.6f,1.2f,4.f+rand()%10, (unsigned char)(frac>0.5f?col.GetR():255),(unsigned char)(frac>0.5f?col.GetG():200),(unsigned char)(frac>0.5f?col.GetB():0),255,2,true};
        }
    }
    void SpawnSmoke(float x,float y,float z,int n=3){
        for(int k=0;k<n;k++){
            int i=AllocParticle(); if(i<0)break;
            float ang=((float)rand()/RAND_MAX)*M_PI*2.f; float spd=20.f+rand()%50;
            parts[i]={x,y,z,cosf(ang)*spd,sinf(ang)*spd*0.2f,40.f+rand()%60, 0.8f+((float)rand()/RAND_MAX)*0.8f,1.5f,8.f+rand()%12, (unsigned char)(80+rand()%60),(unsigned char)(80+rand()%60),(unsigned char)(80+rand()%60),160,3,true};
        }
    }

    void AddShake(float power){ shakeAmt=min(shakeAmt+power,25.f); }

    void Update(float dt){
        deltaTime=dt; if(deltaTime>0.1f)deltaTime=0.1f; fps=1.f/deltaTime; audioTime+=deltaTime;
        int nb=(int)(audioTime*4.f);
        if(nb>currentBeat){
            currentBeat=nb; int step=currentBeat%32;
            if(step%4==0) PlayMidiNote(9,36,110,true);
            if(step%8==4) PlayMidiNote(9,38,100,true);
            if(step%2==0) PlayMidiNote(9,42,70,true);
            int bass[]={36,36,39,36,36,36,39,36,41,41,44,41,41,41,44,41};
            if(step%2==0) PlayMidiNote(0,bass[(step/2)%16],90,true);
            if(step==0||step==6||step==12||step==20) PlayMidiNote(1,60+(step%5)*2,100,true);
        }
        shakeAmt*=1.f-8.f*deltaTime; if(shakeAmt<0.01f)shakeAmt=0;
        if(state==GameState::Playing) UpdateLogic();
        UpdateParticles();
        mergeCheckTimer-=deltaTime; if(mergeCheckTimer<=0){mergeCheckTimer=1.5f;BuildMergeSuggestions();}
    }

    void UpdateParticles(){
        for(int i=0;i<MAX_PARTS;i++){
            if(!parts[i].active) continue;
            parts[i].life-=deltaTime; if(parts[i].life<=0){parts[i].active=false;continue;}
            if(parts[i].type==0) parts[i].vz-=400.f*deltaTime; else if(parts[i].type==1) parts[i].vz+=50.f*deltaTime;
            parts[i].vx*=0.96f; parts[i].vy*=0.96f; parts[i].x+=parts[i].vx*deltaTime; parts[i].y+=parts[i].vy*deltaTime; parts[i].z+=parts[i].vz*deltaTime;
            if(parts[i].z<0&&parts[i].type==0) parts[i].z=0;
        }
    }

    void Draw(Gdiplus::Graphics*g){
        using namespace Gdiplus;
        Font hugeFont(L"Consolas",32,FontStyleBold); Font bigFont (L"Consolas",22,FontStyleBold);
        Font midFont (L"Consolas",15,FontStyleBold); Font smallFont(L"Consolas",12,FontStyleBold);
        SolidBrush gold(Color(255,255,215,0)), white(Color(255,255,255,255)), gray(Color(255,160,160,160));

        if(state==GameState::Title){
            g->Clear(Color(255,20,25,30));
            g->DrawString(L"144MB AutoChess",-1,&hugeFont,PointF(cx-220,cy-220),&gold);
            g->DrawString(L"  Survival",     -1,&hugeFont,PointF(cx-220,cy-175),&gold);
            g->DrawString(L"--- TOP 3 RANKING ---",-1,&bigFont,PointF(cx-195,cy-100),&white);
            if(highScores.empty()) g->DrawString(L"No Records Yet",-1,&midFont,PointF(cx-90,cy-55),&gray);
            else for(int i=0;i<(int)highScores.size()&&i<3;i++){
                wchar_t buf[64]; swprintf_s(buf,L"#%d  %.0f sec",i+1,highScores[i]);
                SolidBrush rb(i==0?Color(255,255,215,0):i==1?Color(255,192,192,192):Color(255,205,127,50));
                g->DrawString(buf,-1,&midFont,PointF(cx-90,cy-55+i*28),&rb);
            }
            SolidBrush btnBg(Color(220,60,120,60)); Pen btnPen(Color(255,100,220,100),2);
            g->FillRectangle(&btnBg,cx-150,cy+100,300.f,70.f); g->DrawRectangle(&btnPen,cx-150,cy+100,300.f,70.f);
            g->DrawString(L"GAME START",-1,&bigFont,PointF(cx-95,cy+118),&white);
            g->DrawString(L"[SPACE] or click",-1,&smallFont,PointF(cx-95,cy+190),&gray);
            g->DrawString(L"[ESC] Settings/Quit",-1,&smallFont,PointF(15,15),&gray); return;
        }

        if(state==GameState::GameOver){
            g->Clear(Color(255,20,0,0)); g->DrawString(L"GAME OVER",-1,&hugeFont,PointF(cx-150,cy-100),&gold);
            wchar_t tb[64]; swprintf_s(tb,L"Survived: %02d:%02d",(int)(gameTime/60),(int)gameTime%60); g->DrawString(tb,-1,&bigFont,PointF(cx-130,cy-30),&white);
            SolidBrush rBtn(Color(220,60,120,60)),gBtn(Color(220,60,60,120)); Pen pPen(Color(255,200,200,200),1);
            g->FillRectangle(&rBtn,(float)(cx-160),(float)(cy+80),140.f,50.f); g->DrawRectangle(&pPen,(float)(cx-160),(float)(cy+80),140.f,50.f); g->DrawString(L"[R] Restart",-1,&smallFont,PointF(cx-145,cy+95),&white);
            g->FillRectangle(&gBtn,(float)(cx+20),(float)(cy+80),140.f,50.f); g->DrawRectangle(&pPen,(float)(cx+20),(float)(cy+80),140.f,50.f); g->DrawString(L"[T] Title",-1,&smallFont,PointF(cx+35,cy+95),&white); return;
        }

        float sx=((float)(rand()%100)/100.f-0.5f)*shakeAmt*2.f, sy=((float)(rand()%100)/100.f-0.5f)*shakeAmt; cx+=sx; cy+=sy;
        g->Clear(Color(255,40,50,40));

        Pen grassPen(Color(255,50,75,45),2);
        float sX=floorf(camX/200.f)*200.f-3000.f,sY=floorf(camY/200.f)*200.f-3000.f; int sGX=(int)(sX/200.f),sGY=(int)(sY/200.f);
        for(int i=0;i<30;i++) for(int j=0;j<30;j++){
            int h=(sGX+i)*37+(sGY+j)*17; if(h%3==0){ float gx=sX+i*200.f+(h%150), gy=sY+j*200.f+((h/10)%150); PointF p1=Project(gx,gy,0), p2=Project(gx+15,gy-15,5); g->DrawLine(&grassPen,p1,p2); }
        }

        std::vector<RenderItem> items;
        for(int i=0;i<MAX_UNITS;i++) if(units[i].active){
            float dx=units[i].x-camX,dy=units[i].y-camY;
            if(units[i].team==3 && (dx*dx+dy*dy)>8000000.f) continue;
            float dz=units[i].z-camZ; items.push_back({i,dy*dy+dz*dz});
        }
        std::sort(items.begin(),items.end());
        for(auto&it:items){
            GameUnit&u=units[it.id];
            if(u.team==4) Draw3DBox(g,u.x,u.y,u.z,3,3,3,u.facingYaw,Color(255,0,255,255));
            else if(u.team==3){ DrawEnvironment(g, u); }
            else if(u.team==2){float lz=u.z+sinf(audioTime*5.f+it.id)*5.f+10.f;Draw3DBox(g,u.x,u.y,lz,6,6,2,audioTime*5.f,Color(255,255,215,0));}
            else DrawUnit(g,u);
        }
        DrawParticles(g);

        if(isDragging&&lmbDown&&state!=GameState::Settings){
            Pen dashPen(Color(200,255,255,255),1); dashPen.SetDashStyle(DashStyleDash);
            g->DrawRectangle(&dashPen,(float)min(lmbStart.x,dragEnd.x),(float)min(lmbStart.y,dragEnd.y),(float)abs(lmbStart.x-dragEnd.x),(float)abs(lmbStart.y-dragEnd.y));
        }

        SolidBrush hudBg(Color(200,20,15,8)); Pen hudPen(Color(255,205,133,63));
        g->FillRectangle(&hudBg,10.f,10.f,340.f,128.f); g->DrawRectangle(&hudPen,10.f,10.f,340.f,128.f);
        wchar_t buf[128]; swprintf_s(buf,L"Time: %02d:%02d",(int)(gameTime/60),(int)gameTime%60); g->DrawString(buf,-1,&bigFont,PointF(18,14),&gold);
        swprintf_s(buf,L"FPS: %.0f",fps); SolidBrush lime(Color(255,0,220,0)); g->DrawString(buf,-1,&midFont,PointF(18,50),&lime);
        swprintf_s(buf,L"Gold: %d  [SPACE +unit]",money); SolidBrush goldBr(Color(255,255,200,0)); g->DrawString(buf,-1,&midFont,PointF(18,76),&goldBr);
        g->DrawString(L"[TAB]Recipe  [P]Pause  [1-8]Merge",-1,&smallFont,PointF(18,106),&gray);
        int unitCounts[256]={0}; int activeTypes=0;
        for(int i=0;i<MAX_UNITS;i++) if(units[i].active&&units[i].team==0&&units[i].type!=UnitType::Skeleton&&units[i].type!=UnitType::Spirit) unitCounts[(int)units[i].type]++;
        for(int t=0;t<256;t++) if(unitCounts[t]>0) activeTypes++;
        if(activeTypes>0){
            float invY=145.f, invH=30.f+activeTypes*20.f;
            g->FillRectangle(&hudBg,10.f,invY,180.f,invH); g->DrawRectangle(&hudPen,10.f,invY,180.f,invH);
            g->DrawString(L"MY UNITS",-1,&midFont,PointF(20.f,invY+6.f),&goldBr);
            float currY=invY+28.f;
            for(int t=0;t<256;t++) if(unitCounts[t]>0){
                UnitType ut=(UnitType)t; Color c=GetColor(ut); SolidBrush cb(Color(255,c.GetR(),c.GetG(),c.GetB()));
                g->FillRectangle(&cb,18.f,currY+4.f,10.f,10.f);
                wchar_t ubuf[64]; swprintf_s(ubuf,L"%s x%d",TypeName(ut),unitCounts[t]);
                g->DrawString(ubuf,-1,&smallFont,PointF(32.f,currY),&white); currY+=20.f;
            }
        }

        btnPauseX=cx*2-sx-180; btnPauseY=10; bool isPaused=(state==GameState::Paused);
        SolidBrush pBtnBg(isPaused?Color(220,60,140,60):Color(220,140,60,60)); Pen pBtnPen(Color(255,220,220,220),1);
        g->FillRectangle(&pBtnBg,btnPauseX,btnPauseY,btnPauseW,btnPauseH); g->DrawRectangle(&pBtnPen,btnPauseX,btnPauseY,btnPauseW,btnPauseH);
        g->DrawString(isPaused?L"> RESUME":L"|| PAUSE",-1,&midFont,PointF(btnPauseX+18,btnPauseY+10),&white);

        if(showRecipe){
            SolidBrush ovBg(Color(240,10,12,18)); g->FillRectangle(&ovBg,0.f,0.f,cx*2.f,cy*2.f);
            g->DrawString(L"RECIPE BOOK  (TAB to close, Scroll up/down)",-1,&bigFont,PointF(cx-300,18),&gold);
            struct TierInfo { int tier; const wchar_t* label; Gdiplus::Color hdr; };
            TierInfo tiers[]={{1,L"[*]  BASE UNITS  (1-Star)",Color(180,60,60,70)},{2,L"[**]  TIER 1 MERGES  (2-Star)",Color(180,30,60,120)},
                              {3,L"[***]  TIER 2 MERGES  (3-Star)",Color(180,100,80,20)},{4,L"[****]  ULTIMATE  (4-Star)",Color(180,80,0,120)},
                              {5,L"[*****]  GOD CLASS  (5-Star)",Color(180,120,0,180)}};
            float ry=60 + recipeScrollY;
            for(auto&ti:tiers){
                std::vector<int> riList; for(int ri=0;ri<(int)recipes.size();ri++) if(GetTier(recipes[ri].res)==ti.tier) riList.push_back(ri);
                if(riList.empty()) continue;
                SolidBrush hdrBrush(ti.hdr); g->FillRectangle(&hdrBrush,10.f,ry,cx*2.f-20.f,28.f);
                SolidBrush starCol(ti.tier==1?Color(255,200,200,200):ti.tier==2?Color(255,120,180,255):ti.tier==3?Color(255,255,215,0):ti.tier==4?Color(255,220,120,255):Color(255,255,50,50));
                g->DrawString(ti.label,-1,&midFont,PointF(18,ry+4),&starCol); ry+=40; float rx=30;
                for(int ri:riList){
                    auto&r=recipes[ri]; DrawPedestal(g,rx+70,ry+75,1.6f);
                    GameUnit du; memset(&du,0,sizeof(du)); du.type=r.res; du.scale=2.0f; du.facingYaw=-M_PI/4.f; du.z=5; du.team=0; DrawUnit(g,du,rx+70,ry+75,2.0f);
                    SolidBrush bdgBg(ti.tier==1?Color(200,80,80,90):ti.tier==2?Color(200,40,80,150):ti.tier==3?Color(200,140,110,20):ti.tier==4?Color(200,120,20,160):Color(200,160,20,20));
                    g->FillRectangle(&bdgBg,rx+140,ry+15,80.f,24.f); g->DrawString(StarStr(ti.tier),-1,&midFont,PointF(rx+145,ry+17),&starCol);
                    g->DrawString(r.name.c_str(),-1,&bigFont,PointF(rx+230,ry+12),&white);
                    wchar_t ibuf[128]; swprintf_s(ibuf,L"= %s + %s + %s",TypeName(r.a),TypeName(r.b),TypeName(r.c)); g->DrawString(ibuf,-1,&midFont,PointF(rx+145,ry+45),&gray);
                    rx+=450; if(rx>cx*2.f-450){ rx=30; ry+=140; }
                }
                ry+=140;
            }
        }
        else if(state==GameState::Paused){
            SolidBrush pvBg(Color(120,0,0,0)); g->FillRectangle(&pvBg,0.f,0.f,cx*2.f,cy*2.f);
            g->DrawString(L"PAUSED",-1,&hugeFont,PointF(cx-90,cy-30),&gold); g->DrawString(L"[P] or click Resume",-1,&midFont,PointF(cx-120,cy+40),&white);
        }
        
        if(state==GameState::Settings){
            SolidBrush bg(Color(220,0,0,0)); g->FillRectangle(&bg, 0.f, 0.f, cx*2.f, cy*2.f);
            g->DrawString(L"SETTINGS", -1, &hugeFont, PointF(cx-90, cy-250), &gold);
            auto DrawBtn = [&](float y, const wchar_t* txt, bool isVol=false, float vol=0){
                SolidBrush bBg(Color(180, 50, 50, 50)); Pen bPen(Color(255,150,150,150),2);
                g->FillRectangle(&bBg, cx-150, y, 300.f, 50.f); g->DrawRectangle(&bPen, cx-150, y, 300.f, 50.f);
                if(!isVol) g->DrawString(txt, -1, &bigFont, PointF(cx-100, y+10), &white);
                else {
                    g->DrawString(L"<", -1, &bigFont, PointF(cx-140, y+10), &gold); g->DrawString(L">", -1, &bigFont, PointF(cx+120, y+10), &gold);
                    wchar_t vbuf[64]; swprintf_s(vbuf, L"%s %d%%", txt, (int)(vol*100)); g->DrawString(vbuf, -1, &midFont, PointF(cx-80, y+14), &white);
                }
            };
            DrawBtn(cy-180, L"RESUME GAME");
            DrawBtn(cy-110, isFullscreen ? L"WINDOWED" : L"FULLSCREEN");
            DrawBtn(cy-30, L"MASTER VOL", true, volMaster);
            DrawBtn(cy+40, L"BGM VOL", true, volBgm);
            DrawBtn(cy+110, L"SFX VOL", true, volSfx);
            DrawBtn(cy+180, L"TO TITLE");
        }

        if(!showRecipe && state!=GameState::Settings && !mergeSuggestions.empty()){
            float panW=420.f; float panX=cx*2.f-sx-panW-10.f; float panY=70.f; float rowH=85.f;
            float panH=50.f+mergeSuggestions.size()*rowH;
            SolidBrush panBg(Color(220,15,20,25)); Pen panPen(Color(255,180,220,255),2);
            g->FillRectangle(&panBg,panX,panY,panW,panH); g->DrawRectangle(&panPen,panX,panY,panW,panH);
            g->DrawString(L"*** AVAILABLE MERGES (Press 1-8) ***",-1,&midFont,PointF(panX+40,panY+15),&goldBr);
            for(int i=0;i<(int)mergeSuggestions.size();i++){
                auto&ms=mergeSuggestions[i]; auto&r=recipes[ms.recipeIdx]; float ry=panY+45.f+i*rowH;
                SolidBrush rowBg(Color(50,255,255,255)); g->FillRectangle(&rowBg,panX+10,ry,panW-20,rowH-6);
                
                GameUnit preview; memset(&preview,0,sizeof(preview)); preview.type=r.res; preview.scale=1.5f; preview.facingYaw=-M_PI/4.f; preview.z=5.f;
                DrawUnit(g,preview,panX+80,ry+60,1.5f);
                
                SolidBrush keyBg(Color(255,60,60,60)); g->FillRectangle(&keyBg,panX+15,ry+25,28.f,28.f);
                wchar_t kbuf[3]=L"1"; kbuf[0]=(wchar_t)(L'1'+i); g->DrawString(kbuf,-1,&bigFont,PointF(panX+18,ry+25),&white);
                
                int tier=GetTier(r.res); SolidBrush tierCol(tier==1?Color(255,200,200,200):tier==2?Color(255,120,180,255):tier==3?Color(255,255,215,0):tier==4?Color(255,220,120,255):Color(255,255,50,50));
                wchar_t nbuf[64]; swprintf_s(nbuf,L"%s %s",StarStr(tier),r.name.c_str()); g->DrawString(nbuf,-1,&midFont,PointF(panX+130,ry+15),&tierCol);
                wchar_t ibuf[128]; swprintf_s(ibuf,L"%s + %s + %s",TypeName(r.a),TypeName(r.b),TypeName(r.c)); g->DrawString(ibuf,-1,&smallFont,PointF(panX+130,ry+45),&gray);
            }
        }
        cx-=sx; cy-=sy;
    }

private:
    void UpdateLogic(){
        gameTime+=deltaTime;
        if(gameTime>300.f&&!bossSpawned){ bossSpawned=true; SpawnUnit(camX,mouseWY-2000.f,1,UnitType::Boss); AddShake(20); }

        float sumX=0,sumY=0; int pc=0;
        for(int i=0;i<MAX_UNITS;i++) if(units[i].active&&units[i].team==0){ sumX+=units[i].x;sumY+=units[i].y;pc++; }
        float targetCamY=camY;
        if(pc>0){
            playerCenterX=sumX/pc; playerCenterY=sumY/pc;
            float ax=sumX/pc,ay=sumY/pc; targetCamY=ay-(camZ*tanf(pitch));
            camX+=(ax-camX)*3.f*deltaTime; camY+=(targetCamY-camY)*3.f*deltaTime;
        } else { state=GameState::GameOver; highScores.push_back(gameTime); std::sort(highScores.begin(),highScores.end(),[](float a,float b){return a>b;}); if(highScores.size()>3)highScores.resize(3); return; }

        {
            float ax=camX, ay=camY; if(pc>0){ ax=sumX/pc; ay=sumY/pc; }
            float dx=mouseWX-ax, dy=mouseWY-ay, dist=sqrtf(dx*dx+dy*dy);
            if(dist>100.f){ threatX=dx/dist; threatY=dy/dist; }
            float perpX=-threatY, perpY=threatX;

            std::vector<int> front,mid,back;
            for(int i=0;i<MAX_UNITS;i++){
                if(!units[i].active||units[i].team!=0) continue;
                int role=GetRole(units[i].type);
                if(role==0) front.push_back(i); else if(role==1) mid.push_back(i); else back.push_back(i);
            }

            auto SlotPos=[&](std::vector<int>& group, float forwardDist, float lateralSpacing) {
                for(int gi=0;gi<(int)group.size();gi++){
                    int i=group[gi]; int col=gi%7, row=gi/7;
                    float lateral=(col-3.f)*lateralSpacing;
                    float fx=mouseWX + threatX*(forwardDist - row*lateralSpacing*0.8f) + perpX*lateral;
                    float fy=mouseWY + threatY*(forwardDist - row*lateralSpacing*0.8f) + perpY*lateral;
                    float mdx=fx-units[i].x, mdy=fy-units[i].y, mdistSq=mdx*mdx+mdy*mdy;
                    if(mdistSq>20.f*20.f){
                        float mdist=sqrtf(mdistSq);
                        float approachSpd=min(units[i].speed*0.18f, 500.f) * min(1.f, mdist/400.f);
                        float tVx=(mdx/mdist)*approachSpd, tVy=(mdy/mdist)*approachSpd;
                        units[i].vx=units[i].vx*0.92f+tVx*0.08f; units[i].vy=units[i].vy*0.92f+tVy*0.08f;
                        units[i].facingYaw=atan2f(mdy,mdx);
                    }
                }
            };
            SlotPos(front,  150.f, 90.f); SlotPos(mid, 0.f, 85.f); SlotPos(back, -150.f, 80.f);
        }

        enemySpawnTimer-=deltaTime;
        if(enemySpawnTimer<=0){
            enemySpawnTimer=max(0.8f, 4.0f-(gameTime/120.f)); // Slowed down spawning
            int count = 1 + (int)(gameTime/150.f); if(count>6)count=6;
            for(int c=0; c<count; c++){
                float ang=((float)rand()/RAND_MAX)*M_PI*2.f; float dist=1800.f+camZ+(rand()%300);
                SpawnUnit(camX+cosf(ang)*dist,mouseWY+sinf(ang)*dist,1,UnitType::Enemy);
            }
        }
        treeSpawnTimer-=deltaTime;
        if(treeSpawnTimer<=0){
            treeSpawnTimer=0.15f;
            float ang=((float)rand()/RAND_MAX)*M_PI*2.f; float dist=2500.f+camZ;
            float tx=camX+cosf(ang)*dist, ty=mouseWY+sinf(ang)*dist; bool ov=false;
            for(int i=0;i<MAX_UNITS;i++) if(units[i].active&&units[i].team==3){ float dx=units[i].x-tx,dy=units[i].y-ty; if(dx*dx+dy*dy<150000.f){ov=true;break;} }
            if(!ov) {
                UnitType envTypes[] = {UnitType::Tree, UnitType::Rock, UnitType::Ruins, UnitType::Windmill, UnitType::Car, UnitType::Obelisk, UnitType::Statue};
                UnitType st = (rand()%10 < 7) ? ((rand()%2==0)?UnitType::Tree:UnitType::Rock) : envTypes[2 + rand()%5];
                int id = SpawnUnit(tx,ty,3,st);
                if(id!=-1) units[id].facingYaw = ((float)rand()/RAND_MAX)*M_PI*2.f;
            }
        }

        memset(gridHead,0,sizeof(gridHead));
        for(int i=0;i<MAX_UNITS;i++){
            if(!units[i].active) continue;
            if(units[i].team==3){ float ds=(units[i].x-camX)*(units[i].x-camX)+(units[i].y-mouseWY)*(units[i].y-mouseWY); if(ds>30000000.f){units[i].active=false;continue;} }
            float gX=(units[i].x+50000.f)/CELL_SIZE,gY=(units[i].y+50000.f)/CELL_SIZE;
            int gxi=max(0,min(COLS-1,(int)gX)),gyi=max(0,min(ROWS-1,(int)gY));
            int idx=gyi*COLS+gxi; gridNext[i]=gridHead[idx]-1; gridHead[idx]=i+1;
        }

        for(int i=0;i<MAX_UNITS;i++){
            if(!units[i].active) continue;
            if(units[i].team==3) continue;
            if(units[i].team==2){
                units[i].hp-=deltaTime;if(units[i].hp<=0)units[i].active=false;
                float closestD=999999.f; int closestP=-1;
                for(int j=0;j<MAX_UNITS;j++) if(units[j].active&&units[j].team==0){
                    float dsq=(units[i].x-units[j].x)*(units[i].x-units[j].x)+(units[i].y-units[j].y)*(units[i].y-units[j].y);
                    if(dsq<closestD){closestD=dsq;closestP=j;}
                }
                if(closestP!=-1 && closestD<640000.f){
                    float d=sqrtf(closestD)+0.1f;
                    units[i].vx+=((units[closestP].x-units[i].x)/d)*1500.f*deltaTime;
                    units[i].vy+=((units[closestP].y-units[i].y)/d)*1500.f*deltaTime;
                    if(closestD<10000.f){ money+=(int)units[i].damage; units[i].active=false; PlayMidiNote(9,80,60,false); }
                }
                units[i].vx*=0.95f; units[i].vy*=0.95f; units[i].x+=units[i].vx*deltaTime; units[i].y+=units[i].vy*deltaTime;
                continue;
            }
            if(units[i].hitFlashTimer>0) units[i].hitFlashTimer-=deltaTime;
            if(units[i].type==UnitType::Skeleton||units[i].type==UnitType::Spirit) units[i].hp-=deltaTime*3.f;
            units[i].attackCooldown-=deltaTime; if(units[i].attackAnimTimer>0) units[i].attackAnimTimer-=deltaTime;

            if(units[i].team==4){
                if(units[i].targetId!=-1&&units[units[i].targetId].active){ float dx=units[units[i].targetId].x-units[i].x,dy=units[units[i].targetId].y-units[i].y; float len=sqrtf(dx*dx+dy*dy)+0.001f; units[i].vx=(dx/len)*units[i].speed; units[i].vy=(dy/len)*units[i].speed; }
                units[i].x+=units[i].vx*deltaTime; units[i].y+=units[i].vy*deltaTime; units[i].z+=units[i].vz*deltaTime;
                units[i].walkTimer+=deltaTime; if(units[i].walkTimer>2.f||units[i].z<0){units[i].active=false;continue;}
            }

            if(units[i].team==1){ float dx=mouseWX-units[i].x,dy=mouseWY-units[i].y; float len=sqrtf(dx*dx+dy*dy)+0.001f; units[i].vx+=(dx/len)*units[i].speed*2.f*deltaTime; units[i].vy+=(dy/len)*units[i].speed*2.f*deltaTime; units[i].facingYaw=atan2f(dy,dx); }

            float cXf=(units[i].x+50000.f)/CELL_SIZE,cYf=(units[i].y+50000.f)/CELL_SIZE;
            int cxg=max(0,min(COLS-1,(int)cXf)),cyg=max(0,min(ROWS-1,(int)cYf));
            int checks=0; float closestEDist=9999999.f; int closestEId=-1; float lowestAHp=9999.f; int lowestAId=-1;

            for(int ny=max(0,cyg-1);ny<=min(ROWS-1,cyg+1);ny++)
            for(int nx=max(0,cxg-1);nx<=min(COLS-1,cxg+1);nx++){
                int j=gridHead[ny*COLS+nx]-1;
                while(j!=-1&&checks<500){
                    if(i!=j&&units[j].active){
                        float dx=units[i].x-units[j].x,dy=units[i].y-units[j].y,dSq=dx*dx+dy*dy;
                        if(units[i].team==4&&units[j].team!=units[i].team&&units[j].team<2){
                            if(dSq<(units[j].scale*units[j].scale*800.f)){ HitUnit(j,i,dx,dy,dSq,units[i].damage); units[i].active=false; break; }
                        } else if(units[i].team!=2&&units[j].team!=2&&units[i].team!=4&&units[j].team!=4){
                            float cs=units[i].scale+units[j].scale; float repF=0,repD=0;
                            if(units[j].team==3){repD=cs*cs*300.f;repF=8000.f;} else if(units[i].team==units[j].team){repD=cs*cs*350.f;repF=12000.f;} else{repD=cs*cs*150.f;repF=6000.f;}
                            if(dSq<repD&&dSq>0){units[i].vx+=(dx/dSq)*repF;units[i].vy+=(dy/dSq)*repF;}
                            if(units[i].team!=units[j].team&&units[j].team!=3) if(dSq<closestEDist){closestEDist=dSq;closestEId=j;}
                            if(units[i].team==0&&units[j].team==0&&units[j].hp<units[j].maxHp&&units[j].hp<lowestAHp){lowestAHp=units[j].hp;lowestAId=j;}
                        }
                    }
                    j=gridNext[j]; checks++;
                }
            }

            if(units[i].team!=4&&units[i].attackCooldown<=0){
                float slowAtk = 1.0f + ((float)rand()/RAND_MAX)*0.8f; // Slow down attacks
                if(units[i].type==UnitType::Healer||units[i].type==UnitType::BloodPriest||units[i].type==UnitType::Saint){
                    if(lowestAId!=-1&&closestEDist<400000.f){
                        units[lowestAId].hp=min(units[lowestAId].maxHp,units[lowestAId].hp+units[i].damage);
                        units[i].attackCooldown=slowAtk; units[i].attackAnimTimer=0.3f; SpawnSpark(units[lowestAId].x,units[lowestAId].y,20,Gdiplus::Color(255,50,255,50),5);
                    }
                } else if(units[i].type==UnitType::Necromancer||units[i].type==UnitType::Lich||units[i].type==UnitType::DeathKnight){
                    if(closestEId!=-1&&closestEDist<400000.f){
                        int sk=SpawnUnit(units[i].x+(rand()%100-50),units[i].y+(rand()%100-50),0,UnitType::Skeleton);
                        if(sk!=-1){units[i].attackCooldown=slowAtk*1.5f;units[i].attackAnimTimer=0.3f;SpawnSmoke(units[i].x,units[i].y,0,3);}
                    }
                } else if(units[i].type==UnitType::Elementalist||units[i].type==UnitType::Archmage||units[i].type==UnitType::IceMage){
                    if(closestEId!=-1&&closestEDist<400000.f){
                        int sp=SpawnUnit(units[i].x+(rand()%100-50),units[i].y+(rand()%100-50),0,UnitType::Spirit);
                        if(sp!=-1){units[i].attackCooldown=slowAtk*1.2f;units[i].attackAnimTimer=0.3f;}
                    }
                } else if(closestEId!=-1){
                    float dx=units[closestEId].x-units[i].x,dy=units[closestEId].y-units[i].y; float cs=units[i].scale+units[closestEId].scale;
                    if(!units[i].isRanged&&closestEDist<cs*cs*200.f){
                        HitUnit(closestEId,i,-(units[i].x-units[closestEId].x),-(units[i].y-units[closestEId].y),closestEDist,units[i].damage);
                        units[i].attackCooldown = (units[i].type==UnitType::Assassin||units[i].type==UnitType::Phantom)?0.6f:slowAtk;
                        units[i].attackAnimTimer=0.2f; units[i].facingYaw=atan2f(dy,dx);
                    } else if(units[i].isRanged&&closestEDist<350000.f){
                        units[i].attackCooldown = (units[i].type==UnitType::Sniper||units[i].type==UnitType::Ranger)?2.0f:slowAtk;
                        units[i].attackAnimTimer=0.2f; units[i].facingYaw=atan2f(dy,dx); float dLen=sqrtf(closestEDist);
                        if(units[i].type==UnitType::Warlock||units[i].type==UnitType::Archmage||units[i].type==UnitType::StormBringer) SpawnFire(units[i].x,units[i].y,15.f*units[i].scale,3);
                        else SpawnSpark(units[i].x,units[i].y,15.f*units[i].scale,GetColor(units[i].type),3);
                        int pId=SpawnUnit(units[i].x,units[i].y,4,UnitType::Projectile);
                        if(pId!=-1){ units[pId].damage=units[i].damage; units[pId].targetId=closestEId; units[pId].vx=(dx/dLen)*2000.f; units[pId].vy=(dy/dLen)*2000.f; units[pId].vz=200.f; units[pId].z=15.f*units[i].scale; }
                    }
                }
            }

            if(units[i].hp<=0&&units[i].team!=4){
                if(units[i].team==1){
                    SpawnDeathParticles(units[i].x,units[i].y,units[i].z+10.f*units[i].scale,GetColor(units[i].type),12+(int)(units[i].maxHp/30));
                    SpawnBlood(units[i].x,units[i].y,units[i].z,8);
                    if(units[i].type==UnitType::Boss){ AddShake(20); }
                    int li=SpawnUnit(units[i].x,units[i].y,2,UnitType::Coin);
                    if(li!=-1) units[li].damage=units[i].type==UnitType::Boss?100.f:(1.f+floorf(gameTime/60.f));
                }
                units[i].active=false; continue;
            }

            if(units[i].team!=4){
                if(units[i].team==1){
                    float spd=sqrtf(units[i].vx*units[i].vx+units[i].vy*units[i].vy);
                    if(spd>units[i].speed){units[i].vx=units[i].vx/spd*units[i].speed;units[i].vy=units[i].vy/spd*units[i].speed;}
                    units[i].vx*=0.82f; units[i].vy*=0.82f;
                } else { units[i].vx*=0.92f; units[i].vy*=0.92f; }
                units[i].x+=units[i].vx*deltaTime; units[i].y+=units[i].vy*deltaTime;
                float spd2=sqrtf(units[i].vx*units[i].vx+units[i].vy*units[i].vy);
                if(spd2>10.f) units[i].walkTimer+=spd2*deltaTime*0.05f;
                units[i].z=fabsf(sinf(units[i].walkTimer))*(12.f*units[i].scale);
            }
        }
    }

    void HitUnit(int victim,int attacker,float dx,float dy,float dSq,float dmg){
        units[victim].hp-=dmg; units[victim].hitFlashTimer=0.2f; float kd=sqrtf(dSq)+0.001f;
        float kbForce = (units[victim].team==0) ? 300.f : 2000.f; units[victim].vx+=(dx/kd)*kbForce; units[victim].vy+=(dy/kd)*kbForce;
        SpawnBlood(units[victim].x,units[victim].y,units[victim].z+10.f*units[victim].scale,5);
        SpawnSpark(units[victim].x,units[victim].y,units[victim].z+10.f*units[victim].scale,GetColor(units[victim].type),3);
        PlayMidiNote(9, (rand()%2==0)?76:77, 80, false);
        if(dmg>30.f) AddShake(dmg*0.15f);
        if(attacker>=0&&(units[attacker].type==UnitType::Warlock||units[attacker].type==UnitType::Boss||units[attacker].type==UnitType::Archmage||units[attacker].type==UnitType::StormBringer))
            SpawnFire(units[victim].x,units[victim].y,units[victim].z,4);
    }

    void GoToTitle(){ state=GameState::Title; }
    void RestartGame(){
        for(int i=0;i<MAX_UNITS;i++) units[i].active=false; for(int i=0;i<MAX_PARTS;i++) parts[i].active=false;
        gameTime=0;enemySpawnTimer=0;bossSpawned=false;money=100;
        state=GameState::Playing;showRecipe=false;shakeAmt=0; camX=7500;camY=6500;camZ=800;mouseWX=7500;mouseWY=7200; threatX=0; threatY=-1.f;
        SpawnUnit(7500,7200,0,UnitType::Red); SpawnUnit(7580,7200,0,UnitType::Blue); SpawnUnit(7420,7200,0,UnitType::Green);SpawnUnit(7500,7280,0,UnitType::Purple);
        UnitType envTypes[] = {UnitType::Tree, UnitType::Rock, UnitType::Ruins, UnitType::Windmill, UnitType::Car, UnitType::Obelisk, UnitType::Statue};
        for(int i=0;i<1200;i++){
            float rx = 7500.f + (rand()%24000 - 12000), ry = 7200.f + (rand()%24000 - 12000);
            if(fabsf(rx-7500.f)<600.f && fabsf(ry-7200.f)<600.f) continue;
            UnitType st = (rand()%10 < 7) ? ((rand()%2==0)?UnitType::Tree:UnitType::Rock) : envTypes[2 + rand()%5];
            int id = SpawnUnit(rx, ry, 3, st);
            if(id!=-1) units[id].facingYaw = ((float)rand()/RAND_MAX)*M_PI*2.f;
        }
        for(int i=0;i<4;i++){ float ang=((float)i/4.f)*M_PI*2.f; SpawnUnit(7500+cosf(ang)*1200.f,7200+sinf(ang)*1200.f,1,UnitType::Enemy); }
        BuildMergeSuggestions();
    }

    void SetupStats(GameUnit&u){
        u.isRanged=false;u.scale=1;u.targetId=-1;u.hitFlashTimer=0;u.selected=false;u.hasTarget=false;
        switch(u.type){
            case UnitType::Red: u.maxHp=30;u.damage=8;u.speed=900;break;
            case UnitType::Green: u.maxHp=60;u.damage=3;u.speed=800;break;
            case UnitType::Blue: u.maxHp=25;u.damage=12;u.speed=900;u.isRanged=true;break;
            case UnitType::Purple: u.maxHp=25;u.damage=15;u.speed=850;u.isRanged=true;break;
            case UnitType::Warrior: u.maxHp=200;u.damage=25;u.speed=900;u.scale=1.5f;break;
            case UnitType::Assassin: u.maxHp=80;u.damage=45;u.speed=1500;u.scale=1.1f;break;
            case UnitType::Paladin: u.maxHp=250;u.damage=15;u.speed=800;u.scale=1.6f;break;
            case UnitType::Sniper: u.maxHp=60;u.damage=80;u.speed=1000;u.scale=1.2f;u.isRanged=true;break;
            case UnitType::Healer: u.maxHp=150;u.damage=40;u.speed=900;u.scale=1.2f;u.isRanged=true;break;
            case UnitType::Necromancer:u.maxHp=120;u.damage=20;u.speed=850;u.scale=1.3f;u.isRanged=true;break;
            case UnitType::Elementalist:u.maxHp=100;u.damage=50;u.speed=950;u.scale=1.2f;u.isRanged=true;break;
            case UnitType::Warlock: u.maxHp=130;u.damage=60;u.speed=900;u.scale=1.3f;u.isRanged=true;break;
            case UnitType::Hero: u.maxHp=800;u.damage=120;u.speed=1100;u.scale=2.2f;break;
            case UnitType::Demigod: u.maxHp=1000;u.damage=200;u.speed=1000;u.scale=2.5f;u.isRanged=true;break;
            case UnitType::Vanguard: u.maxHp=1200;u.damage=90;u.speed=1000;u.scale=2.3f;break;
            case UnitType::Ranger: u.maxHp=300;u.damage=250;u.speed=1200;u.scale=1.8f;u.isRanged=true;break;
            case UnitType::Archmage: u.maxHp=500;u.damage=200;u.speed=1050;u.scale=2.0f;u.isRanged=true;break;
            case UnitType::DeathKnight: u.maxHp=1100;u.damage=150;u.speed=1000;u.scale=2.4f;break;
            case UnitType::BloodPriest: u.maxHp=600;u.damage=150;u.speed=1100;u.scale=2.0f;u.isRanged=true;break;
            case UnitType::Berserker: u.maxHp=900;u.damage=300;u.speed=1300;u.scale=2.1f;break;
            case UnitType::Templar: u.maxHp=1500;u.damage=80;u.speed=950;u.scale=2.4f;break;
            case UnitType::IceMage: u.maxHp=400;u.damage=120;u.speed=1000;u.scale=1.9f;u.isRanged=true;break;
            case UnitType::Supreme: u.maxHp=3000;u.damage=500;u.speed=1300;u.scale=3.f;break;
            case UnitType::Titan: u.maxHp=5000;u.damage=300;u.speed=1100;u.scale=3.5f;break;
            case UnitType::Phantom: u.maxHp=1500;u.damage=800;u.speed=1800;u.scale=2.5f;break;
            case UnitType::Lich: u.maxHp=2000;u.damage=600;u.speed=1200;u.scale=3.f;u.isRanged=true;break;
            case UnitType::Saint: u.maxHp=2500;u.damage=300;u.speed=1200;u.scale=3.f;u.isRanged=true;break;
            case UnitType::DragonKnight: u.maxHp=3500;u.damage=450;u.speed=1400;u.scale=3.2f;break;
            case UnitType::Valkyrie: u.maxHp=2800;u.damage=400;u.speed=1500;u.scale=3.1f;break;
            case UnitType::StormBringer: u.maxHp=2200;u.damage=700;u.speed=1300;u.scale=2.8f;u.isRanged=true;break;
            case UnitType::GodOfWar: u.maxHp=9999;u.damage=1500;u.speed=1500;u.scale=4.5f;break;
            case UnitType::Asura: u.maxHp=8000;u.damage=2500;u.speed=2000;u.scale=4.0f;break;
            case UnitType::Overlord: u.maxHp=12000;u.damage=1000;u.speed=1300;u.scale=4.5f;u.isRanged=true;break;
            case UnitType::Seraphim: u.maxHp=9000;u.damage=1200;u.speed=1600;u.scale=4.2f;u.isRanged=true;break;
            case UnitType::Cosmic: u.maxHp=8500;u.damage=2000;u.speed=1500;u.scale=4.5f;u.isRanged=true;break;
            case UnitType::Chronos: u.maxHp=7000;u.damage=3000;u.speed=1700;u.scale=4.0f;u.isRanged=true;break;
            case UnitType::Abaddon: u.maxHp=15000;u.damage=800;u.speed=1400;u.scale=4.8f;break;
            case UnitType::Enemy: u.maxHp=5+(gameTime/30.f)*10.f;u.damage=1+(gameTime/60.f)*2.f;u.scale=0.8f+(gameTime/250.f);u.speed=300.f+(gameTime/10.f);break;
            case UnitType::Boss: u.maxHp=3000;u.damage=50;u.speed=300;u.scale=6;break;
            case UnitType::Skeleton: u.maxHp=50;u.damage=15;u.speed=1000;u.scale=1;break;
            case UnitType::Spirit: u.maxHp=30;u.damage=25;u.speed=1500;u.scale=0.8f;u.isRanged=true;break;
            case UnitType::Tree: case UnitType::Rock: case UnitType::Ruins: case UnitType::Windmill: case UnitType::Car: case UnitType::Obelisk: case UnitType::Statue: u.hp=1000;u.scale=5;break;
            case UnitType::Projectile:u.hp=1;u.scale=0.5f;u.speed=2000;break;
            case UnitType::Coin: u.hp=20;u.scale=0.8f;break;
        }
        u.hp=u.maxHp;u.patrolX=u.x;u.patrolY=u.y;
    }

    Gdiplus::Color GetColor(UnitType t){
        using namespace Gdiplus;
        switch(t){
            case UnitType::Red: return Color(255,255,60,60); case UnitType::Green: return Color(255,60,220,60);
            case UnitType::Blue: return Color(255,60,144,255);case UnitType::Purple: return Color(255,200,60,255);
            case UnitType::Warrior:return Color(255,180,40,40); case UnitType::Assassin:return Color(255,220,50,80);
            case UnitType::Paladin:return Color(255,218,165,32);case UnitType::Sniper: return Color(255,60,160,220);
            case UnitType::Healer: return Color(255,60,200,120);case UnitType::Necromancer:return Color(255,120,80,200);
            case UnitType::Elementalist:return Color(255,0,200,200);case UnitType::Warlock:return Color(255,160,40,200);
            case UnitType::Hero: return Color(255,255,215,0); case UnitType::Demigod:return Color(255,200,80,255);
            case UnitType::Vanguard:return Color(255,255,100,50);case UnitType::Ranger:return Color(255,100,200,50);
            case UnitType::Archmage:return Color(255,50,150,255);case UnitType::DeathKnight:return Color(255,50,0,50);
            case UnitType::BloodPriest:return Color(255,255,0,0);case UnitType::Berserker:return Color(255,200,20,20);
            case UnitType::Templar:return Color(255,100,255,100);case UnitType::IceMage:return Color(255,100,200,255);
            case UnitType::Supreme:return Color(255,240,240,255);case UnitType::Titan:return Color(255,180,180,180);
            case UnitType::Phantom:return Color(255,80,80,100);case UnitType::Lich:return Color(255,0,255,150);
            case UnitType::Saint:return Color(255,255,240,200);case UnitType::DragonKnight:return Color(255,255,100,0);
            case UnitType::Valkyrie:return Color(255,255,200,255);case UnitType::StormBringer:return Color(255,50,255,255);
            case UnitType::GodOfWar:return Color(255,255,50,50);case UnitType::Asura:return Color(255,100,0,0);
            case UnitType::Overlord:return Color(255,50,0,100);case UnitType::Seraphim:return Color(255,255,255,255);
            case UnitType::Cosmic:return Color(255,50,50,255);case UnitType::Chronos:return Color(255,200,200,0);
            case UnitType::Abaddon:return Color(255,30,30,30);
            case UnitType::Enemy: return Color(255,180,30,30);case UnitType::Boss: return Color(255,80,0,0);
            case UnitType::Skeleton:return Color(255,200,200,180);case UnitType::Spirit: return Color(255,160,200,255);
            default: return Color(255,128,128,128);
        }
    }

        int SpawnUnit(float x,float y,unsigned char team,UnitType type){
        for(int i=0;i<MAX_UNITS;i++) if(!units[i].active){
            units[i].x=x;units[i].y=y;units[i].z=0;units[i].vx=units[i].vy=units[i].vz=0;
            units[i].type=type;units[i].active=true;units[i].team=team;
            units[i].attackAnimTimer=0;units[i].walkTimer=0; SetupStats(units[i]); return i;
        }
        return -1;
    }

    void PlaySnd(const wchar_t*alias){ wchar_t buf[256]; wsprintf(buf,L"seek %s to start",alias); mciSendString(buf,nullptr,0,nullptr); wsprintf(buf,L"play %s",alias); mciSendString(buf,nullptr,0,nullptr); }
    void PlayMidiNote(int ch,int note,int vel,bool isBgm){ 
        int finalVel = (int)(vel * volMaster * (isBgm ? volBgm : volSfx));
        if(finalVel<=0) return; if(finalVel>127) finalVel=127;
        midiOutShortMsg(midiHandle,(DWORD)(0x90|ch|(note<<8)|(finalVel<<16))); 
    }

    Gdiplus::PointF Project(float wx,float wy,float wz){
        float ty=wy-camY,tz=wz-camZ,cp=cosf(pitch),sp=sinf(pitch), rz=ty*sp-tz*cp; if(rz<1.f)rz=1.f; float ry=ty*cp+tz*sp;
        return Gdiplus::PointF((wx-camX)*fov/rz+cx, cy-(ry*fov/rz));
    }

    struct Face{int idx[4];Gdiplus::Color col;float depth;bool operator<(const Face&o)const{return depth>o.depth;}};
    void Draw3DBox(Gdiplus::Graphics*g,float wx,float wy,float wz,float sx,float sy,float sz,float yaw,Gdiplus::Color c,float uiX=0,float uiY=0,float fscale=1.f,bool flash=false){
        using namespace Gdiplus; if(flash) c=Color(255,255,255,255);
        PointF p[8]; float d[8]; float cy2=cosf(yaw),sy2=sinf(yaw);
        float dxv[8]={-sx,sx,sx,-sx,-sx,sx,sx,-sx}, dyv[8]={-sy,-sy,sy,sy,-sy,-sy,sy,sy}, dzv[8]={0,0,0,0,sz,sz,sz,sz}, cp=cosf(pitch),sp=sinf(pitch);
        for(int i=0;i<8;i++){
            float px=wx+dxv[i]*cy2-dyv[i]*sy2, py=wy+dxv[i]*sy2+dyv[i]*cy2, pz=wz+dzv[i];
            if(uiX==0&&uiY==0){ d[i]=(py-camY)*sp-(pz-camZ)*cp; p[i]=Project(px,py,pz); } else { d[i]=-pz; p[i].X=uiX+(px-wx)*fscale; p[i].Y=uiY-(py-wy)*fscale/2.f-pz*fscale; }
        }
        Face faces[]={
            {{0,1,2,3},Color(255,c.GetR()/2,c.GetG()/2,c.GetB()/2),d[0]+d[1]+d[2]+d[3]},{{4,5,6,7},Color(255,c.GetR(),c.GetG(),c.GetB()),d[4]+d[5]+d[6]+d[7]},
            {{0,1,5,4},c,d[0]+d[1]+d[5]+d[4]},{{1,2,6,5},Color(255,c.GetR()*3/4,c.GetG()*3/4,c.GetB()*3/4),d[1]+d[2]+d[6]+d[5]},
            {{2,3,7,6},Color(255,c.GetR()*2/3,c.GetG()*2/3,c.GetB()*2/3),d[2]+d[3]+d[7]+d[6]},{{3,0,4,7},Color(255,c.GetR()*4/5,c.GetG()*4/5,c.GetB()*4/5),d[3]+d[0]+d[4]+d[7]},
        };
        std::sort(faces,faces+6); for(int i=0;i<6;i++){ PointF poly[4]={p[faces[i].idx[0]],p[faces[i].idx[1]],p[faces[i].idx[2]],p[faces[i].idx[3]]}; SolidBrush b(faces[i].col); g->FillPolygon(&b,poly,4); }
    }

    void DrawPedestal(Gdiplus::Graphics*g,float x,float y,float scale){
        using namespace Gdiplus; float pw=30.f*scale,ph=15.f*scale;
        PointF top[]={PointF(x,y-ph),PointF(x+pw,y),PointF(x,y+ph),PointF(x-pw,y)}; SolidBrush ltG(Color(255,211,211,211)); Pen wP(Color(255,255,255,255)); g->FillPolygon(&ltG,top,4); g->DrawPolygon(&wP,top,4);
        PointF left[]={PointF(x-pw,y),PointF(x,y+ph),PointF(x,y+ph+10.f*scale),PointF(x-pw,y+10.f*scale)}; SolidBrush dkG(Color(255,169,169,169)); g->FillPolygon(&dkG,left,4);
        PointF right[]={PointF(x,y+ph),PointF(x+pw,y),PointF(x+pw,y+10.f*scale),PointF(x,y+ph+10.f*scale)}; SolidBrush gray2(Color(255,128,128,128)); g->FillPolygon(&gray2,right,4);
    }

        void DrawParticles(Gdiplus::Graphics*g){
        for(int i=0;i<MAX_PARTS;i++){
            if(!parts[i].active) continue;
            Gdiplus::PointF p=Project(parts[i].x,parts[i].y,parts[i].z);
            float alpha=(parts[i].life/parts[i].maxLife);
            unsigned char a=(unsigned char)(parts[i].a*alpha); float sz=parts[i].size*alpha;
            if(sz<0.5f) continue;
            float dy=parts[i].y-camY; if(dy<-500) continue;
            Gdiplus::SolidBrush br(Gdiplus::Color(a,parts[i].r,parts[i].g,parts[i].b));
            if(parts[i].type==3) sz*=1.5f;
            g->FillEllipse(&br,p.X-sz,p.Y-sz*0.5f,sz*2.f,sz);
        }
    }
    void DrawEnvironment(Gdiplus::Graphics* g, GameUnit& u) {
        using namespace Gdiplus;
        if (u.type == UnitType::Tree) { Draw3DBox(g,u.x,u.y,0,25,25,100,0,Color(255,139,69,19)); Draw3DBox(g,u.x,u.y,100,90,90,75,0,Color(255,34,139,34)); }
        else if (u.type == UnitType::Rock) { Draw3DBox(g,u.x,u.y,0,75,75,75,u.facingYaw,Color(255,105,105,105)); }
        else if (u.type == UnitType::Ruins) { Draw3DBox(g,u.x-50.f,u.y-50.f,0,20,20,80,0.2f,Color(255,120,120,120)); Draw3DBox(g,u.x+40.f,u.y-30.f,0,15,15,40,-0.1f,Color(255,100,100,100)); Draw3DBox(g,u.x-10.f,u.y+40.f,0,30,20,20,0.5f,Color(255,90,90,90)); }
        else if (u.type == UnitType::Windmill) { Draw3DBox(g,u.x,u.y,0,40,40,150,0,Color(255,180,150,120)); float rot=gameTime*2.f; Draw3DBox(g,u.x,u.y-45.f,130,5,80,10,rot,Color(255,200,200,200)); Draw3DBox(g,u.x,u.y-45.f,130,80,5,10,rot,Color(255,200,200,200)); }
        else if (u.type == UnitType::Car) { Draw3DBox(g,u.x,u.y,10,40,20,15,u.facingYaw,Color(255,150,50,50)); Draw3DBox(g,u.x-10.f,u.y,25,20,18,15,u.facingYaw,Color(255,100,150,200)); Draw3DBox(g,u.x+20.f,u.y+22.f,0,10,4,10,u.facingYaw,Color(255,20,20,20)); Draw3DBox(g,u.x-20.f,u.y-22.f,0,10,4,10,u.facingYaw,Color(255,20,20,20)); }
        else if (u.type == UnitType::Obelisk) { Draw3DBox(g,u.x,u.y,0,25,25,200,0,Color(255,20,20,20)); Draw3DBox(g,u.x,u.y,50.f+sinf(gameTime*2.f)*10.f,26,26,20,0,Color(255,0,255,255)); }
        else if (u.type == UnitType::Statue) { Draw3DBox(g,u.x,u.y,0,40,40,30,0,Color(255,150,150,150)); Draw3DBox(g,u.x,u.y,30,10,10,60,u.facingYaw,Color(255,120,120,120)); Draw3DBox(g,u.x,u.y,90,15,15,20,u.facingYaw,Color(255,120,120,120)); }
    }

    void DrawUnit(Gdiplus::Graphics*g,GameUnit&u,float uiX=0,float uiY=0,float fscale=1.f){
        using namespace Gdiplus; Color col=GetColor(u.type); float s=uiX==0?u.scale:fscale; float attack=u.attackAnimTimer>0?sinf((u.attackAnimTimer/0.3f)*M_PI):0;
        float yaw=uiX==0?u.facingYaw:-M_PI/4.f; float z=uiX==0?u.z:0; bool flash=u.hitFlashTimer>0; float wa=sinf(u.walkTimer*5.f);

        if(u.type==UnitType::Assassin||u.type==UnitType::Phantom||u.type==UnitType::Berserker) z -= 3.f*s; // crouching
        if(u.type==UnitType::Overlord||u.type==UnitType::Cosmic||u.type==UnitType::Chronos) z += 15.f*s; // floating highly

        float hitT=flash?(u.hitFlashTimer/0.2f):0.f; float ss=1.f-hitT*0.25f; s*=ss; z+=hitT*6.f*s;
        float hoX=uiX==0?(-cosf(yaw)*hitT*5.f*s):0.f; float hoY=uiX==0?(-sinf(yaw)*hitT*5.f*s):0.f;

        if(u.selected&&uiX==0){ PointF sel[]={Project(u.x-12.f*s,u.y-12.f*s,1),Project(u.x+12.f*s,u.y-12.f*s,1),Project(u.x+12.f*s,u.y+12.f*s,1),Project(u.x-12.f*s,u.y+12.f*s,1)}; Pen lime(Color(255,0,255,0),2); g->DrawPolygon(&lime,sel,4); }
        if(uiX==0){ PointF sh=Project(u.x,u.y,0); SolidBrush shBr(Color(80,0,0,0)); g->FillEllipse(&shBr,sh.X-10.f*s,sh.Y-5.f*s,20.f*s,10.f*s); }

        float bodyZ=z+6.f*s;
        Draw3DBox(g,u.x+hoX,u.y+hoY,bodyZ,4.f*s,4.f*s,10.f*s,yaw,col,uiX,uiY,fscale,flash);
        Draw3DBox(g,u.x+hoX-cosf(yaw+1.57f)*2.f*s+cosf(yaw)*wa*3.f*s,u.y+hoY-sinf(yaw+1.57f)*2.f*s+sinf(yaw)*wa*3.f*s,z,1.5f*s,1.5f*s,6.f*s,yaw,Color(255,47,79,79),uiX,uiY,fscale,flash);
        Draw3DBox(g,u.x+hoX+cosf(yaw+1.57f)*2.f*s-cosf(yaw)*wa*3.f*s,u.y+hoY+sinf(yaw+1.57f)*2.f*s-sinf(yaw)*wa*3.f*s,z,1.5f*s,1.5f*s,6.f*s,yaw,Color(255,47,79,79),uiX,uiY,fscale,flash);
        
        float headZ=z+16.f*s, headTilt=yaw+attack*0.5f+hitT*0.4f;
        Color headCol=(u.team==1||u.type==UnitType::Skeleton||u.type==UnitType::Abaddon)?Color(255,107,142,35):Color(255,255,218,185);
        if(u.type==UnitType::Lich||u.type==UnitType::Chronos) headCol=Color(255,200,255,255);
        Draw3DBox(g,u.x+hoX,u.y+hoY+attack*5.f,headZ,4.f*s,4.f*s,8.f*s,headTilt,headCol,uiX,uiY,fscale,flash);
        
        float armZ=z+12.f*s;
        Draw3DBox(g,u.x+hoX+cosf(yaw-1.57f)*5.f*s,u.y+hoY+sinf(yaw-1.57f)*5.f*s,armZ,1.5f*s,1.5f*s,6.f*s,yaw,col,uiX,uiY,fscale,flash);
        float wX=u.x+hoX+cosf(yaw+1.57f)*5.f*s+cosf(yaw)*attack*15.f*s, wY=u.y+hoY+sinf(yaw+1.57f)*5.f*s+sinf(yaw)*attack*15.f*s;
        Draw3DBox(g,wX,wY,armZ,1.5f*s,1.5f*s,6.f*s,yaw,col,uiX,uiY,fscale,flash);

        // SILHOUETTES & DETAILS
        if(u.type==UnitType::Warrior||u.type==UnitType::Berserker){ Draw3DBox(g,wX,wY,z+6.f*s,1.5f*s,12.f*s,2.f*s,yaw,Color(255,192,192,192),uiX,uiY,fscale,flash); }
        else if(u.type==UnitType::Paladin || u.type==UnitType::Vanguard || u.type==UnitType::Titan || u.type==UnitType::Abaddon){
            Draw3DBox(g,wX,wY,z+6.f*s,2.f*s,12.f*s,2.f*s,yaw,Color(255,192,192,192),uiX,uiY,fscale,flash);
            Draw3DBox(g,u.x+hoX-cosf(yaw+1.57f)*8.f*s,u.y+hoY-sinf(yaw+1.57f)*8.f*s,z+6.f*s,2.f*s,8.f*s,12.f*s,yaw,Color(255,218,165,32),uiX,uiY,fscale,flash);
            if(u.type!=UnitType::Vanguard) { Draw3DBox(g,u.x+hoX,u.y+hoY,headZ+10.f*s, 6.f*s,2.f*s,1.5f*s, yaw, Color(255,255,215,0),uiX,uiY,fscale,flash); Draw3DBox(g,u.x+hoX,u.y+hoY,headZ+10.f*s, 2.f*s,6.f*s,1.5f*s, yaw, Color(255,255,215,0),uiX,uiY,fscale,flash); }
        }
        else if(u.type==UnitType::Sniper || u.type==UnitType::Ranger){
            Draw3DBox(g,wX+cosf(yaw)*7.f*s,wY+sinf(yaw)*7.f*s,z+10.f*s,1.f*s,16.f*s,1.f*s,yaw,Color(255,40,40,40),uiX,uiY,fscale,flash);
            Draw3DBox(g,u.x+hoX,u.y+hoY,headZ+7.f*s,7.f*s,7.f*s,1.f*s,yaw,Color(255,139,69,19),uiX,uiY,fscale,flash); Draw3DBox(g,u.x+hoX,u.y+hoY,headZ+8.f*s,4.f*s,4.f*s,3.f*s,yaw,Color(255,139,69,19),uiX,uiY,fscale,flash);
        }
        else if(u.type==UnitType::Healer || u.type==UnitType::BloodPriest || u.type==UnitType::Templar){
            Draw3DBox(g,wX,wY,z,1.5f*s,1.5f*s,18.f*s,yaw,(u.type==UnitType::BloodPriest)?Color(255,255,0,0):Color(255,255,215,0),uiX,uiY,fscale,flash);
            Draw3DBox(g,wX,wY,z+15.f*s,4.f*s,1.5f*s,1.5f*s,yaw,(u.type==UnitType::BloodPriest)?Color(255,255,0,0):Color(255,255,215,0),uiX,uiY,fscale,flash);
        }
        else if(u.type==UnitType::Necromancer || u.type==UnitType::Lich || u.type==UnitType::DeathKnight){
            Draw3DBox(g,wX,wY,z,1.5f*s,1.5f*s,18.f*s,yaw,Color(255,139,69,19),uiX,uiY,fscale,flash); Draw3DBox(g,wX,wY,z+18.f*s,4.f*s,4.f*s,4.f*s,yaw,Color(255,200,200,200),uiX,uiY,fscale,flash);
            Draw3DBox(g,u.x+hoX-cosf(yaw)*4.f*s,u.y+hoY-sinf(yaw)*4.f*s,bodyZ-2.f*s,5.f*s,1.f*s,12.f*s,yaw,Color(255,60,20,90),uiX,uiY,fscale,flash);
            if(u.type==UnitType::Lich) Draw3DBox(g,u.x+hoX,u.y+hoY,z-10.f,12.f*s,12.f*s,2.f*s,yaw,Color(255,0,255,100),uiX,uiY,fscale,flash);
        }
        else if(u.type==UnitType::Warlock || u.type==UnitType::Archmage || u.type==UnitType::IceMage){
            Draw3DBox(g,wX,wY,z+8.f*s,1.5f*s,1.5f*s,6.f*s,yaw,(u.type==UnitType::IceMage)?Color(255,150,200,255):Color(255,255,140,0),uiX,uiY,fscale,flash);
            Draw3DBox(g,u.x+hoX,u.y+hoY,headZ+8.f*s,7.f*s,7.f*s,2.f*s,yaw,Color(255,75,0,130),uiX,uiY,fscale,flash); Draw3DBox(g,u.x+hoX,u.y+hoY,headZ+10.f*s,5.f*s,5.f*s,3.f*s,yaw,Color(255,75,0,130),uiX,uiY,fscale,flash); Draw3DBox(g,u.x+hoX,u.y+hoY,headZ+13.f*s,2.f*s,2.f*s,4.f*s,yaw,Color(255,75,0,130),uiX,uiY,fscale,flash);
            Draw3DBox(g,u.x+hoX-cosf(yaw)*4.f*s,u.y+hoY-sinf(yaw)*4.f*s,bodyZ,5.f*s,1.f*s,10.f*s,yaw,Color(255,148,0,211),uiX,uiY,fscale,flash);
        }
        else if(u.type==UnitType::Elementalist || u.type==UnitType::StormBringer){
            float orb=u.walkTimer*3.f; Draw3DBox(g,u.x+hoX+cosf(orb)*15.f*s,u.y+hoY+sinf(orb)*15.f*s,z+10.f*s,3.f*s,3.f*s,3.f*s,orb,Color(255,0,255,255),uiX,uiY,fscale,flash); Draw3DBox(g,u.x+hoX-cosf(orb)*15.f*s,u.y+hoY-sinf(orb)*15.f*s,z+10.f*s,3.f*s,3.f*s,3.f*s,orb,Color(255,255,165,0),uiX,uiY,fscale,flash); Draw3DBox(g,u.x+hoX+cosf(orb+1.5f)*15.f*s,u.y+hoY+sinf(orb+1.5f)*15.f*s,z+10.f*s,3.f*s,3.f*s,3.f*s,orb,Color(255,50,255,50),uiX,uiY,fscale,flash);
        }
        else if(u.type==UnitType::Assassin || u.type==UnitType::Phantom){
            Draw3DBox(g,wX,wY,z+6.f*s,1.f*s,6.f*s,1.f*s,yaw,Color(255,192,192,192),uiX,uiY,fscale,flash); float lX=u.x+hoX+cosf(yaw-1.57f)*5.f*s, lY=u.y+hoY+sinf(yaw-1.57f)*5.f*s; Draw3DBox(g,lX,lY,z+6.f*s,1.f*s,6.f*s,1.f*s,yaw,Color(255,192,192,192),uiX,uiY,fscale,flash);
        }
        else if(u.type==UnitType::Hero || u.type==UnitType::Demigod || u.type==UnitType::Supreme || u.type==UnitType::Saint || u.type==UnitType::Seraphim || u.type==UnitType::DragonKnight || u.type==UnitType::Cosmic || u.type==UnitType::GodOfWar || u.type==UnitType::Asura || u.type==UnitType::Overlord || u.type==UnitType::Valkyrie || u.type==UnitType::Chronos){
            Draw3DBox(g,wX,wY,z+4.f*s,3.f*s,16.f*s,3.f*s,yaw,Color(255,255,255,255),uiX,uiY,fscale,flash);
            int wings = (u.type==UnitType::Hero||u.type==UnitType::DragonKnight||u.type==UnitType::Valkyrie)?1: (u.type==UnitType::Seraphim||u.type==UnitType::Asura||u.type==UnitType::Chronos)?3: 2;
            for(int w=0; w<wings*2; w++){
                float wang = yaw + 0.8f + w*0.4f; if(w%2==0) wang = yaw - 0.8f - (w/2)*0.4f; else wang = yaw + 0.8f + (w/2)*0.4f;
                Draw3DBox(g,u.x+hoX-cosf(wang)*8.f*s, u.y+hoY-sinf(wang)*8.f*s, bodyZ+w*3.f*s, 1.f*s,15.f*s,4.f*s, wang, (u.type==UnitType::GodOfWar||u.type==UnitType::Asura)?Color(255,255,50,50):(u.type==UnitType::Cosmic)?Color(255,100,100,255):Color(255,255,255,255),uiX,uiY,fscale,flash);
            }
            if(u.type==UnitType::Supreme||u.type==UnitType::GodOfWar||u.type==UnitType::Chronos) Draw3DBox(g,u.x+hoX,u.y+hoY,headZ+9.f*s, 6.f*s,6.f*s,4.f*s, yaw, Color(255,255,215,0),uiX,uiY,fscale,flash);
            if(u.type==UnitType::Cosmic || u.type==UnitType::Overlord) { float orb=u.walkTimer*2.f; for(int k=0;k<4;k++){ float oa=orb+k*1.57f; Draw3DBox(g,u.x+hoX+cosf(oa)*20.f*s,u.y+hoY+sinf(oa)*20.f*s,z+15.f*s,4.f*s,4.f*s,4.f*s,oa,Color(255,200,50,255),uiX,uiY,fscale,flash); } }
        }

        if(uiX==0&&u.maxHp>0){
            PointF hp=Project(u.x,u.y,z+28.f*s+4.f); float bw=20.f*s,bh=3.f;
            SolidBrush hpBg(Color(180,60,0,0)),hpFg(Color(255,0,200,60));
            g->FillRectangle(&hpBg,hp.X-bw/2,hp.Y,bw,bh); g->FillRectangle(&hpFg,hp.X-bw/2,hp.Y,bw*min(1.f,u.hp/u.maxHp),bh);
        }
    }
};

CGameScene g_Game;
LARGE_INTEGER g_Freq, g_Last;

// --- AutoBattleGame.cpp 맨 밑에 붙여넣기 ---
namespace GameB {
    HWND g_hWndB = NULL;
    ULONG_PTR gdiToken;
    bool gameoverchecker = false;

    void Init(HWND hWnd) {
        g_hWndB = hWnd;
        gameoverchecker = false;
        Gdiplus::GdiplusStartupInput gsi;
        Gdiplus::GdiplusStartup(&gdiToken, &gsi, NULL);

        g_Game.Initialize(hWnd);
        g_Game.state = GameState::Title;
    }

    // 자체 델타타임 계산기
    float GetDeltaTime() {
        static LARGE_INTEGER freq, lastTime;
        static bool init = false;
        if (!init) { QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&lastTime); init = true; }
        LARGE_INTEGER curr; QueryPerformanceCounter(&curr);
        float dt = (float)(curr.QuadPart - lastTime.QuadPart) / freq.QuadPart;
        lastTime = curr;
        return dt;
    }

    void Update() {
        float dt = GetDeltaTime();
        g_Game.Update(dt);

        // ⭐ 실제 게임 오버 시 릴레이(네임 인풋)로 바통 터치!
        if (g_Game.state == GameState::GameOver) {
            gameoverchecker = true;
        }
    }

    void Draw() {
        if (!g_hWndB) return;
        HDC hdc = GetDC(g_hWndB);
        RECT rc; GetClientRect(g_hWndB, &rc);
        HDC hMem = CreateCompatibleDC(hdc);
        HBITMAP hBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP hOld = (HBITMAP)SelectObject(hMem, hBmp);

        Gdiplus::Graphics gr(hMem);
        gr.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g_Game.Draw(&gr);

        // ⭐ 조작 없는 게임을 위한 최소한의 UI 안내 강제 출력
        Gdiplus::Font font(L"Consolas", 18, Gdiplus::FontStyleBold);
        Gdiplus::SolidBrush brush(Gdiplus::Color(255, 255, 255, 0)); // 노란색
        gr.DrawString(L"◆ AUTO BATTLE: Watch & Survive! (ESC to Quit) ◆", -1, &font, Gdiplus::PointF(15.0f, 150.0f), &brush);

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, hMem, 0, 0, SRCCOPY);
        SelectObject(hMem, hOld); DeleteObject(hBmp); DeleteDC(hMem); ReleaseDC(g_hWndB, hdc);
    }

    void Release() {
        g_Game.Release();
        Gdiplus::GdiplusShutdown(gdiToken);
    }

    void InputKey(WPARAM wParam) { g_Game.ProcessKey((int)wParam); }

    void InputMouseClick(int mx, int my) {
        g_Game.ProcessMouse(WM_LBUTTONUP, mx, my, 0);
        // ⭐ ESC 메뉴에서 'TO TITLE' 등을 눌러서 내부 타이틀로 가려고 하면, 
        // 바로 프레임워크 릴레이 종료(네임 인풋)로 납치해 버립니다!
        if (g_Game.state == GameState::Title) {
            gameoverchecker = true;
        }
    }

    void InputMouseMove(int mx, int my) { g_Game.ProcessMouse(WM_MOUSEMOVE, mx, my, 0); }
    bool IsGameOver() { return gameoverchecker; }
    int GetScore() { return (int)g_Game.gameTime; } // 생존 시간을 점수로 반환
}








