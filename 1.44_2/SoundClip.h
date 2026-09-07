#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include "SoundCoroutine.h"

// 사운드 매니저 저수준 MIDI 송신 인터페이스
class IMidiDevice {
public:
    virtual ~IMidiDevice() = default;
    virtual void SendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
    virtual void SendNoteOff(uint8_t channel, uint8_t note) = 0;
    virtual void SendProgramChange(uint8_t channel, uint8_t instrument) = 0;
    virtual void SendPitchBend(uint8_t channel, uint16_t bendValue) = 0;
};

// ==========================================================
// 락프리 동적 채널 보이스 할당기 (Lock-Free Channel Allocator)
// ==========================================================
namespace ChannelAllocator {
    uint8_t AcquireChannel(const std::vector<uint8_t>& pool);
    void ReleaseChannel(uint8_t channel);
    void ResetAll();
}

// ==========================================================
// 사운드 음원(Sound Clip) 기본 인터페이스
// ==========================================================
class ISoundSource {
public:
    virtual ~ISoundSource() = default;
    virtual SoundTask Play(IMidiDevice& device) = 0;
};

// 1. 단발성 타악기/드럼 클립 (예: 탄환 타격, 통통 튀기, 벽 충돌)
class DrumHitClip : public ISoundSource {
private:
    uint8_t m_note;
    uint8_t m_velocity;
    uint8_t m_extraNote;
    uint8_t m_extraVel;
    DWORD m_debounceMs;
    DWORD m_lastPlayTime;

public:
    DrumHitClip(uint8_t note, uint8_t velocity = 100, DWORD debounceMs = 0, uint8_t extraNote = 0, uint8_t extraVel = 0);
    SoundTask Play(IMidiDevice& device) override;
};

// 2. 동적 채널 락프리 피치 스윕 효과음 (예: 레이저, 점프, 아웃, 돌진 빔, 사망)
class PitchSweepClip : public ISoundSource {
private:
    std::vector<uint8_t> m_channels;
    uint8_t m_instrument;
    uint8_t m_note;
    uint8_t m_velocity;
    uint16_t m_startBend;
    uint16_t m_endBend;
    int m_bendStep;
    int m_stepDelayMs;
    uint8_t m_extraDrumNote;
    uint8_t m_extraDrumVel;

public:
    PitchSweepClip(std::vector<uint8_t> channels, uint8_t instrument, uint8_t note, uint8_t velocity,
                   uint16_t startBend, uint16_t endBend, int bendStep, int stepDelayMs,
                   uint8_t extraDrumNote = 0, uint8_t extraDrumVel = 0);
    PitchSweepClip(uint8_t singleChannel, uint8_t instrument, uint8_t note, uint8_t velocity,
                   uint16_t startBend, uint16_t endBend, int bendStep, int stepDelayMs,
                   uint8_t extraDrumNote = 0, uint8_t extraDrumVel = 0);
    SoundTask Play(IMidiDevice& device) override;
};

// 3. 동적 채널 락프리 멜로디/아르페지오 클립 (예: 코인, 아이템 획득, 팡파레)
struct NoteEvent {
    uint8_t note;
    uint8_t velocity;
    int durationMs;
    int pauseMs;
};

class ArpeggioClip : public ISoundSource {
private:
    std::vector<uint8_t> m_channels;
    uint8_t m_instrument;
    std::vector<NoteEvent> m_events;

public:
    ArpeggioClip(std::vector<uint8_t> channels, uint8_t instrument, std::vector<NoteEvent> events);
    ArpeggioClip(uint8_t singleChannel, uint8_t instrument, std::vector<NoteEvent> events);
    SoundTask Play(IMidiDevice& device) override;
};

// 4. 복합/다중 동시 타격 클립 (예: 폭발, 육각형 적 폭발, 빔 지면 격돌)
struct PolyNote {
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
};

class PolyphonicClip : public ISoundSource {
private:
    std::vector<uint8_t> m_channels;
    uint8_t m_instrument;
    std::vector<PolyNote> m_notes;
    int m_durationMs;

public:
    PolyphonicClip(std::vector<uint8_t> channels, uint8_t instrument, std::vector<PolyNote> notes, int durationMs = 300);
    PolyphonicClip(uint8_t singleChannel, uint8_t instrument, std::vector<PolyNote> notes, int durationMs = 300);
    SoundTask Play(IMidiDevice& device) override;
};

// 5. 복합 지이잉~쫙 레이저 클립 (Dual-Phase Laser Zap Clip)
class DualPitchSweepClip : public ISoundSource {
private:
    std::vector<uint8_t> m_channels;
    uint8_t m_instrument;
    uint8_t m_chargeNote;
    uint8_t m_zapNote;

public:
    DualPitchSweepClip(std::vector<uint8_t> channels, uint8_t instrument = 81, uint8_t chargeNote = 72, uint8_t zapNote = 84);
    SoundTask Play(IMidiDevice& device) override;
};

// 6. 커스텀 액션 사운드 클립
class CustomSfxClip : public ISoundSource {
private:
    std::function<SoundTask(IMidiDevice&)> m_action;

public:
    CustomSfxClip(std::function<SoundTask(IMidiDevice&)> action);
    SoundTask Play(IMidiDevice& device) override;
};
