#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Entity.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"


class Leo : public Entity
{
	friend class Map;
public:
	Leo(EntityFaction faction);
	virtual ~Leo();

	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void DebugRender() const override;
	virtual void Die() override;
	virtual void OnCollision(Entity& other) override;
	virtual void Start() override;

private:
	void Perceive();
	void LookUpMap();
	void Execute(float deltaSeconds);

	void Shoot();

private:
	float m_shootCoolDownTimer = 0.f;

	Vec2 m_goalPosition;
	IntVec2 m_goalTileCoords = IntVec2(-1, -1);
	std::vector<IntVec2> m_pathTiles;
	TileHeatMap* m_distanceMapFromGoal = nullptr;
	bool m_isPlayerVisible = false;

	AABB2 m_tankLocalBounds = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);
	Texture* m_baseTexture = nullptr;

//-----------------------------------------------------------------------------------------------
// Settings
private:
	float m_shootCoolDownSeconds = 1.f;
	float m_driveSpeed = 0.5f;
	float m_turnSpeed = 120.f;
	float m_driveAperture = 90.f;
	float m_shootAperture = 10.f;

	float m_enemyVisibleRange = 10.f;
};

