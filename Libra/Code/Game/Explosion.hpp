#pragma once

#include "Game/Entity.hpp"


class Explosion : public Entity
{
	friend class Map;

public:
	Explosion(float importance, Vec2 position);
	virtual ~Explosion();

	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void DebugRender() const override;

private:
	float m_duration = 0.1f;
	float m_size = 0.1f;
	//float m_framesPerSecond = 25.f;

	SpriteAnimDefinition* m_spriteAnimDef = nullptr;
};

