#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Entity.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/AABB2.hpp"

//-----------------------------------------------------------------------------------------------
class Bullet : public Entity
{
	friend class Map;

public:
	Bullet(EntityType type, EntityFaction faction);
	virtual ~Bullet();

	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void DebugRender() const override;
	virtual void Die() override;

	virtual void Start() override;
	virtual void OnCollision(Entity& entity) override;

	int GetDamage() const;

private:
	void SetPropertiesByTypeAndFaction();
	Vec2 GetNormalVector(Vec2 const& currentPos, Vec2 const& nextPos);
private:
	AABB2 m_bulletLocalBounds = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);
	Texture* m_bulletTexture = nullptr;

	int m_damage = 1;
	float m_speed = 4.f;
};

