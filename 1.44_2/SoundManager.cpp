#include "SoundManager.h"
#include "SoundClip.h"
#include "SoundCoroutine.h"
#include <atomic>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <functional>
#include <condition_variable>

// ==========================================================
// MIDI 전역 변수 (완전 락프리 구조: Mutex 100% 제거)
// ==========================================================
HMIDIOUT g_hMidiOut = nullptr;

using namespace std;

float g_masterVolume = 1.0f;
float g_sfxVolume = 1.0f;
float g_bgmVolume = 1.0f;
bool g_soundInitialized = false;

// ==========================================================
// 저수준 MIDI 송신 구현부 (락프리 커널 드라이버 직접 송출)
// ==========================================================
void UpdateBGMVolume() {
    float finalBgmVol = g_masterVolume * g_bgmVolume;
    int mciVol = (int)(finalBgmVol * 1000.0f);
    wchar_t cmd[128];
    swprintf_s(cmd, 128, L"setaudio bgm volume to %d", mciVol);
    mciSendString(cmd, NULL, 0, NULL);
}

void SetInstrument(uint8_t channel, uint8_t instrument) {
    if (!g_hMidiOut) return;
    DWORD msg = (0xC0 | (channel & 0x0F)) | (instrument << 8);
    midiOutShortMsg(g_hMidiOut, msg);
}

void NoteOn(uint8_t channel, uint8_t note, uint8_t velocity = 127) {
    if (!g_hMidiOut) return;
    uint8_t finalVelocity = (uint8_t)(velocity * g_masterVolume * g_sfxVolume);
    if (finalVelocity > 127) finalVelocity = 127;
    if (finalVelocity <= 0) return;
    DWORD msg = (0x90 | (channel & 0x0F)) | (note << 8) | (finalVelocity << 16);
    midiOutShortMsg(g_hMidiOut, msg);
}

void NoteOff(uint8_t channel, uint8_t note) {
    if (!g_hMidiOut) return;
    DWORD msg = (0x80 | (channel & 0x0F)) | (note << 8);
    midiOutShortMsg(g_hMidiOut, msg);
}

void BgmNoteOn(uint8_t channel, uint8_t note, uint8_t velocity = 127) {
    if (!g_hMidiOut) return;
    if (g_masterVolume <= 0.001f || g_bgmVolume <= 0.001f) return;
    uint8_t finalVelocity = (uint8_t)(velocity * g_masterVolume * g_bgmVolume);
    if (finalVelocity > 127) finalVelocity = 127;
    if (finalVelocity <= 0) return;
    DWORD msg = (0x90 | (channel & 0x0F)) | (note << 8) | (finalVelocity << 16);
    midiOutShortMsg(g_hMidiOut, msg);
}

void BgmNoteOff(uint8_t channel, uint8_t note) {
    if (!g_hMidiOut) return;
    DWORD msg = (0x80 | (channel & 0x0F)) | (note << 8);
    midiOutShortMsg(g_hMidiOut, msg);
}

void PitchBend(uint8_t channel, uint16_t bendValue) {
    if (!g_hMidiOut) return;
    uint8_t lsb = bendValue & 0x7F;
    uint8_t msb = (bendValue >> 7) & 0x7F;
    DWORD msg = (0xE0 | (channel & 0x0F)) | (lsb << 8) | (msb << 16);
    midiOutShortMsg(g_hMidiOut, msg);
}

// ==========================================================
// IMidiDevice 구현체 (음원 객체가 사용할 브릿지)
// ==========================================================
class MidiDeviceBridge : public IMidiDevice {
public:
    void SendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) override {
        NoteOn(channel, note, velocity);
    }
    void SendNoteOff(uint8_t channel, uint8_t note) override {
        NoteOff(channel, note);
    }
    void SendProgramChange(uint8_t channel, uint8_t instrument) override {
        SetInstrument(channel, instrument);
    }
    void SendPitchBend(uint8_t channel, uint16_t bendValue) override {
        PitchBend(channel, bendValue);
    }
} g_midiDevice;

// ==========================================================
// 사운드 음원 등록 레지스트리 (Sound Registry)
// ==========================================================
std::unordered_map<SoundManager::SFXId, std::unique_ptr<ISoundSource>> g_soundRegistry;

static const std::vector<uint8_t> kMovementChannels = { 0, 1, 2 };   // 점프, 대시, 액션, UI
static const std::vector<uint8_t> kProjectileChannels = { 3, 4, 5 }; // 레이저, 투사체, 비프, 조준
static const std::vector<uint8_t> kImpactChannels = { 6, 7, 8 };     // 폭발, 팡파레, 기믹, 빔, 실드

void RegisterSound(SoundManager::SFXId id, std::unique_ptr<ISoundSource> sound) {
    g_soundRegistry[id] = std::move(sound);
}

void InitializeSoundRegistry() {
    g_soundRegistry.clear();

    // 1. 플레이어 이동 및 UI/아이템 (Ch 0, 1, 2 락프리 동적 할당)
    RegisterSound(SoundManager::SFXId::Coin,
        std::make_unique<ArpeggioClip>(kMovementChannels, 9, std::vector<NoteEvent>{ {71, 127, 60, 0}, { 76, 127, 200, 0 } }));
    RegisterSound(SoundManager::SFXId::Scratch,
        std::make_unique<ArpeggioClip>(kMovementChannels, 120, std::vector<NoteEvent>{ {75, 127, 100, 0} }));
    RegisterSound(SoundManager::SFXId::Jump,
        std::make_unique<PitchSweepClip>(kMovementChannels, 80, 60, 110, 8192, 15392, 450, 5));
    RegisterSound(SoundManager::SFXId::OutSFX,
        std::make_unique<PitchSweepClip>(kMovementChannels, 80, 76, 100, 8192, 0, 800, 10));
    RegisterSound(SoundManager::SFXId::Dash,
        std::make_unique<ArpeggioClip>(kMovementChannels, 121, std::vector<NoteEvent>{ {60, 100, 200, 0} }));
    RegisterSound(SoundManager::SFXId::InvemaDash,
        std::make_unique<PolyphonicClip>(kMovementChannels, 55, std::vector<PolyNote>{ {2, 64, 110}, { 9, 39, 100 } }, 80));
    RegisterSound(SoundManager::SFXId::InvemaSwordSlash,
        std::make_unique<PolyphonicClip>(kMovementChannels, 55, std::vector<PolyNote>{ {2, 60, 127}, { 9, 40, 127 }, { 9, 49, 120 } }, 150));
    RegisterSound(SoundManager::SFXId::InvemaItemStar,
        std::make_unique<ArpeggioClip>(kMovementChannels, 9, std::vector<NoteEvent>{ {72, 115, 40, 0}, { 76, 115, 40, 0 }, { 79, 115, 40, 0 }, { 84, 115, 40, 0 }, { 88, 115, 40, 0 } }));

    // 2. 투사체, 레이저, 조준 및 돌진 (Ch 3, 4, 5 락프리 동적 할당)
    RegisterSound(SoundManager::SFXId::InvemaLaser,
        std::make_unique<DualPitchSweepClip>(kProjectileChannels, 81, 72, 84));
    RegisterSound(SoundManager::SFXId::InvemaTargetLock,
        std::make_unique<ArpeggioClip>(kProjectileChannels, 80, std::vector<NoteEvent>{ {91, 110, 35, 25}, { 96, 120, 50, 0 } }));
    RegisterSound(SoundManager::SFXId::InvemaChargeRush,
        std::make_unique<PitchSweepClip>(kProjectileChannels, 81, 68, 127, 5000, 13000, 1100, 10, 40, 110));
    RegisterSound(SoundManager::SFXId::InvemaGimmickWarning,
        std::make_unique<ArpeggioClip>(kProjectileChannels, 80, std::vector<NoteEvent>{ {84, 100, 50, 40}, { 84, 100, 50, 40 }, { 84, 100, 50, 40 } }));
    RegisterSound(SoundManager::SFXId::MachineGun,
        std::make_unique<ArpeggioClip>(kProjectileChannels, 125, std::vector<NoteEvent>{ {45, 127, 60, 20}, { 45, 127, 60, 20 }, { 45, 127, 60, 20 }, { 45, 127, 60, 20 }, { 45, 127, 60, 20 } }));
    RegisterSound(SoundManager::SFXId::Cannon,
        std::make_unique<ArpeggioClip>(kProjectileChannels, 126, std::vector<NoteEvent>{ {40, 127, 400, 0} }));

    // [Game B: 오토배틀러 전용 공격 사운드]
    RegisterSound(SoundManager::SFXId::AutoBattleAttackMelee,
        std::make_unique<PitchSweepClip>(kMovementChannels, 30, 45, 115, 8192, 3500, 1200, 8, 38, 100));
    RegisterSound(SoundManager::SFXId::AutoBattleAttackRanged,
        std::make_unique<PitchSweepClip>(kProjectileChannels, 107, 72, 105, 11000, 4000, 1400, 10, 42, 90));
    RegisterSound(SoundManager::SFXId::AutoBattleMagicCast,
        std::make_unique<ArpeggioClip>(kProjectileChannels, 98, std::vector<NoteEvent>{ {72, 100, 35, 10}, { 76, 110, 40, 10 }, { 81, 115, 60, 0 } }));

    // 3. 충격, 폭발, 방어, 팡파레, 빔 기믹 (Ch 6, 7, 8 락프리 동적 할당)
    RegisterSound(SoundManager::SFXId::Explosion,
        std::make_unique<PolyphonicClip>(kImpactChannels, 127, std::vector<PolyNote>{ {9, 35, 127}, { 9, 49, 120 }, { 8, 48, 127 } }, 350));
    RegisterSound(SoundManager::SFXId::InvemaHexExplosion,
        std::make_unique<PolyphonicClip>(kImpactChannels, 127, std::vector<PolyNote>{ {9, 35, 127}, { 9, 36, 127 }, { 9, 49, 127 }, { 9, 57, 127 }, { 8, 36, 127 } }, 350));
    RegisterSound(SoundManager::SFXId::InvemaDeath,
        std::make_unique<PitchSweepClip>(kImpactChannels, 30, 48, 127, 8192, 3000, 500, 20, 49, 127));
    RegisterSound(SoundManager::SFXId::InvemaShieldDeflect,
        std::make_unique<PolyphonicClip>(kImpactChannels, 14, std::vector<PolyNote>{ {7, 84, 127}, { 9, 81, 120 } }, 200));
    RegisterSound(SoundManager::SFXId::InvemaItemShield,
        std::make_unique<PolyphonicClip>(kImpactChannels, 88, std::vector<PolyNote>{ {7, 60, 110}, { 7, 65, 110 }, { 7, 69, 110 }, { 7, 72, 110 } }, 150));
    RegisterSound(SoundManager::SFXId::Fanfare_0,
        std::make_unique<ArpeggioClip>(kImpactChannels, 56, std::vector<NoteEvent>{ {60, 127, 120, 30}, { 60, 127, 120, 30 }, { 60, 127, 120, 30 } }));
    RegisterSound(SoundManager::SFXId::Fanfare_1,
        std::make_unique<ArpeggioClip>(kImpactChannels, 56, std::vector<NoteEvent>{ {65, 127, 250, 30}, { 69, 127, 250, 30 } }));
    RegisterSound(SoundManager::SFXId::Fanfare_2,
        std::make_unique<ArpeggioClip>(kImpactChannels, 56, std::vector<NoteEvent>{ {65, 127, 120, 30}, { 69, 127, 120, 30 }, { 72, 127, 450, 30 } }));
    RegisterSound(SoundManager::SFXId::Charging,
        std::make_unique<ArpeggioClip>(kImpactChannels, 119, std::vector<NoteEvent>{ {60, 127, 800, 0} }));
    RegisterSound(SoundManager::SFXId::EnergyBeam,
        std::make_unique<ArpeggioClip>(kImpactChannels, 122, std::vector<NoteEvent>{ {50, 127, 800, 0} }));
    RegisterSound(SoundManager::SFXId::Stun,
        std::make_unique<ArpeggioClip>(kImpactChannels, 123, std::vector<NoteEvent>{ {72, 110, 120, 40}, { 72, 110, 120, 40 }, { 72, 110, 120, 40 } }));
    RegisterSound(SoundManager::SFXId::Alarm,
        std::make_unique<PitchSweepClip>(kImpactChannels, 124, 70, 127, 12000, 8192, 400, 40));
    RegisterSound(SoundManager::SFXId::InvemaBeamExtend,
        std::make_unique<PitchSweepClip>(kImpactChannels, 30, 50, 127, 4000, 14000, 1200, 12, 36, 127));
    RegisterSound(SoundManager::SFXId::InvemaBeamImpact,
        std::make_unique<PolyphonicClip>(kImpactChannels, 127, std::vector<PolyNote>{ {9, 35, 127}, { 9, 49, 120 }, { 5, 36, 127 } }, 200));

    // 4. 단발 드럼 및 타악기 (Ch 9 드럼 전용)
    RegisterSound(SoundManager::SFXId::InvemaBulletHit,
        std::make_unique<DrumHitClip>(75, 85, 25));
    RegisterSound(SoundManager::SFXId::InvemaEnemyBounce,
        std::make_unique<DrumHitClip>(76, 80, 40));
    RegisterSound(SoundManager::SFXId::InvemaPillarHit,
        std::make_unique<DrumHitClip>(66, 110, 35, 38, 90));
    RegisterSound(SoundManager::SFXId::InvemaSmallPillarRise,
        std::make_unique<DrumHitClip>(45, 120, 0, 38, 100));
    RegisterSound(SoundManager::SFXId::ShootSFX,
        std::make_unique<DrumHitClip>(75, 120, 0));
}

namespace SoundManager {

    std::atomic<BGMType> g_activeBgmType{ BGMType::None };
    std::atomic<uint32_t> g_bgmGeneration{ 0 };

    // ⭐ 효과음 전용 채널(0~8번)의 피치 및 컨트롤러를 기본값으로 강제 정규화
    void ResetSFXChannels() {
        if (!g_hMidiOut) return;
        for (uint8_t ch = 0; ch <= 8; ++ch) {
            // 1. 피치 벤드 중앙값(8192)으로 원복
            PitchBend(ch, 8192);

            // 2. All Sound Off (CC 120) & Reset All Controllers (CC 121)
            DWORD msgSoundOff = (0xB0 | (ch & 0x0F)) | (120 << 8);
            midiOutShortMsg(g_hMidiOut, msgSoundOff);

            DWORD msgResetCtrl = (0xB0 | (ch & 0x0F)) | (121 << 8);
            midiOutShortMsg(g_hMidiOut, msgResetCtrl);
        }
    }

    void SilenceBgmChannels() {
        if (!g_hMidiOut) return;
        uint8_t bgmChannels[] = { 9, 10, 11, 12 };
        for (uint8_t ch : bgmChannels) {
            for (uint8_t note = 0; note < 128; ++note) {
                DWORD offMsg = (0x80 | (ch & 0x0F)) | (note << 8);
                midiOutShortMsg(g_hMidiOut, offMsg);
            }
            DWORD msgOff = (0xB0 | (ch & 0x0F)) | (120 << 8) | (0 << 16);
            midiOutShortMsg(g_hMidiOut, msgOff);
            DWORD msgNotesOff = (0xB0 | (ch & 0x0F)) | (123 << 8) | (0 << 16);
            midiOutShortMsg(g_hMidiOut, msgNotesOff);
            DWORD msgSustainOff = (0xB0 | (ch & 0x0F)) | (64 << 8) | (0 << 16);
            midiOutShortMsg(g_hMidiOut, msgSustainOff);
            PitchBend(ch, 8192);
        }
    }

    SoundTask BgmSequencerCoroutine(BGMType startType, uint32_t myGeneration) {
        int currentStep = 0;
        BGMType type = startType;
        uint8_t activeBassNote = 0;
        uint8_t activeLeadNote = 0;

        auto InitInstruments = [&](BGMType t) {
            switch (t) {
            case BGMType::Title:    SetInstrument(10, 36); SetInstrument(11, 80); break;
            case BGMType::GameA:    SetInstrument(10, 38); SetInstrument(11, 81); break;
            case BGMType::GameB:    SetInstrument(10, 47); SetInstrument(11, 30); break;
            case BGMType::GameC:    SetInstrument(10, 33); SetInstrument(11, 80); break;
            case BGMType::GameD:    SetInstrument(10, 45); SetInstrument(11, 107); break;
            case BGMType::GameE:    SetInstrument(10, 32); SetInstrument(11, 11); break;
            case BGMType::Settings: SetInstrument(10, 32); SetInstrument(11, 4);  break;
            case BGMType::Ranking:  SetInstrument(10, 39); SetInstrument(11, 61); break;
            default: break;
            }
        };
        InitInstruments(type);

        while (true) {
            if (g_bgmGeneration.load() != myGeneration) {
                if (activeBassNote > 0) BgmNoteOff(10, activeBassNote);
                if (activeLeadNote > 0) BgmNoteOff(11, activeLeadNote);
                co_return;
            }

            if (activeBassNote > 0) { BgmNoteOff(10, activeBassNote); activeBassNote = 0; }
            if (activeLeadNote > 0) { BgmNoteOff(11, activeLeadNote); activeLeadNote = 0; }

            int stepDelay = 125;
            switch (type) {
            case BGMType::Title:
                stepDelay = 125;
                if (currentStep % 4 == 0) BgmNoteOn(9, 36, 110);
                if (currentStep % 8 == 4) BgmNoteOn(9, 40, 100);
                if (currentStep % 2 == 0) BgmNoteOn(9, 42, 70);
                { uint8_t n[] = { 36,36,48,36,41,36,43,36,36,36,48,36,46,45,43,41 }; activeBassNote = n[currentStep]; BgmNoteOn(10, activeBassNote, 100); }
                if (currentStep % 4 == 0) { uint8_t n[] = { 60,63,65,67 }; activeLeadNote = n[(currentStep / 4) % 4]; BgmNoteOn(11, activeLeadNote, 85); }
                break;
            case BGMType::GameA:
                stepDelay = 110;
                if (currentStep % 4 == 0) BgmNoteOn(9, 35, 115);
                if (currentStep % 8 == 4) BgmNoteOn(9, 38, 105);
                if (currentStep % 2 == 0) BgmNoteOn(9, 42, 80);
                { uint8_t n[] = { 33,33,45,33,36,33,38,33,33,33,45,33,41,40,38,36 }; activeBassNote = n[currentStep]; BgmNoteOn(10, activeBassNote, 110); }
                if (currentStep == 0 || currentStep == 3 || currentStep == 6 || currentStep == 10 || currentStep == 12) { uint8_t n[] = { 69,72,74,76,77 }; activeLeadNote = n[(currentStep / 3) % 5]; BgmNoteOn(11, activeLeadNote, 90); }
                break;
            case BGMType::GameB:
                stepDelay = 135;
                if (currentStep % 4 == 0) BgmNoteOn(9, 36, 120);
                if (currentStep % 8 == 4) BgmNoteOn(9, 38, 110);
                if (currentStep % 2 == 0) BgmNoteOn(9, 46, 75);
                if (currentStep % 4 == 0) { uint8_t n[] = { 36,38,41,43 }; activeBassNote = n[(currentStep / 4) % 4]; BgmNoteOn(10, activeBassNote, 105); }
                if (currentStep == 0 || currentStep == 6 || currentStep == 10) { uint8_t n[] = { 53,57,60 }; activeLeadNote = n[(currentStep / 4) % 3]; BgmNoteOn(11, activeLeadNote, 95); }
                break;
            case BGMType::GameC:
                stepDelay = 100;
                if (currentStep % 4 == 0) BgmNoteOn(9, 35, 105);
                if (currentStep % 8 == 4) BgmNoteOn(9, 40, 95);
                if (currentStep % 2 == 1) BgmNoteOn(9, 42, 65);
                { uint8_t n[] = { 48,48,55,48,52,48,53,55,48,48,55,48,57,55,53,52 }; activeBassNote = n[currentStep]; BgmNoteOn(10, activeBassNote, 95); }
                { uint8_t n[] = { 60,64,67,72,71,67,64,62,59,62,67,71,69,67,64,60 }; activeLeadNote = n[currentStep]; BgmNoteOn(11, activeLeadNote, 85); }
                break;
            case BGMType::GameD:
                stepDelay = 130;
                if (currentStep % 4 == 0) BgmNoteOn(9, 36, 100);
                if (currentStep % 8 == 4) BgmNoteOn(9, 38, 90);
                if (currentStep % 2 == 0) BgmNoteOn(9, 42, 70);
                { uint8_t n[] = { 45,45,48,45,50,45,52,45,45,45,48,45,53,52,50,48 }; activeBassNote = n[currentStep]; BgmNoteOn(10, activeBassNote, 90); }
                if (currentStep % 2 == 0) { uint8_t n[] = { 69,71,72,76,79,81,84,86 }; activeLeadNote = n[(currentStep / 2) % 8]; BgmNoteOn(11, activeLeadNote, 85); }
                break;
            case BGMType::GameE:
                stepDelay = 140;
                if (currentStep % 4 == 0) BgmNoteOn(9, 35, 95);
                if (currentStep % 8 == 4) BgmNoteOn(9, 40, 85);
                if (currentStep % 4 == 2) BgmNoteOn(9, 42, 60);
                { uint8_t n[] = { 36,40,43,45,48,45,43,40,36,40,43,45,48,45,43,40 }; activeBassNote = n[currentStep]; BgmNoteOn(10, activeBassNote, 85); }
                if (currentStep % 2 == 0) { uint8_t n[] = { 60,64,67,71,72,71,67,64 }; activeLeadNote = n[(currentStep / 2) % 8]; BgmNoteOn(11, activeLeadNote, 80); }
                break;
            case BGMType::Settings:
                stepDelay = 160;
                if (currentStep % 8 == 0) BgmNoteOn(9, 35, 80);
                if (currentStep % 8 == 4) BgmNoteOn(9, 42, 55);
                { uint8_t n[] = { 36,0,43,0,41,0,40,0,36,0,43,0,45,0,43,0 }; if (n[currentStep] > 0) { activeBassNote = n[currentStep]; BgmNoteOn(10, activeBassNote, 75); } }
                if (currentStep % 4 == 0) { uint8_t n[] = { 60,64,65,67 }; activeLeadNote = n[(currentStep / 4) % 4]; BgmNoteOn(11, activeLeadNote, 70); }
                break;
            case BGMType::Ranking:
                stepDelay = 120;
                if (currentStep % 4 == 0) BgmNoteOn(9, 36, 115);
                if (currentStep % 8 == 4) BgmNoteOn(9, 38, 105);
                if (currentStep % 2 == 0) BgmNoteOn(9, 49, 70);
                { uint8_t n[] = { 48,48,51,48,53,48,55,48,48,48,51,48,56,55,53,51 }; activeBassNote = n[currentStep]; BgmNoteOn(10, activeBassNote, 100); }
                if (currentStep % 4 == 0) { uint8_t n[] = { 60,63,67,72 }; activeLeadNote = n[(currentStep / 4) % 4]; BgmNoteOn(11, activeLeadNote, 90); }
                break;
            default: break;
            }

            int gateTime = (stepDelay * 3) / 4;
            int releaseTime = stepDelay - gateTime;

            co_await SoundDelay{ gateTime };

            if (activeBassNote > 0) { BgmNoteOff(10, activeBassNote); activeBassNote = 0; }
            if (activeLeadNote > 0) { BgmNoteOff(11, activeLeadNote); activeLeadNote = 0; }

            if (g_bgmGeneration.load() != myGeneration) co_return;

            co_await SoundDelay{ releaseTime };

            if (g_bgmGeneration.load() != myGeneration) co_return;

            currentStep = (currentStep + 1) % 16;
        }
    }

    void Initialize() {
        if (g_hMidiOut == nullptr) {
            midiOutOpen(&g_hMidiOut, MIDI_MAPPER, 0, 0, CALLBACK_NULL);
        }
        if (!g_soundInitialized) {
            g_soundInitialized = true;
            g_masterVolume = 1.0f;
            g_sfxVolume = 1.0f;
            g_bgmVolume = 1.0f;
            AudioScheduler::Initialize();
            InitializeSoundRegistry();
        }
    }

    void SetMasterVolume(float volume) {
        g_masterVolume = clamp(volume, 0.0f, 1.0f);
        UpdateBGMVolume();
        if (g_masterVolume <= 0.001f || g_bgmVolume <= 0.001f) {
            SilenceBgmChannels();
        }
    }

    void SetSFXVolume(float volume) {
        g_sfxVolume = clamp(volume, 0.0f, 1.0f);
    }

    void SetBGMVolume(float volume) {
        g_bgmVolume = clamp(volume, 0.0f, 1.0f);
        UpdateBGMVolume();
        if (g_masterVolume <= 0.001f || g_bgmVolume <= 0.001f) {
            SilenceBgmChannels();
        }
    }

    void PlaySceneBGM(BGMType type) {
        if (g_activeBgmType.load() == type) return;

        uint32_t newGen = ++g_bgmGeneration;
        g_activeBgmType = type;
        SilenceBgmChannels();
        AudioScheduler::WakeAll();

        BgmSequencerCoroutine(type, newGen);
    }

    void StopSceneBGM() {
        if (g_activeBgmType.load() == BGMType::None) return;

        ++g_bgmGeneration;
        g_activeBgmType = BGMType::None;
        SilenceBgmChannels();
        AudioScheduler::WakeAll();
    }

    void Release()
    {
        ++g_bgmGeneration;
        g_activeBgmType = BGMType::None;

        AudioScheduler::Release();
        SilenceBgmChannels();
        ResetSFXChannels(); // SFX 채널도 함께 초기화
        g_soundRegistry.clear();
        ChannelAllocator::ResetAll();

        if (g_hMidiOut != nullptr)
        {
            midiOutClose(g_hMidiOut);
            g_hMidiOut = nullptr;
        }
    }

    void StopBGM()
    {
        StopSceneBGM();
        mciSendString(L"stop bgm", NULL, 0, NULL);
        mciSendString(L"close bgm", NULL, 0, NULL);
    }

    void StopAllSounds()
    {
        ++g_bgmGeneration;
        g_activeBgmType = BGMType::None;
        SilenceBgmChannels();

        AudioScheduler::CancelAll();
        ChannelAllocator::ResetAll();
        ResetSFXChannels(); // 잔류 효과음 피치 원복

        mciSendString(L"stop bgm", NULL, 0, NULL);
        if (!g_hMidiOut) return;
        for (uint8_t ch = 0; ch < 16; ++ch) {
            for (uint8_t note = 0; note < 128; ++note) {
                DWORD offMsg = (0x80 | (ch & 0x0F)) | (note << 8);
                midiOutShortMsg(g_hMidiOut, offMsg);
            }
            DWORD msgOff = (0xB0 | (ch & 0x0F)) | (120 << 8) | (0 << 16);
            midiOutShortMsg(g_hMidiOut, msgOff);
            DWORD msgNotesOff = (0xB0 | (ch & 0x0F)) | (123 << 8) | (0 << 16);
            midiOutShortMsg(g_hMidiOut, msgNotesOff);
            DWORD msgSustainOff = (0xB0 | (ch & 0x0F)) | (64 << 8) | (0 << 16);
            midiOutShortMsg(g_hMidiOut, msgSustainOff);
            PitchBend(ch, 8192);
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

    void Play(SFXId id) {
        auto it = g_soundRegistry.find(id);
        if (it == g_soundRegistry.end() || !it->second) return;

        it->second->Play(g_midiDevice);
    }

    void PlayCoin() { Play(SFXId::Coin); }
    void PlayScratch() { Play(SFXId::Scratch); }
    void PlayJump() { Play(SFXId::Jump); }
    void PlayOutSFX() { Play(SFXId::OutSFX); }
    void PlayDash() { Play(SFXId::Dash); }
    void PlayExplosion() { Play(SFXId::Explosion); }
    void PlayFanfare_0() { Play(SFXId::Fanfare_0); }
    void PlayFanfare_1() { Play(SFXId::Fanfare_1); }
    void PlayFanfare_2() { Play(SFXId::Fanfare_2); }
    void PlayCharging() { Play(SFXId::Charging); }
    void PlayEnergyBeam() { Play(SFXId::EnergyBeam); }
    void PlayStun() { Play(SFXId::Stun); }
    void PlayAlarm() { Play(SFXId::Alarm); }
    void PlayMachineGun() { Play(SFXId::MachineGun); }
    void PlayCannon() { Play(SFXId::Cannon); }

    void PlayInvemaLaser() { Play(SFXId::InvemaLaser); }
    void PlayInvemaBulletHit() { Play(SFXId::InvemaBulletHit); }
    void PlayInvemaEnemyBounce() { Play(SFXId::InvemaEnemyBounce); }
    void PlayInvemaPillarHit() { Play(SFXId::InvemaPillarHit); }
    void PlayInvemaDash() { Play(SFXId::InvemaDash); }
    void PlayInvemaSwordSlash() { Play(SFXId::InvemaSwordSlash); }
    void PlayInvemaShieldDeflect() { Play(SFXId::InvemaShieldDeflect); }
    void PlayInvemaItemStar() { Play(SFXId::InvemaItemStar); }
    void PlayInvemaItemShield() { Play(SFXId::InvemaItemShield); }
    void PlayInvemaHexExplosion() { Play(SFXId::InvemaHexExplosion); }
    void PlayInvemaDeath() { Play(SFXId::InvemaDeath); }
    void PlayInvemaGimmickWarning() { Play(SFXId::InvemaGimmickWarning); }
    void PlayInvemaBeamExtend() { Play(SFXId::InvemaBeamExtend); }
    void PlayInvemaBeamImpact() { Play(SFXId::InvemaBeamImpact); }
    void PlayInvemaSmallPillarRise() { Play(SFXId::InvemaSmallPillarRise); }
    void PlayInvemaTargetLock() { Play(SFXId::InvemaTargetLock); }
    void PlayInvemaChargeRush() { Play(SFXId::InvemaChargeRush); }
    void PlayShootSFX() { Play(SFXId::ShootSFX); }

    void PlayAutoBattleAttackMelee() { Play(SFXId::AutoBattleAttackMelee); }
    void PlayAutoBattleAttackRanged() { Play(SFXId::AutoBattleAttackRanged); }
    void PlayAutoBattleMagicCast() { Play(SFXId::AutoBattleMagicCast); }

    // ⭐ 반납 직전 피치를 항상 기본값(8192)으로 돌려놓도록 수정
    static SoundTask AutoNoteOffTask(uint8_t ch, uint8_t note, uint8_t vel, int delayMs) {
        NoteOn(ch, note, vel);
        co_await SoundDelay{ delayMs };
        NoteOff(ch, note);
        PitchBend(ch, 8192); // 피치 중립 복원
        ChannelAllocator::ReleaseChannel(ch);
        co_return;
    }

    void PlayHitSFX(int combo) {
        uint8_t note = (uint8_t)(min)(72 + (combo * 2), 96);
        uint8_t ch = ChannelAllocator::AcquireChannel(kMovementChannels);
        PitchBend(ch, 8192); // 진입 전 피치 상태 보정
        AutoNoteOffTask(ch, note, 127, 100);
    }

    void PlayMidiNote(BYTE channel, BYTE note, BYTE velocity, float volumeMultiplier) {
        if (!g_hMidiOut) return;
        if (channel == 9) {
            NoteOn(9, note, (uint8_t)(velocity * volumeMultiplier));
        }
        else {
            PitchBend(channel, 8192);
            AutoNoteOffTask(channel, note, (uint8_t)(velocity * volumeMultiplier), 80);
        }
    }

    void PlayRetroBeep(int note, int durationMs) {
        if (!g_hMidiOut) return;
        uint8_t ch = ChannelAllocator::AcquireChannel(kMovementChannels);
        PitchBend(ch, 8192); // 획득 시 피치 보정
        AutoNoteOffTask(ch, (uint8_t)note, 75, durationMs > 0 ? durationMs : 60);
    }

    void PlayJumpBGMNote(BYTE channel, BYTE note, BYTE velocity) {
        if (!g_hMidiOut) return;
        PitchBend(channel, 8192);
        AutoNoteOffTask(channel, note, velocity, 100);
    }
}