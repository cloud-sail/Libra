#include "Game/Aries.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerTank.hpp"
#include "Game/Sound.hpp"
#include "Game/Game.hpp"
#include "Game/Bullet.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"

Aries::Aries(EntityFaction faction)
	: Entity(EntityType::ARIES, faction)
{
	m_physicsRadius		= 0.3f;
	m_driveSpeed		= g_gameConfigBlackboard.GetValue("ariesDriveSpeed", m_driveSpeed);
	m_turnSpeed			= g_gameConfigBlackboard.GetValue("ariesTurnRate", m_turnSpeed);
	m_driveAperture		= g_gameConfigBlackboard.GetValue("ariesDriveAperture", m_driveAperture);
	m_maxHealth			= g_gameConfigBlackboard.GetValue("ariesHealth", m_maxHealth);
	m_enemyVisibleRange = g_gameConfigBlackboard.GetValue("enemyVisibleRange", m_enemyVisibleRange);

	// Initialize Value
	m_health = m_maxHealth;

	// Collision
	m_isPushedByEntities	= true;
	m_doesPushEntities		= true;
	m_isPushedByWalls		= true;
	m_isHitByBullets		= true;
	// TODO change sprite according to the faction
	m_baseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyAries.png");
}

Aries::~Aries()
{
}

void Aries::Update(float deltaSeconds)
{
	m_existSeconds += deltaSeconds;

	Perceive();
	LookUpMap();
	Execute(deltaSeconds);
}

void Aries::Render() const
{
	std::vector<Vertex_PCU> tankVerts;
	AddVertsForAABB2D(tankVerts, m_tankLocalBounds, Rgba8::OPAQUE_WHITE);
	TransformVertexArrayXY3D((int)tankVerts.size(), tankVerts.data(), 1.f, m_orientationDegrees, m_position);
	g_theRenderer->BindTexture(m_baseTexture);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(tankVerts);
}

void Aries::DebugRender() const
{
	static const float s_outerRadius = 0.75f;
	static const float s_targetRadius = 0.85f;

	static const float s_thinnerThickness = 0.025f;
	static const float s_thinThickness = 0.05f;
	static const float s_thickThickness = 0.15f;


	std::vector<Vertex_PCU> verts;
	AddVertsForRing2D(verts, m_position, m_physicsRadius, s_thinThickness, Rgba8::CYAN);
	// local space i & j
	AddVertsForLineSegment2D(verts, m_position, m_position + s_outerRadius * Vec2::MakeFromPolarDegrees(m_orientationDegrees), s_thinThickness, Rgba8::RED);
	AddVertsForLineSegment2D(verts, m_position, m_position + s_outerRadius * Vec2::MakeFromPolarDegrees(m_orientationDegrees).GetRotated90Degrees(), s_thinThickness, Rgba8::GREEN);

	// Velocity
	AddVertsForLineSegment2D(verts, m_position, m_position + m_velocity, s_thinnerThickness, Rgba8::YELLOW);

	AddVertsForDisc2D(verts, m_goalPosition, 0.04f, Rgba8(150, 150, 150));
	AddVertsForLineSegment2D(verts, m_position, m_goalPosition, 0.02f, Rgba8(150, 150, 150, 180));

	int numPathTiles = (int)m_pathTiles.size();
	for (int i = 0; i < numPathTiles; ++i)
	{
		Vec2 pathPointCenter = m_map->GetTileCenter(m_pathTiles[i]);
		AddVertsForDisc2D(verts, pathPointCenter, 0.1f, Rgba8::RED);
	}
	// Animation
	if (numPathTiles > 0)
	{
		int index = static_cast<int>(m_existSeconds * 4.f) % numPathTiles;
		Vec2 pathPointCenter = m_map->GetTileCenter(m_pathTiles[numPathTiles -1 - index]);
		AddVertsForDisc2D(verts, pathPointCenter, 0.15f, Rgba8::RED);
	}

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);

}

void Aries::Die()
{
	g_theAudio->StartSound(Sound::ENEMY_DIE);
	m_map->SpawnExplosion(6.f, m_position);
	m_isDead = true;
	m_isGarbage = true;
}

void Aries::OnCollision(Entity& other)
{
	if (m_map->IsBullet(&other))
	{
		Vec2 tankToBullet = other.m_position - m_position;
		float deltaDegrees = GetAngleDegreesBetweenVectors2D(tankToBullet, Vec2::MakeFromPolarDegrees(m_orientationDegrees));
		if (deltaDegrees < 45.f)
		{
			Vec2 normal = tankToBullet.GetNormalized();
			other.m_velocity.Reflect(normal);
			PushDiscOutOfFixedDisc2D(other.m_position, other.m_physicsRadius, m_position, m_physicsRadius);
			other.Damage(1);
			g_theAudio->StartSound(Sound::BULLET_RICOCHET);
		}
		else
		{
			Bullet* bullet = dynamic_cast<Bullet*>(&other);
			if (bullet)
			{
				Damage(bullet->GetDamage());
				bullet->Die();
				g_theAudio->StartSound(Sound::ENEMY_HIT);
			}
		}
	}
}

void Aries::Start()
{
	m_goalPosition = m_position;
	m_distanceMapFromGoal = new TileHeatMap(m_map->GetDimensions(), SPECIAL_VALUE);
}

void Aries::Perceive()
{
	PlayerTank* playerTank = g_theGame->GetPlayerTank();
	m_isPlayerVisible = m_map->IsAlive(playerTank) && m_map->HasLineOfSight(m_position, playerTank->m_position, m_enemyVisibleRange);
	
	if (m_isPlayerVisible)
	{
		m_goalPosition = playerTank->m_position;
	}
	if (IsPointInsideDisc2D(m_goalPosition, m_position, m_physicsRadius + 0.01f))
	{
		m_pathTiles.clear();
	}
}

void Aries::LookUpMap()
{
	if (m_map->GetTileCoordsForWorldPos(m_goalPosition) != m_goalTileCoords) // it means need to generate new map and path 
	{
		m_goalTileCoords = m_map->GetTileCoordsForWorldPos(m_goalPosition);
		m_map->GenerateDistanceMapFromSolidMap(*m_distanceMapFromGoal, m_goalTileCoords, !m_canSwim); // water
		IntVec2 currentTileCoords = m_map->GetTileCoordsForWorldPos(m_position);
		m_map->GeneratePathToGoal(m_pathTiles, currentTileCoords, *m_distanceMapFromGoal);
	}

	if (m_pathTiles.size() >= 2)
	{
		IntVec2 secondPathTileCoords = m_pathTiles[m_pathTiles.size() - 2];
		Vec2 secondPathPoint = m_map->GetTileCenter(secondPathTileCoords);
		// try to do raycast jump the next way point
		Vec2 jBasis = (secondPathPoint - m_position).GetNormalized().GetRotated90Degrees();

		if (m_map->HasLineOfSight(m_position, secondPathPoint, m_enemyVisibleRange, !m_canSwim) &&
			m_map->HasLineOfSight(m_position + m_physicsRadius * jBasis, secondPathPoint + m_physicsRadius * jBasis, m_enemyVisibleRange, !m_canSwim) &&
			m_map->HasLineOfSight(m_position - m_physicsRadius * jBasis, secondPathPoint - m_physicsRadius * jBasis, m_enemyVisibleRange, !m_canSwim)) // water, just raycast not sight
		{
			m_pathTiles.pop_back();
		}
	}
	if (!m_pathTiles.empty())
	{
		IntVec2 nextWayPointTileCoords = m_pathTiles.back();
		Vec2 nextWayPoint = m_map->GetTileCenter(nextWayPointTileCoords);
		if (IsPointInsideDisc2D(nextWayPoint, m_position, m_physicsRadius + 0.01f))
		{
			// pop out the reached way point
			m_pathTiles.pop_back();
		}
	}
	if (m_pathTiles.empty() && !m_isPlayerVisible)
	{
		/*
		Pick a random wander goal tile that's non-solid.  If you pick a solid tile, pick again (max of 10 or 100 attempts).
		If you found a valid wander goal tile, try to generate a path to it.  If you fail, wait until next frame to try again.
		*/
		for (int tries = 0; tries < 50; ++tries)
		{
			IntVec2 randomTileCoords = m_map->GenerateRandomTileCoordsInMap();
			bool isSolid = m_map->IsTileSolid(randomTileCoords, !m_canSwim); // water
			if (!isSolid)
			{
				m_goalPosition = m_map->GetTileCenter(randomTileCoords);
				m_goalTileCoords = randomTileCoords;
				break;
			}
		}
		m_map->GenerateDistanceMapFromSolidMap(*m_distanceMapFromGoal, m_goalTileCoords, !m_canSwim); // water
		IntVec2 currentTileCoords = m_map->GetTileCoordsForWorldPos(m_position);
		m_map->GeneratePathToGoal(m_pathTiles, currentTileCoords, *m_distanceMapFromGoal);
	}
}

void Aries::Execute(float deltaSeconds)
{
	Vec2 targetPos;
	if (m_pathTiles.size() <= 1)
	{
		targetPos = m_goalPosition;
	}
	else
	{
		IntVec2 nextWayPointTileCoords = m_pathTiles.back();
		targetPos = m_map->GetTileCenter(nextWayPointTileCoords);
	}


	// Rotate
	float targetDegrees = (targetPos - m_position).GetOrientationDegrees();
	m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, targetDegrees, m_turnSpeed * deltaSeconds);

	float deltaDegrees = GetShortestAngularDispDegrees(m_orientationDegrees, targetDegrees);

	// Drive
	if (deltaDegrees < (m_driveAperture * 0.5f) && deltaDegrees >(m_driveAperture * -0.5f))
	{
		m_velocity = Vec2::MakeFromPolarDegrees(m_orientationDegrees) * m_driveSpeed;
		m_position += m_velocity * deltaSeconds;
	}
}
