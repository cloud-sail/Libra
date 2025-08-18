#include "Game/PlayerTank.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/Sound.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"


PlayerTank::PlayerTank(EntityFaction faction)
	: Entity(EntityType::PLAYERTANK, faction)
{
	// Initialize Unchanged Value
	m_physicsRadius = 0.3f;
	m_shootCoolDownSeconds = g_gameConfigBlackboard.GetValue("playerShootCooldownSeconds", m_shootCoolDownSeconds);
	m_driveSpeed = g_gameConfigBlackboard.GetValue("playerDriveSpeed", m_driveSpeed);
	m_tankTurnSpeed = g_gameConfigBlackboard.GetValue("playerTurnRate", m_tankTurnSpeed);
	m_turretTurnSpeed = g_gameConfigBlackboard.GetValue("playerGunTurnRate", m_turretTurnSpeed);
	m_maxHealth = g_gameConfigBlackboard.GetValue("playerHealth", m_maxHealth);

	// Initialize Value
	m_health = m_maxHealth;

	// Collision
	m_isPushedByEntities = true;
	m_doesPushEntities = true;
	m_isPushedByWalls = true;
	m_isHitByBullets = true;

	// TODO change sprite according to the faction
	m_baseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PlayerTankBase.png");
	m_turretTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PlayerTankTop.png");
}

PlayerTank::~PlayerTank()
{
}

void PlayerTank::Update(float deltaSeconds)
{
	m_shootCoolDownTimer -= deltaSeconds;

	MoveTank(deltaSeconds);
	MoveTurret(deltaSeconds);
	Shoot();
}

void PlayerTank::Render() const
{
	std::vector<Vertex_PCU> tankVerts;
	AddVertsForAABB2D(tankVerts, m_tankLocalBounds, Rgba8::OPAQUE_WHITE);
	TransformVertexArrayXY3D((int)tankVerts.size(), tankVerts.data(), 1.f, m_orientationDegrees, m_position);
	g_theRenderer->BindTexture(m_baseTexture);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(tankVerts);


	std::vector<Vertex_PCU> turretVerts;
	AddVertsForAABB2D(turretVerts, m_turretLocalBounds, Rgba8::OPAQUE_WHITE);
	TransformVertexArrayXY3D((int)turretVerts.size(), turretVerts.data(), 1.f, m_orientationDegrees + m_turretRelativeOffsetDegrees, m_position);
	g_theRenderer->BindTexture(m_turretTexture);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(turretVerts);

	std::vector<Vertex_PCU> verts;
	if (g_theGame->m_isInvincible)
	{
		AddVertsForRing2D(verts, m_position, m_physicsRadius + 0.03f, 0.05f, Rgba8::OPAQUE_WHITE);
	}
	if (g_theGame->m_isNoClip)
	{
		AddVertsForRing2D(verts, m_position, m_physicsRadius - 0.03f, 0.05f, Rgba8(0, 0, 0, 255));
	}
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}

void PlayerTank::DebugRender() const
{
	static const float s_outerRadius = 0.75f;
	static const float s_targetRadius = 0.85f;

	std::vector<Vertex_PCU> verts;
	// turret current
	AddVertsForLineSegment2D(verts, m_position, m_position + s_outerRadius * Vec2::MakeFromPolarDegrees(m_orientationDegrees + m_turretRelativeOffsetDegrees), DEBUG_DRAW_THICKNESS * 3.f, Rgba8::BLUE);
	// turret target
	AddVertsForLineSegment2D(verts, m_position + s_outerRadius * Vec2::MakeFromPolarDegrees(m_turretTargetDegrees), m_position + s_targetRadius * Vec2::MakeFromPolarDegrees(m_turretTargetDegrees), DEBUG_DRAW_THICKNESS * 3.f, Rgba8::BLUE);

	// inner & outer circle
	AddVertsForRing2D(verts, m_position, m_physicsRadius, DEBUG_DRAW_THICKNESS, Rgba8::CYAN);


	AddVertsForRing2D(verts, m_position, s_outerRadius, DEBUG_DRAW_THICKNESS, Rgba8::MAGENTA);
	// local space i & j
	AddVertsForLineSegment2D(verts, m_position, m_position + s_outerRadius * Vec2::MakeFromPolarDegrees(m_orientationDegrees), DEBUG_DRAW_THICKNESS, Rgba8::RED);
	AddVertsForLineSegment2D(verts, m_position, m_position + s_outerRadius * Vec2::MakeFromPolarDegrees(m_orientationDegrees).GetRotated90Degrees(), DEBUG_DRAW_THICKNESS, Rgba8::GREEN);

	// Tank target
	AddVertsForLineSegment2D(verts, m_position + s_outerRadius * Vec2::MakeFromPolarDegrees(m_tankTargetDegrees), m_position + s_targetRadius * Vec2::MakeFromPolarDegrees(m_tankTargetDegrees), DEBUG_DRAW_THICKNESS, Rgba8::RED);

	// Velocity
	AddVertsForLineSegment2D(verts, m_position, m_position + m_velocity, DEBUG_DRAW_THICKNESS, Rgba8::YELLOW);


	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}

void PlayerTank::Die()
{
	g_theAudio->StartSound(Sound::PLAYER_DIE);
	m_map->SpawnExplosion(6.f, m_position);
	m_isDead = true;
	g_theGame->OnPlayerDie();
}

void PlayerTank::OnCollision(Entity& other)
{
	UNUSED(other);
	if (g_theGame->m_isInvincible)
	{
		g_theAudio->StartSound(Sound::BULLET_BOUNCE);
	}
	else
	{
		g_theAudio->StartSound(Sound::PLAYER_HIT);
	}

}

void PlayerTank::Damage(int damage)
{
	if (g_theGame->m_isInvincible)
	{
		return;
	}
	m_health -= damage;
	if (m_health <= 0)
	{
		m_health = 0;
		Die();
	}
}

void PlayerTank::Revive()
{
	g_theAudio->StartSound(Sound::PLAYER_REVIVE);
	m_health = m_maxHealth;
	m_isDead = false;
}

void PlayerTank::MoveTank(float deltaSeconds)
{
	// Tank Movement
	XboxController const& controller = g_theInput->GetController(0);
	Vec2 moveIntention = controller.GetLeftStick().GetPosition();
	if (g_theInput->IsKeyDown(KEYCODE_E))
	{
		moveIntention += Vec2(0.f, 1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_D))
	{
		moveIntention += Vec2(0.f, -1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_S))
	{
		moveIntention += Vec2(-1.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_F))
	{
		moveIntention += Vec2(1.f, 0.f);
	}
	moveIntention.ClampLength(1.f);
	if (moveIntention != Vec2::ZERO)
	{
		m_tankTargetDegrees = moveIntention.GetOrientationDegrees();
		m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, m_tankTargetDegrees, m_tankTurnSpeed * deltaSeconds);
	}
	m_velocity = m_driveSpeed * moveIntention.GetLength() * Vec2::MakeFromPolarDegrees(m_orientationDegrees);
	// Move position
	m_position += m_velocity * deltaSeconds;

}

void PlayerTank::MoveTurret(float deltaSeconds)
{
	// Turret Rotation
	XboxController const& controller = g_theInput->GetController(0);
	Vec2 turretTurnIntention = controller.GetRightStick().GetPosition();
	if (g_theInput->IsKeyDown(KEYCODE_I))
	{
		turretTurnIntention += Vec2(0.f, 1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_K))
	{
		turretTurnIntention += Vec2(0.f, -1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_J))
	{
		turretTurnIntention += Vec2(-1.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_L))
	{
		turretTurnIntention += Vec2(1.f, 0.f);
	}
	turretTurnIntention.ClampLength(1.f);
	if (turretTurnIntention != Vec2::ZERO)
	{
		m_turretTargetDegrees = turretTurnIntention.GetOrientationDegrees();
		float turrentOrientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees + m_turretRelativeOffsetDegrees, m_turretTargetDegrees, m_turretTurnSpeed * deltaSeconds);
		m_turretRelativeOffsetDegrees = GetShortestAngularDispDegrees(m_orientationDegrees, turrentOrientationDegrees);
	}

}

void PlayerTank::Shoot()
{
	XboxController const& controller = g_theInput->GetController(0);
	if (g_theInput->IsKeyDown(KEYCODE_SPACE) || controller.GetRightTrigger() > 0.f)
	{
		if (m_shootCoolDownTimer <= 0.f)
		{
			float turretDegrees = m_orientationDegrees + m_turretRelativeOffsetDegrees;
			Vec2 muzzlePos = m_position + Vec2::MakeFromPolarDegrees(turretDegrees, 0.3f);
			m_map->SpawnNewEntity(EntityType::BULLET, EntityFaction::GOOD, muzzlePos, turretDegrees);
			g_theAudio->StartSound(Sound::PLAYER_SHOOT);
			m_map->SpawnExplosion(2.f, muzzlePos);

			m_shootCoolDownTimer = m_shootCoolDownSeconds;
		}
	}
}
