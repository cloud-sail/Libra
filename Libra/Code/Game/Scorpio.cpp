#include "Game/Scorpio.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerTank.hpp"
#include "Game/Sound.hpp"
#include "Game/Game.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"


Scorpio::Scorpio(EntityFaction faction)
	: Entity(EntityType::SCORPIO, faction)
{
	m_physicsRadius = 0.4f;
	m_turnSpeed = g_gameConfigBlackboard.GetValue("scorpioTurnRate", m_turnSpeed);
	m_halfApertureDegrees = g_gameConfigBlackboard.GetValue("scorpioTurnAperture", m_halfApertureDegrees * 2.f) * 0.5f;
	m_shootCoolDownSeconds = g_gameConfigBlackboard.GetValue("scorpioShootCooldownSeconds", m_shootCoolDownSeconds);
	m_maxHealth = g_gameConfigBlackboard.GetValue("scorpioHealth", m_maxHealth);
	m_enemyVisibleRange = g_gameConfigBlackboard.GetValue("enemyVisibleRange", m_enemyVisibleRange);

	// Initialize Value
	m_health = m_maxHealth;

	// Collision
	m_isPushedByEntities = false;
	m_doesPushEntities = true;
	m_isPushedByWalls = true;
	m_isHitByBullets = true;
	// TODO change sprite according to the faction
	m_baseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyTurretBase.png");
	m_turretTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyCannon.png");
}

Scorpio::~Scorpio()
{
}

void Scorpio::Update(float deltaSeconds)
{
	SenseTheWorld();
	m_shootCoolDownTimer -= deltaSeconds;
	m_flameCoolDownTimer -= deltaSeconds;
	if (m_isPlayerVisible)
	{
		UpdateStatePursuit(deltaSeconds);
	}
	else
	{
		UpdateStateWander(deltaSeconds);
	}

}

void Scorpio::Render() const
{
	std::vector<Vertex_PCU> tankVerts;
	AddVertsForAABB2D(tankVerts, m_tankLocalBounds, Rgba8::OPAQUE_WHITE);
	TransformVertexArrayXY3D((int)tankVerts.size(), tankVerts.data(), 1.f, m_orientationDegrees, m_position);
	g_theRenderer->BindTexture(m_baseTexture);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(tankVerts);

	std::vector<Vertex_PCU> laserVerts;
	Vec2 laserDirection = Vec2::MakeFromPolarDegrees(m_turretOrientationDegrees);
	RaycastResult2D result = m_map->FastVoxelRaycast(Ray2(m_position, laserDirection, m_enemyVisibleRange));
	AddVertsForLineSegment2D(laserVerts, m_position, m_position + result.m_impactDist * laserDirection, 0.05f, Rgba8::RED, Rgba8(255, 0, 0, 50));
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(laserVerts);

	std::vector<Vertex_PCU> turretVerts;
	AddVertsForAABB2D(turretVerts, m_turretLocalBounds, Rgba8::OPAQUE_WHITE);
	TransformVertexArrayXY3D((int)turretVerts.size(), turretVerts.data(), 1.f, m_turretOrientationDegrees, m_position);
	g_theRenderer->BindTexture(m_turretTexture);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(turretVerts);
}

void Scorpio::DebugRender() const
{
	std::vector<Vertex_PCU> verts;
	
	AddVertsForRing2D(verts, m_position, m_physicsRadius, 0.03f, Rgba8::CYAN);
	AddVertsForLineSegment2D(verts, m_position, m_position + 0.75f * Vec2::MakeFromPolarDegrees(m_turretOrientationDegrees), DEBUG_DRAW_THICKNESS, Rgba8::RED);
	AddVertsForLineSegment2D(verts, m_position, m_position + 0.75f * Vec2::MakeFromPolarDegrees(m_turretOrientationDegrees).GetRotated90Degrees(), DEBUG_DRAW_THICKNESS, Rgba8::GREEN);
	if (m_isPlayerVisible)
	{
		AddVertsForDisc2D(verts, m_targetPos, 0.04f, Rgba8(150, 150, 150));
		AddVertsForLineSegment2D(verts, m_position, m_targetPos, 0.02f, Rgba8(150, 150, 150, 180));
	}

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}

void Scorpio::Die()
{
	g_theAudio->StartSound(Sound::ENEMY_DIE);
	m_map->SpawnExplosion(6.f, m_position);
	m_map->SetSolidMapDirty();
	m_isDead = true;
	m_isGarbage = true;
}

void Scorpio::OnCollision(Entity& other)
{
	UNUSED(other);
	g_theAudio->StartSound(Sound::ENEMY_HIT);
}

void Scorpio::SenseTheWorld()
{
	PlayerTank* playerTank = g_theGame->GetPlayerTank();
	if (m_map->IsAlive(playerTank))
	{
		m_targetPos = playerTank->m_position;
		m_isPlayerVisible = m_map->HasLineOfSight(m_position, m_targetPos, m_enemyVisibleRange);
	}
	else
	{
		m_isPlayerVisible = false;
	}
}

void Scorpio::UpdateStateWander(float deltaSeconds)
{
	m_turretOrientationDegrees += m_turnSpeed * deltaSeconds;
}

void Scorpio::UpdateStatePursuit(float deltaSeconds)
{

	float targetDegrees = (m_targetPos - m_position).GetOrientationDegrees();
	m_turretOrientationDegrees = GetTurnedTowardDegrees(m_turretOrientationDegrees, targetDegrees, m_turnSpeed * deltaSeconds);
	float dispDegrees = GetShortestAngularDispDegrees(targetDegrees, m_turretOrientationDegrees);

	if ((m_targetPos - m_position).GetLengthSquared() > 6.25f)
	{
		if (dispDegrees < m_halfApertureDegrees && dispDegrees > -m_halfApertureDegrees)
		{
			Shoot();
		}
	}
	else
	{
		if (dispDegrees < 45.f && dispDegrees > -45.f)
		{
			Spray();
		}
	}

}

void Scorpio::Shoot()
{
	if (m_shootCoolDownTimer <= 0.f)
	{
		Vec2 muzzlePos = m_position + Vec2::MakeFromPolarDegrees(m_turretOrientationDegrees, 0.5f);
		m_map->SpawnNewEntity(EntityType::BOLT, EntityFaction::EVIL, muzzlePos, m_turretOrientationDegrees);
		g_theAudio->StartSound(Sound::ENEMY_SHOOT);
		m_map->SpawnExplosion(2.f, muzzlePos);
		m_shootCoolDownTimer = m_shootCoolDownSeconds;
	}
}

void Scorpio::Spray()
{
	if (m_flameCoolDownTimer <= 0.f)
	{
		Vec2 muzzlePos = m_position + Vec2::MakeFromPolarDegrees(m_turretOrientationDegrees, 0.5f);
		float varianceAngle = g_rng.RollRandomFloatInRange(-15.f, 15.f);
		m_map->SpawnNewEntity(EntityType::FLAME, EntityFaction::EVIL, muzzlePos, m_turretOrientationDegrees + varianceAngle);
		m_flameCoolDownTimer = m_flameCoolDownSeconds;
	}
}
