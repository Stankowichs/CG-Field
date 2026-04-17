#ifndef GAME_H
#define GAME_H

// Cabeçalho central do projeto.
// Reúne constantes, estruturas e variáveis globais compartilhadas
// entre renderização, lógica, áudio e o arquivo principal.

#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

#define PI 3.14159265358979323846

// Dimensões fixas da janela.
const int WINDOW_WIDTH  = 1024;
const int WINDOW_HEIGHT = 900;

// Limites do campo no espaço 2D usado pelo OpenGL.
const float FIELD_LEFT   = -8.5f;
const float FIELD_RIGHT  =  8.5f;
const float FIELD_TOP    =  5.5f;
const float FIELD_BOTTOM = -5.5f;
const float FIELD_W      = FIELD_RIGHT - FIELD_LEFT;
const float FIELD_H      = FIELD_TOP   - FIELD_BOTTOM;

// Medidas do gol.
const float GOAL_WIDTH   = 1.8f;
const float GOAL_DEPTH   = 0.4f;

// Propriedades físicas e visuais da bola.
const float BALL_RADIUS  = 0.18f;
const float BALL_SPEED   = 0.08f;
const float BALL_TURBO   = 0.12f;

// Configuração dos jogadores.
const float PLAYER_R     = 0.28f;
const int   NUM_PLAYERS  = 4;

// Configuração da estrela e do turbo.
const float STAR_RADIUS  = 0.22f;
const float TURBO_TIME   = 5.0f;
const float STAR_RESET_DELAY = 2.0f;

// Tempo em que o jogo fica “congelado” após um gol.
const float GOAL_FREEZE_TIME = 6.0f;

// Vetor 2D básico usado em praticamente toda a movimentação do jogo.
struct Vec2 {
    float x, y;
    Vec2(float x=0, float y=0): x(x), y(y){}
    Vec2 operator+(const Vec2& o) const { return {x+o.x, y+o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x-o.x, y-o.y}; }
    Vec2 operator*(float s) const { return {x*s, y*s}; }
    float len() const { return sqrtf(x*x + y*y); }
    Vec2 norm() const {
        float l = len();
        return l > 0.0001f ? Vec2(x/l, y/l) : Vec2(0, 0);
    }
};

// Estado da bola.
struct Ball {
    Vec2  pos;
    Vec2  vel;
    float angle;
    bool  turbo;
    float turboTimer;
};

// Estado de um jogador individual.
struct Player {
    Vec2  pos;
    Vec2  vel;
    int   team;
    float animTimer;
    bool  isGoalie;
};

// Estado do power-up de estrela.
struct Star {
    Vec2  pos;
    bool  active;
    float rotAngle;
    float respawnTimer;
};

// Placar da partida.
struct Score {
    int home, away;
};

// Cada torcedor desenhado na arquibancada.
struct Fan {
    float x, y;
    int tipo;
    float escala;
};

// Estado global compartilhado entre os módulos.
extern Ball ball;
extern Player players[NUM_PLAYERS*2 + 2];
extern Star star;
extern Score score;
extern std::vector<Fan> crowd;

extern bool keys[256];
extern bool specialKeys[256];

extern float deltaTime;
extern int lastTime;

extern bool goalAnimation;
extern float goalAnimTimer;
extern int lastGoalTeam;

extern float fieldGrassAnim;
extern float cornerTrapTimer;

// Utilitários usados pela lógica do jogo.
float randf(float a, float b);
float dist(Vec2 a, Vec2 b);
void clampPlayerToField(Player& pl);
Vec2 clampTargetFromEdges(Vec2 target, float margin);
bool isBallNearCorner(float radius);
void dispersePlayersFromBall(float dt);
void resolvePlayerCollisions();

// Rotinas de reinicialização do estado do jogo.
void resetBall();
void resetPlayers();
void resetStar();
void initGame();

#endif
