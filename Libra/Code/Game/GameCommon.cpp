#include "Game/GameCommon.hpp"
#include "Engine/Math/Vec2.hpp"

Vec2 GetPointOnSquare(float t)
{
	float tMod = fmodf(t, 8.0f);

	Vec2 pos;
	if (tMod < 1.0f) {
		pos.x = tMod;
		pos.y = 0.0f;
	}
	else if (tMod < 2.0f)
	{
		pos.x = 1.0f;
		pos.y = 0.0f;
	}
	else if (tMod < 3.0f) {
		pos.x = 1.0;
		pos.y = tMod - 2.0f;
	}
	else if (tMod < 4.0f)
	{
		pos.x = 1.0f;
		pos.y = 1.0f;
	}
	else if (tMod < 5.0f) {
		pos.x = 1.0f - (tMod - 4.0f);
		pos.y = 1.0f;
	}
	else if (tMod < 6.0f)
	{
		pos.x = 0.0f;
		pos.y = 1.0f;
	}
	else if (tMod < 7.0) {
		pos.x = 0.0f;
		pos.y = 1.0f - (tMod - 6.0f);
	}
	else
	{
		pos.x = 0.0f;
		pos.y = 0.0f;
	}
	return pos;
}
