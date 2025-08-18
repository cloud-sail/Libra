#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Entity.hpp"
#include "Engine/Math/AABB2.hpp"


class PlayerTank : public Entity
{
	friend class Map;
public:
	PlayerTank(EntityFaction faction);
	virtual ~PlayerTank();

	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void DebugRender() const override;
	virtual void Die() override;
	virtual void OnCollision(Entity& other) override;
	virtual void Damage(int damage) override;

	void Revive();

private:

	void MoveTank(float deltaSeconds);
	void MoveTurret(float deltaSeconds);
	void Shoot();

private:
	float m_turretRelativeOffsetDegrees = 0.f;
	float m_tankTargetDegrees = 0.f;
	float m_turretTargetDegrees = 0.f;
	float m_shootCoolDownTimer = 0.f;

	AABB2 m_tankLocalBounds = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);
	AABB2 m_turretLocalBounds = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);
	Texture* m_baseTexture = nullptr;
	Texture* m_turretTexture = nullptr;

//-----------------------------------------------------------------------------------------------
// Settings
private:
	float m_shootCoolDownSeconds = 0.1f;

	float m_driveSpeed = 1.f;
	float m_tankTurnSpeed = 180.f;
	float m_turretTurnSpeed = 360.f;
};

