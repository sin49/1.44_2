#include "Player.h"

#include "../DX2DClasses/Image.h"

using namespace DX2DClasses;

CPlayer::CPlayer()
	: m_pIdleImage(nullptr)
	, m_pRunImage(nullptr)
	, m_eAnimation(EPlayerAnimation::Idle)
	, m_isMoving(false)
	, m_isLeft(false)
	, m_prevIsLeft(false)
{
}

CPlayer::~CPlayer()
{
	Release();
}

void CPlayer::Release()
{
	CGameObject::Release();
}

void CPlayer::Update()
{
	Move();
	UpdateAnimation();

	CGameObject::Update();
}

void CPlayer::Draw()
{
	CGameObject::Draw();
}

void CPlayer::SetIdleImage(CImage* image)
{
	m_pIdleImage = image;

	// 현재 이미지가 없다면 Idle을 기본 이미지로 사용
	if (GetImage() == nullptr)
	{
		SetImage(image);
	}
}

void CPlayer::SetRunImage(CImage* image)
{
	m_pRunImage = image;
}

void CPlayer::Move()
{
	m_isMoving = false;

	CTransform& transform = GetTransform();

	SVector2 pos = transform.GetTransrate();

	const float speed = 2.0f;
	const float flipOffset = 30.0f;

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		pos.x -= speed;
		m_isMoving = true;
		m_isLeft = true;
	}
	else if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		pos.x += speed;
		m_isMoving = true;
		m_isLeft = false;
	}

	// 방향이 바뀌었을 때만 위치 보정
	if (m_isLeft != m_prevIsLeft)
	{
		if (m_isLeft)
		{
			pos.x += flipOffset;
		}
		else
		{
			pos.x -= flipOffset;
		}

		m_prevIsLeft = m_isLeft;
	}

	transform.SetTransrate(pos);
	transform.SetFlipX(m_isLeft);
}

void CPlayer::UpdateAnimation()
{
	if (m_isMoving)
	{
		if (m_eAnimation != EPlayerAnimation::Run)
		{
			m_eAnimation = EPlayerAnimation::Run;

			SetImage(m_pRunImage);

			GetAnimator()->SetMaxSize(
				m_pRunImage->GetAnimationCount()
			);

			GetAnimator()->SetFrame(0);
		}
	}
	else
	{
		if (m_eAnimation != EPlayerAnimation::Idle)
		{
			m_eAnimation = EPlayerAnimation::Idle;

			SetImage(m_pIdleImage);

			GetAnimator()->SetMaxSize(
				m_pIdleImage->GetAnimationCount()
			);

			GetAnimator()->SetFrame(0);
		}
	}
}

bool CPlayer::IsLeft() const
{
	return m_isLeft;
}
