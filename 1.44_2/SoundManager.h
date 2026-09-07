#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <cstdint>
#include <filesystem>
#include <thread>
#include <algorithm>
#include "SoundClip.h"

namespace SoundManager {
    enum class BGMType {
        None,
        Title,
        GameA,
        GameB,
        GameC,
        GameD,
        GameE,
        Settings,
        Ranking
    };

    enum class SFXId {
        Coin,
        Jump,
        Explosion,
        Fanfare_0,
        Fanfare_1,
        Fanfare_2,
        Charging,
        Scratch,
        Dash,
        EnergyBeam,
        Stun,
        Alarm,
        MachineGun,
        Cannon,
        InvemaLaser,
        InvemaBulletHit,
        InvemaEnemyBounce,
        InvemaPillarHit,
        InvemaDash,
        InvemaSwordSlash,
        InvemaShieldDeflect,
        InvemaItemStar,
        InvemaItemShield,
        InvemaHexExplosion,
        InvemaDeath,
        InvemaGimmickWarning,
        InvemaBeamExtend,
        InvemaBeamImpact,
        InvemaSmallPillarRise,
        InvemaTargetLock,
        InvemaChargeRush,
        ShootSFX,
        OutSFX,
        AutoBattleAttackMelee,
        AutoBattleAttackRanged,
        AutoBattleMagicCast
    };

    void Initialize();
    void Release();

    void SetMasterVolume(float volume);
    void SetSFXVolume(float volume);
    void SetBGMVolume(float volume);

    // 통합 음원 객체 재생 API
    void Play(SFXId id);
    void PlaySceneBGM(BGMType type);
    void StopSceneBGM();
    void StopAllSounds();
    void ResetSFXChannels(); // ⭐ 피치 벤드/컨트롤러 오염 강제 복구 API 추가
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

    // [Game A: INVEMA 전용 풍성한 미디 효과음]
    void PlayInvemaLaser();
    void PlayInvemaBulletHit();
    void PlayInvemaEnemyBounce();
    void PlayInvemaPillarHit();
    void PlayInvemaDash();
    void PlayInvemaSwordSlash();
    void PlayInvemaShieldDeflect();
    void PlayInvemaItemStar();
    void PlayInvemaItemShield();
    void PlayInvemaHexExplosion();
    void PlayInvemaDeath();
    void PlayInvemaGimmickWarning();
    void PlayInvemaBeamExtend();
    void PlayInvemaBeamImpact();
    void PlayInvemaSmallPillarRise();
    void PlayInvemaTargetLock();
    void PlayInvemaChargeRush();

    // [Game B: AutoBattle 전용 효과음]
    void PlayAutoBattleAttackMelee();
    void PlayAutoBattleAttackRanged();
    void PlayAutoBattleMagicCast();
}