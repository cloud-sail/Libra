#include "Game/Bullet.hpp"
#include "Game/Map.hpp"
#include "Game/Sound.hpp"
#include "Game/Game.hpp"
#include "Game/PlayerTank.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/Renderer.hpp"

Bullet::Bullet(EntityType type, EntityFaction faction)
	: Entity(type, faction)
{

	SetPropertiesByTypeAndFaction();

	m_physicsRadius = 0.05f;
	// Collision
	m_isPushedByEntities = false;
	m_doesPushEntities = false;
	m_isPushedByWalls = false;
	m_isHitByBullets = false;
}

Bullet::~Bullet()
{
}

void Bullet::Update(float deltaSeconds)
{
	if (m_type == EntityType::MISSILE)
	{
		PlayerTank* playerTank = g_theGame->GetPlayerTank();
		if (m_map->IsAlive(playerTank))
		{
			float targetDegrees = (playerTank->m_position - m_position).GetOrientationDegrees();
			m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, targetDegrees, 90.f * deltaSeconds);
			m_velocity.SetOrientationDegrees(m_orientationDegrees);
		}
		m_position += (m_velocity * deltaSeconds);
		if (m_map->IsPointInSolid(m_position))
		{
			Die();
		}
		return;
	}

	Vec2 nextVelocity = m_velocity;
	Vec2 nextPosition = m_position + m_velocity * deltaSeconds;

	if (m_map->IsPointInSolid(nextPosition))
	{
		Vec2 normalVector = GetNormalVector(m_position, nextPosition);
		nextVelocity.Reflect(normalVector);
		PushDiscOutOfFixedAABB2D(nextPosition, m_physicsRadius, m_map->GetTileBounds(m_map->GetTileCoordsForWorldPos(nextPosition)));
		g_theAudio->StartSound(Sound::BULLET_BOUNCE);
		Damage(1);
	}

	// Orientation Degrees not involved in physics, it just show the direction of the velocity
	// e.g. Aries only change its speed
	m_velocity = nextVelocity;
	m_position = nextPosition;
	m_orientationDegrees = m_velocity.GetOrientationDegrees();

}

void Bullet::Render() const
{
	std::vector<Vertex_PCU> tankVerts;
	AddVertsForAABB2D(tankVerts, m_bulletLocalBounds, Rgba8::OPAQUE_WHITE);
	TransformVertexArrayXY3D((int)tankVerts.size(), tankVerts.data(), 0.25f, m_orientationDegrees, m_position);
	g_theRenderer->BindTexture(m_bulletTexture);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(tankVerts);
}

void Bullet::DebugRender() const
{
	std::vector<Vertex_PCU> verts;
	AddVertsForRing2D(verts, m_position, m_physicsRadius, 0.01f, Rgba8::CYAN);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}

void Bullet::Die()
{
	Entity::Die();
	m_map->SpawnExplosion(1.f, m_position);
}

void Bullet::Start()
{
	m_velocity = Vec2::MakeFromPolarDegrees(m_orientationDegrees, m_speed);
}

void Bullet::OnCollision(Entity& entity)
{
	if (entity.m_type == EntityType::ARIES)
	{
		return;
	}
	entity.Damage(m_damage);
	Die();
}

int Bullet::GetDamage() const
{
	return m_damage;
}

void Bullet::SetPropertiesByTypeAndFaction()
{
	if (m_type == EntityType::BOLT && m_faction == EntityFaction::EVIL)
	{
		m_bulletLocalBounds = AABB2(-0.5f, -0.25f, 0.5f, 0.25f);
		m_bulletTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyBolt.png");
		// different damage speed health
		m_maxHealth = 1;
		m_health = 1;
		m_speed = g_gameConfigBlackboard.GetValue("defaultBoltSpeed", m_speed);
		return;
	}
	if (m_type == EntityType::BOLT && m_faction == EntityFaction::GOOD)
	{
		m_bulletLocalBounds = AABB2(-0.5f, -0.25f, 0.5f, 0.25f);
		m_bulletTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyBolt.png");
		m_maxHealth = 3;
		m_health = 3;
		m_speed = g_gameConfigBlackboard.GetValue("defaultBoltSpeed", m_speed);
		return;
	}
	if (m_type == EntityType::BULLET && m_faction == EntityFaction::EVIL)
	{
		m_bulletLocalBounds = AABB2(-0.5f, -0.25f, 0.5f, 0.25f);
		m_bulletTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyBullet.png");
		m_maxHealth = 1;
		m_health = 1;
		m_speed = g_gameConfigBlackboard.GetValue("defaultBulletSpeed", m_speed);
		return;
	}
	if (m_type == EntityType::BULLET && m_faction == EntityFaction::GOOD)
	{
		m_bulletLocalBounds = AABB2(-0.5f, -0.25f, 0.5f, 0.25f);
		m_bulletTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyBullet.png");
		m_maxHealth = 3;
		m_health = 3;
		m_speed = g_gameConfigBlackboard.GetValue("defaultBulletSpeed", m_speed);
		return;
	}

	if (m_type == EntityType::MISSILE && m_faction == EntityFaction::EVIL)
	{
		m_bulletLocalBounds = AABB2(-0.5f, -0.25f, 0.5f, 0.25f);
		m_bulletTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyShell.png");
		m_maxHealth = 1;
		m_health = 1;
		m_speed = g_gameConfigBlackboard.GetValue("defaultMissileSpeed", m_speed);
		return;
	}

	ERROR_AND_DIE("Bullet type or faction not supported!");
}

Vec2 Bullet::GetNormalVector(Vec2 const& currentPos, Vec2 const& nextPos)
{
	IntVec2 currentTileCoords = m_map->GetTileCoordsForWorldPos(currentPos);
	IntVec2 nextTileCoords = m_map->GetTileCoordsForWorldPos(nextPos);
	Vec2 normalVector = Vec2(currentTileCoords - nextTileCoords);
	// TOFIX in SD2, speed is too fast

	return normalVector;
}
