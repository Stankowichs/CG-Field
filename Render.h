#ifndef RENDER_H
#define RENDER_H

#include "Game.h"

void drawCircle(float cx, float cy, float r, int segments=32);
void drawCircleOutline(float cx, float cy, float r, int segments=64);
void drawSemiCircle(float cx, float cy, float r, int side, int segments=32);
void drawRect(float x1, float y1, float x2, float y2);
void drawText(float x, float y, const char* str, void* font=GLUT_BITMAP_HELVETICA_18);
void drawTextLarge(float x, float y, const char* str);

void drawFanSimple(float x, float y, int tipo, float s);
void drawCrowd();
void createCrowd();
void drawStairAisle(float x, float w);
void drawShadow(float y);

void drawField();
void drawBall();
void drawPlayer(const Player& p);
void drawStar();
void drawScoreboard();
void drawGoalAnimation();
void drawStands();

#endif
