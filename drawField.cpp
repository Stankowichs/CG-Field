#include <GL/glut.h>
#include <cmath>

#define PI 3.14159265358979323846

// Dimensões da janela
const int WINDOW_WIDTH  = 1024;
const int WINDOW_HEIGHT = 768;

// Dimensões do campo (coordenadas OpenGL)
const float FIELD_LEFT   = -8.5f;
const float FIELD_RIGHT  =  8.5f;
const float FIELD_TOP    =  5.5f;
const float FIELD_BOTTOM = -5.5f;
const float FIELD_W      = FIELD_RIGHT - FIELD_LEFT;   // 17.0
const float FIELD_H      = FIELD_TOP   - FIELD_BOTTOM; // 11.0

// Gol
const float GOAL_WIDTH   = 1.8f;   // meia-largura (±)
const float GOAL_DEPTH   = 0.4f;

// Bola
const float BALL_RADIUS  = 0.18f;
const float BALL_SPEED   = 0.06f;
const float BALL_TURBO   = 0.12f;

// Jogadores
const float PLAYER_R     = 0.28f;  // raio do círculo do jogador
const int   NUM_PLAYERS  = 4;      // por time (excluindo goleiro)

// Power-up
const float STAR_RADIUS  = 0.22f;
const float TURBO_TIME   = 5.0f;   // segundos

// Círculo preenchido
void drawCircle(float cx, float cy, float r, int segments=32) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for(int i = 0; i <= segments; i++) {
        float a = 2*PI * i / segments;
        glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
    }
    glEnd();
}

// Arco (linha)
void drawCircleOutline(float cx, float cy, float r, int segments=64) {
    glBegin(GL_LINE_LOOP);
    for(int i = 0; i < segments; i++) {
        float a = 2*PI * i / segments;
        glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
    }
    glEnd();
}

// Semicírculo preenchido (lado: -1=esquerdo, 1=direito)
void drawSemiCircle(float cx, float cy, float r, int side, int segments=32) {
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

// Retângulo preenchido
void drawRect(float x1, float y1, float x2, float y2) {
    glBegin(GL_QUADS);
    glVertex2f(x1,y1); glVertex2f(x2,y1);
    glVertex2f(x2,y2); glVertex2f(x1,y2);
    glEnd();
}


void drawField() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Fundo verde (grama) com listras
    for(int i = 0; i < 17; i++) {
        float x1 = FIELD_LEFT  + i * (FIELD_W/17.0f);
        float x2 = x1 + FIELD_W/17.0f;
        if(i % 2 == 0) glColor3f(0.18f, 0.55f, 0.18f);
        else           glColor3f(0.15f, 0.48f, 0.15f);
        drawRect(x1, FIELD_BOTTOM, x2, FIELD_TOP);
    }

    // Linhas do campo
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);

    // Borda externa
    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_LEFT,  FIELD_BOTTOM);
    glVertex2f(FIELD_RIGHT, FIELD_BOTTOM);
    glVertex2f(FIELD_RIGHT, FIELD_TOP);
    glVertex2f(FIELD_LEFT,  FIELD_TOP);
    glEnd();

    // Linha do meio
    glBegin(GL_LINES);
    glVertex2f(0, FIELD_BOTTOM);
    glVertex2f(0, FIELD_TOP);
    glEnd();

    // Círculo central
    glLineWidth(2.0f);
    drawCircleOutline(0, 0, 1.5f);

    // Ponto central
    glColor3f(1,1,1);
    drawCircle(0, 0, 0.05f);

    // Área grande - Esquerda
    float bigBoxW = 2.5f, bigBoxH = 3.2f;
    glColor3f(1,1,1);
    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_LEFT,           -bigBoxH/2);
    glVertex2f(FIELD_LEFT+bigBoxW,   -bigBoxH/2);
    glVertex2f(FIELD_LEFT+bigBoxW,    bigBoxH/2);
    glVertex2f(FIELD_LEFT,            bigBoxH/2);
    glEnd();

    // Área grande - Direita
    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_RIGHT,           -bigBoxH/2);
    glVertex2f(FIELD_RIGHT-bigBoxW,   -bigBoxH/2);
    glVertex2f(FIELD_RIGHT-bigBoxW,    bigBoxH/2);
    glVertex2f(FIELD_RIGHT,            bigBoxH/2);
    glEnd();

    // Área pequena - Esquerda
    float sBoxW = 1.0f, sBoxH = 1.6f;
    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_LEFT,          -sBoxH/2);
    glVertex2f(FIELD_LEFT+sBoxW,    -sBoxH/2);
    glVertex2f(FIELD_LEFT+sBoxW,     sBoxH/2);
    glVertex2f(FIELD_LEFT,           sBoxH/2);
    glEnd();

    // Área pequena - Direita
    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_RIGHT,          -sBoxH/2);
    glVertex2f(FIELD_RIGHT-sBoxW,    -sBoxH/2);
    glVertex2f(FIELD_RIGHT-sBoxW,     sBoxH/2);
    glVertex2f(FIELD_RIGHT,           sBoxH/2);
    glEnd();

    // Pontos de pênalti
    glColor3f(1,1,1);
    drawCircle(FIELD_LEFT  + 2.0f, 0, 0.05f);
    drawCircle(FIELD_RIGHT - 2.0f, 0, 0.05f);

    // Arco de pênalti (esquerdo)
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    for(int i = -30; i <= 30; i++) {
        float a = (float)i * PI / 180.0f;
        float px = (FIELD_LEFT + 2.0f) + cosf(a)*1.2f;
        float py = sinf(a)*1.2f;
        if(px > FIELD_LEFT + bigBoxW)
            glVertex2f(px, py);
    }
    glEnd();

    // Arco de pênalti (direito)
    glBegin(GL_LINE_STRIP);
    for(int i = 150; i <= 210; i++) {
        float a = (float)i * PI / 180.0f;
        float px = (FIELD_RIGHT - 2.0f) + cosf(a)*1.2f;
        float py = sinf(a)*1.2f;
        if(px < FIELD_RIGHT - bigBoxW)
            glVertex2f(px, py);
    }
    glEnd();

    // -------- Gols (redes) --------
    // Gol Esquerdo
    glColor3f(0.9f, 0.9f, 0.9f);
    glLineWidth(2.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_LEFT,             -GOAL_WIDTH);
    glVertex2f(FIELD_LEFT - GOAL_DEPTH,-GOAL_WIDTH);
    glVertex2f(FIELD_LEFT - GOAL_DEPTH, GOAL_WIDTH);
    glVertex2f(FIELD_LEFT,              GOAL_WIDTH);
    glEnd();

    // Rede do gol esquerdo (linhas horizontais e verticais)
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

    // Gol Direito
    glColor3f(0.9f, 0.9f, 0.9f);
    glLineWidth(2.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(FIELD_RIGHT,             -GOAL_WIDTH);
    glVertex2f(FIELD_RIGHT + GOAL_DEPTH,-GOAL_WIDTH);
    glVertex2f(FIELD_RIGHT + GOAL_DEPTH, GOAL_WIDTH);
    glVertex2f(FIELD_RIGHT,              GOAL_WIDTH);
    glEnd();

    // Rede do gol direito
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

void init() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-8.0f, 8.0f, -5.5f, 5.5f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void display() {
    drawField();
    glutSwapBuffers();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1200, 800);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Soccer Field");
    
    init();
    
    glutDisplayFunc(display);
    glutMainLoop();
    
    return 0;
}