#pragma once

#include "../DX2DClasses/GameObject.h"
#include "../DX2DClasses/Vector2.h"

namespace DX2DClasses
{
    class CColorBrush;
	class CDriect2DFramwork;
}

class CBasket : public DX2DClasses::CGameObject
{
private:
    DX2DClasses::SVector2 m_vOffset;

public:
    CBasket();
    virtual ~CBasket();

public:
    void SetOffset(const DX2DClasses::SVector2& offset);

    void UpdatePosition(
        const DX2DClasses::SVector2& playerPos,
        bool isLeft
    );

    void InitializeCollider();
    void RemoveCollider();

    virtual void Release() override;
    virtual void Update() override;
    virtual void Draw() override;
};
