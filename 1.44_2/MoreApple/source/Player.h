#pragma once

#include "../DX2DClasses/GameObject.h"
#include "../DX2DClasses/Vector2.h"

enum class EPlayerAnimation
{
    Idle,
    Run
};

class CPlayer : public DX2DClasses::CGameObject
{
private:
    //DX2DClasses::SVector2 m_vPlayerScale;
    DX2DClasses::CImage* m_pIdleImage;
    DX2DClasses::CImage* m_pRunImage;

	EPlayerAnimation m_eAnimation;

    bool m_isMoving;
    bool m_isLeft;
    bool m_prevIsLeft;

public:
    CPlayer();
    virtual ~CPlayer();

public:
    void SetIdleImage(DX2DClasses::CImage* image);
    void SetRunImage(DX2DClasses::CImage* image);

public:
    virtual void Release() override;
    virtual void Update() override;
    virtual void Draw() override;

public:
    void Move();
    void UpdateAnimation();
    bool IsLeft() const;
};
