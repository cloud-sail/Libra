#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Entity.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/AABB2.hpp"


class Scorpio : public Entity
{
	friend class Map;
public:
	Scorpio(EntityFaction faction);
	virtual ~Scorpio();

	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void DebugRender() const override;
	virtual void Die() override;
	virtual void OnCollision(Entity& other) override;

private:
	void SenseTheWorld();

	void UpdateStateWander(float deltaSeconds);
	void UpdateStatePursuit(float deltaSeconds);

	void Shoot();
	void Spray();

private:
	float m_shootCoolDownTimer = 0.f;
	float m_flameCoolDownTimer = 0.f;
	float m_turretOrientationDegrees = 0.f;

	Vec2 m_targetPos;
	bool m_isPlayerVisible = false;

	AABB2 m_tankLocalBounds = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);
	AABB2 m_turretLocalBounds = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);
	Texture* m_baseTexture = nullptr;
	Texture* m_turretTexture = nullptr;

//-----------------------------------------------------------------------------------------------
// Settings
private:
	float m_shootCoolDownSeconds = 0.3f;
	float m_flameCoolDownSeconds = 0.06f;
	float m_turnSpeed = 30.f;
	float m_halfApertureDegrees = 5.f;
	float m_enemyVisibleRange = 10.f;
};


