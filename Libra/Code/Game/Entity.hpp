#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/math/Vec2.hpp"
#include <vector>


//-----------------------------------------------------------------------------------------------
enum class EntityFaction
{
	GOOD,
	NEUTRAL,
	EVIL,
	NUM
};

enum class EntityType
{
	UNKNOWN = -1, // Search every entity / no value / do not care
	PLAYERTANK,
	SCORPIO,
	LEO,
	ARIES,
	CAPRICORN,
	BULLET,
	BOLT,
	MISSILE,
	FLAME,
	NUM
}; // this is also the draw order of the entities by type

//-----------------------------------------------------------------------------------------------
typedef std::vector<Entity*> EntityList;

//-----------------------------------------------------------------------------------------------
class Entity
{
public:
	Entity(EntityType type, EntityFaction faction);
	virtual ~Entity() = default;
	virtual void Update(float deltaSeconds) = 0;
	virtual void Render() const = 0;
	virtual void DebugRender() const = 0;
	virtual void Start();
	virtual void Die();
	virtual void OnCollision(Entity& other);
	virtual void Damage(int damage);
	virtual void RenderHealthBar() const;

public:
	EntityType m_type;
	EntityFaction m_faction;
	Map*	m_map = nullptr;

	float m_existSeconds = 0.f;

	float	m_orientationDegrees = 0.f;
	float	m_physicsRadius = 1.f;
	Vec2	m_position;
	Vec2	m_velocity;

	int m_health = 1;
	int m_maxHealth = 1;

	bool m_isDead = false;
	bool m_isGarbage = false;
	
	bool m_isPushedByEntities = false;
	bool m_doesPushEntities = false;
	bool m_isPushedByWalls = false;
	bool m_isHitByBullets = false;

	bool m_canSwim = false;
};

