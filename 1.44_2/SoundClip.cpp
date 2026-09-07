#include "SoundClip.h"
#include <array>

// ==========================================================
// 락프리 동적 채널 보이스 할당기 구현 (Ch 0 ~ 8 SFX 전용)
// ==========================================================
namespace ChannelAllocator {
    struct ChannelVoiceState {
        std::atomic<bool> isBusy{ false };
    };

    static std::array<ChannelVoiceState, 16> g_channelVoices;
    static std::atomic<uint32_t> g_fallbackIndex{ 0 };

    uint8_t AcquireChannel(const std::vector<uint8_t>& pool) {
        if (pool.empty()) return 0;

        // 1. 풀 내에서 현재 비어 있는(idle) 채널 원자적 검색 및 점유
        for (uint8_t ch : pool) {
            if (ch >= 16) continue;
            bool expected = false;
            if (g_channelVoices[ch].isBusy.compare_exchange_strong(expected, true)) {
                return ch; // 락 없이 빈 채널 즉시 획득!
            }
        }

        // 2. 만약 풀 내의 모든 채널이 연주 중이라면 라운드로빈 방식으로 할당
        uint32_t idx = g_fallbackIndex.fetch_add(1);
        return pool[idx % pool.size()];
    }

    void ReleaseChannel(uint8_t channel) {
        if (channel < 16) {
            g_channelVoices[channel].isBusy.store(false); // 사운드 종료 시 원자적 반납!
        }
    }

    void ResetAll() {
        for (auto& v : g_channelVoices) {
            v.isBusy.store(false);
        }
    }
}

// ==========================================================
// 1. DrumHitClip (타악기 단발)
// ==========================================================
DrumHitClip::DrumHitClip(uint8_t note, uint8_t velocity, DWORD debounceMs, uint8_t extraNote, uint8_t extraVel)
    : m_note(note), m_velocity(velocity), m_debounceMs(debounceMs), m_lastPlayTime(0),
      m_extraNote(extraNote), m_extraVel(extraVel) {}

SoundTask DrumHitClip::Play(IMidiDevice& device) {
    if (m_debounceMs > 0) {
        DWORD now = GetTickCount();
        if (now - m_lastPlayTime < m_debounceMs) co_return;
        m_lastPlayTime = now;
    }
    device.SendNoteOn(9, m_note, m_velocity);
    if (m_extraNote > 0 && m_extraVel > 0) {
        device.SendNoteOn(9, m_extraNote, m_extraVel);
    }
    co_return;
}

// ==========================================================
// 2. PitchSweepClip (락프리 동적 채널 피치 스윕)
// ==========================================================
PitchSweepClip::PitchSweepClip(std::vector<uint8_t> channels, uint8_t instrument, uint8_t note, uint8_t velocity,
                               uint16_t startBend, uint16_t endBend, int bendStep, int stepDelayMs,
                               uint8_t extraDrumNote, uint8_t extraDrumVel)
    : m_channels(std::move(channels)), m_instrument(instrument), m_note(note), m_velocity(velocity),
      m_startBend(startBend), m_endBend(endBend), m_bendStep(bendStep), m_stepDelayMs(stepDelayMs),
      m_extraDrumNote(extraDrumNote), m_extraDrumVel(extraDrumVel) {
    if (m_channels.empty()) m_channels.push_back(0);
}

PitchSweepClip::PitchSweepClip(uint8_t singleChannel, uint8_t instrument, uint8_t note, uint8_t velocity,
                               uint16_t startBend, uint16_t endBend, int bendStep, int stepDelayMs,
                               uint8_t extraDrumNote, uint8_t extraDrumVel)
    : PitchSweepClip(std::vector<uint8_t>{ singleChannel }, instrument, note, velocity, startBend, endBend, bendStep, stepDelayMs, extraDrumNote, extraDrumVel) {}

SoundTask PitchSweepClip::Play(IMidiDevice& device) {
    uint8_t channel = ChannelAllocator::AcquireChannel(m_channels);

    device.SendProgramChange(channel, m_instrument);
    device.SendPitchBend(channel, m_startBend);
    device.SendNoteOn(channel, m_note, m_velocity);

    if (m_extraDrumNote > 0 && m_extraDrumVel > 0) {
        device.SendNoteOn(9, m_extraDrumNote, m_extraDrumVel);
    }

    if (m_startBend < m_endBend) {
        for (int b = m_startBend; b <= m_endBend; b += m_bendStep) {
            device.SendPitchBend(channel, (uint16_t)b);
            co_await SoundDelay{ m_stepDelayMs };
        }
    } else {
        for (int b = m_startBend; b >= m_endBend; b -= m_bendStep) {
            device.SendPitchBend(channel, (uint16_t)b);
            co_await SoundDelay{ m_stepDelayMs };
        }
    }

    device.SendNoteOff(channel, m_note);
    device.SendPitchBend(channel, 8192); // 항상 기준 피치(8192) 복구
    ChannelAllocator::ReleaseChannel(channel);
    co_return;
}

// ==========================================================
// 3. ArpeggioClip (락프리 동적 채널 멜로디/아르페지오)
// ==========================================================
ArpeggioClip::ArpeggioClip(std::vector<uint8_t> channels, uint8_t instrument, std::vector<NoteEvent> events)
    : m_channels(std::move(channels)), m_instrument(instrument), m_events(std::move(events)) {
    if (m_channels.empty()) m_channels.push_back(0);
}

ArpeggioClip::ArpeggioClip(uint8_t singleChannel, uint8_t instrument, std::vector<NoteEvent> events)
    : ArpeggioClip(std::vector<uint8_t>{ singleChannel }, instrument, std::move(events)) {}

SoundTask ArpeggioClip::Play(IMidiDevice& device) {
    uint8_t channel = ChannelAllocator::AcquireChannel(m_channels);

    device.SendProgramChange(channel, m_instrument);
    for (const auto& ev : m_events) {
        device.SendNoteOn(channel, ev.note, ev.velocity);
        co_await SoundDelay{ ev.durationMs };
        device.SendNoteOff(channel, ev.note);
        if (ev.pauseMs > 0) {
            co_await SoundDelay{ ev.pauseMs };
        }
    }
    ChannelAllocator::ReleaseChannel(channel);
    co_return;
}

// ==========================================================
// 4. PolyphonicClip (락프리 동적 채널 복합/동시 타격)
// ==========================================================
PolyphonicClip::PolyphonicClip(std::vector<uint8_t> channels, uint8_t instrument, std::vector<PolyNote> notes, int durationMs)
    : m_channels(std::move(channels)), m_instrument(instrument), m_notes(std::move(notes)), m_durationMs(durationMs) {
    if (m_channels.empty()) m_channels.push_back(0);
}

PolyphonicClip::PolyphonicClip(uint8_t singleChannel, uint8_t instrument, std::vector<PolyNote> notes, int durationMs)
    : PolyphonicClip(std::vector<uint8_t>{ singleChannel }, instrument, std::move(notes), durationMs) {}

SoundTask PolyphonicClip::Play(IMidiDevice& device) {
    uint8_t baseChannel = ChannelAllocator::AcquireChannel(m_channels);

    if (m_instrument > 0) {
        device.SendProgramChange(baseChannel, m_instrument);
    }
    for (const auto& pn : m_notes) {
        uint8_t targetCh = (pn.channel == 9) ? 9 : baseChannel;
        device.SendNoteOn(targetCh, pn.note, pn.velocity);
    }
    co_await SoundDelay{ m_durationMs };
    for (const auto& pn : m_notes) {
        uint8_t targetCh = (pn.channel == 9) ? 9 : baseChannel;
        device.SendNoteOff(targetCh, pn.note);
    }
    ChannelAllocator::ReleaseChannel(baseChannel);
    co_return;
}

// ==========================================================
// 5. DualPitchSweepClip (복합 지이잉~쫙 레이저 클립)
// ==========================================================
DualPitchSweepClip::DualPitchSweepClip(std::vector<uint8_t> channels, uint8_t instrument, uint8_t chargeNote, uint8_t zapNote)
    : m_channels(std::move(channels)), m_instrument(instrument), m_chargeNote(chargeNote), m_zapNote(zapNote) {
    if (m_channels.empty()) m_channels.push_back(3);
}

SoundTask DualPitchSweepClip::Play(IMidiDevice& device) {
    uint8_t channel = ChannelAllocator::AcquireChannel(m_channels);

    device.SendProgramChange(channel, m_instrument);

    // [Phase 1: 지이잉~ (충전/상승 스윕) ~60ms]
    // 베이스 피치 4000에서 13000으로 빠르게 상승하며 에너지가 응축되는 소리
    device.SendPitchBend(channel, 4000);
    device.SendNoteOn(channel, m_chargeNote, 105);

    for (int b = 4000; b <= 13000; b += 2250) {
        device.SendPitchBend(channel, (uint16_t)b);
        co_await SoundDelay{ 15 };
    }
    device.SendNoteOff(channel, m_chargeNote);

    // [Phase 2: 쫙! (순간 방출/초고속 하강 스윕 + 스네어 타격) ~100ms]
    // 15000에서 2000으로 급격히 떨어지며 타격감 있는 드럼과 함께 쫙 뻗어나가는 소리
    device.SendPitchBend(channel, 15000);
    device.SendNoteOn(channel, m_zapNote, 127);
    device.SendNoteOn(9, 40, 115); // 일렉트릭 스네어 샷 동시 타격

    for (int b = 15000; b >= 2000; b -= 2600) {
        device.SendPitchBend(channel, (uint16_t)b);
        co_await SoundDelay{ 20 };
    }

    device.SendNoteOff(channel, m_zapNote);
    device.SendPitchBend(channel, 8192); // 항상 기준 피치(8192) 복구
    ChannelAllocator::ReleaseChannel(channel);
    co_return;
}

// ==========================================================
// 6. CustomSfxClip (커스텀 액션 클립)
// ==========================================================
CustomSfxClip::CustomSfxClip(std::function<SoundTask(IMidiDevice&)> action)
    : m_action(std::move(action)) {}

SoundTask CustomSfxClip::Play(IMidiDevice& device) {
    if (m_action) {
        m_action(device);
    }
    co_return;
}
