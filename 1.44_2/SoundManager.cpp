#include "SoundManager.h"

// 미디 출력
HMIDIOUT g_hMidiOut = nullptr;

using namespace std;

float g_masterVolume = 1.0f;
float g_sfxVolume = 1.0f;
float g_bgmVolume = 1.0f;

void UpdateBGMVolume() {
    float finalBgmVol = g_masterVolume * g_bgmVolume;
    int mciVol = (int)(finalBgmVol * 1000.0f); // 0~1000 스케일로 변환

    // 윈도우 MCI 볼륨 조절 명령어 전송
    wchar_t cmd[128];
    swprintf_s(cmd, 128, L"setaudio bgm volume to %d", mciVol);
    mciSendString(cmd, NULL, 0, NULL);
}

void SetInstrument(uint8_t channel, uint8_t instrument) {
    DWORD msg = (0xC0 | (channel & 0x0F)) | (instrument << 8);
    midiOutShortMsg(g_hMidiOut, msg);
}

void NoteOn(uint8_t channel, uint8_t note, uint8_t velocity = 127) {
    uint8_t finalVelocity = (uint8_t)(velocity * g_masterVolume * g_sfxVolume);

    if (finalVelocity > 127) finalVelocity = 127;
    DWORD msg = (0x90 | (channel & 0x0F)) | (note << 8) | (velocity << 16);
    midiOutShortMsg(g_hMidiOut, msg);
}

void NoteOff(uint8_t channel, uint8_t note) {
    DWORD msg = (0x80 | (channel & 0x0F)) | (note << 8);
    midiOutShortMsg(g_hMidiOut, msg);
}

void PitchBend(uint8_t channel, uint16_t bendValue) {
    uint8_t lsb = bendValue & 0x7F;
    uint8_t msb = (bendValue >> 7) & 0x7F;
    DWORD msg = (0xE0 | (channel & 0x0F)) | (lsb << 8) | (msb << 16);
    midiOutShortMsg(g_hMidiOut, msg);
}

namespace SoundManager {

    void Initialize() {
        //윈도우 기본 미디 오픈
        if (g_hMidiOut == nullptr) {
            midiOutOpen(&g_hMidiOut, MIDI_MAPPER, 0, 0, CALLBACK_NULL);
        }
        g_masterVolume = 1.0f;
        g_sfxVolume = 1.0f;
        g_bgmVolume = 1.0f;
    }

    void SetMasterVolume(float volume) {
        g_masterVolume = clamp(volume, 0.0f, 1.0f);
        UpdateBGMVolume();
    }

    void SetSFXVolume(float volume) {
        g_sfxVolume = clamp(volume, 0.0f, 1.0f);
    }

    void SetBGMVolume(float volume) {
        g_bgmVolume = clamp(volume, 0.0f, 1.0f);
        UpdateBGMVolume();
    }

    void Release()
    {
        if (g_hMidiOut != nullptr)
        {
            midiOutClose(g_hMidiOut); // 장치 닫기
            g_hMidiOut = nullptr;
        }
    }

    void PlayNote(int instrument, int note)
    {
        if (g_hMidiOut == nullptr) return;

        DWORD instrumentMsg = 0xC0 | (instrument << 8);
        midiOutShortMsg(g_hMidiOut, instrumentMsg);

        DWORD noteMsg = 0x90 | (note << 8) | (0x7F << 16);
        midiOutShortMsg(g_hMidiOut, noteMsg);
    }

    void PlayBGM(const wchar_t* filename)
    {
        StopBGM();
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);

        filesystem::path basePath = exePath;
        filesystem::path assetPath = basePath.parent_path() / L"BGM" / filename;

        wchar_t cmd[512];
        swprintf_s(cmd, 512, L"open \"%s\" type sequencer alias bgm", assetPath.c_str());

        MCIERROR err = mciSendString(cmd, NULL, 0, NULL);
        if (err == 0) {
            UpdateBGMVolume();
            mciSendString(L"play bgm repeat", NULL, 0, NULL);
        }
    }

    void StopBGM()
    {
        mciSendString(L"stop bgm", NULL, 0, NULL);
        mciSendString(L"close bgm", NULL, 0, NULL);
    }

    // 1. 코인음 재생
    void PlayCoin() { if (!g_hMidiOut) return; thread([]() { const uint8_t ch = 0; SetInstrument(ch, 9); NoteOn(ch, 71, 127); Sleep(60); NoteOff(ch, 71); NoteOn(ch, 76, 127); Sleep(300); NoteOff(ch, 76); }).detach(); }
    // 2. 점프음 재생
    void PlayJump() { if (!g_hMidiOut) return; thread([]() { const uint8_t ch = 1; SetInstrument(ch, 80); PitchBend(ch, 8192); NoteOn(ch, 60, 110); for (int i = 0; i < 16; ++i) { PitchBend(ch, 8192 + (i * 450)); Sleep(5); } NoteOff(ch, 60); PitchBend(ch, 8192); }).detach(); }
    // 3. 폭발음 재생
    void PlayExplosion() { if (!g_hMidiOut) return; thread([]() { const uint8_t drumCh = 9; const uint8_t fxCh = 2; SetInstrument(fxCh, 127); NoteOn(drumCh, 35, 127); NoteOn(drumCh, 49, 120); NoteOn(fxCh, 48, 127); Sleep(400); NoteOff(drumCh, 35); NoteOff(drumCh, 49); NoteOff(fxCh, 48); }).detach(); }
    // 4. 팡파레 재생
    void PlayFanfare_0() { if (!g_hMidiOut) return; thread([]() { const uint8_t ch = 3; SetInstrument(ch, 56); NoteOn(ch, 60); Sleep(120); NoteOff(ch, 60); Sleep(30); NoteOn(ch, 60); Sleep(120); NoteOff(ch, 60); Sleep(30); NoteOn(ch, 60); Sleep(120); NoteOff(ch, 60); Sleep(30); }).detach(); }
    void PlayFanfare_1() { if (!g_hMidiOut) return; thread([]() { const uint8_t ch = 3; SetInstrument(ch, 56); NoteOn(ch, 65); Sleep(360); NoteOff(ch, 65); Sleep(30); NoteOn(ch, 69); Sleep(360); NoteOff(ch, 69); Sleep(30); }).detach(); }
    void PlayFanfare_2() { if (!g_hMidiOut) return; thread([]() { const uint8_t ch = 3; SetInstrument(ch, 56); NoteOn(ch, 65); Sleep(120); NoteOff(ch, 65); Sleep(30); NoteOn(ch, 69); Sleep(120); NoteOff(ch, 69); Sleep(30); NoteOn(ch, 72); Sleep(600); NoteOff(ch, 72); Sleep(30); }).detach(); }
    void PlayCharging() { if (!g_hMidiOut) return; thread([]() { const uint8_t ch = 5; SetInstrument(ch, 119); NoteOn(ch, 60, 127); Sleep(1500); NoteOff(ch, 60); }).detach(); }
    void PlayScratch() { if (!g_hMidiOut) return; thread([]() { const uint8_t ch = 5; SetInstrument(ch, 120); NoteOn(ch, 75, 127); Sleep(150); NoteOff(ch, 75); }).detach(); }
    void PlayDash() { if (!g_hMidiOut) return; thread([]() { const uint8_t ch = 5; SetInstrument(ch, 121); NoteOn(ch, 60, 100); Sleep(250); NoteOff(ch, 60); }).detach(); }
    void PlayEnergyBeam() { if (!g_hMidiOut) return; thread([]() { const uint8_t ch = 5; SetInstrument(ch, 122); NoteOn(ch, 50, 127); Sleep(1200); NoteOff(ch, 50); }).detach(); }
    void PlayStun() { if (!g_hMidiOut) return; thread([]() { const uint8_t ch = 5; SetInstrument(ch, 123); for (int i = 0; i < 3; ++i) { NoteOn(ch, 72, 110); Sleep(150); NoteOff(ch, 72); Sleep(50); } }).detach(); }
    void PlayAlarm() { if (!g_hMidiOut) return; thread([]() { const uint8_t ch = 5; SetInstrument(ch, 124); PitchBend(ch, 12000); NoteOn(ch, 70, 127); Sleep(800); NoteOff(ch, 70); PitchBend(ch, 8192); }).detach(); }
    void PlayMachineGun() { if (!g_hMidiOut) return; thread([]() { const uint8_t ch = 5; SetInstrument(ch, 125); for (int i = 0; i < 5; ++i) { NoteOn(ch, 45, 127); Sleep(80); NoteOff(ch, 45); Sleep(20); } }).detach(); }
    void PlayCannon() { if (!g_hMidiOut) return; thread([]() { const uint8_t ch = 5; SetInstrument(ch, 126); NoteOn(ch, 40, 127); Sleep(600); NoteOff(ch, 40); }).detach(); }

    // ==========================================================
    // ⭐ 편입된 전용 사운드 함수들의 구현부 (에러 해결 핵심 구역!)
    // ==========================================================

    // [Game C & B] 공용 미디 노트 재생
    void PlayMidiNote(BYTE channel, BYTE note, BYTE velocity, float volumeMultiplier) {
        if (!g_hMidiOut) return;
        uint8_t finalVel = (uint8_t)(velocity * g_masterVolume * g_sfxVolume * volumeMultiplier);
        if (finalVel > 127) finalVel = 127;
        if (finalVel <= 0) return;
        DWORD msg = (finalVel << 16) | (note << 8) | (0x90 | (channel & 0x0F));
        midiOutShortMsg(g_hMidiOut, msg);
    }

    // [Game A] 레트로 삐비빅 사운드
    void PlayRetroBeep(int note, int durationMs) {
        if (!g_hMidiOut) return;
        DWORD msg = 0x00400090 | (note << 8);
        midiOutShortMsg(g_hMidiOut, msg);
    }

    // [Game C] 로봇 점프 BGM 재생용
    void PlayJumpBGMNote(BYTE channel, BYTE note, BYTE velocity) {
        if (!g_hMidiOut) return;
        DWORD msg = (velocity << 16) | (note << 8) | (0x90 | (channel & 0x0F));
        midiOutShortMsg(g_hMidiOut, msg);
    }

    // [Game D] 바둑 슈팅 특수음
    void PlayShootSFX() {
        if (!g_hMidiOut) return;
        DWORD msg = (0x90 | 9) | (75 << 8) | (120 << 16);
        midiOutShortMsg(g_hMidiOut, msg);
    }

    void PlayHitSFX(int combo) {
        if (!g_hMidiOut) return;
        uint8_t note = (uint8_t)(min)(72 + (combo * 2), 96);
        DWORD msg = (0x90 | 1) | (note << 8) | (127 << 16);
        midiOutShortMsg(g_hMidiOut, msg);
    }

    void PlayOutSFX() {
        if (!g_hMidiOut) return;
        thread([]() {
            uint8_t ch = 0;
            uint8_t note = 76;
            SetInstrument(ch, 80);
            PitchBend(ch, 8192);
            DWORD onMsg = (0x90 | ch) | (note << 8) | (100 << 16);
            midiOutShortMsg(g_hMidiOut, onMsg);
            for (int bend = 8192; bend >= 0; bend -= 800) { PitchBend(ch, bend); Sleep(10); }
            DWORD offMsg = (0x80 | ch) | (note << 8) | (0 << 16);
            midiOutShortMsg(g_hMidiOut, offMsg);
            PitchBend(ch, 8192);
        }).detach();
    }
}