#include "Render.h"

// Módulo de desenho do jogo.
// Aqui ficam tanto as primitivas gráficas quanto os elementos visuais
// completos: campo, torcida, jogadores, placar e animações.

// Desenha um círculo preenchido usando um leque de triângulos.
void drawCircle(float cx, float cy, float r, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for(int i = 0; i <= segments; i++) {
        float a = 2*PI * i / segments;
        glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
    }
    glEnd();
}

// Desenha apenas o contorno de um círculo.
void drawCircleOutline(float cx, float cy, float r, int segments) {
    glBegin(GL_LINE_LOOP);
    for(int i = 0; i < segments; i++) {
        float a = 2*PI * i / segments;
        glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
    }
    glEnd();
}

// Desenha metade de um círculo. É usado para detalhes como cabelo.
void drawSemiCircle(float cx, float cy, float r, int side, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    float start = (side > 0) ? -PI/2 : PI/2;
    float end   = (side > 0) ?  PI/2 : 3*PI/2;
    for(int i = 0; i <= segments; i++) {
        float a = start + (end-start) * i / segments;
        glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
    }
    glEnd();
}

// Retângulo preenchido, uma das primitivas mais reutilizadas do projeto.
void drawRect(float x1, float y1, float x2, float y2) {
    glBegin(GL_QUADS);
    glVertex2f(x1,y1); glVertex2f(x2,y1);
    glVertex2f(x2,y2); glVertex2f(x1,y2);
    glEnd();
}

// Representação simples de um torcedor para compor a arquibancada.
void drawFanSimple(float x, float y, int tipo, float s) {
    if (tipo == 0) glColor3f(1.0f, 1.0f, 0.0f);
    else if (tipo == 1) glColor3f(0.0f, 0.3f, 1.0f);
    else glColor3f(0.0f, 0.7f, 0.0f);

    glBegin(GL_POLYGON);
        glVertex2f(x - 0.11f*s, y - 0.12f*s);
        glVertex2f(x + 0.11f*s, y - 0.12f*s);
        glVertex2f(x + 0.10f*s, y + 0.02f*s);
        glVertex2f(x + 0.06f*s, y + 0.10f*s);
        glVertex2f(x - 0.06f*s, y + 0.10f*s);
        glVertex2f(x - 0.10f*s, y + 0.02f*s);
    glEnd();

    glColor3f(0.95f, 0.78f, 0.60f);
    drawCircle(x, y + 0.16f*s, 0.06f*s, 12);

    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x - 0.11f*s, y - 0.12f*s);
        glVertex2f(x + 0.11f*s, y - 0.12f*s);
        glVertex2f(x + 0.10f*s, y + 0.02f*s);
        glVertex2f(x + 0.06f*s, y + 0.10f*s);
        glVertex2f(x - 0.06f*s, y + 0.10f*s);
        glVertex2f(x - 0.10f*s, y + 0.02f*s);
    glEnd();

    drawCircleOutline(x, y + 0.16f*s, 0.06f*s, 12);
}

// Percorre a lista de torcedores gerada em createCrowd e desenha cada um.
void drawCrowd() {
    for (int i = (int)crowd.size() - 1; i >= 0; i--) {
        drawFanSimple(crowd[i].x, crowd[i].y, crowd[i].tipo, crowd[i].escala);
    }
}

// Gera a torcida em posições fixas, deixando corredores livres
// para escadas e intervalos visuais na arquibancada.
void createCrowd() {
    crowd.clear();

    for (float y = 6.7f; y <= 8.1f; y += 0.22f) {
        for (float x = -8.0f; x <= 8.09f; x += 0.15f) {
            bool escada =
                (x >= -2.88f && x <= -2.4f) ||
                (x >= 2.78f && x <= 3.25f);

            bool teto =
                (y >= 7.1f && y <= 7.3f) ||
                (y >= 7.7f && y < 8.1f);

            if(escada || teto) continue;

            Fan f;
            f.x = x;
            f.y = y;
            f.tipo = rand() % 3;
            f.escala = 1.0f;
            crowd.push_back(f);
        }
    }

    for (float y = 8.1f; y <= 10.0f; y += 0.22f) {
        for (float x = -8.0f; x <= 8.09f; x += 0.15f) {
            bool escada =
                (x >= -2.88f && x <= -2.4f) ||
                (x >= 2.78f && x <= 3.25f);

            bool teto =
                (y >= 8.5f && y <= 8.6f) ||
                (y >= 9.2f && y <= 9.4f);

            if(escada || teto) continue;

            Fan f;
            f.x = x;
            f.y = y;
            f.tipo = rand() % 3;
            f.escala = 1.0f;
            crowd.push_back(f);
        }
    }
}

// Desenha uma faixa vertical que simula escada/corredor.
void drawStairAisle(float x, float w) {
    glColor3f(0.78f, 0.78f, 0.78f);
    drawRect(x, 6.5f, x + w, 10.0f);

    glColor3f(0.3f, 0.3f, 0.3f);
    glLineWidth(1.5f);
    for(float y = 6.6f; y <= 10.0f; y+= 0.15f){
        glBegin(GL_LINES);
            glVertex2f(x, y);
            glVertex2f(x + w + 0.01f, y);
        glEnd();
    }
    glColor3f(0.55f, 0.55f, 0.55f);
    glLineWidth(1.0f);
    for(float y = 6.57f; y <= 10.0f; y+= 0.15f){
        glBegin(GL_LINES);
            glVertex2f(x, y);
            glVertex2f(x + w + 0.01f, y);
        glEnd();
    }
}

// Pequena sombra horizontal para dar profundidade entre setores da arquibancada.
void drawShadow(float y) {
    glColor3f(0.62f, 0.62f, 0.62f);
    drawRect(-8.52f, y, 8.52f, y + 0.08f);

    glColor3f(0.38f, 0.38f, 0.38f);
    drawRect(-8.52f, y - 0.08f, 8.52f, y);
}

// Texto bitmap básico do GLUT para HUD e labels.
void drawText(float x, float y, const char* str, void* font) {
    glRasterPos2f(x, y);
    for(const char* c = str; *c; c++) {
        glutBitmapCharacter(font, *c);
    }
}

// Versão maior do texto para destacar placar e animações.
void drawTextLarge(float x, float y, const char* str) {
    drawText(x, y, str, GLUT_BITMAP_TIMES_ROMAN_24);
}

// Desenha o campo completo: gramado, linhas, áreas, arcos, escanteios e gols.
void drawField() {
    for(int i = 0; i < 17; i++) {
        float x1 = FIELD_LEFT  + i * (FIELD_W/17.0f);
        float x2 = x1 + FIELD_W/17.0f;
        if(i % 2 == 0) glColor3f(0.18f, 0.55f, 0.18f);
        else           glColor3f(0.15f, 0.48f, 0.15f);
        drawRect(x1, FIELD_BOTTOM, x2, FIELD_TOP);
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);

    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_LEFT,  FIELD_BOTTOM);
    glVertex2f(FIELD_RIGHT, FIELD_BOTTOM);
    glVertex2f(FIELD_RIGHT, FIELD_TOP);
    glVertex2f(FIELD_LEFT,  FIELD_TOP);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(0, FIELD_BOTTOM);
    glVertex2f(0, FIELD_TOP);
    glEnd();

    glLineWidth(2.0f);
    drawCircleOutline(0, 0, 1.5f);

    glColor3f(1,1,1);
    drawCircle(0, 0, 0.05f);

    float bigBoxW = 3.5f, bigBoxH = 7.2f;
    glColor3f(1,1,1);
    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_LEFT,           -bigBoxH/2);
    glVertex2f(FIELD_LEFT+bigBoxW,   -bigBoxH/2);
    glVertex2f(FIELD_LEFT+bigBoxW,    bigBoxH/2);
    glVertex2f(FIELD_LEFT,            bigBoxH/2);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_RIGHT,           -bigBoxH/2);
    glVertex2f(FIELD_RIGHT-bigBoxW,   -bigBoxH/2);
    glVertex2f(FIELD_RIGHT-bigBoxW,    bigBoxH/2);
    glVertex2f(FIELD_RIGHT,            bigBoxH/2);
    glEnd();

    float sBoxW = 1.0f, sBoxH = 4.6f;
    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_LEFT,          -sBoxH/2);
    glVertex2f(FIELD_LEFT+sBoxW,    -sBoxH/2);
    glVertex2f(FIELD_LEFT+sBoxW,     sBoxH/2);
    glVertex2f(FIELD_LEFT,           sBoxH/2);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_RIGHT,          -sBoxH/2);
    glVertex2f(FIELD_RIGHT-sBoxW,    -sBoxH/2);
    glVertex2f(FIELD_RIGHT-sBoxW,     sBoxH/2);
    glVertex2f(FIELD_RIGHT,           sBoxH/2);
    glEnd();

    glColor3f(1,1,1);
    drawCircle(FIELD_LEFT  + 2.75f, 0, 0.05f);
    drawCircle(FIELD_RIGHT - 2.75f, 0, 0.05f);

    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    for(int i = -70; i <= 130; i++) {
        float a = (float)i * PI / 180.0f;
        float px = (FIELD_LEFT + 3.0f) + cosf(a)*1.2f;
        float py = sinf(a)*1.2f;
        if(px > FIELD_LEFT + bigBoxW) glVertex2f(px, py);
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    for(int i = 60; i <= 300; i++) {
        float a = (float)i * PI / 180.0f;
        float px = (FIELD_RIGHT - 3.0f) + cosf(a)*1.2f;
        float py = sinf(a)*1.2f;
        if(px < FIELD_RIGHT - bigBoxW) glVertex2f(px, py);
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    for(int i = 180; i <= 270; i++) {
        float a = (float)i * PI / 180.0f;
        float px = (FIELD_RIGHT) + cosf(a)*0.7f;
        float py = (FIELD_TOP) + sinf(a)*0.7f;
        glVertex2f(px, py);
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    for(int i = 90; i <= 180; i++) {
        float a = (float)i * PI / 180.0f;
        float px = (FIELD_RIGHT) + cosf(a)*0.7f;
        float py = (FIELD_BOTTOM) + sinf(a)*0.7f;
        glVertex2f(px, py);
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    for(int i = 0; i <= 90; i++) {
        float a = (float)i * PI / 180.0f;
        float px = (FIELD_LEFT) + cosf(a)*0.7f;
        float py = (FIELD_BOTTOM) + sinf(a)*0.7f;
        glVertex2f(px, py);
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    for(int i = 270; i <= 360; i++) {
        float a = (float)i * PI / 180.0f;
        float px = (FIELD_LEFT) + cosf(a)*0.7f;
        float py = (FIELD_TOP) + sinf(a)*0.7f;
        glVertex2f(px, py);
    }
    glEnd();

    glColor3f(0.9f, 0.9f, 0.9f);
    glLineWidth(2.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_LEFT,             -GOAL_WIDTH);
    glVertex2f(FIELD_LEFT - GOAL_DEPTH,-GOAL_WIDTH);
    glVertex2f(FIELD_LEFT - GOAL_DEPTH, GOAL_WIDTH);
    glVertex2f(FIELD_LEFT,              GOAL_WIDTH);
    glEnd();

    glColor3f(0.85f, 0.85f, 0.85f);
    glLineWidth(0.8f);
    for(int i = 1; i <= 4; i++) {
        float y = -GOAL_WIDTH + i * (2*GOAL_WIDTH/5);
        glBegin(GL_LINES);
        glVertex2f(FIELD_LEFT, y);
        glVertex2f(FIELD_LEFT - GOAL_DEPTH, y);
        glEnd();
    }
    for(int i = 1; i <= 3; i++) {
        float x = FIELD_LEFT - i * (GOAL_DEPTH/4);
        glBegin(GL_LINES);
        glVertex2f(x, -GOAL_WIDTH);
        glVertex2f(x,  GOAL_WIDTH);
        glEnd();
    }

    glColor3f(0.9f, 0.9f, 0.9f);
    glLineWidth(2.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_RIGHT,             -GOAL_WIDTH);
    glVertex2f(FIELD_RIGHT + GOAL_DEPTH,-GOAL_WIDTH);
    glVertex2f(FIELD_RIGHT + GOAL_DEPTH, GOAL_WIDTH);
    glVertex2f(FIELD_RIGHT,              GOAL_WIDTH);
    glEnd();

    glColor3f(0.85f, 0.85f, 0.85f);
    glLineWidth(0.8f);
    for(int i = 1; i <= 4; i++) {
        float y = -GOAL_WIDTH + i * (2*GOAL_WIDTH/5);
        glBegin(GL_LINES);
        glVertex2f(FIELD_RIGHT, y);
        glVertex2f(FIELD_RIGHT + GOAL_DEPTH, y);
        glEnd();
    }
    for(int i = 1; i <= 3; i++) {
        float x = FIELD_RIGHT + i * (GOAL_DEPTH/4);
        glBegin(GL_LINES);
        glVertex2f(x, -GOAL_WIDTH);
        glVertex2f(x,  GOAL_WIDTH);
        glEnd();
    }

    glLineWidth(1.0f);
}

// Desenha a bola com sombra, manchas clássicas e efeito visual de turbo.
void drawBall() {
    float bx = ball.pos.x;
    float by = ball.pos.y;
    float r  = BALL_RADIUS;

    glPushMatrix();
    glTranslatef(bx, by, 0);
    glRotatef(ball.angle, 0, 0, 1);

    glColor4f(0,0,0,0.25f);
    drawCircle(0.04f, -0.04f, r * 1.1f, 24);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(0, 0, r, 32);

    glColor3f(0.05f, 0.05f, 0.05f);
    float hr = r * 0.42f;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 0);
    for(int i = 0; i <= 6; i++) {
        float a = 2*PI * i / 6 + PI/6;
        glVertex2f(cosf(a)*hr, sinf(a)*hr);
    }
    glEnd();

    for(int k = 0; k < 5; k++) {
        float a  = 2*PI * k / 5;
        float px = cosf(a) * r * 0.65f;
        float py = sinf(a) * r * 0.65f;
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(px, py);
        for(int i = 0; i <= 5; i++) {
            float aa = 2*PI * i / 5 + a;
            glVertex2f(px + cosf(aa)*r*0.22f, py + sinf(aa)*r*0.22f);
        }
        glEnd();
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(-r*0.28f, r*0.35f, r*0.18f, 16);

    if(ball.turbo) {
        glLineWidth(2.0f);
        float t = (float)glutGet(GLUT_ELAPSED_TIME) * 0.005f;
        for(int ring = 0; ring < 3; ring++) {
            float alpha = 0.6f - ring * 0.18f;
            glColor4f(1.0f, 0.8f, 0.0f, alpha);
            drawCircleOutline(0, 0, r + 0.06f + ring*0.07f + sinf(t+ring)*0.02f, 24);
        }
        glLineWidth(1.0f);
    }

    glPopMatrix();
}

// Desenha um jogador estilizado com variação de uniforme por time e goleiro.
void drawPlayer(const Player& p) {
    float px = p.pos.x;
    float py = p.pos.y;
    float r  = PLAYER_R;
    glPushMatrix();
    glTranslatef(px, py, 0);

    glColor4f(0,0,0,0.2f);
    drawCircle(0.05f, -r*0.9f, r*0.5f, 16);

    if(p.team == 0) {
        if(p.isGoalie) glColor3f(0.05f, 0.16f, 0.55f);
        else           glColor3f(0.82f, 0.68f, 0.10f);
    } else {
        if(p.isGoalie) glColor3f(0.82f, 0.12f, 0.12f);
        else           glColor3f(0.07f, 0.23f, 0.78f);
    }
    drawCircle(0, 0, r, 24);

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.5f);
    drawCircleOutline(0, 0, r, 24);

    glColor3f(0.95f, 0.78f, 0.60f);
    drawCircle(0, r*0.72f, r*0.38f, 20);

    if(p.team == 0) {
        if(p.isGoalie) glColor3f(0.95f, 0.82f, 0.18f);
        else           glColor3f(0.05f, 0.16f, 0.55f);
    } else {
        if(p.isGoalie) glColor3f(0.82f, 0.12f, 0.12f);
        else           glColor3f(1.0f, 1.0f, 1.0f);
    }
    drawSemiCircle(0, r*0.72f, r*0.38f, 1, 16);

    if(p.isGoalie) {
        glColor4f(1,1,1,0.5f);
        drawCircle(-r*0.1f, r*0.85f, r*0.1f, 10);
    }

    glPopMatrix();
}

// Desenha o power-up estrela com rotação, halo e leve flutuação.
void drawStar() {
    if(!star.active) return;

    float sx = star.pos.x;
    float sy = star.pos.y;
    float r  = STAR_RADIUS;
    float t  = (float)glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    float bounce = sinf(t * 3.0f) * 0.08f;

    glPushMatrix();
    glTranslatef(sx, sy + bounce, 0);
    glRotatef(star.rotAngle, 0, 0, 1);

    float halo = 0.5f + 0.5f * sinf(t * 4.0f);
    glColor4f(1.0f, 0.9f, 0.0f, 0.3f * halo);
    drawCircle(0, 0, r * 1.8f, 24);

    glColor3f(1.0f, 0.85f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 0);
    for(int i = 0; i <= 10; i++) {
        float a    = PI/2 + 2*PI * i / 10;
        float rad  = (i % 2 == 0) ? r : r*0.4f;
        glVertex2f(cosf(a)*rad, sinf(a)*rad);
    }
    glEnd();

    glColor3f(1.0f, 0.5f, 0.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    for(int i = 0; i < 10; i++) {
        float a   = PI/2 + 2*PI * i / 10;
        float rad = (i % 2 == 0) ? r : r*0.4f;
        glVertex2f(cosf(a)*rad, sinf(a)*rad);
    }
    glEnd();

    glColor3f(1.0f, 1.0f, 0.8f);
    drawCircle(-r*0.15f, r*0.25f, r*0.12f, 8);

    glLineWidth(1.0f);
    glPopMatrix();
}

// HUD inferior com placar, nomes dos times, indicador de turbo e controles.
void drawScoreboard() {
    float sbX  = -2.2f, sbY = -5.6f;
    float sbW  =  4.4f, sbH = 1.0f;

    glColor4f(0,0,0,0.4f);
    drawRect(sbX+0.05f, sbY-sbH-0.05f, sbX+sbW+0.05f, sbY-0.05f);

    glColor3f(0.05f, 0.05f, 0.15f);
    drawRect(sbX, sbY-sbH, sbX+sbW, sbY);

    glColor3f(0.85f, 0.65f, 0.0f);
    drawRect(sbX, sbY-0.18f, sbX+sbW, sbY);

    glColor3f(0.85f, 0.65f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(sbX, sbY-sbH); glVertex2f(sbX+sbW, sbY-sbH);
    glVertex2f(sbX+sbW, sbY); glVertex2f(sbX, sbY);
    glEnd();
    glLineWidth(1.0f);

    glColor3f(0.05f, 0.05f, 0.15f);
    drawText(sbX+0.8f, sbY-0.16f, "FIFA WORLD CUP 2026", GLUT_BITMAP_HELVETICA_12);

    glColor3f(0.5f, 0.7f, 1.0f);
    drawText(sbX+0.15f, sbY-0.52f, "BRASIL", GLUT_BITMAP_HELVETICA_18);

    glColor3f(1.0f, 0.5f, 0.5f);
    drawText(sbX+2.8f, sbY-0.52f, "JAPAO", GLUT_BITMAP_HELVETICA_18);

    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_LINES);
    glVertex2f(0, sbY-sbH); glVertex2f(0, sbY);
    glEnd();

    char buf[8];
    glColor3f(1.0f, 1.0f, 0.3f);
    sprintf(buf, "%d", score.home);
    drawTextLarge(-0.55f, sbY-0.82f, buf);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawTextLarge(-0.12f, sbY-0.82f, ":");
    glColor3f(1.0f, 1.0f, 0.3f);
    sprintf(buf, "%d", score.away);
    drawTextLarge(0.22f, sbY-0.82f, buf);

    if(ball.turbo) {
        float tx = 3.0f;
        glColor3f(1.0f, 0.8f, 0.0f);
        drawText(tx, sbY-0.4f, "TURBO!", GLUT_BITMAP_HELVETICA_18);
        char tt[32];
        sprintf(tt, "%.1fs", ball.turboTimer);
        glColor3f(1.0f, 1.0f, 0.4f);
        drawText(tx+0.05f, sbY-0.75f, tt, GLUT_BITMAP_HELVETICA_12);
    }

    glColor3f(0.6f, 0.6f, 0.6f);
    drawText(-8.4f, FIELD_BOTTOM - 0.4f,
             "W/A/S/D ou SETAS: Mover | ESPACO: Turbo | R: Reset | ESC: Sair",
             GLUT_BITMAP_HELVETICA_10);
}

// Animação temporária que toma a tela após um gol.
void drawGoalAnimation() {
    if(!goalAnimation) return;

    const float fadeIn  = 1.5f;
    const float fadeOut = GOAL_FREEZE_TIME;
    const float hold    = 3.0f;
    float alpha;
    if(goalAnimTimer <= fadeIn) {
        alpha = goalAnimTimer / fadeIn;
    } else if(goalAnimTimer <= hold) {
        alpha = 1.0f;
    } else if(goalAnimTimer <= fadeOut) {
        alpha = 1.0f - (goalAnimTimer - hold) / (fadeOut - hold);
    } else {
        alpha = 0.0f;
    }

    float flash = (goalAnimTimer < 0.3f) ? 0.5f * (1.0f - goalAnimTimer/0.3f) : 0.0f;
    glColor4f(1.0f, 1.0f, 0.0f, flash);
    drawRect(FIELD_LEFT, FIELD_BOTTOM, FIELD_RIGHT, FIELD_TOP);

    float scale = 1.0f + 0.3f * sinf(goalAnimTimer * 8.0f);
    glPushMatrix();
    glTranslatef(-1.5f * scale, 0.3f, 0);
    glScalef(scale, scale, 1);
    glColor4f(1.0f, 0.9f, 0.0f, alpha);
    drawTextLarge(-0.5f, 0, "G O O O O L !");
    if(lastGoalTeam == 0) {
        glColor4f(0.5f, 0.7f, 1.0f, alpha);
        drawText(-0.3f, -0.5f, "BRASIL MARCA!", GLUT_BITMAP_HELVETICA_18);
    } else {
        glColor4f(1.0f, 0.5f, 0.5f, alpha);
        drawText(-0.35f, -0.5f, "GOL DA JAPAO!", GLUT_BITMAP_HELVETICA_18);
    }
    glPopMatrix();

    if(lastGoalTeam == 0) {
        for(int f = 0; f < 8; f++) {
            float a = 2*PI * f / 8 + goalAnimTimer * 2.0f;
            float fr = 1.5f + 0.5f * sinf(goalAnimTimer * 5.0f + f);
            float fx = cosf(a) * fr;
            float fy = sinf(a) * fr * 0.6f + 2.5f;
            glColor4f(
                0.5f + 0.5f*sinf(f*1.3f),
                0.5f + 0.5f*cosf(f*0.9f),
                0.3f, alpha * 0.9f);
            drawCircle(fx, fy, 0.12f, 8);
        }
    }
}

// Desenha a parte superior do estádio: faixa temática, degraus e torcida.
void drawStands() {
    glColor3f(0.0f, 0.8f, 0.0f);
    drawRect(-8.52f, 5.55f, 8.52f, 6.55f);
    glColor3f(0.9f, 1.0f, 0.0f);
    drawRect(-8.52f, 5.65f, 8.52f, 5.80f);
    drawRect(-8.52f, 5.95f, 8.52f, 6.1f);
    drawRect(-8.52f, 6.25f, 8.52f, 6.4f);
    glColor3f(0.0f, 0.0f, 0.9f);
    drawText(-8.2f, 5.9f, "MOVIMENTO VERDE AMARELO  MOVIMENTO VERDE AMARELO  MOVIMENTO VERDE AMARELO", GLUT_BITMAP_HELVETICA_18);

    glColor3f(0.55f, 0.55f, 0.55f);
    drawRect(-8.52f, 6.5f, 8.52f, 10.0f);

    drawStairAisle(-8.52f, 0.35f);
    drawStairAisle(-2.8f, 0.35f);
    drawStairAisle( 2.8f, 0.35f);
    drawStairAisle( 8.16f, 0.35f);

    drawShadow(7.2f);
    drawShadow(7.9f);
    drawShadow(8.6f);
    drawShadow(9.3f);

    drawCrowd();

    glColor3f(0.0f, 0.0f, 0.0f);
    drawRect(-8.52f, 7.2f, 8.52f, 7.24f);
    drawRect(-8.52f, 7.9f, 8.52f, 7.95f);
    drawRect(-8.52f, 8.6f, 8.52f, 8.64f);
    drawRect(-8.52f, 9.3f, 8.52f, 9.34f);
}
