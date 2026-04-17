#include "Audio.h"
#include "Game.h"
#include "Logic.h"
#include "Render.h"

// =====================================================================
// Estado global do jogo
// =====================================================================
// Como o projeto usa callbacks do GLUT, este estado fica acessível
// globalmente para que os módulos de lógica, render e áudio enxerguem
// a mesma partida.

Ball   ball;
Player players[NUM_PLAYERS*2 + 2];
Star   star;
Score  score;
std::vector<Fan> crowd;

bool keys[256];
bool specialKeys[256];

float deltaTime = 0.016f;
int   lastTime  = 0;

bool goalAnimation   = false;
float goalAnimTimer  = 0.0f;
int   lastGoalTeam   = -1;

float fieldGrassAnim = 0.0f;
float cornerTrapTimer = 0.0f;

// =====================================================================
// Inicialização e reset
// =====================================================================

// Reinicia apenas a bola.
// Também limpa o timer de “bola presa na quina” para não herdar
// estado de uma jogada anterior.
void resetBall() {
    ball.pos      = Vec2(0, 0);
    ball.vel      = Vec2(0, 0);
    ball.angle    = 0;
    ball.turbo    = false;
    ball.turboTimer = 0;
    cornerTrapTimer = 0.0f;
}

// Recoloca todos os jogadores em suas posições iniciais.
void resetPlayers() {
    float blueX[NUM_PLAYERS] = {-5.0f, -3.5f, -3.5f, -1.5f};
    float blueY[NUM_PLAYERS] = { 0.0f,  2.0f, -2.0f,  0.0f};

    for(int i = 0; i < NUM_PLAYERS; i++) {
        players[i].pos      = Vec2(blueX[i], blueY[i]);
        players[i].vel      = Vec2(0,0);
        players[i].team     = 0;
        players[i].animTimer= (float)i * 0.5f;
        players[i].isGoalie = false;
    }

    players[NUM_PLAYERS].pos      = Vec2(FIELD_LEFT + 0.6f, 0);
    players[NUM_PLAYERS].vel      = Vec2(0,0);
    players[NUM_PLAYERS].team     = 0;
    players[NUM_PLAYERS].animTimer= 0;
    players[NUM_PLAYERS].isGoalie = true;

    float redX[NUM_PLAYERS] = { 5.0f,  3.5f,  3.5f,  1.5f};
    float redY[NUM_PLAYERS] = { 0.0f,  2.0f, -2.0f,  0.0f};

    for(int i = 0; i < NUM_PLAYERS; i++) {
        players[NUM_PLAYERS+1+i].pos      = Vec2(redX[i], redY[i]);
        players[NUM_PLAYERS+1+i].vel      = Vec2(0,0);
        players[NUM_PLAYERS+1+i].team     = 1;
        players[NUM_PLAYERS+1+i].animTimer= (float)i * 0.5f;
        players[NUM_PLAYERS+1+i].isGoalie = false;
    }

    players[NUM_PLAYERS*2+1].pos      = Vec2(FIELD_RIGHT - 0.6f, 0);
    players[NUM_PLAYERS*2+1].vel      = Vec2(0,0);
    players[NUM_PLAYERS*2+1].team     = 1;
    players[NUM_PLAYERS*2+1].animTimer= 0;
    players[NUM_PLAYERS*2+1].isGoalie = true;
}

// Reposiciona a estrela e força um pequeno atraso antes de reaparecer.
void resetStar() {
    star.pos         = Vec2(randf(-3,3), randf(-2,2));
    star.active      = false;
    star.rotAngle    = 0;
    star.respawnTimer= STAR_RESET_DELAY;
}

// Prepara o estado inicial completo da partida.
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
// Callbacks principais do GLUT
// =====================================================================

// Callback de exibição.
// É chamado sempre que a cena precisa ser redesenhada.
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    drawField();
    drawStands();
    drawStar();

    int totalPlayers = NUM_PLAYERS * 2 + 2;
    for(int i = 0; i < totalPlayers; i++) {
        drawPlayer(players[i]);
    }

    drawBall();
    drawScoreboard();

    if(goalAnimation) {
        drawGoalAnimation();
    }

    glutSwapBuffers();
}

// Callback de atualização periódica.
// Calcula deltaTime, avança a lógica e agenda o próximo frame.
void update(int) {
    int now = glutGet(GLUT_ELAPSED_TIME);
    deltaTime = (now - lastTime) / 1000.0f;
    if(deltaTime > 0.05f) deltaTime = 0.05f;
    lastTime = now;

    if(!goalAnimation) {
        // Enquanto não há animação de gol, o jogo roda normalmente.
        updateBall(deltaTime);
        updatePlayers(deltaTime);

        const float cornerRadius = 1.35f;
        if(isBallNearCorner(cornerRadius)) {
            // Se a bola ficar tempo demais na quina, abre espaço ao redor.
            cornerTrapTimer += deltaTime;
            if(cornerTrapTimer >= 2.0f) {
                dispersePlayersFromBall(deltaTime);
            }
        } else {
            cornerTrapTimer = 0.0f;
        }
    } else {
        goalAnimTimer += deltaTime;
        if(goalAnimTimer > GOAL_FREEZE_TIME) {
            goalAnimation = false;
            // Ao fim da animação, a música ambiente volta.
            startAmbientSoundLoop();
        }
    }

    // Reservado para animações de cenário.
    fieldGrassAnim += deltaTime;

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// Callback de tecla comum pressionada.
void keyDown(unsigned char key, int, int) {
    keys[key] = true;
    if(key == 27) {
        // ESC encerra o jogo tocando antes o som de fim.
        playEndGameSoundSyncAndStop();
        exit(0);
    }
    if(key == 'r' || key == 'R') {
        // Reset rápido da jogada.
        resetBall();
        resetPlayers();
        resetStar();
    }
    if(key == ' ') {
        // Chute forte: se a bola estiver quase parada, impulsiona para a direita.
        // Caso contrário, amplifica a direção atual.
        float spd = ball.turbo ? BALL_TURBO : BALL_SPEED;
        if(fabsf(ball.vel.x) < 0.01f && fabsf(ball.vel.y) < 0.01f)
            ball.vel = Vec2(spd, 0);
        else
            ball.vel = ball.vel.norm() * spd * 1.8f;
    }
}

// Callbacks de liberação de teclas.
void keyUp(unsigned char key, int, int) { keys[key] = false; }
void specialDown(int key, int, int) { specialKeys[key] = true; }
void specialUp  (int key, int, int) { specialKeys[key] = false; }

// Callback de redimensionamento da janela.
// Recalcula a projeção ortográfica mantendo o campo proporcional.
void reshape(int w, int h) {
    if(h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)w / h;
    float halfH = 10.0f / aspect;
    gluOrtho2D(-10.0f, 10.0f, -halfH, halfH + 1.5f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// =====================================================================
// Entrada principal do programa
// =====================================================================
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("Copa do Mundo 2026 - Computacao Grafica");

    // Configuração visual base do OpenGL.
    glClearColor(0.05f, 0.08f, 0.05f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    initGame();
    lastTime = glutGet(GLUT_ELAPSED_TIME);

    // A música ambiente começa junto da partida.
    startAmbientSoundLoop();

    // Registro dos callbacks principais do GLUT.
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

    // O controle do programa passa a ser do loop do GLUT.
    glutMainLoop();
}
