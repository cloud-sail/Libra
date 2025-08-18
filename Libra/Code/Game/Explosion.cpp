#include "Game/Explosion.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/Renderer.hpp"

Explosion::Explosion(float importance, Vec2 position)
	: Entity(EntityType::NUM, EntityFaction::NUM)
{
	constexpr int EXPLOSION_SPRITE_NUM = 25;
	m_position = position;
	m_orientationDegrees = g_rng.RollRandomFloatInRange(0.f, 360.f);

	m_duration = m_duration * importance;
	m_size = m_size * importance;
	float framesPerSecond = static_cast<float>(EXPLOSION_SPRITE_NUM) / m_duration;

	m_spriteAnimDef = new SpriteAnimDefinition(*g_explosionSpriteSheet, 0, EXPLOSION_SPRITE_NUM - 1, framesPerSecond, SpriteAnimPlaybackType::ONCE);
}

Explosion::~Explosion()
{
	delete m_spriteAnimDef;
}

void Explosion::Update(float deltaSeconds)
{
	m_existSeconds += deltaSeconds;
	if (m_existSeconds > m_duration)
	{
		Die();
	}

}

void Explosion::Render() const
{
	std::vector<Vertex_PCU> explosionVerts;
	SpriteDefinition spriteDefExplosion = m_spriteAnimDef->GetSpriteDefAtTime(m_existSeconds);
	AddVertsForAABB2D(explosionVerts, AABB2(-0.5f, -0.5f, 0.5f, 0.5f), Rgba8::OPAQUE_WHITE, spriteDefExplosion.GetUVs());
	TransformVertexArrayXY3D((int)explosionVerts.size(), explosionVerts.data(), m_size, m_orientationDegrees, m_position);
	g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
	g_theRenderer->BindTexture(&g_explosionSpriteSheet->GetTexture());
	g_theRenderer->DrawVertexArray(explosionVerts);
}

void Explosion::DebugRender() const
{

}
