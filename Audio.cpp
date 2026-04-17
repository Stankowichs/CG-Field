#include "Audio.h"

// Camada de áudio simples baseada em PlaySound da API do Windows.
// O restante do projeto só chama estas funções de alto nível.

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>

// Carrega a DLL de áudio uma única vez e reutiliza o handle.
static HMODULE getWinmmModule() {
    static HMODULE hWinmm = NULL;
    if(!hWinmm) hWinmm = LoadLibraryA("winmm.dll");
    return hWinmm;
}

// Interrompe qualquer som atual, seja música ambiente ou efeito.
void stopAudio() {
    using PlaySoundAFn = BOOL (WINAPI*)(LPCSTR, HMODULE, DWORD);
    HMODULE hWinmm = getWinmmModule();
    if(!hWinmm) return;
    auto playFn = reinterpret_cast<PlaySoundAFn>(GetProcAddress(hWinmm, "PlaySoundA"));
    if(!playFn) return;
    playFn(NULL, NULL, SND_PURGE);
}

// Inicia a música ambiente em loop.
void startAmbientSoundLoop() {
    using PlaySoundAFn = BOOL (WINAPI*)(LPCSTR, HMODULE, DWORD);
    HMODULE hWinmm = getWinmmModule();
    if(!hWinmm) return;
    auto playFn = reinterpret_cast<PlaySoundAFn>(GetProcAddress(hWinmm, "PlaySoundA"));
    if(!playFn) return;

    stopAudio();
    playFn("audio\\som-ambiente.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP | SND_NODEFAULT);
}

// Toca o efeito de gol sem bloquear o restante do jogo.
void playGoalSoundAsync() {
    using PlaySoundAFn = BOOL (WINAPI*)(LPCSTR, HMODULE, DWORD);
    HMODULE hWinmm = getWinmmModule();
    if(!hWinmm) return;
    auto playFn = reinterpret_cast<PlaySoundAFn>(GetProcAddress(hWinmm, "PlaySoundA"));
    if(!playFn) return;

    stopAudio();
    playFn("audio\\gol1.wav", NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

// Toca o som de encerramento de forma síncrona para não ser cortado
// pelo exit imediato do programa.
void playEndGameSoundSyncAndStop() {
    using PlaySoundAFn = BOOL (WINAPI*)(LPCSTR, HMODULE, DWORD);
    HMODULE hWinmm = getWinmmModule();
    if(!hWinmm) return;
    auto playFn = reinterpret_cast<PlaySoundAFn>(GetProcAddress(hWinmm, "PlaySoundA"));
    if(!playFn) return;

    stopAudio();
    playFn("audio\\fimjogo.wav", NULL, SND_FILENAME | SND_SYNC | SND_NODEFAULT);
    stopAudio();
}
#else
// Em plataformas sem winmm, as funções viram no-ops para manter
// a compilação do projeto.
void stopAudio() {}
void startAmbientSoundLoop() {}
void playGoalSoundAsync() {}
void playEndGameSoundSyncAndStop() {}
#endif
