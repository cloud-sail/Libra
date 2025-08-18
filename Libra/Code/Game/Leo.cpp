#include "Game/Leo.hpp"
#include "Game/Map.hpp"
#include "Game/PlayerTank.hpp"
#include "Game/Sound.hpp"
#include "Game/Game.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"

Leo::Leo(EntityFaction faction)
	: Entity(EntityType::LEO, faction)
{
	m_physicsRadius			= 0.25f;
	m_shootCoolDownSeconds	= g_gameConfigBlackboard.GetValue("leoShootCooldownSeconds", m_shootCoolDownSeconds);
	m_driveSpeed			= g_gameConfigBlackboard.GetValue("leoDriveSpeed", m_driveSpeed);
	m_turnSpeed				= g_gameConfigBlackboard.GetValue("leoTurnRate", m_turnSpeed);
	m_driveAperture			= g_gameConfigBlackboard.GetValue("leoDriveAperture", m_driveAperture);
	m_shootAperture			= g_gameConfigBlackboard.GetValue("leoShootAperture", m_shootAperture);
	m_maxHealth				= g_gameConfigBlackboard.GetValue("leoHealth", m_maxHealth);
	m_enemyVisibleRange		= g_gameConfigBlackboard.GetValue("enemyVisibleRange", m_enemyVisibleRange);

	// Initialize Value
	m_health = m_maxHealth;

	// Collision
	m_isPushedByEntities	= true;
	m_doesPushEntities		= true;
	m_isPushedByWalls		= true;
	m_isHitByBullets		= true;
	// TODO change sprite according to the faction
	m_baseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyTank4.png");
}

Leo::~Leo()
{
}

void Leo::Update(float deltaSeconds)
{
	m_existSeconds += deltaSeconds;
	m_shootCoolDownTimer -= deltaSeconds;

	Perceive();
	LookUpMap();
	Execute(deltaSeconds);
}

void Leo::Render() const
{
	std::vector<Vertex_PCU> tankVerts;
	AddVertsForAABB2D(tankVerts, m_tankLocalBounds, Rgba8::OPAQUE_WHITE);
	TransformVertexArrayXY3D((int)tankVerts.size(), tankVerts.data(), 1.f, m_orientationDegrees, m_position);
	g_theRenderer->BindTexture(m_baseTexture);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(tankVerts);
}

void Leo::DebugRender() const
{
	std::vector<Vertex_PCU> verts;
	AddVertsForRing2D(verts, m_position, m_physicsRadius, DEBUG_DRAW_THICKNESS, Rgba8::CYAN);
	// local space i & j
	AddVertsForLineSegment2D(verts, m_position, m_position + 0.75f * Vec2::MakeFromPolarDegrees(m_orientationDegrees), DEBUG_DRAW_THICKNESS, Rgba8::RED);
	AddVertsForLineSegment2D(verts, m_position, m_position + 0.75f * Vec2::MakeFromPolarDegrees(m_orientationDegrees).GetRotated90Degrees(), DEBUG_DRAW_THICKNESS, Rgba8::GREEN);

	// Velocity
	AddVertsForLineSegment2D(verts, m_position, m_position + m_velocity, DEBUG_DRAW_THICKNESS, Rgba8::YELLOW);

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
		Vec2 pathPointCenter = m_map->GetTileCenter(m_pathTiles[numPathTiles - 1 - index]);
		AddVertsForDisc2D(verts, pathPointCenter, 0.15f, Rgba8::RED);
	}


	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}

void Leo::Die()
{
	g_theAudio->StartSound(Sound::ENEMY_DIE);
	m_map->SpawnExplosion(6.f, m_position);
	m_isDead = true;
	m_isGarbage = true;
}

void Leo::OnCollision(Entity& other)
{
	UNUSED(other);
	g_theAudio->StartSound(Sound::ENEMY_HIT);
}

void Leo::Start()
{
	m_goalPosition = m_position;
	m_distanceMapFromGoal = new TileHeatMap(m_map->GetDimensions(), SPECIAL_VALUE);
}

void Leo::Perceive()
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

void Leo::LookUpMap()
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

void Leo::Execute(float deltaSeconds)
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

	// Shoot
	if (m_isPlayerVisible)
	{
		if (deltaDegrees < (m_shootAperture * 0.5f) && deltaDegrees > (m_shootAperture * -0.5f))
		{
			Shoot();
		}
	}

}

void Leo::Shoot()
{
	if (m_shootCoolDownTimer <= 0.f)
	{
		Vec2 muzzlePos = m_position + Vec2::MakeFromPolarDegrees(m_orientationDegrees, 0.3f);
		m_map->SpawnNewEntity(EntityType::BULLET, EntityFaction::EVIL, muzzlePos, m_orientationDegrees);
		g_theAudio->StartSound(Sound::ENEMY_SHOOT);
		m_map->SpawnExplosion(2.f, muzzlePos);

		m_shootCoolDownTimer = m_shootCoolDownSeconds;
	}
}
