/*
    James William Fletcher (github.com/mrbid)
        April 2022 - October 2023

    Info:
        PoryDrive, a simple 3D driving game where you have to catch Porygon!

    Keyboard:
        ESCAPE = Unlock Mouse
        N = New Game and Car Color
        
        W,A,S,D = Drive Car
        Space = Brake
        L-Shift = Boost
        
        C = Random Car Colors
        R = Accelerate/step DNA color peel
        F = FPS to console
        P = Player stats to console
        O = Toggle auto drive

    Mouse:
        RIGHT CLICK/MOUSE4 = Zoom Snap Close/Ariel
        Scroll = Zoom in/out
    
    Notes:
        The predecessors where different;
        https://github.com/mrbid/porydrive
        https://github.com/PoryDrive/PoryDriveFNN (this was my favorite version)
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef WEB
    #include <emscripten.h>
    #include <emscripten/html5.h>
    #define GL_GLEXT_PROTOTYPES
    #define EGL_EGLEXT_PROTOTYPES
#endif

#define uint GLushort
#define sint GLshort
#define f32 GLfloat

#include "inc/gl.h"
#define GLFW_INCLUDE_NONE
#include "inc/glfw3.h"

#include "inc/esAux4.h"

#include "inc/res.h"
#include "assets/purplecube.h"
#include "assets/porygon.h"
#include "assets/dna.h"
#include "assets/body.h"
#include "assets/windows.h"
#include "assets/wheel.h"

//*************************************
// globals
//*************************************
GLFWwindow* window;
uint winw = 1024, winh = 768;
double t = 0;   // time
f32 dt = 0;     // delta time
double fc = 0;  // frame count
double lfct = 0;// last frame count time
f32 aspect;
double x,y,lx,ly,ww,wh;

// render state id's
GLint projection_id;
GLint modelview_id;
GLint position_id;
GLint lightpos_id;
GLint color_id;
GLint opacity_id;
GLint normal_id;

// render state matrices
mat projection;
mat view;
mat model;
mat modelview;

// render state inputs
vec lightpos = {0.f, 0.f, 0.f};

// models
sint bindstate = -1;
sint bindstate2 = -1;
uint keystate[6] = {0};
ESModel mdlPurpleCube;
GLuint mdlBlueCubeColors;
ESModel mdlPorygon;
ESModel mdlDNA;
ESModel mdlBody;
GLuint mdlBodyColors2;
ESModel mdlWindows;
ESModel mdlWheel;
f32 sc1, sc2, sc3, bc1, bc2, bc3;

// game vars
#define FAR_DISTANCE 60.f
#define NEWGAME_SEED 1337
double st=0; // start time
char tts[32];// time taken string

// camera vars
uint focus_cursor = 1;
double sens = 0.003f;
f32 xrot = PI;
f32 yrot = 1.3f;
f32 zoom = -0.3f;

// player vars
f32 pr; // rotation
f32 sr; // steering rotation
vec pp; // position
vec pv; // velocity
vec pd; // wheel direction
vec pbd;// body direction
f32 sp; // speed
uint cp;// collected porygon count
uint cc;// collision count
f32 pc = 0.f;// is player colliding
f32 bs;// boost seconds
f32 bss;// boost start time

// basic autodrive
uint auto_drive=0;
f32 ad_min_dstep = 0.01f;
f32 ad_max_dstep = 0.06f;
f32 ad_min_speedswitch = 2.f;
f32 ad_maxspeed_reductor = 0.5f;

// porygon vars
vec zp; // position
vec zd; // direction
f32 zr; // rotation
f32 zs; // speed
double za;// alive state
f32 zt; // twitch radius

// configurable vars
f32 maxspeed =      0.0265f;
f32 acceleration =  0.0028f;
f32 inertia =       0.0022f;
f32 drag =          0.00038f;
f32 steeringspeed = 0.04f;
f32 steerinertia =  120.f;
f32 minsteer =      0.3f;
f32 maxsteer =      0.36f;
f32 steering_deadzone = 0.033f;
f32 steeringtransfer = 0.023f;
f32 steeringtransferinertia = 280.f;
f32 suspension_pitch = 3.f;
f32 suspension_pitch_limit = 0.06f;
f32 suspension_roll = 30.f;
f32 suspension_roll_limit = 0.16f;
uint sticky_collisions = 0;

char cname[256] = {0};

//*************************************
// utility functions
//*************************************
void timestamp(char* ts)
{
    const time_t tt = time(0);
    strftime(ts, 16, "%H:%M:%S", localtime(&tt));
}
#ifndef WEB
void loadConfig(uint type)
{
    FILE* f = fopen("config.txt", "r");
    if(f)
    {
        sprintf(cname, "config.txt");
        
        if(type == 1)
        {
            char strts[16];
            timestamp(&strts[0]);
            printf("[%s] CONFIG: config.txt loaded.\n", strts);
        }
        else
            printf("\nDetected config.txt loading settings...\n");

        char line[256];
        while(fgets(line, 256, f) != NULL)
        {
            char set[64];
            memset(set, 0, 64);
            f32 val;
            
            if(sscanf(line, "%63s %f", set, &val) == 2)
            {
                if(type == 0)
                    printf("Setting Loaded: %s %g\n", set, val);

                // car physics
                if(strcmp(set, "maxspeed") == 0){maxspeed = val;}
                if(strcmp(set, "acceleration") == 0){acceleration = val;}
                if(strcmp(set, "inertia") == 0){inertia = val;}
                if(strcmp(set, "drag") == 0){drag = val;}
                if(strcmp(set, "steeringspeed") == 0){steeringspeed = val;}
                if(strcmp(set, "steerinertia") == 0){steerinertia = val;}
                if(strcmp(set, "minsteer") == 0){minsteer = val;}
                if(strcmp(set, "maxsteer") == 0){maxsteer = val;}
                if(strcmp(set, "steering_deadzone") == 0){steering_deadzone = val;}
                if(strcmp(set, "steeringtransfer") == 0){steeringtransfer = val;}
                if(strcmp(set, "steeringtransferinertia") == 0){steeringtransferinertia = val;}
                if(strcmp(set, "suspension_pitch") == 0){suspension_pitch = val;}
                if(strcmp(set, "suspension_pitch_limit") == 0){suspension_pitch_limit = val;}
                if(strcmp(set, "suspension_roll") == 0){suspension_roll = val;}
                if(strcmp(set, "suspension_roll_limit") == 0){suspension_roll_limit = val;}
                if(strcmp(set, "sticky_collisions") == 0){sticky_collisions = (uint)val;}

                // auto drive
                if(strcmp(set, "ad_min_dstep") == 0){ad_min_dstep = val;}
                if(strcmp(set, "ad_max_dstep") == 0){ad_max_dstep = val;}
                if(strcmp(set, "ad_min_speedswitch") == 0){ad_min_speedswitch = val;}
                if(strcmp(set, "ad_maxspeed_reductor") == 0){ad_maxspeed_reductor = val;}
            }
        }
        fclose(f);
    }
    else
    {
        if(type == 1)
        {
            char strts[16];
            timestamp(&strts[0]);
            printf("[%s] CONFIG: No config.txt file detected.\n", strts);
        }
    }
}
#endif
void timeTaken(uint ss)
{
    if(ss == 1)
    {
        const double tt = t-st;
        if(tt < 60.0)
            sprintf(tts, "%.0f Sec", tt);
        else if(tt < 3600.0)
            sprintf(tts, "%.2f Min", tt * 0.016666667);
        else if(tt < 216000.0)
            sprintf(tts, "%.2f Hr", tt * 0.000277778);
        else if(tt < 12960000.0)
            sprintf(tts, "%.2f Days", tt * 0.00000463);
    }
    else
    {
        const double tt = t-st;
        if(tt < 60.0)
            sprintf(tts, "%.0f Seconds", tt);
        else if(tt < 3600.0)
            sprintf(tts, "%.2f Minutes", tt * 0.016666667);
        else if(tt < 216000.0)
            sprintf(tts, "%.2f Hours", tt * 0.000277778);
        else if(tt < 12960000.0)
            sprintf(tts, "%.2f Days", tt * 0.00000463);
    }
}

//*************************************
// render functions
//*************************************
void modelBind(const ESModel* mdl)
{
    glBindBuffer(GL_ARRAY_BUFFER, mdl->cid);
    glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(color_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdl->vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdl->nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdl->iid);
}
#ifndef WEB
void iterBody()
{
    static const uint mi = body_numvert*3;
    static f32 cd = 1.f;
    for(uint i = 0; i < mi; i++)
    {
        body_colors2[i] += fRandFloat(0.1f, 0.6f) * cd;
        if(body_colors2[i] >= 1.f)
            cd = -1.f;
        else if(body_colors2[i] <= 0.f)
            cd = 1.f;
    }
    esRebind(GL_ARRAY_BUFFER, &mdlBodyColors2, body_colors2, sizeof(body_colors2), GL_STATIC_DRAW);
}
#endif
void iterDNA()
{
    static const uint mi = dna_numvert*3;
    static f32 cd = 1.f;
    for(uint i = 0; i < mi; i++)
    {
        dna_colors[i] += fRandFloat(0.1f, 0.6f) * cd;
        if(dna_colors[i] >= 1.f)
            cd = -1.f;
        else if(dna_colors[i] <= 0.f)
            cd = 1.f;
    }
    esRebind(GL_ARRAY_BUFFER, &mdlDNA.cid, dna_colors, sizeof(dna_colors), GL_STATIC_DRAW);
#ifndef WEB
    iterBody();
#endif
}
void rCube(f32 x, f32 y)
{
    mIdent(&model);
    mTranslate(&model, x, y, 0.f);
    mMul(&modelview, &model, &view);
    glUniform1f(opacity_id, 1.0f);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    if(bindstate != 1)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mdlPurpleCube.vid);
        glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlPurpleCube.nid);
        glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normal_id);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlPurpleCube.iid);

        bindstate = 1;
        bindstate2 = -1;
    }

    // cube collisions
    const f32 dlap = vDistLa(zp, (vec){x, y, 0.f}); // porygon
    if(dlap < 0.15f)
    {
        vec nf;
        vSub(&nf, zp, (vec){x, y, 0.f});
        vNorm(&nf);
        vMulS(&nf, nf, 0.15f-dlap);
        vAdd(&zp, zp, nf);
    }

    // if car is moving compute collisions
    if(sp > inertia || sp < -inertia)
    {
        // front collision cube point
        vec cp1 = pp;
        vec cd1 = pbd;
        vMulS(&cd1, cd1, 0.0525f);
        vAdd(&cp1, cp1, cd1);

        // back collision cube point
        vec cp2 = pp;
        vec cd2 = pbd;
        vMulS(&cd2, cd2, -0.0525f);
        vAdd(&cp2, cp2, cd2);

        // do Axis-Aligned Cube collisions for points against rCube() being rendered
        const f32 dla1 = vDistLa(cp1, (vec){x, y, 0.f}); // front car
        const f32 dla0 = vDistLa(pp, (vec){x, y, 0.f}); // center car
        const f32 dla2 = vDistLa(cp2, (vec){x, y, 0.f}); // back car
        if(dla1 <= 0.097f)
        {
            vec nf;
            vSub(&nf, pp, (vec){x, y, 0.f});
            vNorm(&nf);
            vMulS(&nf, nf, 0.097f-dla1);
            vAdd(&pv, pv, nf);
            if(sticky_collisions){sp *= 0.5f;}
        }
        else if(dla0 <= 0.097f)
        {
            vec nf;
            vSub(&nf, pp, (vec){x, y, 0.f});
            vNorm(&nf);
            vMulS(&nf, nf, 0.097f-dla0);
            vAdd(&pv, pv, nf);
            if(sticky_collisions){sp *= 0.5f;}
        }
        else if(dla2 <= 0.097f)
        {
            vec nf;
            vSub(&nf, pp, (vec){x, y, 0.f});
            vNorm(&nf);
            vMulS(&nf, nf, 0.097f-dla2);
            vAdd(&pv, pv, nf);
            if(sticky_collisions){sp *= 0.5f;}
        }
    }

    // check to see if cube needs to be blue
    const f32 dla = vDist(pp, (vec){x, y, 0.f}); // worth it to prevent the flicker

    // official colliding count
    static f32 colliding = 0.f;
    if(dla <= 0.13f)
    {
        if(colliding == 0.f)
        {
            colliding = x*y+x;
            cc++;

            // char strts[16];
            // timestamp(&strts[0]);
            // printf("[%s] Collisions: %u\n", strts, cc);
        }
    }
    else if(x*y+x == colliding)
    {
        colliding = 0.f;
    }

    // player colliding
    if(dla <= 0.17f)
    {
        bs += 0.006f; // little boost
        if(pc == 0.f)
        {
            pc = x*y+x;
            cc++;

            // char strts[16];
            // timestamp(&strts[0]);
            // printf("[%s] Collisions: %u\n", strts, cc);
        }
    }
    else if(x*y+x == pc)
    {
        pc = 0.f;
    }

    const uint collision = (dla < 0.17f || dlap < 0.16f);
    if(collision == 1 && bindstate2 <= 1)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mdlBlueCubeColors);
        glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(color_id);
        bindstate2 = 2;
    }
    else if(collision == 0 && bindstate2 != 1)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mdlPurpleCube.cid);
        glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(color_id);
        bindstate2 = 1;
    }

    glDrawElements(GL_TRIANGLES, purplecube_numind, GL_UNSIGNED_SHORT, 0);
}

void rPorygon(f32 x, f32 y, f32 r)
{
    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, 0.f);
    mRotZ(&model, r);

    const f32 zss = 4.f + (8.f*(1.f-(zs*111.111111111f)));

    if(za != 0.0)
        mScale(&model, zss, zss, 0.1f);
    else
        mScale(&model, zss, zss, zss);

    mMul(&modelview, &model, &view);

    // returns direction
    mGetDirY(&zd, model);
    vInv(&zd);

    if(za != 0.0)
        glUniform1f(opacity_id, (za-t)/6.0);
    else
        glUniform1f(opacity_id, 1.0f);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    modelBind(&mdlPorygon);

    if(za != 0.0)
        glEnable(GL_BLEND);
    glDrawElements(GL_TRIANGLES, porygon_numind, GL_UNSIGNED_BYTE, 0);;
    if(za != 0.0)
        glDisable(GL_BLEND);
}

void rDNA(f32 x, f32 y, f32 z)
{
    static f32 dr = 0.f;
    dr += 1.f * dt;

    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotZ(&model, dr);
    mMul(&modelview, &model, &view);

    glUniform1f(opacity_id, 1.0f);
    
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    modelBind(&mdlDNA);
    glDrawElements(GL_TRIANGLES, dna_numind, GL_UNSIGNED_SHORT, 0);
}

void rCar(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = -1;

    // opaque
    glUniform1f(opacity_id, 1.0f);

    // wheel spin speed
    static f32 wr = 0.f;
    const f32 speed = sp * 33.f;
    if(sp > inertia || sp < -inertia)
        wr += speed;

    // wheel; front left
    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotZ(&model, -rx);
    mTranslate(&model, 0.026343f, -0.054417f, 0.012185f);
    mRotZ(&model, sr);

    // returns direction
    mGetDirY(&pd, model);
    vInv(&pd);

    //
    mRotY(&model, -wr);
    mMul(&modelview, &model, &view);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    modelBind(&mdlWheel);
    glDrawElements(GL_TRIANGLES, wheel_numind, GL_UNSIGNED_SHORT, 0);

    // wheel; back left
    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotZ(&model, -rx);
    mTranslate(&model, 0.026343f, 0.045294f, 0.012185f);
    mRotY(&model, -wr);
    mMul(&modelview, &model, &view);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    modelBind(&mdlWheel);
    glDrawElements(GL_TRIANGLES, wheel_numind, GL_UNSIGNED_SHORT, 0);

    // wheel; front right
    mIdent(&model);
    mRotZ(&model, PI);
    mTranslate(&model, -x, -y, -z);
    mRotZ(&model, -rx);
    mTranslate(&model, 0.026343f, 0.054417f, 0.012185f);
    mRotZ(&model, sr);
    mRotY(&model, wr);
    mMul(&modelview, &model, &view);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    modelBind(&mdlWheel);
    glDrawElements(GL_TRIANGLES, wheel_numind, GL_UNSIGNED_SHORT, 0);

    // wheel; back right
    mIdent(&model);
    mRotZ(&model, PI);
    mTranslate(&model, -x, -y, -z);
    mRotZ(&model, -rx);
    mTranslate(&model, 0.026343f, -0.045294f, 0.012185f);
    mRotY(&model, wr);
    mMul(&modelview, &model, &view);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    modelBind(&mdlWheel);
    glDrawElements(GL_TRIANGLES, wheel_numind, GL_UNSIGNED_SHORT, 0);

    // body & window matrix
    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotZ(&model, -rx);

    // returns direction
    mGetDirY(&pbd, model);
    vInv(&pbd);

    //
    f32 sy = sp*suspension_pitch;
    if(fabsf(sy) < inertia)
    {
        sy = 0.f;
    }
    else
    {
        if(sy > suspension_pitch_limit){sy = suspension_pitch_limit;}
        if(sy < -suspension_pitch_limit){sy = -suspension_pitch_limit;}
    }
    mRotY(&model, sy);
    f32 sx = sr*suspension_roll*sp; // turning suspension
    static f32 lsx = 0.f;
    if(fabsf(sp) < inertia)
    {
        sx = lsx;
    }
    if(sx > suspension_roll_limit){sx = suspension_roll_limit;}
    if(sx < -suspension_roll_limit){sx = -suspension_roll_limit;}
    lsx = sx;
    mRotX(&model, sx);
    mMul(&modelview, &model, &view);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    
    // body
    modelBind(&mdlBody);
#ifdef WEB
    if(pc == 0.f)
    {
        glDisable(GL_CULL_FACE);
        glDrawElements(GL_TRIANGLES, body_numind, GL_UNSIGNED_SHORT, 0);
        glEnable(GL_CULL_FACE);
    }
#else
    if(pc == 0.f && cp > 0)
    {
        glDisable(GL_CULL_FACE);
        glDrawElements(GL_TRIANGLES, body_numind, GL_UNSIGNED_SHORT, 0);
        glEnable(GL_CULL_FACE);
    }
    else
    {
        if(cp == 0)
        {
            glDisable(GL_CULL_FACE);
            glDrawElements(GL_TRIANGLES, body_numind, GL_UNSIGNED_SHORT, 0);
            glEnable(GL_CULL_FACE);
        }
        else
        {
            if(cp > 1)
            {
                glDisable(GL_CULL_FACE);
                glDrawElements(GL_TRIANGLES, body_numind, GL_UNSIGNED_SHORT, 0);
                glEnable(GL_CULL_FACE);

                glBindBuffer(GL_ARRAY_BUFFER, mdlBodyColors2);
                glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(color_id);
            }

            glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, body_numind, GL_UNSIGNED_SHORT, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_DEPTH_TEST);
        }
    }
#endif

    // transparent
    glUniform1f(opacity_id, 0.3f);

    // windows
    modelBind(&mdlWindows);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glDrawElements(GL_TRIANGLES, windows_numind, GL_UNSIGNED_SHORT, 0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

//*************************************
// game functions
//*************************************
void newGame(unsigned int seed)
{
    srand(seed);
    srandf(seed);

    char strts[16];
    timestamp(&strts[0]);
    printf("[%s] Game Start [%u].\n", strts, seed);
    
    glfwSetWindowTitle(window, "PoryDrive");
    
    pp = (vec){0.f, 0.f, 0.f};
    pv = (vec){0.f, 0.f, 0.f};
    pd = (vec){0.f, 0.f, 0.f};

    st = t;

    cp = 0;
    cc = 0;
    pr = 0.f;
    sr = 0.f;
    sp = 0.f;
    bs = 1.f;
    bss = 0.f;

    zp = (vec){esRandFloat(-18.f, 18.f), esRandFloat(-18.f, 18.f), 0.f};
    zs = 0.002f;
    za = 0.0;
    zt = 0.08f;

    // random car body colors
    if(seed != 1337)
    {
        const f32 nbc1 = esRandFloat(0.f, 1.f);
        const f32 nbc2 = esRandFloat(0.f, 1.f);
        const f32 nbc3 = esRandFloat(0.f, 1.f);
        const f32 nsc1 = esRandFloat(0.f, 1.f);
        const f32 nsc2 = esRandFloat(0.f, 1.f);
        const f32 nsc3 = esRandFloat(0.f, 1.f);
        const uint maxv = body_numvert*3;
        for(uint v = 0; v < maxv; v+=3)
        {
            if( body_colors[v]   == sc1 && // seats
                body_colors[v+1] == sc2 &&
                body_colors[v+2] == sc3 )
            {
                body_colors[v]   = nsc1;
                body_colors[v+1] = nsc2;
                body_colors[v+2] = nsc3;
            }

            if( body_colors[v]   == bc1 && // body
                body_colors[v+1] == bc2 &&
                body_colors[v+2] == bc3 )
            {
                body_colors[v]   = nbc1;
                body_colors[v+1] = nbc2;
                body_colors[v+2] = nbc3;
            }
        }
        bc1 = nbc1, bc2 = nbc2, bc3 = nbc3;
        sc1 = nsc1, sc2 = nsc2, sc3 = nsc3;
        esRebind(GL_ARRAY_BUFFER, &mdlBody.cid, body_colors, sizeof(body_colors), GL_STATIC_DRAW);
    }
}

//*************************************
// update & render
//*************************************
void main_loop()
{
    glfwPollEvents();
    fc++;
//*************************************
// time delta for interpolation
//*************************************
    static f32 lt = 0;
    t = glfwGetTime();
    dt = t-lt;
    lt = t;

#ifdef WEB
    EmscriptenPointerlockChangeEvent e;
    if(emscripten_get_pointerlock_status(&e) == EMSCRIPTEN_RESULT_SUCCESS)
    {
        if(focus_cursor == 0 && e.isActive == 1)
        {
            glfwGetCursorPos(window, &lx, &ly);
        }
        focus_cursor = e.isActive;
    }
#endif
    
    static f32 lbs = 0.f; // increase boost
    if(t > lbs)
    {
#ifdef WEB
        bs += 0.02f;
#else
        bs += 0.01f;
#endif
        if(bs > 5.f){bs = 5.f;}
        lbs = t+0.1f;
    }

//*************************************
// 60fps limited code
//*************************************
    static f32 lut = 0.f;
    if(t > lut)
    {
        // keystates
        f32 tr = maxsteer * ((maxspeed-sp) * steerinertia);
        if(tr < minsteer){tr = minsteer;}

        if(keystate[0] == 1)
        {
            sr -= steeringspeed;
            if(sr < -tr){sr += steeringspeed;}
        }

        if(keystate[1] == 1)
        {
            sr += steeringspeed;
            if(sr > tr){sr -= steeringspeed;}
        }

        if(keystate[0] == 0 && keystate[1] == 0)
        {
            if(sr > steering_deadzone)
                sr -= steeringspeed;
            else if(sr < -steering_deadzone)
                sr += steeringspeed;
            else
                sr = 0.f;
        }
        
        if(keystate[4] == 1)
        {
            sp *= 0.90f;
        }
        else
        {
            if(keystate[2] == 1)
            {
                f32 tacc = acceleration;
                if(keystate[5] == 1 && bs > 0.f && bss != 0.f) // boost
                {
                    bs -= t-bss;
                    bss = t;
                    tacc *= 14.f;
                }
                else if(keystate[5] == 1 && bss == 0.f)
                {
                    bss = t;
                }
                if(bs < 0.f){bs = 0.f;}
                //printf("boost: %g\n", bs);

                vec inc;
                sp += tacc;
                vMulS(&inc, pd, tacc);
                vAdd(&pv, pv, inc);
            }

            if(keystate[3] == 1)
            {    
                vec inc;
                sp -= acceleration;
                vMulS(&inc, pd, -acceleration);
                vAdd(&pv, pv, inc);
            }
        }

        // update title bar stats
        static double ltut = 3.0;
        if(t > ltut)
        {
            timeTaken(1);
            char title[512];
            const f32 dsp = fabsf(sp*(1.f/maxspeed)*130.f);
            if(cname[0] != 0x00)
                sprintf(title, "%u Pory | %s | %.f MPH | %s", cp, tts, dsp, cname);
            else
                sprintf(title, "%u Pory | %s | %.f MPH", cp, tts, dsp);
            glfwSetWindowTitle(window, title);
            ltut = t + 1.0;
        }

        // auto drive
        if(auto_drive == 1)
        {
            vec lad = pp;
            vSub(&lad, lad, zp);
            vNorm(&lad);
            const f32 as = fabsf(vDot(pbd, lad)+1.f) * 0.5f;
            static f32 ld = 0.f, td = 1.f;
            const f32 d = vDist(pp, zp);
            f32 ds = d * 0.01f;
            if(ds < ad_min_dstep){ds = ad_min_dstep;}
            else if(ds > ad_max_dstep){ds = ad_max_dstep;}
            if(fabsf(ld-d) > ds && ld < d){td *= -1.f;}
            ld = d;
            sr = (tr * as) * td;
            if(d < ad_min_speedswitch)
                sp = maxspeed * (d*ad_maxspeed_reductor)+0.003f;
            else
                sp = maxspeed;
        }

        // simulate car
        if(sp > 0.f)
            sp -= drag;
        else
            sp += drag;

        if(fabsf(sp) > maxspeed)
        {
            if(sp > 0.f)
                sp = maxspeed;
            else
                sp = -maxspeed;
        }

        if(sp > inertia || sp < -inertia)
        {
            vAdd(&pp, pp, pv);
            vMulS(&pv, pd, sp);
            pr -= sr * steeringtransfer * (sp*steeringtransferinertia);
        }

        if(pp.x > 17.5f){pp.x = 17.5f;}
        else if(pp.x < -17.5f){pp.x = -17.5f;}
        if(pp.y > 17.5f){pp.y = 17.5f;}
        else if(pp.y < -17.5f){pp.y = -17.5f;}

        // simulate porygon
        if(za == 0.0)
        {
            vec inc;
            vMulS(&inc, zd, zs);
            vAdd(&zp, zp, inc);
            zr += fRandFloat(-zt, zt);

            if(zp.x > 17.5f){zp.x = 17.5f; zr = fRandFloat(-PI, PI);}
            else if(zp.x < -17.5f){zp.x = -17.5f; zr = fRandFloat(-PI, PI);}
            if(zp.y > 17.5f){zp.y = 17.5f; zr = fRandFloat(-PI, PI);}
            else if(zp.y < -17.5f){zp.y = -17.5f; zr = fRandFloat(-PI, PI);}

            // front collision cube point
            vec cp1 = pp;
            vec cd1 = pbd;
            vMulS(&cd1, cd1, 0.0525f);
            vAdd(&cp1, cp1, cd1);

            // back collision cube point
            vec cp2 = pp;
            vec cd2 = pbd;
            vMulS(&cd2, cd2, -0.0525f);
            vAdd(&cp2, cp2, cd2);

            // do Axis-Aligned Cube collisions for both points against porygon
            const f32 dla1 = vDistLa(cp1, zp); // front car
            const f32 dla2 = vDistLa(cp2, zp); // back car
            if(dla1 < 0.14f || dla2 < 0.14f)
            {
                cp++;
                za = t+6.0;
                iterDNA();

                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] Porygon collected: %u, collisions: %u\n", strts, cp, cc);
                cc = 0;
            }
        }
        else if(t > za)
        {
            srand(time(0));
            zp = (vec){esRandFloat(-18.f, 18.f), esRandFloat(-18.f, 18.f), 0.f};
            zs = esRandFloat(0.002f, 0.009f);
            zt = esRandFloat(0.08f, 0.16f);
            za = 0.0;
        }

        lut = t + 0.016666667f;
    }

//*************************************
// camera
//*************************************
    if(focus_cursor == 1)
    {
        glfwGetCursorPos(window, &x, &y);

        xrot += (lx-x)*sens;
        yrot += (ly-y)*sens;

        if(yrot > 1.5f)
            yrot = 1.5f;
        if(yrot < 0.5f)
            yrot = 0.5f;

        lx = x, ly = y;
    }

    mIdent(&view);
    mTranslate(&view, 0.f, -0.033f, zoom);
    mRotate(&view, yrot, 1.f, 0.f, 0.f);
    mRotate(&view, xrot, 0.f, 0.f, 1.f);
    mTranslate(&view, -pp.x, -pp.y, -pp.z);

//*************************************
// begin render
//*************************************
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//*************************************
// main render
//*************************************

    // render scene
    srand(NEWGAME_SEED);
    for(f32 i = -17.5f; i <= 18.f; i += 0.53f)
        for(f32 j = -17.5f; j <= 18.f; j += 0.53f)
            if((i < -0.1f || i > 0.1f) || (j < -0.1f || j > 0.1f))
                if(esRand(0, 100) < 50)
                    rCube(i, j);

    // render porygon
    rPorygon(zp.x, zp.y, zr);

    // render dna
    rDNA(0.f, 0.f, 0.1f);

    // render player
    rCar(pp.x, pp.y, pp.z, pr);

//*************************************
// swap buffers / display render
//*************************************
    glfwSwapBuffers(window);
}

//*************************************
// Input Handelling
//*************************************
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // control
    if(action == GLFW_PRESS)
    {
        if(     key == GLFW_KEY_A || key == GLFW_KEY_LEFT)  { keystate[0] = 1; keystate[1] = 0; }
        else if(key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) { keystate[1] = 1; keystate[0] = 0; }
        else if(key == GLFW_KEY_W || key == GLFW_KEY_UP)    { keystate[2] = 1; }
        else if(key == GLFW_KEY_S || key == GLFW_KEY_DOWN)  { keystate[3] = 1; }
        else if(key == GLFW_KEY_SPACE)                      { keystate[4] = 1; }
        else if(key == GLFW_KEY_LEFT_SHIFT ||
                key == GLFW_KEY_RIGHT_CONTROL)              { keystate[5] = 1; }

        // new game
        else if(key == GLFW_KEY_N)
        {
            // end
            timeTaken(0);
            char strts[16];
            timestamp(&strts[0]);
            printf("[%s] Game End.\n", strts);
            printf("[%s] Porygon Collected: %u\n", strts, cp);
            printf("[%s] Time-Taken: %s or %g Seconds\n\n", strts, tts, t-st);
            
            // new
            newGame(time(0));
        }

        else if(key == GLFW_KEY_R)
        {
            printf("%g %g %g\n", dna_colors[0], dna_colors[1], dna_colors[2]);
            iterDNA();
            cp++;
        }

        // stats
        else if(key == GLFW_KEY_P)
        {
            char strts[16];
            timestamp(&strts[0]);
            printf("[%s] Porygon Collected: %u\n", strts, cp);
        }

        // toggle mouse focus
        else if(key == GLFW_KEY_ESCAPE)
        {
            focus_cursor = 0;
#ifndef WEB
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwGetCursorPos(window, &lx, &ly);
#endif
        }

        // show average fps
        else if(key == GLFW_KEY_F)
        {
            if(t-lfct > 2.0)
            {
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] FPS: %g\n", strts, fc/(t-lfct));
                lfct = t;
                fc = 0;
            }
        }

        else if(key == GLFW_KEY_C)
        {
            srand(t*100.0);
            const f32 nbc1 = esRandFloat(0.f, 1.f);
            const f32 nbc2 = esRandFloat(0.f, 1.f);
            const f32 nbc3 = esRandFloat(0.f, 1.f);
            const f32 nsc1 = esRandFloat(0.f, 1.f);
            const f32 nsc2 = esRandFloat(0.f, 1.f);
            const f32 nsc3 = esRandFloat(0.f, 1.f);
            const uint maxv = body_numvert*3;
            for(uint v = 0; v < maxv; v+=3)
            {
                if( body_colors[v]   == sc1 && // seats
                    body_colors[v+1] == sc2 &&
                    body_colors[v+2] == sc3 )
                {
                    body_colors[v]   = nsc1;
                    body_colors[v+1] = nsc2;
                    body_colors[v+2] = nsc3;
                }

                if( body_colors[v]   == bc1 && // body
                    body_colors[v+1] == bc2 &&
                    body_colors[v+2] == bc3 )
                {
                    body_colors[v]   = nbc1;
                    body_colors[v+1] = nbc2;
                    body_colors[v+2] = nbc3;
                }
            }
            bc1 = nbc1, bc2 = nbc2, bc3 = nbc3;
            sc1 = nsc1, sc2 = nsc2, sc3 = nsc3;
            esRebind(GL_ARRAY_BUFFER, &mdlBody.cid, body_colors, sizeof(body_colors), GL_STATIC_DRAW);
        }

        // toggle auto drive
        else if(key == GLFW_KEY_O)
        {
            auto_drive = 1 - auto_drive;
            if(auto_drive == 0)
            {
                sp = 0.f;
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] Auto Drive: OFF\n", strts);
            }
            else
            {
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] Auto Drive: ON\n", strts);
            }
        }
    }
    else if(action == GLFW_RELEASE)
    {
        if(     key == GLFW_KEY_A || key == GLFW_KEY_LEFT)  { keystate[0] = 0; }
        else if(key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) { keystate[1] = 0; }
        else if(key == GLFW_KEY_W || key == GLFW_KEY_UP)    { keystate[2] = 0; }
        else if(key == GLFW_KEY_S || key == GLFW_KEY_DOWN)  { keystate[3] = 0; }
        else if(key == GLFW_KEY_SPACE)                      { keystate[4] = 0; }
        else if(key == GLFW_KEY_LEFT_SHIFT ||
                key == GLFW_KEY_RIGHT_CONTROL)              { keystate[5] = 0; bss = 0.f; }
    }
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if(yoffset < 0){zoom += 0.06f * zoom;}else{zoom -= 0.06f * zoom;}
    if(zoom > -0.11f){zoom = -0.11f;}
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        if(focus_cursor == 0)
        {
            focus_cursor = 1;
#ifndef WEB
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwGetCursorPos(window, &lx, &ly);
#endif
        }
        if(button == GLFW_MOUSE_BUTTON_4 || button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            if(zoom != -0.3f){zoom = -0.3f;}else{zoom = -3.3f;}
        }
    }
}
void window_size_callback(GLFWwindow* window, int width, int height)
{
    winw = width, winh = height;
    glViewport(0, 0, winw, winh);
    aspect = (f32)winw / (f32)winh;
    ww = winw, wh = winh;
    mIdent(&projection);
    mPerspective(&projection, 60.0f, aspect, 0.01f, FAR_DISTANCE);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*)&projection.m[0][0]);
}
#ifdef WEB
EM_BOOL emscripten_resize_event(int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
{
    winw = uiEvent->documentBodyClientWidth;
    winh = uiEvent->documentBodyClientHeight;
    window_size_callback(window, winw, winh);
    emscripten_set_canvas_element_size("canvas", winw, winh);
    return EM_FALSE;
}
#endif

//*************************************
// Process Entry Point
//*************************************
int main(int argc, char** argv)
{
#ifdef WEB
    focus_cursor = 0;
#endif

    // allow custom msaa level
    int msaa = 16;
    if(argc >= 2){msaa = atoi(argv[1]);}

    // help
    printf("----\n");
    printf("PoryDrive 2.0 (more fun one)\n");
    printf("----\n");
    printf("James William Fletcher (github.com/mrbid)\n");
    printf("----\n");
#ifndef WEB
    printf("One command line argument, msaa 0-16.\n");
    printf("e.g; ./porydrive 16\n");
    printf("----\n");
#endif
    printf("~ Keyboard Input:\n");
    printf("ESCAPE = Focus/Unfocus Mouse Look\n");
    printf("F = FPS to console\n");
    printf("P = Player stats to console\n");
    printf("O = Toggle auto drive\n");
    printf("C = Random Car Colors\n");
    printf("N = New Game\n");
    printf("W = Drive Forward\n");
    printf("A = Turn Left\n");
    printf("S = Drive Backward\n");
    printf("D = Turn Right\n");
    printf("Space = Brake\n");
    printf("L-Shift/R-CTRL = Boost\n");
    printf("----\n");
    printf("~ Mouse Input:\n");
    printf("RIGHT/MOUSE4 = Zoom Snap Close/Ariel\n");
    printf("Scroll = Zoom in/out\n");
    printf("----\n");
    printf("~ How to play:\n");
    printf("Drive around and \"collect\" Porygon, each time you collect a Porygon a new one will randomly spawn somewhere on the map. A Porygon colliding with a purple cube will cause it to light up blue, this can help you find them. Upon right clicking the mouse you will switch between Ariel and Close views, in the Ariel view it is easier to see which of the purple cubes that the Porygon is colliding with. You can coast the side of collisions to speed up the recharging of boost.\n");
    printf("----\n");
#ifndef WEB
    printf("~ Create custom car physics:\n");
    printf("It is possible to tweak the car physics by creating a config.txt file in the exec/working directory of the game, here is an example of such config file with the default car phsyics variables.\n");
    printf("~ config.txt:\n");
    printf("maxspeed 0.0265\n");
    printf("acceleration 0.0028\n");
    printf("inertia 0.0022\n");
    printf("drag 0.00038\n");
    printf("steeringspeed 0.04\n");
    printf("steerinertia 120\n");
    printf("minsteer 0.3\n");
    printf("maxsteer 0.36\n");
    printf("steering_deadzone 0.033\n");
    printf("steeringtransfer 0.023\n");
    printf("steeringtransferinertia 280\n");
    printf("suspension_pitch 3\n");
    printf("suspension_pitch_limit 0.06\n");
    printf("suspension_roll 30\n");
    printf("suspension_roll_limit 0.3\n");
    printf("sticky_collisions 0\n");
    printf("ad_min_dstep 0.01\n");
    printf("ad_max_dstep 0.06\n");
    printf("ad_min_speedswitch 2\n");
    printf("ad_maxspeed_reductor 0.5\n");
    printf("----\n");
#endif
    printf("https://notabug.org/Vandarin/PoryDrive\n");
    printf("https://github.com/mrbid/PoryDrive-2.0\n");
    printf("https://flathub.org/apps/com.voxdsp.PoryDrive\n");
    printf("https://snapcraft.io/porydrive\n");
    printf("https://github.com/mrbid/porydrive\n");
    printf("https://github.com/PoryDrive/PoryDriveFNN\n");
    printf("----\n");
    printf("BMW E34 Model is made by Krzysztof Stolorz (KrStolorz) (https://sketchfab.com/KrStolorz)\n");
    printf("----\n");
    printf("%s\n", glfwGetVersionString());
    printf("----\n");

    // init glfw
    if(!glfwInit()){printf("glfwInit() failed.\n"); exit(EXIT_FAILURE);}
#ifdef WEB
    double width, height;
    emscripten_get_element_css_size("body", &width, &height);
    winw = (uint)width, winh = (uint)height;
#endif
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, msaa);
    window = glfwCreateWindow(winw, winh, "PoryDrive", NULL, NULL);
    if(!window)
    {
        printf("glfwCreateWindow() failed.\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    const GLFWvidmode* desktop = glfwGetVideoMode(glfwGetPrimaryMonitor());
#ifndef WEB
    glfwSetWindowPos(window, (desktop->width/2)-(winw/2), (desktop->height/2)-(winh/2)); // center window on desktop
    glfwSetCursorPos(window, (desktop->width/2)-(winw/2), (desktop->height/2)-(winh/2)); // center cursor in window
    glfwGetCursorPos(window, &lx, &ly);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // if(glfwRawMouseMotionSupported() == GLFW_TRUE){glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);}
#endif
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync

    // set icon
    glfwSetWindowIcon(window, 1, &(GLFWimage){16, 16, (unsigned char*)&icon_image.pixel_data});

//*************************************
// bind vertex and index buffers
//*************************************

    // ***** BIND BODY *****
    esBind(GL_ARRAY_BUFFER, &mdlBody.vid, body_vertices, sizeof(body_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlBody.nid, body_normals, sizeof(body_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlBody.iid, body_indices, sizeof(body_indices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlBodyColors2, body_colors2, sizeof(body_colors2), GL_STATIC_DRAW);
    bc1 = 0.906f, bc2 = 0.f, bc3 = 0.f;
    sc1 = 0.49f, sc2 = 0.0824f, sc3 = 0.0824f;
    esBind(GL_ARRAY_BUFFER, &mdlBody.cid, body_colors, sizeof(body_colors), GL_STATIC_DRAW);
    
    // ***** BIND WINDOWS *****
    esBind(GL_ARRAY_BUFFER, &mdlWindows.vid, windows_vertices, sizeof(windows_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlWindows.nid, windows_normals, sizeof(windows_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlWindows.iid, windows_indices, sizeof(windows_indices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlWindows.cid, windows_colors, sizeof(windows_colors), GL_STATIC_DRAW);

    // ***** BIND WHEEL *****
    esBind(GL_ARRAY_BUFFER, &mdlWheel.vid, wheel_vertices, sizeof(wheel_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlWheel.nid, wheel_normals, sizeof(wheel_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlWheel.iid, wheel_indices, sizeof(wheel_indices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlWheel.cid, wheel_colors, sizeof(wheel_colors), GL_STATIC_DRAW);

    // ***** BIND PURPLE CUBE *****
    esBind(GL_ARRAY_BUFFER, &mdlPurpleCube.vid, purplecube_vertices, sizeof(purplecube_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPurpleCube.nid, purplecube_normals, sizeof(purplecube_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlPurpleCube.iid, purplecube_indices, sizeof(purplecube_indices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPurpleCube.cid, purplecube_colors, sizeof(purplecube_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlBlueCubeColors, bluecube_colors, sizeof(bluecube_colors), GL_STATIC_DRAW);

    // ***** BIND PORYGON *****
    esBind(GL_ARRAY_BUFFER, &mdlPorygon.vid, porygon_vertices, sizeof(porygon_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPorygon.nid, porygon_normals, sizeof(porygon_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlPorygon.iid, porygon_indices, sizeof(porygon_indices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPorygon.cid, porygon_colors, sizeof(porygon_colors), GL_STATIC_DRAW);

    // ***** BIND DNA *****
    esBind(GL_ARRAY_BUFFER, &mdlDNA.vid, dna_vertices, sizeof(dna_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlDNA.nid, dna_normals, sizeof(dna_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlDNA.iid, dna_indices, sizeof(dna_indices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlDNA.cid, dna_colors, sizeof(dna_colors), GL_STATIC_DRAW);

//*************************************
// compile & link shader programs
//*************************************
    makeLambert3();

//*************************************
// configure render options
//*************************************
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);
    
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.13, 0.13, 0.13, 0.0);

    shadeLambert3(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    window_size_callback(window, winw, winh);

//*************************************
// execute update / render loop
//*************************************

    // init
#ifndef WEB
    loadConfig(0);
#endif
    newGame(NEWGAME_SEED);
    t = glfwGetTime();
    lfct = t;

#ifdef WEB
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_FALSE, emscripten_resize_event);
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while(!glfwWindowShouldClose(window)){main_loop();}
#endif

    // end
    timeTaken(0);
    char strts[16];
    timestamp(&strts[0]);
    printf("[%s] Game End.\n", strts);
    printf("[%s] Porygon Collected: %u\n", strts, cp);
    printf("[%s] Time-Taken: %s or %g Seconds\n\n", strts, tts, t-st);

    // done
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
    return 0;
}
