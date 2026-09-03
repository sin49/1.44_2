#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <cstdint>
#include <filesystem>
#include <thread>
#include <algorithm>

namespace SoundManager {
    void Initialize();
    void Release();

    void SetMasterVolume(float volume);
    void SetSFXVolume(float volume);
    void SetBGMVolume(float volume);

    void PlayBGM(const wchar_t* filename);
    void StopBGM();

    void PlayNote(int instrument, int note);
    void PlayCoin();
    void PlayJump();
    void PlayExplosion();
    void PlayFanfare_0();
    void PlayFanfare_1();
    void PlayFanfare_2();
    void PlayCharging();
    void PlayScratch();
    void PlayDash();
    void PlayEnergyBeam();
    void PlayStun();
    void PlayAlarm();
    void PlayMachineGun();
    void PlayCannon();

    // ⭐ 각 게임에서 편입된 전용 사운드 함수들
    void PlayMidiNote(BYTE channel, BYTE note, BYTE velocity, float volumeMultiplier = 1.0f);
    void PlayRetroBeep(int note, int durationMs = 100);
    void PlayJumpBGMNote(BYTE channel, BYTE note, BYTE velocity);
    void PlayShootSFX();
    void PlayHitSFX(int combo);
    void PlayOutSFX();
}