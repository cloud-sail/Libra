#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Entity.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/AABB2.hpp"

// rotate random
// size grow with time grow
// alpha go down
// not support Friend

class Flame : public Entity
{
	friend class Map;

public:
	Flame(EntityType type, EntityFaction faction);
	virtual ~Flame();

	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void DebugRender() const override;

	virtual void Start() override;
	virtual void OnCollision(Entity& entity) override;

	int GetDamage() const;


private:
	AABB2 m_localBounds = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);
	Texture* m_texture = nullptr;
	SpriteAnimDefinition* m_spriteAnimDef = nullptr;

	int m_damage = 1;
	float m_speed = 4.f;
	float m_size = 0.3f;
	float m_startRotateRate = 500.f;
	float m_rotateRate = 0.f;

	const float m_duration = 1.0f;
	const float m_endRotateRate = 0.f;
	const float m_startSize = 0.1f;
	const float m_endSize = 0.5f;
};

