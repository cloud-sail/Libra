#include "Game/Flame.hpp"
#include "Game/Map.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"


Flame::Flame(EntityType type, EntityFaction faction)
	:Entity(type, faction)
{
	GUARANTEE_OR_DIE(faction == EntityFaction::EVIL, "Flame Bullet Only support Enemy faction now!");
	constexpr int EXPLOSION_SPRITE_NUM = 25;
	float framesPerSecond = static_cast<float>(EXPLOSION_SPRITE_NUM) / m_duration;
	m_spriteAnimDef = new SpriteAnimDefinition(*g_explosionSpriteSheet, 0, EXPLOSION_SPRITE_NUM - 1, framesPerSecond, SpriteAnimPlaybackType::ONCE);
	m_speed = g_gameConfigBlackboard.GetValue("defaultFlameSpeed", m_speed);
	// +/- 100~500 degrees/sec
	m_startRotateRate = g_rng.RollRandomFloatInRange(-400.f, 400.f);
	if (m_startRotateRate >= 0.f)
	{
		m_startRotateRate += 100.f;
	}
	else
	{
		m_startRotateRate -= 100.f;
	}


	m_physicsRadius = 0.05f;
	// Collision
	m_isPushedByEntities = false;
	m_doesPushEntities = false;
	m_isPushedByWalls = false;
	m_isHitByBullets = false;
}

Flame::~Flame()
{
	delete m_spriteAnimDef;
}

void Flame::Update(float deltaSeconds)
{
	m_existSeconds += deltaSeconds;
	if (m_existSeconds > m_duration)
	{
		Die();
		return;
	}

	float t = m_existSeconds / m_duration;
	t = 1 - (1 - t) * (1 - t) * (1 - t);
	m_size = Interpolate(m_startSize, m_endSize, t);
	m_rotateRate = Interpolate(m_startRotateRate, m_endRotateRate, t);

	m_orientationDegrees = m_orientationDegrees + m_rotateRate * deltaSeconds;
	m_position = m_position + m_velocity * deltaSeconds;

	if (m_map->IsPointInSolid(m_position))
	{
		Die();
		return;
	}
}

void Flame::Render() const
{
	std::vector<Vertex_PCU> explosionVerts;
	SpriteDefinition spriteDefExplosion = m_spriteAnimDef->GetSpriteDefAtTime(m_existSeconds);
	AddVertsForAABB2D(explosionVerts, AABB2(-0.5f, -0.5f, 0.5f, 0.5f), Rgba8::OPAQUE_WHITE, spriteDefExplosion.GetUVs());
	TransformVertexArrayXY3D((int)explosionVerts.size(), explosionVerts.data(), m_size, m_orientationDegrees, m_position);
	g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
	g_theRenderer->BindTexture(&g_explosionSpriteSheet->GetTexture());
	g_theRenderer->DrawVertexArray(explosionVerts);
}

void Flame::DebugRender() const
{
	std::vector<Vertex_PCU> verts;
	//// local space i & j
	//AddVertsForLineSegment2D(verts, m_position, m_position + 0.25f * Vec2::MakeFromPolarDegrees(m_orientationDegrees), DEBUG_DRAW_THICKNESS, Rgba8::RED);
	//AddVertsForLineSegment2D(verts, m_position, m_position + 0.25f * Vec2::MakeFromPolarDegrees(m_orientationDegrees).GetRotated90Degrees(), DEBUG_DRAW_THICKNESS, Rgba8::GREEN);

	//// Velocity
	//AddVertsForLineSegment2D(verts, m_position, m_position + m_velocity, DEBUG_DRAW_THICKNESS, Rgba8::YELLOW);

	AddVertsForRing2D(verts, m_position, m_physicsRadius, 0.01f, Rgba8::CYAN);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}


void Flame::Start()
{
	m_velocity = Vec2::MakeFromPolarDegrees(m_orientationDegrees, m_speed);
	m_orientationDegrees = g_rng.RollRandomFloatInRange(0.f, 360.f);
}

void Flame::OnCollision(Entity& entity)
{
	entity.Damage(m_damage);
	Die();
}

int Flame::GetDamage() const
{
	return m_damage;
}
