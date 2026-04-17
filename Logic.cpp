#include "Logic.h"

#include "Audio.h"
#include "Game.h"

// Gera um float aleatório no intervalo [a, b].
float randf(float a, float b) {
    return a + (b-a) * (rand() / (float)RAND_MAX);
}

// Distância euclidiana entre dois pontos 2D.
float dist(Vec2 a, Vec2 b) {
    return (a-b).len();
}

// Garante que um jogador nunca saia dos limites jogáveis do campo.
void clampPlayerToField(Player& pl) {
    if(pl.pos.x < FIELD_LEFT  + PLAYER_R) pl.pos.x = FIELD_LEFT  + PLAYER_R;
    if(pl.pos.x > FIELD_RIGHT - PLAYER_R) pl.pos.x = FIELD_RIGHT - PLAYER_R;
    if(pl.pos.y < FIELD_BOTTOM + PLAYER_R) pl.pos.y = FIELD_BOTTOM + PLAYER_R;
    if(pl.pos.y > FIELD_TOP   - PLAYER_R) pl.pos.y = FIELD_TOP   - PLAYER_R;
}

// Quando um alvo cai muito perto das bordas, essa função puxa esse
// ponto um pouco para dentro para evitar travamentos na parede.
Vec2 clampTargetFromEdges(Vec2 target, float margin) {
    if(target.x < FIELD_LEFT + margin)   target.x = FIELD_LEFT + margin;
    if(target.x > FIELD_RIGHT - margin)  target.x = FIELD_RIGHT - margin;
    if(target.y < FIELD_BOTTOM + margin) target.y = FIELD_BOTTOM + margin;
    if(target.y > FIELD_TOP - margin)    target.y = FIELD_TOP - margin;
    return target;
}

// Detecta se a bola entrou em uma região crítica perto de qualquer quina.
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

// Abre espaço ao redor da bola quando ela fica presa na quina por muito
// tempo, empurrando levemente os jogadores próximos para longe.
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

// Resolve sobreposição entre jogadores de linha para evitar “pilhas”
// artificiais ocupando o mesmo espaço.
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

// Atualiza o comportamento de todos os jogadores.
// Brasil joga com suporte posicional; Japão joga de forma mais agressiva.
void updatePlayers(float dt) {
    int totalPlayers = NUM_PLAYERS * 2 + 2;

    for(int i = 0; i < totalPlayers; i++) {
        Player& pl = players[i];
        pl.animTimer += dt;

        if(pl.team == 0) {
            // Jogadores de linha do Brasil se reposicionam em torno da bola.
            if(!pl.isGoalie) {
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
                // O goleiro só acompanha a altura da bola dentro da faixa do gol.
                float targetY = ball.pos.y;
                if(targetY >  GOAL_WIDTH * 0.9f) targetY =  GOAL_WIDTH * 0.9f;
                if(targetY < -GOAL_WIDTH * 0.9f) targetY = -GOAL_WIDTH * 0.9f;
                float dy = targetY - pl.pos.y;
                if(fabsf(dy) > 0.02f)
                    pl.pos.y += (dy > 0 ? 1 : -1) * 0.05f;
                pl.pos.x = FIELD_LEFT + 0.6f;
            }
        } else {
            // O Japão distribui papéis mais agressivos: atacante, meias e defensor.
            if(!pl.isGoalie) {
                Vec2 target;
                int ridx = i - (NUM_PLAYERS + 1);
                float agression = 0.055f;

                if(ridx == 0) {
                    target = ball.pos;
                    agression = 0.065f;
                    const float edgeMargin = PLAYER_R + BALL_RADIUS + 0.22f;
                    target = clampTargetFromEdges(target, edgeMargin);
                } else if(ridx == 1) {
                    target = Vec2(ball.pos.x + 1.5f, ball.pos.y + 1.2f);
                    if(target.x > FIELD_RIGHT - 0.5f) target.x = FIELD_RIGHT - 0.5f;
                } else if(ridx == 2) {
                    target = Vec2(ball.pos.x + 1.5f, ball.pos.y - 1.2f);
                    if(target.x > FIELD_RIGHT - 0.5f) target.x = FIELD_RIGHT - 0.5f;
                } else {
                    target = Vec2(ball.pos.x + 0.8f, ball.pos.y);
                    if(target.x < 0) target.x = 0.5f;
                }

                Vec2 dir = (target - pl.pos).norm();
                float d  = dist(pl.pos, target);
                if(d > 0.25f) {
                    pl.pos.x += dir.x * agression;
                    pl.pos.y += dir.y * agression;
                }

                if(dist(pl.pos, ball.pos) < PLAYER_R + BALL_RADIUS + 0.05f) {
                    // O “chute” do atacante é uma força aplicada na bola
                    // apontando para o gol da esquerda.
                    Vec2 kickDir = Vec2(FIELD_LEFT - ball.pos.x, 0 - ball.pos.y).norm();
                    ball.vel = ball.vel + kickDir * 0.04f;
                    if(ball.vel.len() > BALL_SPEED * 2.5f)
                        ball.vel = ball.vel.norm() * BALL_SPEED * 2.5f;
                }
            } else {
                // Goleiro do Japão espelha a lógica do goleiro brasileiro.
                float targetY = ball.pos.y;
                if(targetY >  GOAL_WIDTH * 0.9f) targetY =  GOAL_WIDTH * 0.9f;
                if(targetY < -GOAL_WIDTH * 0.9f) targetY = -GOAL_WIDTH * 0.9f;
                float dy = targetY - pl.pos.y;
                if(fabsf(dy) > 0.02f)
                    pl.pos.y += (dy > 0 ? 1 : -1) * 0.055f;
                pl.pos.x = FIELD_RIGHT - 0.6f;
            }
        }

        // Mesmo antes da colisão entre jogadores, cada um é mantido dentro do campo.
        clampPlayerToField(pl);
    }

    // A separação final evita empilhamento depois que todos se moveram.
    resolvePlayerCollisions();
}

// Atualiza física, entradas do jogador, colisões, gol e power-up da bola.
void updateBall(float dt) {
    float spd = ball.turbo ? BALL_TURBO : BALL_SPEED;

    // Entrada do teclado: altera a velocidade da bola, não a posição diretamente.
    if(keys['w'] || keys['W'] || specialKeys[GLUT_KEY_UP]) {
        ball.vel.y += spd * 0.15f;
    }
    if(keys['s'] || keys['S'] || specialKeys[GLUT_KEY_DOWN]) {
        ball.vel.y -= spd * 0.15f;
    }
    if(keys['a'] || keys['A'] || specialKeys[GLUT_KEY_LEFT]) {
        ball.vel.x -= spd * 0.15f;
    }
    if(keys['d'] || keys['D'] || specialKeys[GLUT_KEY_RIGHT]) {
        ball.vel.x += spd * 0.15f;
    }

    // Limita a velocidade máxima para manter o jogo controlável.
    float maxV = ball.turbo ? BALL_TURBO : BALL_SPEED;
    if(ball.vel.len() > maxV)
        ball.vel = ball.vel.norm() * maxV;

    // Atrito simples para a bola perder velocidade sozinha.
    ball.vel.x *= 0.93f;
    ball.vel.y *= 0.93f;

    ball.pos.x += ball.vel.x;
    ball.pos.y += ball.vel.y;

    // A rotação visual depende da velocidade para sugerir rolamento.
    ball.angle += ball.vel.len() * 300.0f * dt;

    // O turbo é temporário e volta ao normal quando o tempo acaba.
    if(ball.turbo) {
        ball.turboTimer -= dt;
        if(ball.turboTimer <= 0) {
            ball.turbo = false;
            ball.turboTimer = 0;
        }
    }

    if(ball.pos.y + BALL_RADIUS > FIELD_TOP) {
        ball.pos.y = FIELD_TOP - BALL_RADIUS;
        ball.vel.y = -fabsf(ball.vel.y) * 0.7f;
    }
    if(ball.pos.y - BALL_RADIUS < FIELD_BOTTOM) {
        ball.pos.y = FIELD_BOTTOM + BALL_RADIUS;
        ball.vel.y =  fabsf(ball.vel.y) * 0.7f;
    }

    // Nas laterais horizontais, a bola pode gerar gol ou apenas ricochetear.
    if(ball.pos.x - BALL_RADIUS < FIELD_LEFT) {
        if(ball.pos.y > -GOAL_WIDTH && ball.pos.y < GOAL_WIDTH) {
            if(!goalAnimation) {
                // Gol do Japão.
                score.away++;
                lastGoalTeam    = 1;
                goalAnimation   = true;
                goalAnimTimer   = 0;
                playGoalSoundAsync();
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
                // Gol do Brasil.
                score.home++;
                lastGoalTeam    = 0;
                goalAnimation   = true;
                goalAnimTimer   = 0;
                playGoalSoundAsync();
                resetBall();
                resetPlayers();
                resetStar();
            }
        } else {
            ball.pos.x = FIELD_RIGHT - BALL_RADIUS;
            ball.vel.x = -fabsf(ball.vel.x) * 0.7f;
        }
    }

    int totalPlayers = NUM_PLAYERS * 2 + 2;
    for(int i = 0; i < totalPlayers; i++) {
        float d = dist(ball.pos, players[i].pos);
        float minD = BALL_RADIUS + PLAYER_R;
        if(d < minD && d > 0.001f) {
            // A bola é empurrada para fora do corpo do jogador e rebate na normal.
            Vec2 normal = (ball.pos - players[i].pos).norm();
            ball.pos = players[i].pos + normal * (minD + 0.01f);
            float dot = ball.vel.x * normal.x + ball.vel.y * normal.y;
            ball.vel.x -= 2 * dot * normal.x;
            ball.vel.y -= 2 * dot * normal.y;
            ball.vel = ball.vel * 0.65f;
        }
    }

    // Revalidação após colisão com jogador.
    // Sem isso, a correção de interseção pode jogar a bola para fora do campo.
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
                // Também trata o caso em que a bola foi empurrada ao gol
                // depois da colisão com um jogador.
                score.away++;
                lastGoalTeam    = 1;
                goalAnimation   = true;
                goalAnimTimer   = 0;
                playGoalSoundAsync();
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
                playGoalSoundAsync();
                resetBall();
                resetPlayers();
                resetStar();
            }
        } else {
            ball.pos.x = FIELD_RIGHT - BALL_RADIUS;
            ball.vel.x = -fabsf(ball.vel.x) * 0.7f;
        }
    }

    // Gestão do power-up estrela: coleta, turbo e respawn.
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
