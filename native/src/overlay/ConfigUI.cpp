#include "Overlay.h"
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

static int  gMX, gMY;
static bool gDn, gDnP;
static int  gScroll;
static int  gHkListen = -1;

static void uiInput(Overlay& o) {
    POINT pt; GetCursorPos(&pt);
    RECT wr; GetWindowRect(o.hwnd(), &wr);
    gMX = pt.x - wr.left; gMY = pt.y - wr.top;
    gDn  = o.m_lmbDown;
    gDnP = o.m_lmbWasDown;
    if (GetAsyncKeyState(VK_DOWN) & 1) gScroll += 30;
    if (GetAsyncKeyState(VK_UP)   & 1) gScroll -= 30;
    if (gScroll < 0) gScroll = 0;
    if (gHkListen >= 0) {
        Settings& s = o.settings();
        for (int k = VK_F1; k <= VK_F24; k++) {
            if (GetAsyncKeyState(k) & 1) {
                s.hk[gHkListen] = { (UINT)k, (GetAsyncKeyState(VK_CONTROL)&0x8000)!=0,
                    (GetAsyncKeyState(VK_SHIFT)&0x8000)!=0, (GetAsyncKeyState(VK_MENU)&0x8000)!=0 };
                gHkListen = -1; o.hotkeysDirty = true; break;
            }
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 1) gHkListen = -1;
    }
}

static const uint32_t C_BG=0x05070B, C_CARD=0x000000, C_BORDER=0xFFFFFF;
static const uint32_t C_FG=0xF1F5F9, C_DIM=0x64748B, C_DIM2=0x94A3B8;
static uint32_t C_AC=0x00FF88;

static bool HIT(float x,float y,float w,float h){ return gMX>=x&&gMX<x+w&&gMY>=y&&gMY<y+h; }

static void F(Overlay&o,float x,float y,float w,float h,uint32_t c,float a,float r=0){
    ID2D1SolidColorBrush*b=nullptr;o.rt()->CreateSolidColorBrush(toColorF(c,a),&b);if(!b)return;
    if(r>0.1f){D2D1_ROUNDED_RECT rr{D2D1::RectF(x,y,x+w,y+h),r,r};o.rt()->FillRoundedRectangle(&rr,b);}
    else o.rt()->FillRectangle(D2D1::RectF(x,y,x+w,y+h),b);b->Release();
}
static void S(Overlay&o,float x,float y,float w,float h,uint32_t c,float a,float r=0,float sw=1){
    ID2D1SolidColorBrush*b=nullptr;o.rt()->CreateSolidColorBrush(toColorF(c,a),&b);if(!b)return;
    if(r>0.1f){D2D1_ROUNDED_RECT rr{D2D1::RectF(x,y,x+w,y+h),r,r};o.rt()->DrawRoundedRectangle(&rr,b,sw);}
    else o.rt()->DrawRectangle(D2D1::RectF(x,y,x+w,y+h),b,sw);b->Release();
}
static void T(Overlay&o,float x,float y,const wchar_t*s,uint32_t c,float sz,bool bd=false,float a=1.f){o.text(x,y,s,c,sz,bd,a);}
static float TW(Overlay&o,const wchar_t*s,float sz,bool bd=false){return o.textW(s,sz,bd);}

static bool BTN(Overlay&o,float x,float y,float w,float h,const wchar_t*l,bool act=false){
    bool hv=HIT(x,y,w,h);
    F(o,x,y,w,h,act?C_AC:0xFFFFFF,act?0.10f:(hv?0.06f:0.04f),8.f);
    S(o,x,y,w,h,act?C_AC:C_BORDER,act?0.60f:(hv?0.25f:0.12f),8.f);
    float lw=TW(o,l,9.5f,true);
    T(o,x+(w-lw)*.5f,y+h*.5f-5.5f,l,act?C_AC:(hv?0xFFFFFF:C_DIM2),9.5f,true);
    return hv&&!gDn&&gDnP;
}
static bool TOG(Overlay&o,float x,float y,float w,const wchar_t*l,bool&v){
    float tx=x+w-36;
    T(o,x,y+2,l,v?C_FG:C_DIM,10.5f,false,0.95f);
    F(o,tx,y,30,16,v?C_AC:C_BORDER,v?1.f:0.12f,8.f);
    F(o,tx+(v?16.f:2.f),y+2,12,12,v?C_BG:C_DIM2,1.f,6.f);
    if(HIT(tx,y,30,16)&&!gDn&&gDnP){v=!v;return true;}return false;
}
static bool SLD(Overlay&o,float x,float y,float w,const wchar_t*l,float&v,float mn,float mx){
    T(o,x,y+2,l,C_DIM2,10.f,false,0.95f);
    wchar_t vb[24];swprintf_s(vb,L"%.0f",v);float vw=TW(o,vb,10.f,true);
    T(o,x+w-vw,y+2,vb,C_AC,10.f,true);
    float ty=y+18,tw=w;
    F(o,x,ty,tw,4,C_BORDER,0.10f,2.f);
    float p=(v-mn)/(mx-mn);if(p<0)p=0;if(p>1)p=1;
    F(o,x,ty,tw*p,4,C_AC,0.90f,2.f);
    F(o,x+tw*p-7,ty-5,14,14,C_AC,1.f,3.f);
    if(HIT(x,ty-6,tw,20)&&gDn){float np=((float)gMX-x)/tw;if(np<0)np=0;if(np>1)np=1;v=mn+np*(mx-mn);return true;}
    return false;
}

void drawConfigUI(Overlay& o) {
    uiInput(o);
    Settings& s = o.settings();
    C_AC = s.themeDef().accent;
    const float PW=338,PH=(float)o.m_h,PX=(float)o.m_cfgPanelX,PY=0;

    F(o,PX+4,PY+12,PW+4,PH+4,0x000000,0.60f,14.f);
    F(o,PX,PY,PW,PH,C_BG,0.97f,12.f);
    S(o,PX,PY,PW,PH,C_AC,0.25f,12.f);

    // header
    F(o,PX+1,PY+1,PW-2,36,0xFFFFFF,0.03f,11.f);
    F(o,PX,PY+37,PW,1,0xFFFFFF,0.10f);
    F(o,PX+12,PY+16,5,5,C_AC,1.f,2.5f);
    T(o,PX+22,PY+12,L"YAWAFPS HUD",C_AC,10.f,true);
    S(o,PX+108,PY+11,32,14,C_BORDER,0.12f,3.f);
    T(o,PX+112,PY+12,L"0.0.2",C_DIM,8.f);
    T(o,PX+PW-80,PY+13,L"Shift+F11",C_DIM,8.5f,false,0.9f);
    float cx=PX+PW-24;
    if(HIT(cx,PY+10,20,20))F(o,cx,PY+10,20,20,0xFFFFFF,0.10f,4.f);
    T(o,cx+5,PY+12,L"\u00D7",HIT(cx,PY+10,20,20)?0xFFFFFF:C_DIM,12.f,true);
    if(HIT(cx,PY+10,20,20)&&!gDn&&gDnP)o.toggleConfig();

    // tabs
    static const wchar_t*TABS[]={L"БЛОКИ",L"ТЕМЫ",L"СТИЛЬ",L"КЛАВИШИ",L"СИСТЕМА"};
    float tw=PW/5;
    for(int i=0;i<5;i++){
        bool on=(o.m_cfgTab==i);float bx=PX+i*tw;
        if(on)F(o,bx,PY+38,tw,28,C_AC,0.08f);
        if(HIT(bx,PY+38,tw,28)&&!on)F(o,bx,PY+38,tw,28,0xFFFFFF,0.04f);
        if(HIT(bx,PY+38,tw,28)&&!gDn&&gDnP)o.m_cfgTab=i;
        if(on)F(o,bx,PY+64,tw,2,C_AC,0.90f);
        float lw=TW(o,TABS[i],8.5f);
        T(o,bx+(tw-lw)*.5f,PY+46,TABS[i],on?C_AC:C_DIM,8.5f,false,on?1.f:0.9f);
    }
    F(o,PX,PY+66,PW,1,0xFFFFFF,0.08f);

    D2D1_RECT_F clip=D2D1::RectF(PX,PY+70,PX+PW,PY+PH-44);
    o.rt()->PushAxisAlignedClip(clip,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    float cy=PY+78-(float)gScroll,lx=PX+10,cw=PW-20;

    // TAB 0: BLOCKS
    if(o.m_cfgTab==0){
        static const wchar_t*HW_W[]={L"FPS",L"CPU",L"GPU",L"RAM",L"I/O"};
        static uint32_t HW_C[]={0x22FF66,0x4DB3FF,0xB9A7FF,0xFFCC4D,0x6EF3D6};
        for(size_t bi=0;bi<s.blocks.size();bi++){
            const BlockCfg&b=s.blocks[bi];int hw=b.hw<0?0:(b.hw>4?4:b.hw);float bh=50;
            F(o,lx,cy,cw,bh,C_CARD,0.25f,8.f);
            S(o,lx,cy,cw,bh,b.enabled?C_AC:C_BORDER,b.enabled?0.30f:0.12f,8.f);
            F(o,lx+10,cy+10,22,22,HW_C[hw],0.12f,6.f);
            T(o,lx+12,cy+13,HW_W[hw],HW_C[hw],9.f,true);
            std::wstring title=b.title.empty()?L"(no title)":b.title;
            T(o,lx+38,cy+8,title.c_str(),C_FG,10.5f,true);
            wchar_t cnt[32];swprintf_s(cnt,L"%d датч.",(int)b.items.size());
            T(o,lx+38,cy+24,cnt,C_DIM,8.5f,false,0.9f);
            F(o,lx+cw-14,cy+bh*.5f-4,8,8,b.enabled?C_AC:C_DIM,b.enabled?0.8f:0.3f,4.f);
            cy+=bh+8;
        }
        S(o,lx,cy,cw,32,C_BORDER,0.15f,8.f);
        T(o,lx+cw*.5f-40,cy+10,L"+ добавить блок",C_DIM2,10.f);
    }

    // TAB 1: THEMES
    else if(o.m_cfgTab==1){
        for(int ti=0;ti<THEME_COUNT;ti++){
            const ThemeDef&t=THEMES[ti];bool on=(themeIndex(s.theme)==ti);
            F(o,lx,cy,cw,52,on?C_AC:C_CARD,on?0.07f:0.15f,8.f);
            S(o,lx,cy,cw,52,on?C_AC:C_BORDER,on?0.60f:0.12f,8.f);
            for(int k=0;k<5;k++)F(o,lx+10+k*9,cy+10,7,7,t.palette[k],1.f,2.f);
            std::wstring nm=utf8ToWide(t.display);
            T(o,lx+10,cy+24,nm.c_str(),on?C_AC:C_FG,10.5f,true);
            static const wchar_t*H[]={L"тёмный+неон",L"без подложки",L"матовое стекло",L"чёрный янтарь",
                L"пурпур",L"минимал",L"эспорт",L"фосфор CRT"};
            T(o,lx+10,cy+38,H[ti],C_DIM,8.5f,false,0.85f);
            if(on)T(o,lx+cw-16,cy+20,L"\u2713",C_AC,13.f,true);
            if(HIT(lx,cy,cw,52)&&!gDn&&gDnP){s.applyTheme(t.id);o.applySettings(true);}
            cy+=60;
        }
    }

    // TAB 2: STYLE
    else if(o.m_cfgTab==2){
        if(SLD(o,lx,cy,cw,L"Размер шрифта",s.fontSize,9,28))o.markDirty();cy+=30;
        float op=s.opacity*100;if(SLD(o,lx,cy,cw,L"Прозрачность",op,20,100)){s.opacity=op/100;o.markDirty();}cy+=30;
        float bg=s.bgOpacity*100;if(SLD(o,lx,cy,cw,L"Заливка фона",bg,0,100)){s.bgOpacity=bg/100;o.markDirty();}cy+=30;
        float pd=(float)s.padding;if(SLD(o,lx,cy,cw,L"Поля",pd,2,22)){s.padding=(int)pd;o.markDirty();}cy+=30;
        float rg=(float)s.rowGap;if(SLD(o,lx,cy,cw,L"Между строками",rg,0,12)){s.rowGap=(int)rg;o.markDirty();}cy+=30;
        float bg2=(float)s.blockGap;if(SLD(o,lx,cy,cw,L"Между блоками",bg2,0,24)){s.blockGap=(int)bg2;o.markDirty();}cy+=30;
        F(o,lx,cy,cw,1,C_BORDER,0.08f);cy+=8;
        T(o,lx,cy,L"ПОЗИЦИЯ",C_DIM,8.5f,false,0.9f);cy+=16;
        struct{int p;const wchar_t*l;}C4[4]={{0,L"\u25E4 верх-лево"},{1,L"\u25E5 верх-право"},{2,L"\u25E3 низ-лево"},{3,L"\u25E2 низ-право"}};
        float bw4=(cw-8)*.5f;
        for(int k=0;k<4;k++){float bx=lx+(k&1)*(bw4+8);float by=cy+(k>>1)*30;
            if(BTN(o,bx,by,bw4,26,C4[k].l,s.position==C4[k].p)){s.position=C4[k].p;o.markDirty();}}
        cy+=64;
        F(o,lx,cy,cw,1,C_BORDER,0.08f);cy+=8;
        if(TOG(o,lx,cy,cw,L"Рамка",s.showBorder))o.markDirty();cy+=24;
        if(TOG(o,lx,cy,cw,L"Разделители",s.showSeparators))o.markDirty();cy+=24;
        if(TOG(o,lx,cy,cw,L"Жирный шрифт",s.fontBold)){o.applySettings(false);}cy+=24;
        if(TOG(o,lx,cy,cw,L"ЗАГЛАВНЫЕ имена",s.uppercase))o.markDirty();cy+=24;
        F(o,lx,cy,cw,1,C_BORDER,0.08f);cy+=8;
        float bw2=(cw-8)*.5f;
        if(BTN(o,lx,cy,bw2,26,L"СТОЛБЦОМ",s.direction==0)){s.direction=0;o.markDirty();}
        if(BTN(o,lx+bw2+8,cy,bw2,26,L"СТРОКОЙ",s.direction==1)){s.direction=1;o.markDirty();}
    }

    // TAB 3: KEYS
    else if(o.m_cfgTab==3){
        T(o,lx,cy,L"СВОИ СОЧЕТАНИЯ",C_DIM,8.5f,false,0.9f);cy+=18;
        static const wchar_t*HKN[HK_COUNT]={L"Показать/скрыть",L"Редактор",L"Тема",L"Угол",L"Сброс AVG",L"Захват OBS"};
        for(int hi=0;hi<HK_COUNT;hi++){
            bool lis=(gHkListen==hi);
            F(o,lx,cy,cw,32,C_CARD,0.02f,6.f);S(o,lx,cy,cw,32,C_BORDER,0.07f,6.f);
            T(o,lx+10,cy+10,HKN[hi],C_FG,10.f,false,0.95f);
            std::wstring hlw=utf8ToWide(hkLabel(s.hk[hi]).c_str());
            float hw=TW(o,hlw.c_str(),9.5f,true);float bx=lx+cw-hw-22;
            F(o,bx,cy+5,hw+16,22,lis?C_AC:C_CARD,lis?0.15f:0.04f,5.f);
            S(o,bx,cy+5,hw+16,22,lis?C_AC:C_BORDER,lis?0.90f:0.12f,5.f);
            T(o,bx+8,cy+9,lis?L"НАЖМИТЕ...":hlw.c_str(),lis?C_AC:C_FG,9.5f,true);
            if(HIT(bx,cy+5,hw+16,22)&&!gDn&&gDnP)gHkListen=hi;
            cy+=38;
        }
        if(BTN(o,lx,cy,cw,28,L"Стандартные")){
            s.hk[0]={VK_F12,false,true,false};s.hk[1]={VK_F11,false,true,false};
            s.hk[2]={VK_F10,false,true,false};s.hk[3]={VK_F9,false,true,false};
            s.hk[4]={VK_F8,false,true,false};s.hk[5]={VK_F7,false,true,false};
            o.hotkeysDirty=true;
        }
    }

    // TAB 4: SYSTEM
    else if(o.m_cfgTab==4){
        T(o,lx,cy,L"ЗАХВАТ (OBS)",C_DIM,8.5f,false,0.9f);cy+=18;
        float bw5=(cw-8)*.5f;
        if(BTN(o,lx,cy,bw5,28,L"РАЗРЕШЁН",s.captureAllow)){s.captureAllow=true;o.applyCapture();}
        if(BTN(o,lx+bw5+8,cy,bw5,28,L"СКРЫТ",!s.captureAllow)){s.captureAllow=false;o.applyCapture();}
        cy+=34;
        T(o,lx,cy,s.captureAllow?L"WDA_NONE":L"WDA_EXCLUDEFROMCAPTURE",C_DIM,8.5f,false,0.85f);cy+=18;
        F(o,lx,cy,cw,1,C_BORDER,0.08f);cy+=8;
        if(TOG(o,lx,cy,cw,L"Автозапуск",s.autostart))o.markDirty();cy+=24;
        if(s.autostart){
            if(TOG(o,lx,cy,cw,L"Свёрнутый старт",s.autostartMinimized))o.markDirty();cy+=24;
            if(BTN(o,lx,cy,cw,28,L"Применить"))o.applyAutostart();cy+=34;
        }
        F(o,lx,cy,cw,1,C_BORDER,0.08f);cy+=8;
        float upd=(float)s.updateIntervalMs;
        if(SLD(o,lx,cy,cw,L"Опрос железа",upd,100,2000))s.updateIntervalMs=(int)upd;cy+=30;
        float fps=(float)s.fpsIntervalMs;
        if(SLD(o,lx,cy,cw,L"FPS",fps,50,1000))s.fpsIntervalMs=(int)fps;cy+=30;
        F(o,lx,cy,cw,1,C_BORDER,0.08f);cy+=8;
        if(TOG(o,lx,cy,cw,L"Сквозные клики",s.clickThrough))o.applyClickThrough();cy+=24;
        if(TOG(o,lx,cy,cw,L"Поверх всех окон",s.alwaysOnTop))o.markDirty();cy+=24;
        F(o,lx,cy,cw,56,C_AC,0.04f,10.f);S(o,lx,cy,cw,56,C_AC,0.20f,10.f);
        PROCESS_MEMORY_COUNTERS pmc{};wchar_t mem[32]=L"?.?";
        if(GetProcessMemoryInfo(GetCurrentProcess(),&pmc,sizeof(pmc)))
            swprintf_s(mem,L"%.1f MB",(double)pmc.WorkingSetSize/1048576.0);
        T(o,lx+12,cy+10,mem,C_AC,11.f,true);
        T(o,lx+12,cy+30,L"1 процесс \u00B7 Direct2D \u00B7 %LOCALAPPDATA%\\YawaFPSHUD",C_DIM,8.f,false,0.85f);
    }

    o.rt()->PopAxisAlignedClip();

    // footer
    float fy=PY+PH-42;
    F(o,PX,fy,PW,42,0xFFFFFF,0.02f);
    F(o,PX,fy,PW,1,0xFFFFFF,0.10f);
    float fbw=PW-24;bool fhv=HIT(PX+12,fy+8,fbw,26);
    F(o,PX+12,fy+8,fbw,26,C_AC,fhv?1.f:0.95f,6.f);
    float flw=TW(o,L"Сохранить config.ini",10.f,true);
    T(o,PX+12+(fbw-flw)*.5f,fy+14,L"Сохранить config.ini",C_BG,10.f,true);
    if(fhv&&!gDn&&gDnP)s.save();

    o.m_lmbWasDown = o.m_lmbDown;
}
