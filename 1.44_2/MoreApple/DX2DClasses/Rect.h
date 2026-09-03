#pragma once
#include <d2d1.h>
#include "Vector2.h"

namespace DX2DClasses
{
	struct SRect
	{
		float left;
		float top;
		float right;
		float bottom;

		SRect(D2D1_RECT_F rect)
		{
			left = rect.left;
			top = rect.top;
			right = rect.right;
			bottom = rect.bottom;
		}

		SRect(float _left = 0, float _top = 0, float _right = 0, float _bottom = 0)
		{
			left = _left;
			top = _top;
			right = _right;
			bottom = _bottom;
		}

		D2D1_RECT_F ToD2D1Rect()
		{
			return  D2D1::RectF(left, top, right, bottom);
		}

		SVector2 GetSize()
		{
			return SVector2(right - left, bottom - top);
		}

		SVector2 GetCenter()
		{
			SVector2 vSizeHalf = GetSize() * 0.5f;
			return SVector2(left + vSizeHalf.x, top + vSizeHalf.y);
		}

		float GetRadius()
		{
			SVector2 vSizeHalf = GetSize()*0.5f ;
			return vSizeHalf.Magnitude();
		}
	};
}