#pragma once

#include<Windows.h>
#include<mmsystem.h>//미디 헤더
#include <thread>
#include <algorithm>
#include <filesystem>

#pragma comment(lib,"winMM.lib")

namespace SoundManager {

	void Initialize();//생성+초기화
	void Release();//회수<-메모리 관리

	void SetMasterVolume(float volume);//볼륨 조절(0.0-무음,1.0최대)
	void SetSFXVolume(float volume);    
	void SetBGMVolume(float volume);
	
	void PlayNote(int instrument, int note);//악기 재생 int로 작동

	void PlayBGM(const wchar_t* filename);
	void StopBGM();

	//

	void PlayCoin();//코인
	void PlayJump();//점프
	void PlayExplosion();//폭팔음
	void PlayFanfare_0();//팡파레1
	void PlayFanfare_1();//팡파레2
	void PlayFanfare_2();//팡파레3


	void PlayCharging();   // 기 모으기, 보스 등장 워프 (120)
	void PlayScratch();    // 긁힘, 가벼운 피격음 (121)
	void PlayDash();       // 대시, 회피, 바람 소리 (122)
	void PlayEnergyBeam(); // 지속적인 에너지 방출 (123)
	void PlayStun();       // 스턴(별 핑핑), 숲 앰비언스 (124)
	void PlayAlarm();      // 비상 경보 사이렌 (125)
	void PlayMachineGun(); // 기관총 연사, 기계 가동 (126)
	void PlayCannon();     // 대포 발사, 강한 타격 (127)
}
