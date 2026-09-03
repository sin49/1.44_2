#include "SoundManager.h"

// 미디 출력
HMIDIOUT g_hMidiOut = nullptr;

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
    uint8_t finalVelocity = (uint8_t)(velocity * g_masterVolume*g_sfxVolume);

    if (finalVelocity > 127) finalVelocity = 127;
    DWORD msg = (0x90 | (channel & 0x0F)) | (note << 8) | (finalVelocity << 16);
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

namespace SoundManager{
	
    
    
    
    void Initialize() {
	//윈도우 기본 미디 오픈
	midiOutOpen(&g_hMidiOut,MIDI_MAPPER,0,0,CALLBACK_NULL);
}




    void SetMasterVolume(float volume) {
        g_masterVolume = std::clamp(volume, 0.0f, 1.0f);
        UpdateBGMVolume(); 
    }

    void SetSFXVolume(float volume) {
        g_sfxVolume = std::clamp(volume, 0.0f, 1.0f);
    }
    
    void SetBGMVolume(float volume) {
        g_bgmVolume = std::clamp(volume, 0.0f, 1.0f);
        UpdateBGMVolume(); 
    }
    void Release()
    {
        if (g_hMidiOut != nullptr)
        {
            midiOutClose(g_hMidiOut); // 장치 닫기
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

         std::filesystem::path basePath = exePath;
         std::filesystem::path assetPath = basePath.parent_path()/L"BGM"/filename;


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
    void PlayCoin()
    {
        if (!g_hMidiOut) return;


         std::thread([]() {
            const uint8_t ch = 0;
            SetInstrument(ch, 9);
            NoteOn(ch, 71, 127);
            Sleep(60); 
            NoteOff(ch, 71);
            NoteOn(ch, 76, 127);
            Sleep(300);
            NoteOff(ch, 76);
        }).detach(); 
    }

    // 2. 점프음 재생
    void PlayJump()
    {
        if (!g_hMidiOut) return;

         std::thread([]() {
            const uint8_t ch = 1;
            SetInstrument(ch, 80);
            PitchBend(ch, 8192);
            NoteOn(ch, 60, 110);
            for (int i = 0; i < 16; ++i) {
                PitchBend(ch, 8192 + (i * 450));
                Sleep(5);
            }
            NoteOff(ch, 60);
            PitchBend(ch, 8192);
        }).detach();
    }

    // 3. 폭발음 재생
    void PlayExplosion()
    {
        if (!g_hMidiOut) return;

         std::thread([]() {
            const uint8_t drumCh = 9;
            const uint8_t fxCh = 2;
            SetInstrument(fxCh, 127);
            NoteOn(drumCh, 35, 127);
            NoteOn(drumCh, 49, 120);
            NoteOn(fxCh, 48, 127);
            Sleep(400);
            NoteOff(drumCh, 35);
            NoteOff(drumCh, 49);
            NoteOff(fxCh, 48);
        }).detach();
    }

    // 4. 팡파레 재생
    void PlayFanfare_0()
    {
        if (!g_hMidiOut) return;

         std::thread([]() {
            const uint8_t ch = 3;
            SetInstrument(ch, 56);

            // C4, C4, C4
            NoteOn(ch, 60); Sleep(120); NoteOff(ch, 60); Sleep(30);
            NoteOn(ch, 60); Sleep(120); NoteOff(ch, 60); Sleep(30);
            NoteOn(ch, 60); Sleep(120); NoteOff(ch, 60); Sleep(30);

           
        }).detach();
    }
    void PlayFanfare_1()
    {
        if (!g_hMidiOut) return;

         std::thread([]() {
            const uint8_t ch = 3;
            SetInstrument(ch, 56);

       

            // F4, A4
            NoteOn(ch, 65); Sleep(360); NoteOff(ch, 65); Sleep(30);
            NoteOn(ch, 69); Sleep(360); NoteOff(ch, 69); Sleep(30);

       
        }).detach();
    }
    void PlayFanfare_2()
    {
        if (!g_hMidiOut) return;

         std::thread([]() {
            const uint8_t ch = 3;
            SetInstrument(ch, 56);

            // F4, A4, C5
            NoteOn(ch, 65); Sleep(120); NoteOff(ch, 65); Sleep(30);
            NoteOn(ch, 69); Sleep(120); NoteOff(ch, 69); Sleep(30);
            NoteOn(ch, 72); Sleep(600); NoteOff(ch, 72); Sleep(30);
        }).detach();
    }
    // 1. 기 모으기 / 워프 (Reverse Cymbal)
    void PlayCharging() {
        if (!g_hMidiOut) return;
         std::thread([]() {
            const uint8_t ch = 5;
            SetInstrument(ch, 119); // 0-indexed로 120번 악기는 119
            NoteOn(ch, 60, 127);
            Sleep(1500); // 리버스 심벌은 소리가 서서히 커지므로 길게 대기
            NoteOff(ch, 60);
        }).detach();
    }

    // 2. 긁힘 / 가벼운 피격음 (Guitar Fret Noise)
    void PlayScratch() {
        if (!g_hMidiOut) return;
         std::thread([]() {
            const uint8_t ch = 5;
            SetInstrument(ch, 120);
            NoteOn(ch, 75, 127); // 약간 높은 피치로 긁는 소리
            Sleep(150);          // 아주 짧고 경쾌하게 끊음
            NoteOff(ch, 75);
        }).detach();
    }

    // 3. 대시 / 회피 (Breath Noise)
    void PlayDash() {
        if (!g_hMidiOut) return;
         std::thread([]() {
            const uint8_t ch = 5;
            SetInstrument(ch, 121);
            NoteOn(ch, 60, 100);
            Sleep(250); // 쉭! 하는 바람 소리
            NoteOff(ch, 60);
        }).detach();
    }

    // 4. 에너지 방출 / 파도 (Seashore)
    void PlayEnergyBeam() {
        if (!g_hMidiOut) return;
         std::thread([]() {
            const uint8_t ch = 5;
            SetInstrument(ch, 122);
            NoteOn(ch, 50, 127); // 묵직하게 쫙 깔리는 소리
            Sleep(1200);
            NoteOff(ch, 50);
        }).detach();
    }

    // 5. 스턴 / 핑핑 도는 효과 (Bird Tweet)
    void PlayStun() {
        if (!g_hMidiOut) return;
         std::thread([]() {
            const uint8_t ch = 5;
            SetInstrument(ch, 123);
            // 새소리를 3번 연속으로 짧게 재생해서 빙빙 도는 느낌 연출
            for (int i = 0; i < 3; ++i) {
                NoteOn(ch, 72, 110);
                Sleep(150);
                NoteOff(ch, 72);
                Sleep(50);
            }
        }).detach();
    }

    // 6. 비상 경보 사이렌 (Telephone Ring)
    void PlayAlarm() {
        if (!g_hMidiOut) return;
         std::thread([]() {
            const uint8_t ch = 5;
            SetInstrument(ch, 124);
            // 피치를 극단적으로 올려서 띠리리링! 하는 긴박한 사이렌 연출
            PitchBend(ch, 12000);
            NoteOn(ch, 70, 127);
            Sleep(800);
            NoteOff(ch, 70);
            PitchBend(ch, 8192); // 피치 원상복구
        }).detach();
    }

    // 7. 기관총 연사 (Helicopter)
    void PlayMachineGun() {
        if (!g_hMidiOut) return;
         std::thread([]() {
            const uint8_t ch = 5;
            SetInstrument(ch, 125);
            // 투두두두두! 쏘는 느낌을 위해 짧게 5연사
            for (int i = 0; i < 5; ++i) {
                NoteOn(ch, 45, 127);
                Sleep(80);
                NoteOff(ch, 45);
                Sleep(20);
            }
        }).detach();
    }

    // 8. 대포 발사 / 강한 타격 (Gunshot)
    void PlayCannon() {
        if (!g_hMidiOut) return;
         std::thread([]() {
            const uint8_t ch = 5;
            SetInstrument(ch, 126);
            NoteOn(ch, 40, 127); // 아주 낮은 피치로 묵직한 '쾅!'
            Sleep(600);
            NoteOff(ch, 40);
        }).detach();
    }
}