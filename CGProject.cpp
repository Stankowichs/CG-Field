/*
 * =====================================================================
 *   COPA DO MUNDO 2026 - Projeto de Computação Gráfica
 *   Disciplina: Computação Gráfica
 *   Tecnologia: C++ + OpenGL (GL/glut.h)
 * =====================================================================
 *
 *  CONTROLES:
 *   W/A/S/D ou Setas  - Mover a bola
 *   ESPAÇO            - Chute forte (turbo)
 *   R                 - Reiniciar posição da bola
 *   ESC               - Sair
 *
 *  REGRAS:
 *   - Marque gol passando a bola pela linha do gol adversário
 *   - Jogadores adversários perseguem a bola automaticamente
 *   - Jogadores aliados se posicionam para apoiar
 *   - Power-up estrela: bola fica turbo por 5 segundos
 *   - Placar atualizado automaticamente
 * =====================================================================
 */

#include <GL/glut.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>

// =====================================================================
// Constantes
// =====================================================================
#define PI 3.14159265358979323846

// Dimensões da janela
const int WINDOW_WIDTH  = 1024;
const int WINDOW_HEIGHT = 900;

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
const float BALL_SPEED   = 0.08f;
const float BALL_TURBO   = 0.12f;

// Jogadores
const float PLAYER_R     = 0.28f;  // raio do círculo do jogador
const int   NUM_PLAYERS  = 4;      // por time (excluindo goleiro)

// Power-up
const float STAR_RADIUS  = 0.22f;
const float TURBO_TIME   = 5.0f;   // segundos
const float STAR_RESET_DELAY = 2.0f;

// =====================================================================
// Estruturas
// =====================================================================
struct Vec2 {
    float x, y;
    Vec2(float x=0, float y=0): x(x), y(y){}
    Vec2 operator+(const Vec2& o) const { return {x+o.x, y+o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x-o.x, y-o.y}; }
    Vec2 operator*(float s)       const { return {x*s,   y*s};   }
    float len() const { return sqrtf(x*x + y*y); }
    Vec2 norm() const {
        float l = len();
        return l > 0.0001f ? Vec2(x/l, y/l) : Vec2(0,0);
    }
};

struct Ball {
    Vec2  pos;
    Vec2  vel;
    float angle;       // rotação visual
    bool  turbo;
    float turboTimer;
};

struct Player {
    Vec2  pos;
    Vec2  vel;
    int   team;        // 0 = azul (jogador), 1 = vermelho (IA)
    float animTimer;   // para animação de pernas
    bool  isGoalie;
};

struct Star {           // power-up
    Vec2  pos;
    bool  active;
    float rotAngle;
    float respawnTimer;
};

struct Score {
    int home, away;
};

struct Fan {
    float x, y;
    int tipo;
    float escala;
};

// =====================================================================
// Estado global
// =====================================================================
Ball   ball;
Player players[NUM_PLAYERS*2 + 2]; // +2 goleiros
Star   star;
Score  score;
std::vector<Fan> crowd;

void createCrowd();
void drawCrowd();
void drawFanSimple(float x, float y, int tipo, float s);

bool keys[256];
bool specialKeys[256];

float deltaTime = 0.016f;
int   lastTime  = 0;

bool goalAnimation   = false;
float goalAnimTimer  = 0.0f;
int   lastGoalTeam   = -1;   // 0=home, 1=away

float fieldGrassAnim = 0.0f; // animação sutil da grama
float cornerTrapTimer = 0.0f;

// =====================================================================
// Utilitários
// =====================================================================
float randf(float a, float b) {
    return a + (b-a) * (rand() / (float)RAND_MAX);
}

float dist(Vec2 a, Vec2 b) {
    return (a-b).len();
}

void clampPlayerToField(Player& pl) {
    if(pl.pos.x < FIELD_LEFT  + PLAYER_R) pl.pos.x = FIELD_LEFT  + PLAYER_R;
    if(pl.pos.x > FIELD_RIGHT - PLAYER_R) pl.pos.x = FIELD_RIGHT - PLAYER_R;
    if(pl.pos.y < FIELD_BOTTOM + PLAYER_R) pl.pos.y = FIELD_BOTTOM + PLAYER_R;
    if(pl.pos.y > FIELD_TOP   - PLAYER_R) pl.pos.y = FIELD_TOP   - PLAYER_R;
}

Vec2 clampTargetFromEdges(Vec2 target, float margin) {
    if(target.x < FIELD_LEFT + margin)   target.x = FIELD_LEFT + margin;
    if(target.x > FIELD_RIGHT - margin)  target.x = FIELD_RIGHT - margin;
    if(target.y < FIELD_BOTTOM + margin) target.y = FIELD_BOTTOM + margin;
    if(target.y > FIELD_TOP - margin)    target.y = FIELD_TOP - margin;
    return target;
}

bool isBallNearCorner(float radius) {
    Vec2 corners[4] = {
        Vec2(FIELD_LEFT,  FIELD_BOTTOM),
        Vec2(FIELD_LEFT,  FIELD_TOP),
        Vec2(FIELD_RIGHT, FIELD_BOTTOM),
        Vec2(FIELD_RIGHT, FIELD_TOP)
    };

    for(int i = 0; i < 4; i++) {
        if(dist(ball.pos, corners[i]) <= radius) return true;
    }
    return false;
}

void dispersePlayersFromBall(float dt) {
    const float affectRadius = 1.9f;
    const float pushSpeed = 0.12f;
    Vec2 fieldCenter(0.0f, 0.0f);
    int totalPlayers = NUM_PLAYERS * 2 + 2;

    for(int i = 0; i < totalPlayers; i++) {
        Player& pl = players[i];
        if(pl.isGoalie) continue;

        float d = dist(pl.pos, ball.pos);
        if(d > affectRadius) continue;

        Vec2 pushDir = (pl.pos - ball.pos).norm();
        if(pushDir.len() < 0.001f) pushDir = (fieldCenter - ball.pos).norm();

        pl.pos = pl.pos + pushDir * (pushSpeed * dt / 0.016f);
        clampPlayerToField(pl);
    }
}

void resolvePlayerCollisions() {
    int totalPlayers = NUM_PLAYERS * 2 + 2;
    float minD = PLAYER_R * 2.0f;

    for(int i = 0; i < totalPlayers; i++) {
        for(int j = i + 1; j < totalPlayers; j++) {
            if(players[i].isGoalie || players[j].isGoalie) continue;

            Vec2 delta = players[j].pos - players[i].pos;
            float d = delta.len();
            if(d >= minD) continue;

            Vec2 normal = (d > 0.0001f) ? delta * (1.0f / d) : Vec2(1, 0);
            float overlap = minD - d;
            Vec2 push = normal * (overlap * 0.5f + 0.001f);

            players[i].pos = players[i].pos - push;
            players[j].pos = players[j].pos + push;

            clampPlayerToField(players[i]);
            clampPlayerToField(players[j]);
        }
    }
}

void setColor3f(float r, float g, float b) {
    glColor3f(r, g, b);
}

// =====================================================================
// Inicialização
// =====================================================================
void resetBall() {
    ball.pos      = Vec2(0, 0);
    ball.vel      = Vec2(0, 0);
    ball.angle    = 0;
    ball.turbo    = false;
    ball.turboTimer = 0;
    cornerTrapTimer = 0.0f;
}

void resetPlayers() {
    // Time Azul (controlado pelo jogador - lado esquerdo)
    float blueX[NUM_PLAYERS] = {-5.0f, -3.5f, -3.5f, -1.5f};
    float blueY[NUM_PLAYERS] = { 0.0f,  2.0f, -2.0f,  0.0f};

    for(int i = 0; i < NUM_PLAYERS; i++) {
        players[i].pos      = Vec2(blueX[i], blueY[i]);
        players[i].vel      = Vec2(0,0);
        players[i].team     = 0;
        players[i].animTimer= (float)i * 0.5f;
        players[i].isGoalie = false;
    }
    // Goleiro azul
    players[NUM_PLAYERS].pos      = Vec2(FIELD_LEFT + 0.6f, 0);
    players[NUM_PLAYERS].vel      = Vec2(0,0);
    players[NUM_PLAYERS].team     = 0;
    players[NUM_PLAYERS].animTimer= 0;
    players[NUM_PLAYERS].isGoalie = true;

    // Time Vermelho (IA - lado direito)
    float redX[NUM_PLAYERS] = { 5.0f,  3.5f,  3.5f,  1.5f};
    float redY[NUM_PLAYERS] = { 0.0f,  2.0f, -2.0f,  0.0f};

    for(int i = 0; i < NUM_PLAYERS; i++) {
        players[NUM_PLAYERS+1+i].pos      = Vec2(redX[i], redY[i]);
        players[NUM_PLAYERS+1+i].vel      = Vec2(0,0);
        players[NUM_PLAYERS+1+i].team     = 1;
        players[NUM_PLAYERS+1+i].animTimer= (float)i * 0.5f;
        players[NUM_PLAYERS+1+i].isGoalie = false;
    }
    // Goleiro vermelho
    players[NUM_PLAYERS*2+1].pos      = Vec2(FIELD_RIGHT - 0.6f, 0);
    players[NUM_PLAYERS*2+1].vel      = Vec2(0,0);
    players[NUM_PLAYERS*2+1].team     = 1;
    players[NUM_PLAYERS*2+1].animTimer= 0;
    players[NUM_PLAYERS*2+1].isGoalie = true;
}

void resetStar() {
    star.pos         = Vec2(randf(-3,3), randf(-2,2));
    star.active      = false;
    star.rotAngle    = 0;
    star.respawnTimer= STAR_RESET_DELAY;
}

void initGame() {
    srand((unsigned)time(NULL));
    score = {0, 0};
    resetBall();
    resetPlayers();
    resetStar();
    createCrowd();
    memset(keys, 0, sizeof(keys));
    memset(specialKeys, 0, sizeof(specialKeys));
    goalAnimation = false;
    goalAnimTimer = 0;
}

// =====================================================================
// Desenho - Funções auxiliares
// =====================================================================

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

// Torcedor
void drawFanSimple(float x, float y, int tipo, float s) {
    // corpo / camisa
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

    // cabeça arredondada
    glColor3f(0.95f, 0.78f, 0.60f);
    drawCircle(x, y + 0.16f*s, 0.06f*s, 12);

    // contorno do corpo
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

    // contorno da cabeça
    drawCircleOutline(x, y + 0.16f*s, 0.06f*s, 12);
}

void drawCrowd() {
    for (int i = crowd.size() - 1; i >= 0; i--) {
        drawFanSimple(crowd[i].x, crowd[i].y, crowd[i].tipo, crowd[i].escala);
    }
}

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
            f.x = x;                  // sem aleatoriedade no x
            f.y = y;                  // sem aleatoriedade no y
            f.tipo = rand() % 3;
            f.escala = 1.0f;          // tamanho fixo
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
            f.x = x;                  // sem aleatoriedade no x
            f.y = y;                  // sem aleatoriedade no y
            f.tipo = rand() % 3;
            f.escala = 1.0f;          // tamanho fixo
            crowd.push_back(f);
        }
    }
}

//////////////////////////////////////////
void drawStairAisle(float x, float w) {
    // faixa principal da escada
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

void drawShadow(float y) {
    glColor3f(0.62f, 0.62f, 0.62f);
    drawRect(-8.52f, y, 8.52f, y + 0.08f);

    glColor3f(0.38f, 0.38f, 0.38f);
    drawRect(-8.52f, y - 0.08f, 8.52f, y);
}

// Texto bitmap simples
void drawText(float x, float y, const char* str, void* font=GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for(const char* c = str; *c; c++)
        glutBitmapCharacter(font, *c);
}

void drawTextLarge(float x, float y, const char* str) {
    drawText(x, y, str, GLUT_BITMAP_TIMES_ROMAN_24);
}

// =====================================================================
// Desenho - Campo
// =====================================================================
void drawField() {
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
    float bigBoxW = 3.5f, bigBoxH = 7.2f;
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
    float sBoxW = 1.0f, sBoxH = 4.6f;
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
    drawCircle(FIELD_LEFT  + 2.75f, 0, 0.05f);
    drawCircle(FIELD_RIGHT - 2.75f, 0, 0.05f);

    // Arco de pênalti (esquerdo)
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    for(int i = -70; i <= 130; i++) {
        float a = (float)i * PI / 180.0f; //graus para radianos
        float px = (FIELD_LEFT + 3.0f) + cosf(a)*1.2f;
        float py = sinf(a)*1.2f;
        if(px > FIELD_LEFT + bigBoxW)
            glVertex2f(px, py);
    }
    glEnd();

    // Arco de pênalti (direito)
    glBegin(GL_LINE_STRIP);
    for(int i = 60; i <= 300; i++) {
        float a = (float)i * PI / 180.0f;
        float px = (FIELD_RIGHT - 3.0f) + cosf(a)*1.2f;
        float py = sinf(a)*1.2f;
        if(px < FIELD_RIGHT - bigBoxW)
            glVertex2f(px, py);
    }
    glEnd();

    //Escanteio (direito cima)
    glBegin(GL_LINE_STRIP);
    for(int i = 180; i <= 270; i++) {
        float a = (float)i * PI / 180.0f;
        float px = (FIELD_RIGHT) + cosf(a)*0.7f;
        float py = (FIELD_TOP) + sinf(a)*0.7f;
        glVertex2f(px, py);
    }
    glEnd();

    //Escanteio (direito baixo)
    glBegin(GL_LINE_STRIP);
    for(int i = 90; i <= 180; i++) {
        float a = (float)i * PI / 180.0f;
        float px = (FIELD_RIGHT) + cosf(a)*0.7f;
        float py = (FIELD_BOTTOM) + sinf(a)*0.7f;
        glVertex2f(px, py);
    }
    glEnd();

    //Escanteio (esquerdo baixo)
    glBegin(GL_LINE_STRIP);
    for(int i = 0; i <= 90; i++) {
        float a = (float)i * PI / 180.0f;
        float px = (FIELD_LEFT) + cosf(a)*0.7f;
        float py = (FIELD_BOTTOM) + sinf(a)*0.7f;
        glVertex2f(px, py);
    }
    glEnd();

    //Escanteio (esquerdo cima)
    glBegin(GL_LINE_STRIP);
    for(int i = 270; i <= 360; i++) {
        float a = (float)i * PI / 180.0f;
        float px = (FIELD_LEFT) + cosf(a)*0.7f;
        float py = (FIELD_TOP) + sinf(a)*0.7f;
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

// =====================================================================
// Desenho - Bola
// =====================================================================
void drawBall() {
    float bx = ball.pos.x;
    float by = ball.pos.y;
    float r  = BALL_RADIUS;

    glPushMatrix();
    glTranslatef(bx, by, 0);
    glRotatef(ball.angle, 0, 0, 1);

    // Sombra suave
    glColor4f(0,0,0,0.25f);
    drawCircle(0.04f, -0.04f, r * 1.1f, 24);

    // Base branca
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(0, 0, r, 32);

    // Pentágonos pretos (estilo bola clássica)
    glColor3f(0.05f, 0.05f, 0.05f);
    // Hexágono central
    float hr = r * 0.42f;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 0);
    for(int i = 0; i <= 6; i++) {
        float a = 2*PI * i / 6 + PI/6;
        glVertex2f(cosf(a)*hr, sinf(a)*hr);
    }
    glEnd();
    // Manchas ao redor
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

    // Brilho
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(-r*0.28f, r*0.35f, r*0.18f, 16);

    // Efeito turbo
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

// =====================================================================
// Desenho - Jogador (estilizado, com número)
// =====================================================================
void drawPlayer(const Player& p) {
    float px = p.pos.x;
    float py = p.pos.y;
    float r  = PLAYER_R;
    glPushMatrix();
    glTranslatef(px, py, 0);

    // Sombra
    glColor4f(0,0,0,0.2f);
    drawCircle(0.05f, -r*0.9f, r*0.5f, 16);

    // Corpo (círculo maior - camisa)
    if(p.team == 0) {
        if(p.isGoalie) glColor3f(0.05f, 0.16f, 0.55f); // goleiro azul escuro
        else           glColor3f(0.82f, 0.68f, 0.10f); // amarelo Brasil mais escuro
    } else {
        if(p.isGoalie) glColor3f(0.82f, 0.12f, 0.12f); // goleiro vermelho
        else           glColor3f(0.07f, 0.23f, 0.78f); // azul intenso do Japao
    }
    drawCircle(0, 0, r, 24);

    // Borda (número da camisa / detalhe)
    if(p.team == 0) {
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        if(p.isGoalie) glColor3f(1.0f, 1.0f, 1.0f);
        else           glColor3f(1.0f, 1.0f, 1.0f);
    }
    glLineWidth(1.5f);
    drawCircleOutline(0, 0, r, 24);

    // Cabeça (círculo menor, tom de pele)
    glColor3f(0.95f, 0.78f, 0.60f);
    drawCircle(0, r*0.72f, r*0.38f, 20);

    // Cabelo
    if(p.team == 0) {
        if(p.isGoalie) glColor3f(0.95f, 0.82f, 0.18f);
        else           glColor3f(0.05f, 0.16f, 0.55f);
    } else {
        if(p.isGoalie) glColor3f(0.82f, 0.12f, 0.12f);
        else           glColor3f(1.0f, 1.0f, 1.0f);
    }
    drawSemiCircle(0, r*0.72f, r*0.38f, 1, 16);

    // Brilho no capacete do goleiro
    if(p.isGoalie) {
        glColor4f(1,1,1,0.5f);
        drawCircle(-r*0.1f, r*0.85f, r*0.1f, 10);
    }

    glPopMatrix();
}

// =====================================================================
// Desenho - Power-up Estrela
// =====================================================================
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

    // Brilho externo (halo)
    float halo = 0.5f + 0.5f * sinf(t * 4.0f);
    glColor4f(1.0f, 0.9f, 0.0f, 0.3f * halo);
    drawCircle(0, 0, r * 1.8f, 24);

    // Estrela de 5 pontas
    glColor3f(1.0f, 0.85f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 0);
    for(int i = 0; i <= 10; i++) {
        float a    = PI/2 + 2*PI * i / 10;
        float rad  = (i % 2 == 0) ? r : r*0.4f;
        glVertex2f(cosf(a)*rad, sinf(a)*rad);
    }
    glEnd();

    // Contorno
    glColor3f(1.0f, 0.5f, 0.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    for(int i = 0; i < 10; i++) {
        float a   = PI/2 + 2*PI * i / 10;
        float rad = (i % 2 == 0) ? r : r*0.4f;
        glVertex2f(cosf(a)*rad, sinf(a)*rad);
    }
    glEnd();

    // Brilhinho
    glColor3f(1.0f, 1.0f, 0.8f);
    drawCircle(-r*0.15f, r*0.25f, r*0.12f, 8);

    glLineWidth(1.0f);
    glPopMatrix();
}

// =====================================================================
// Desenho - Placar
// =====================================================================
void drawScoreboard() {
    // Fundo do placar
    float sbX  = -2.2f, sbY = -5.6f;
    float sbW  =  4.4f, sbH = 1.0f;

    // Sombra
    glColor4f(0,0,0,0.4f);
    drawRect(sbX+0.05f, sbY-sbH-0.05f, sbX+sbW+0.05f, sbY-0.05f);

    // Corpo
    glColor3f(0.05f, 0.05f, 0.15f);
    drawRect(sbX, sbY-sbH, sbX+sbW, sbY);

    // Faixa superior dourada
    glColor3f(0.85f, 0.65f, 0.0f);
    drawRect(sbX, sbY-0.18f, sbX+sbW, sbY);

    // Borda
    glColor3f(0.85f, 0.65f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(sbX, sbY-sbH); glVertex2f(sbX+sbW, sbY-sbH);
    glVertex2f(sbX+sbW, sbY); glVertex2f(sbX, sbY);
    glEnd();
    glLineWidth(1.0f);

    // Título
    glColor3f(0.05f, 0.05f, 0.15f);
    drawText(sbX+0.8f, sbY-0.16f, "FIFA WORLD CUP 2026", GLUT_BITMAP_HELVETICA_12);

    // Nome times
    glColor3f(0.5f, 0.7f, 1.0f);
    drawText(sbX+0.15f, sbY-0.52f, "BRASIL", GLUT_BITMAP_HELVETICA_18);

    glColor3f(1.0f, 0.5f, 0.5f);
    drawText(sbX+2.8f, sbY-0.52f, "JAPAO", GLUT_BITMAP_HELVETICA_18);

    // Divisor central
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_LINES);
    glVertex2f(0, sbY-sbH); glVertex2f(0, sbY);
    glEnd();

    // Placar numérico
    char buf[8];
    glColor3f(1.0f, 1.0f, 0.3f);
    sprintf(buf, "%d", score.home);
    drawTextLarge(-0.55f, sbY-0.82f, buf);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawTextLarge(-0.12f, sbY-0.82f, ":");
    glColor3f(1.0f, 1.0f, 0.3f);
    sprintf(buf, "%d", score.away);
    drawTextLarge(0.22f, sbY-0.82f, buf);

    // Indicador turbo
    if(ball.turbo) {
        float tx = 3.0f;
        glColor3f(1.0f, 0.8f, 0.0f);
        drawText(tx, sbY-0.4f, "TURBO!", GLUT_BITMAP_HELVETICA_18);
        char tt[32];
        sprintf(tt, "%.1fs", ball.turboTimer);
        glColor3f(1.0f, 1.0f, 0.4f);
        drawText(tx+0.05f, sbY-0.75f, tt, GLUT_BITMAP_HELVETICA_12);
    }

    // Controles (rodapé)
    glColor3f(0.6f, 0.6f, 0.6f);
    drawText(-8.4f, FIELD_BOTTOM - 0.4f,
             "W/A/S/D ou SETAS: Mover | ESPACO: Turbo | R: Reset | ESC: Sair",
             GLUT_BITMAP_HELVETICA_10);
}

// =====================================================================
// Animação de Gol
// =====================================================================
void drawGoalAnimation() {
    if(!goalAnimation) return;

    float alpha = (goalAnimTimer > 1.5f) ? 1.0f : goalAnimTimer / 1.5f;
    if(goalAnimTimer > 2.0f) alpha = 1.0f - (goalAnimTimer - 2.0f) / 1.0f;

    // Flash de fundo
    float flash = (goalAnimTimer < 0.3f) ? 0.5f * (1.0f - goalAnimTimer/0.3f) : 0.0f;
    glColor4f(1.0f, 1.0f, 0.0f, flash);
    drawRect(FIELD_LEFT, FIELD_BOTTOM, FIELD_RIGHT, FIELD_TOP);

    // Texto GOOOOL
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

    // Fogos de artifício simples
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

void drawStands(){
    //bandeiras 
    glColor3f(0.0f, 0.8f, 0.0f);
    drawRect(-8.52f, 5.55f, 8.52f, 6.55f);
    glColor3f(0.9f, 1.0f, 0.0f);
    drawRect(-8.52f, 5.65f, 8.52f, 5.80f);
    drawRect(-8.52f, 5.95f, 8.52f, 6.1f);
    drawRect(-8.52f, 6.25f, 8.52f, 6.4f);
    glColor3f(0.0f, 0.0f, 0.9f);
    drawText(-8.2f, 5.9f, "MOVIMENTO VERDE AMARELO  MOVIMENTO VERDE AMARELO  MOVIMENTO VERDE AMARELO", GLUT_BITMAP_HELVETICA_18);

    //arquibancada
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

    // publico
    drawCrowd();

    //divisões arquibancada
    glColor3f(0.0f, 0.0f, 0.0f);
    drawRect(-8.52f, 7.2f, 8.52f, 7.24f);
    drawRect(-8.52f, 7.9f, 8.52f, 7.95f);
    drawRect(-8.52f, 8.6f, 8.52f, 8.64f);
    drawRect(-8.52f, 9.3f, 8.52f, 9.34f);

    
}

// =====================================================================
// Lógica - IA dos Jogadores
// =====================================================================
void updatePlayers(float dt) {
    int totalPlayers = NUM_PLAYERS * 2 + 2;

    for(int i = 0; i < totalPlayers; i++) {
        Player& pl = players[i];
        pl.animTimer += dt;

        if(pl.team == 0) {
            // ---- TIME AZUL (semi-IA: segue a bola, mas com posicionamento) ----
            if(!pl.isGoalie) {
                // Companheiros se movem para posição de suporte
                Vec2 support;
                int idx = i;
                if(idx == 0) support = Vec2(-5.0f, ball.pos.y);
                else if(idx == 1) support = Vec2(ball.pos.x - 2.0f, ball.pos.y + 1.5f);
                else if(idx == 2) support = Vec2(ball.pos.x - 2.0f, ball.pos.y - 1.5f);
                else              support = Vec2(ball.pos.x - 1.0f, ball.pos.y);

                const float edgeMargin = PLAYER_R + BALL_RADIUS + 0.22f;
                Vec2 target = clampTargetFromEdges(support, edgeMargin);
                Vec2 dir = (target - pl.pos).norm();
                float d   = dist(pl.pos, target);
                float spd = 0.04f;
                if(d > 0.3f) {
                    pl.pos.x += dir.x * spd;
                    pl.pos.y += dir.y * spd;
                }
            } else {
                // Goleiro azul: defende o gol esquerdo, segue y da bola
                float targetY = ball.pos.y;
                if(targetY >  GOAL_WIDTH * 0.9f) targetY =  GOAL_WIDTH * 0.9f;
                if(targetY < -GOAL_WIDTH * 0.9f) targetY = -GOAL_WIDTH * 0.9f;
                float dy = targetY - pl.pos.y;
                if(fabsf(dy) > 0.02f)
                    pl.pos.y += (dy > 0 ? 1 : -1) * 0.05f;
                pl.pos.x = FIELD_LEFT + 0.6f;
            }
        } else {
            // ---- TIME VERMELHO (IA agressiva: persegue a bola) ----
            if(!pl.isGoalie) {
                // Cada jogador tem um papel diferente
                Vec2 target;
                int ridx = i - (NUM_PLAYERS + 1);
                float agression = 0.055f;

                if(ridx == 0) {
                    // Atacante: vai direto na bola, mas evita mirar na borda
                    target = ball.pos;
                    agression = 0.065f;
                    const float edgeMargin = PLAYER_R + BALL_RADIUS + 0.22f;
                    target = clampTargetFromEdges(target, edgeMargin);
                } else if(ridx == 1) {
                    // Meio-campo: posição à frente
                    target = Vec2(ball.pos.x + 1.5f, ball.pos.y + 1.2f);
                    if(target.x > FIELD_RIGHT - 0.5f) target.x = FIELD_RIGHT - 0.5f;
                } else if(ridx == 2) {
                    target = Vec2(ball.pos.x + 1.5f, ball.pos.y - 1.2f);
                    if(target.x > FIELD_RIGHT - 0.5f) target.x = FIELD_RIGHT - 0.5f;
                } else {
                    // Defensor
                    target = Vec2(ball.pos.x + 0.8f, ball.pos.y);
                    if(target.x < 0) target.x = 0.5f;
                }

                Vec2 dir = (target - pl.pos).norm();
                float d  = dist(pl.pos, target);
                if(d > 0.25f) {
                    pl.pos.x += dir.x * agression;
                    pl.pos.y += dir.y * agression;
                }

                // Colisão com a bola: "chuta" para o lado do gol
                if(dist(pl.pos, ball.pos) < PLAYER_R + BALL_RADIUS + 0.05f) {
                    Vec2 kickDir = Vec2(FIELD_LEFT - ball.pos.x, 0 - ball.pos.y).norm();
                    ball.vel = ball.vel + kickDir * 0.04f;
                    // limita velocidade
                    if(ball.vel.len() > BALL_SPEED * 2.5f)
                        ball.vel = ball.vel.norm() * BALL_SPEED * 2.5f;
                }
            } else {
                // Goleiro vermelho
                float targetY = ball.pos.y;
                if(targetY >  GOAL_WIDTH * 0.9f) targetY =  GOAL_WIDTH * 0.9f;
                if(targetY < -GOAL_WIDTH * 0.9f) targetY = -GOAL_WIDTH * 0.9f;
                float dy = targetY - pl.pos.y;
                if(fabsf(dy) > 0.02f)
                    pl.pos.y += (dy > 0 ? 1 : -1) * 0.055f;
                pl.pos.x = FIELD_RIGHT - 0.6f;
            }
        }

        clampPlayerToField(pl);
    }

    resolvePlayerCollisions();
}

// =====================================================================
// Lógica - Bola
// =====================================================================
void updateBall(float dt) {
    float spd = ball.turbo ? BALL_TURBO : BALL_SPEED;

    // Leitura do teclado
    bool moved = false;
    if(keys['w'] || keys['W'] || specialKeys[GLUT_KEY_UP]) {
        ball.vel.y += spd * 0.15f;  moved = true;
    }
    if(keys['s'] || keys['S'] || specialKeys[GLUT_KEY_DOWN]) {
        ball.vel.y -= spd * 0.15f;  moved = true;
    }
    if(keys['a'] || keys['A'] || specialKeys[GLUT_KEY_LEFT]) {
        ball.vel.x -= spd * 0.15f;  moved = true;
    }
    if(keys['d'] || keys['D'] || specialKeys[GLUT_KEY_RIGHT]) {
        ball.vel.x += spd * 0.15f;  moved = true;
    }

    // Limitar velocidade
    float maxV = ball.turbo ? BALL_TURBO : BALL_SPEED;
    if(ball.vel.len() > maxV)
        ball.vel = ball.vel.norm() * maxV;

    // Atrito
    ball.vel.x *= 0.93f;
    ball.vel.y *= 0.93f;

    // Aplicar movimento
    ball.pos.x += ball.vel.x;
    ball.pos.y += ball.vel.y;

    // Rotação visual baseada na velocidade
    ball.angle += ball.vel.len() * 300.0f * dt;

    // Turbo timer
    if(ball.turbo) {
        ball.turboTimer -= dt;
        if(ball.turboTimer <= 0) {
            ball.turbo = false;
            ball.turboTimer = 0;
        }
    }

    // ---- Colisão com bordas do campo ----
    // Topo e fundo
    if(ball.pos.y + BALL_RADIUS > FIELD_TOP) {
        ball.pos.y = FIELD_TOP - BALL_RADIUS;
        ball.vel.y = -fabsf(ball.vel.y) * 0.7f;
    }
    if(ball.pos.y - BALL_RADIUS < FIELD_BOTTOM) {
        ball.pos.y = FIELD_BOTTOM + BALL_RADIUS;
        ball.vel.y =  fabsf(ball.vel.y) * 0.7f;
    }

    // Lado esquerdo (verifica gol)
    if(ball.pos.x - BALL_RADIUS < FIELD_LEFT) {
        // Está na faixa do gol?
        if(ball.pos.y > -GOAL_WIDTH && ball.pos.y < GOAL_WIDTH) {
            // GOL do time vermelho (away)
            if(!goalAnimation) {
                score.away++;
                lastGoalTeam    = 1;
                goalAnimation   = true;
                goalAnimTimer   = 0;
                resetBall();
                resetPlayers();
                resetStar();
            }
        } else {
            ball.pos.x = FIELD_LEFT + BALL_RADIUS;
            ball.vel.x =  fabsf(ball.vel.x) * 0.7f;
        }
    }

    // Lado direito (verifica gol)
    if(ball.pos.x + BALL_RADIUS > FIELD_RIGHT) {
        if(ball.pos.y > -GOAL_WIDTH && ball.pos.y < GOAL_WIDTH) {
            // GOL do time azul (home)
            if(!goalAnimation) {
                score.home++;
                lastGoalTeam    = 0;
                goalAnimation   = true;
                goalAnimTimer   = 0;
                resetBall();
                resetPlayers();
                resetStar();
            }
        } else {
            ball.pos.x = FIELD_RIGHT - BALL_RADIUS;
            ball.vel.x = -fabsf(ball.vel.x) * 0.7f;
        }
    }

    // ---- Colisão com jogadores ----
    int totalPlayers = NUM_PLAYERS * 2 + 2;
    for(int i = 0; i < totalPlayers; i++) {
        float d = dist(ball.pos, players[i].pos);
        float minD = BALL_RADIUS + PLAYER_R;
        if(d < minD && d > 0.001f) {
            Vec2 normal = (ball.pos - players[i].pos).norm();
            ball.pos = players[i].pos + normal * (minD + 0.01f);
            // Rebater
            float dot = ball.vel.x * normal.x + ball.vel.y * normal.y;
            ball.vel.x -= 2 * dot * normal.x;
            ball.vel.y -= 2 * dot * normal.y;
            ball.vel = ball.vel * 0.65f;
        }
    }

    // A colisao com jogadores pode empurrar a bola de volta para fora.
    // Reaplica os limites para evitar que ela fique presa fora do campo.
    if(ball.pos.y + BALL_RADIUS > FIELD_TOP) {
        ball.pos.y = FIELD_TOP - BALL_RADIUS;
        ball.vel.y = -fabsf(ball.vel.y) * 0.7f;
    }
    if(ball.pos.y - BALL_RADIUS < FIELD_BOTTOM) {
        ball.pos.y = FIELD_BOTTOM + BALL_RADIUS;
        ball.vel.y =  fabsf(ball.vel.y) * 0.7f;
    }

    if(ball.pos.x - BALL_RADIUS < FIELD_LEFT) {
        if(ball.pos.y > -GOAL_WIDTH && ball.pos.y < GOAL_WIDTH) {
            if(!goalAnimation) {
                score.away++;
                lastGoalTeam    = 1;
                goalAnimation   = true;
                goalAnimTimer   = 0;
                resetBall();
                resetPlayers();
                resetStar();
            }
        } else {
            ball.pos.x = FIELD_LEFT + BALL_RADIUS;
            ball.vel.x =  fabsf(ball.vel.x) * 0.7f;
        }
    }

    if(ball.pos.x + BALL_RADIUS > FIELD_RIGHT) {
        if(ball.pos.y > -GOAL_WIDTH && ball.pos.y < GOAL_WIDTH) {
            if(!goalAnimation) {
                score.home++;
                lastGoalTeam    = 0;
                goalAnimation   = true;
                goalAnimTimer   = 0;
                resetBall();
                resetPlayers();
                resetStar();
            }
        } else {
            ball.pos.x = FIELD_RIGHT - BALL_RADIUS;
            ball.vel.x = -fabsf(ball.vel.x) * 0.7f;
        }
    }

    // ---- Power-up Estrela ----
    if(star.active) {
        if(dist(ball.pos, star.pos) < BALL_RADIUS + STAR_RADIUS) {
            star.active      = false;
            star.respawnTimer= 8.0f;
            ball.turbo       = true;
            ball.turboTimer  = TURBO_TIME;
        }
    } else {
        star.respawnTimer -= dt;
        if(star.respawnTimer <= 0) {
            star.pos = Vec2(randf(-4, 4), randf(-2, 2));
            star.active = true;
        }
    }
    star.rotAngle += 90.0f * dt;
}

// =====================================================================
// Callback de Exibição
// =====================================================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // ---- Campo ----
    drawField();

    // ---- Arquibancada ----
    drawStands();

    // ---- Power-up ----
    drawStar();

    // ---- Jogadores ----
    int totalPlayers = NUM_PLAYERS * 2 + 2;
    for(int i = 0; i < totalPlayers; i++)
        drawPlayer(players[i]);

    // ---- Bola ----
    drawBall();

    // ---- Placar ----
    drawScoreboard();

    // ---- Animação de gol ----
    if(goalAnimation)
        drawGoalAnimation();

    glutSwapBuffers();
}

// =====================================================================
// Callback de Atualização (Timer)
// =====================================================================
void update(int) {
    int now = glutGet(GLUT_ELAPSED_TIME);
    deltaTime = (now - lastTime) / 1000.0f;
    if(deltaTime > 0.05f) deltaTime = 0.05f;
    lastTime = now;

    if(!goalAnimation) {
        updateBall(deltaTime);
        updatePlayers(deltaTime);

        const float cornerRadius = 1.35f;
        if(isBallNearCorner(cornerRadius)) {
            cornerTrapTimer += deltaTime;
            if(cornerTrapTimer >= 3.0f) {
                dispersePlayersFromBall(deltaTime);
            }
        } else {
            cornerTrapTimer = 0.0f;
        }
    } else {
        goalAnimTimer += deltaTime;
        if(goalAnimTimer > 3.0f) {
            goalAnimation = false;
        }
    }

    fieldGrassAnim += deltaTime;

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// =====================================================================
// Callbacks de Teclado
// =====================================================================
void keyDown(unsigned char key, int, int) {
    keys[key] = true;
    if(key == 27) exit(0);           // ESC
    if(key == 'r' || key == 'R') {
        resetBall();
        resetPlayers();
        resetStar();
    }
    if(key == ' ') {                 // Espaço: chute
        float spd = ball.turbo ? BALL_TURBO : BALL_SPEED;
        if(fabsf(ball.vel.x) < 0.01f && fabsf(ball.vel.y) < 0.01f)
            ball.vel = Vec2(spd, 0);
        else
            ball.vel = ball.vel.norm() * spd * 1.8f;
    }
}

void keyUp(unsigned char key, int, int) { keys[key] = false; }

void specialDown(int key, int, int) { specialKeys[key] = true; }
void specialUp  (int key, int, int) { specialKeys[key] = false; }

// =====================================================================
// Reshape
// =====================================================================
void reshape(int w, int h) {
    if(h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)w / h;
    // Campo visível: -10..10 horizontal, ajustado pelo aspect
    float halfH = 10.0f / aspect;
    gluOrtho2D(-10.0f, 10.0f, -halfH, halfH + 1.5f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// =====================================================================
// Main
// =====================================================================
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("Copa do Mundo 2026 - Computacao Grafica");

    // Fundo preto (fora do campo)
    glClearColor(0.05f, 0.08f, 0.05f, 1.0f);

    // Habilita blending para transparência
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    initGame();
    lastTime = glutGet(GLUT_ELAPSED_TIME);

    // Registra callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutTimerFunc(16, update, 0);

    printf("=== Copa do Mundo 2026 ===\n");
    printf("Controles:\n");
    printf("  W/A/S/D ou SETAS - Mover a bola\n");
    printf("  ESPACO           - Chute forte\n");
    printf("  R                - Resetar posicao\n");
    printf("  ESC              - Sair\n");
    printf("Pegue a ESTRELA DOURADA para ativar TURBO!\n");
    printf("Marque gols passando a bola pela linha do gol adversario!\n\n");

    glutMainLoop();
    return 0;
}
