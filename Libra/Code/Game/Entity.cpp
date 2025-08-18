#include "Game/Entity.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/Renderer.hpp"

Entity::Entity(EntityType type, EntityFaction faction)
	: m_type(type)
	, m_faction(faction)
{
}

void Entity::Start()
{
}

void Entity::Die()
{
	m_isDead = true;
	m_isGarbage = true;
}

void Entity::OnCollision(Entity& other)
{
	UNUSED(other);
}

void Entity::Damage(int damage)
{
	m_health -= damage;
	if (m_health <= 0)
	{
		m_health = 0;
		Die();
	}
}

void Entity::RenderHealthBar() const
{
	constexpr float thickness = 0.03f;
	constexpr float verticalOffset = 0.5f;
	constexpr float barLength = 0.4f;

	Vec2 barStartPos = m_position + Vec2(barLength * -0.5f, verticalOffset);
	Vec2 barBackgroundEndPos = barStartPos + Vec2(barLength, 0.f);
	Vec2 barFrontEndPos = barStartPos + Vec2(barLength * static_cast<float>(m_health) / static_cast<float>(m_maxHealth), 0.f);

	std::vector<Vertex_PCU> verts;
	AddVertsForLineSegment2D(verts, barStartPos, barBackgroundEndPos, thickness, Rgba8::RED);
	AddVertsForLineSegment2D(verts, barStartPos, barFrontEndPos, thickness, Rgba8::GREEN);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);

}
