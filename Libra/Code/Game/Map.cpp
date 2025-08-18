#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/Entity.hpp"
#include "Game/PlayerTank.hpp"
#include "Game/Scorpio.hpp"
#include "Game/Leo.hpp"
#include "Game/Aries.hpp"
#include "Game/Bullet.hpp"
#include "Game/Flame.hpp"
#include "Game/Capricorn.hpp"
#include "Game/Explosion.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"


Map::Map(MapDefinition const& mapDef)
	: m_mapDef(mapDef)
	, m_dimensions(mapDef.m_dimensions)
{
	m_exitPosition = GetTileCenter(m_dimensions - IntVec2(2, 2));
	InitializeTiles();
	SpawnInitialNPCs();

	UpdateDistanceMapFromStart();
	UpdateSolidMaps(); // Solid Map need the position of the scorpios
}

Map::~Map()
{
	for (int index = 0; index < (int)m_allEntities.size(); ++index)
	{
		delete m_allEntities[index];
	}
	m_allEntities.clear();

	if (m_solidMapForAmphibians)
	{
		delete m_solidMapForAmphibians;
	}
	if (m_solidMapForLandBased)
	{
		delete m_solidMapForLandBased;
	}
}

void Map::Update(float deltaSeconds)
{
	UpdateEntities(deltaSeconds);
	UpdateExplosions(deltaSeconds);
	UpdateSolidMaps(); // Solid Map need the position of the scorpios
	CheckIfPlayerReachExit();
	PushOffBetweenEntities();
	PushEntitiesOutofWalls();
	CheckBulletVsEntities();
	DeleteGarbageEntities();

	UpdateCameras(deltaSeconds);
}

void Map::Render() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	RenderTiles();
	RenderEntities();
	RenderHealthBar();
	RenderDebugDraw();
	RenderExplosions();
	g_theRenderer->EndCamera(m_worldCamera);

	Camera uiCamera = g_theGame->GetUICamera();
	g_theRenderer->BeginCamera(uiCamera);
	RenderUI();
	g_theRenderer->EndCamera(uiCamera);
}

int Map::GetTileIndexForCoords(IntVec2 const& tileCoords) const
{
	return tileCoords.x + tileCoords.y * m_dimensions.x;;
}

int Map::GetTileIndexForCoords(int coordX, int coordY)
{
	return coordX + coordY * m_dimensions.x;
}

IntVec2 Map::GetTileCoordsForWorldPos(Vec2 const& worldPos) const
{
	int tileX = RoundDownToInt(worldPos.x);
	int tileY = RoundDownToInt(worldPos.y);
	return IntVec2(tileX, tileY);
}

AABB2 Map::GetTileBounds(IntVec2 const& tileCoords) const
{
	Vec2 mins = Vec2(static_cast<float>(tileCoords.x), static_cast<float>(tileCoords.y));
	return AABB2(mins, mins + Vec2::ONE);
}

AABB2 Map::GetTileBounds(int tileIndex) const
{
	IntVec2 tileCoords = m_tiles[tileIndex].m_tileCoords;
	return GetTileBounds(tileCoords);
}

bool Map::IsTileInBounds(IntVec2 const& tileCoords) const
{
	return	(tileCoords.x >= 0) && (tileCoords.x < m_dimensions.x) && 
			(tileCoords.y >= 0) && (tileCoords.y < m_dimensions.y);
}

bool Map::IsTileInEdges(IntVec2 const& tileCoords) const
{
	return tileCoords.x == 0 || tileCoords.y == 0 || tileCoords.x == (m_dimensions.x - 1) || tileCoords.y == (m_dimensions.y - 1);
}

bool Map::IsTileInStartArea(IntVec2 const& tileCoords) const
{
	int startAreaSize = g_gameConfigBlackboard.GetValue("startAreaSize", DEFAULT_START_AREA);

	return IsIntVec2InRectangle(IntVec2(1, 1), IntVec2(1 + startAreaSize, 1 + startAreaSize), tileCoords);
}

bool Map::IsTileInEndArea(IntVec2 const& tileCoords) const
{
	int endAreaSize = g_gameConfigBlackboard.GetValue("endAreaSize", DEFAULT_END_AREA);

	return IsIntVec2InRectangle(m_dimensions - IntVec2(endAreaSize + 1, endAreaSize + 1), m_dimensions - IntVec2(1, 1), tileCoords);
}

bool Map::IsIntVec2InRectangle(IntVec2 const& mins, IntVec2 const& maxs, IntVec2 const& point) const
{
	return point.x >= mins.x && point.y >= mins.y && point.x < maxs.x && point.y < maxs.y;
}

bool Map::IsTileSolid(IntVec2 const& tileCoords, bool treatWaterAsSolid) const
{
	if (!IsTileInBounds(tileCoords))
	{
		return true;
	}

	int tileIndex = GetTileIndexForCoords(tileCoords);
	Tile const& tile = m_tiles[tileIndex];
	bool isWater = tile.m_tileDef->m_isWater;
	bool isSolid = tile.m_tileDef->m_isSolid;
	
	return isSolid || (treatWaterAsSolid && isWater);
}

bool Map::IsPointInSolid(Vec2 const& worldPos, bool treatWaterAsSolid) const
{
	IntVec2 tileCoords = GetTileCoordsForWorldPos(worldPos);
	return IsTileSolid(tileCoords, treatWaterAsSolid);
}

Vec2 Map::GetTileCenter(IntVec2 const& tileCoords) const
{
	return Vec2(static_cast<float>(tileCoords.x) + 0.5f, static_cast<float>(tileCoords.y) + 0.5f);
}



#pragma region Update

void Map::UpdateEntities(float deltaSeconds)
{
	for (int entityType = 0; entityType < (int)EntityType::NUM; ++entityType)
	{
		EntityList const& entitiesOfType = m_entitiesByType[entityType];
		for (int entityIndex = 0; entityIndex < (int)entitiesOfType.size(); ++entityIndex)
		{
			Entity* entity = entitiesOfType[entityIndex];
			if (IsAlive(entity))
			{
				entity->Update(deltaSeconds);
			}
		}
	}
}

void Map::CheckIfPlayerReachExit()
{
	PlayerTank* playerTank = g_theGame->GetPlayerTank();
	if (IsAlive(playerTank))
	{
		if (IsPointInsideDisc2D(m_exitPosition, playerTank->m_position, playerTank->m_physicsRadius))
		{
			g_theGame->MoveToNextMap();
		}
	}
	
}

void Map::PushOffBetweenEntities()
{
	for (int i = 0; i < (int)m_entitesWithPhysics.size(); ++i)
	{
		for (int j = i + 1; j < (int)m_entitesWithPhysics.size(); ++j)
		{
			Entity* a = m_entitesWithPhysics[i];
			Entity* b = m_entitesWithPhysics[j];
			if (a && b)
			{
				PushOffBetweenBothEntities(*a, *b);
			}
		}
	}
}

void Map::PushEntitiesOutofWalls()
{
	for (int entityType = 0; entityType < (int)EntityType::NUM; ++entityType)
	{
		if (g_theGame->m_isNoClip && entityType == static_cast<int>(EntityType::PLAYERTANK))
		{
			continue;
		}
		EntityList const& entitiesOfType = m_entitiesByType[entityType];
		for (int entityIndex = 0; entityIndex < (int)entitiesOfType.size(); ++entityIndex)
		{
			Entity* entity = entitiesOfType[entityIndex];
			if (entity && entity->m_isPushedByWalls)
			{
				PushEntityOutOfSolidTiles(*entity);
			}
		}
	}
}

void Map::CheckBulletVsEntities()
{
	CheckBulletListVsEntityList(m_bulletsByFaction[(int)EntityFaction::EVIL], m_agentsByFaction[(int)EntityFaction::GOOD]);
	CheckBulletListVsEntityList(m_bulletsByFaction[(int)EntityFaction::GOOD], m_agentsByFaction[(int)EntityFaction::EVIL]);



}

void Map::DeleteGarbageEntities()
{
	for (int i = 0; i < (int)m_allEntities.size(); ++i)
	{
		Entity* entity = m_allEntities[i];
		if (IsGarbage(entity))
		{
			RemoveEntityFromMap(entity);
			delete entity;
		}
	}
	for (int i = 0; i < (int)m_explosions.size(); ++i)
	{
		Entity* entity = m_explosions[i];
		if (IsGarbage(entity))
		{
			m_explosions[i] = nullptr;
			delete entity;
		}
	}
}

bool Map::IsAlive(Entity* entity) const
{
	if (entity == nullptr)
	{
		return false;
	}
	return !entity->m_isDead;
}

bool Map::IsGarbage(Entity* entity) const
{
	return entity && entity->m_isGarbage;
}

void Map::PushOffBetweenBothEntities(Entity& a, Entity& b)
{
	bool aPushesB = a.m_doesPushEntities && b.m_isPushedByEntities;
	bool bPushesA = b.m_doesPushEntities && a.m_isPushedByEntities;

	if (aPushesB && bPushesA)
	{
		PushDiscsOutOfEachOther2D(a.m_position, a.m_physicsRadius, b.m_position, b.m_physicsRadius);
	}
	else if (aPushesB)
	{
		PushDiscOutOfFixedDisc2D(b.m_position, b.m_physicsRadius, a.m_position, a.m_physicsRadius);
	}
	else if (bPushesA)
	{
		PushDiscOutOfFixedDisc2D(a.m_position, a.m_physicsRadius, b.m_position, b.m_physicsRadius);
	}
}


void Map::PushEntityOutOfSolidTiles(Entity& entity)
{
	IntVec2 currentTileCoords = GetTileCoordsForWorldPos(entity.m_position);
	IntVec2 directions[8] = { IntVec2(0,1), IntVec2(0,-1), IntVec2(1,0), IntVec2(-1,0), IntVec2(1,1), IntVec2(-1,1), IntVec2(-1,-1), IntVec2(1,-1) };

	for (int i = 0; i < 8; ++i)
	{
		IntVec2 tileCoords = currentTileCoords + directions[i];
		bool treatWaterAsSolid = !entity.m_canSwim;
		if (!IsTileSolid(tileCoords, treatWaterAsSolid))
		{
			continue;
		}
		AABB2 tileBounds = GetTileBounds(tileCoords);
		PushDiscOutOfFixedAABB2D(entity.m_position, entity.m_physicsRadius, tileBounds);
	}
}

void Map::CheckBulletListVsEntityList(EntityList const& bulletList, EntityList const& entityList)
{
	for (int bulletIndex = 0; bulletIndex < (int)bulletList.size(); ++bulletIndex)
	{
		Entity* bullet = bulletList[bulletIndex];
		if (!IsAlive(bullet))
		{
			continue;
		}
		for (int entityIndex = 0; entityIndex < (int)entityList.size(); ++entityIndex)
		{
			Entity* entity = entityList[entityIndex];
			if (!IsAlive(entity))
			{
				continue;
			}
			HandleCollision(*bullet, *entity);
		}
	}
}

void Map::HandleCollision(Entity& a, Entity& b)
{
	if (DoDiscsOverlap(a.m_position, a.m_physicsRadius, b.m_position, b.m_physicsRadius))
	{
		a.OnCollision(b);
		b.OnCollision(a);
	}
}

#pragma endregion Update

#pragma region Render
void Map::RenderTiles() const
{
	switch (m_currentTileRenderMode)
	{
	case TileRenderMode::DEFAULT_MAP:
		RenderDefaultMap();
		break;
	case TileRenderMode::DISTANCE_MAP_FROM_START:
		RenderDistanceMapFromStart();
		break;
	case TileRenderMode::SOLID_MAP_FOR_AMPHIBIANS:
		RenderSolidMapForAmphibians();
		break;
	case TileRenderMode::SOLID_MAP_FOR_LAND_BASED:
		RenderSolidMapForLandBased();
		break;
	case TileRenderMode::DISTANCE_MAP_FOR_SELECTED:
		RenderDefaultMap();
		break;
	}
}

void Map::RenderEntities() const
{
	for (int entityType = 0; entityType < (int)EntityType::NUM; ++entityType)
	{
		EntityList const& entitiesOfType = m_entitiesByType[entityType];
		for (int entityIndex = 0; entityIndex < (int)entitiesOfType.size(); ++entityIndex)
		{
			Entity* entity = entitiesOfType[entityIndex];
			if (IsAlive(entity))
			{
				entity->Render();
			}
		}
	}
}

void Map::RenderDebugDraw() const
{
	if (!g_isDebugDraw)
	{
		return;
	}

	for (int entityType = 0; entityType < (int)EntityType::NUM; ++entityType)
	{
		EntityList const& entitiesOfType = m_entitiesByType[entityType];
		for (int entityIndex = 0; entityIndex < (int)entitiesOfType.size(); ++entityIndex)
		{
			Entity* entity = entitiesOfType[entityIndex];
			if (entity) // if not alive still debug draw
			{
				entity->DebugRender();
			}
		}
	}
}
void Map::RenderUI() const
{
	RenderTileRenderModeInfo();
}
void Map::RenderHealthBar() const
{
	for (int entityType = 0; entityType < (int)EntityType::NUM; ++entityType)
	{
		if (entityType == (int)EntityType::BOLT || entityType == (int)EntityType::BULLET || entityType == (int)EntityType::MISSILE || entityType == (int)EntityType::FLAME) continue;

		EntityList const& entitiesOfType = m_entitiesByType[entityType];
		for (int entityIndex = 0; entityIndex < (int)entitiesOfType.size(); ++entityIndex)
		{
			Entity* entity = entitiesOfType[entityIndex];
			if (IsAlive(entity))
			{
				//entity->Render();
				entity->RenderHealthBar();
			}
		}
	}
}
#pragma endregion Render

#pragma region Camera

void Map::UpdateCameras(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	AABB2 cameraView;
	if (g_theGame->m_isWholeMapView)
	{
		cameraView = CalculateDebugCameraView();
	}
	else
	{
		cameraView = CalculateFollowCameraView();
		ClampCameraView(cameraView);
	}
	m_worldCamera.SetOrthoView(cameraView.m_mins, cameraView.m_maxs);
}

AABB2 Map::CalculateDebugCameraView()
{
	AABB2 cameraView;
	float mapAspectRatio = static_cast<float>(m_dimensions.x) / static_cast<float>(m_dimensions.y);
	float screenAspectRatio = SCREEN_SIZE_X / SCREEN_SIZE_Y;

	if (mapAspectRatio > screenAspectRatio)
	{
		cameraView.SetDimensions(Vec2(static_cast<float>(m_dimensions.x), static_cast<float>(m_dimensions.x) * SCREEN_SIZE_Y / SCREEN_SIZE_X));
	}
	else
	{
		cameraView.SetDimensions(Vec2(static_cast<float>(m_dimensions.y) * SCREEN_SIZE_X / SCREEN_SIZE_Y, static_cast<float>(m_dimensions.y)));
	}
	cameraView.SetCenter(Vec2(static_cast<float>(m_dimensions.x), static_cast<float>(m_dimensions.y)) * 0.5f);
	return cameraView;
}

AABB2 Map::CalculateFollowCameraView()
{
	AABB2 cameraView;
	cameraView.SetDimensions(Vec2(g_theGame->m_numTilesInViewVertically * SCREEN_SIZE_X / SCREEN_SIZE_Y, g_theGame->m_numTilesInViewVertically));

	// TODO later there exists multiple player tank

	PlayerTank* playerTank = g_theGame->GetPlayerTank();

	if (playerTank)
	{
		cameraView.SetCenter(playerTank->m_position);
	}

	return cameraView;
}

void Map::ClampCameraView(AABB2& cameraView)
{
	if (cameraView.m_mins.x < 0.f)
	{
		cameraView.Translate(Vec2(-cameraView.m_mins.x, 0.f));
	}
	if (cameraView.m_mins.y < 0.f)
	{
		cameraView.Translate(Vec2(0.f, -cameraView.m_mins.y));
	}
	if (cameraView.m_maxs.x > m_dimensions.x)
	{
		cameraView.Translate(Vec2(m_dimensions.x - cameraView.m_maxs.x, 0.f));
	}
	if (cameraView.m_maxs.y > m_dimensions.y)
	{
		cameraView.Translate(Vec2(0.f, m_dimensions.y - cameraView.m_maxs.y));
	}
}
#pragma endregion Camera


#pragma region Initialization

void Map::InitializeTiles()
{
	int numTiles = m_dimensions.x * m_dimensions.y;
	m_tiles.resize(numTiles);

	constexpr int MAX_TRIES = 100;
	bool isValid = false;

	for (int attempt = 0; attempt < MAX_TRIES; ++attempt)
	{
		FillTiles();
		PopulateTiles();
		isValid = IsMapValid();
		if (isValid)
		{
			break;
		}
	}

	if (!isValid)
	{
		ERROR_AND_DIE(Stringf("Failed to generate map(%s)", m_mapDef.m_name.c_str()));
	}

	FillNonSolidAndUnreachableTiles();
}

void Map::FillTiles()
{
	// Set each tile's internal TileCoords, and initialize type to fill
	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
		{
			int tileIndex = GetTileIndexForCoords(IntVec2(tileX, tileY));
			Tile& tile = m_tiles[tileIndex];
			tile.m_tileCoords = IntVec2(tileX, tileY);
			tile.m_tileDef = TileDefinition::GetTileDef(m_mapDef.m_fillTileType);
		}
	}
}

void Map::PopulateTiles()
{
	//GenerateSpinkles();

	BuildWorms();
	BuildStartArea();
	BuildEndArea();
	BuildEdges();
	// MapColor out of bound is ok not replace
	// TODO Map Color
}

void Map::BuildWorms()
{
	BuildWormTilesWithParameters(m_mapDef.m_worm1TileType, m_mapDef.m_worm1Count, m_mapDef.m_worm1MaxLength);
	BuildWormTilesWithParameters(m_mapDef.m_worm2TileType, m_mapDef.m_worm2Count, m_mapDef.m_worm2MaxLength);
}

void Map::BuildWormTilesWithParameters(std::string tileType, int count, int maxLength)
{
	IntVec2 directions[4] = { IntVec2(0,1), IntVec2(0,-1), IntVec2(1,0), IntVec2(-1,0) };

	for (int wormIndex = 0; wormIndex < count; ++wormIndex)
	{
		int tileIndex = g_rng.RollRandomIntInRange(0, m_dimensions.x * m_dimensions.y - 1);
		Tile const& tile = m_tiles[tileIndex];
		IntVec2 currentCoords = tile.m_tileCoords;
		for (int i = 0; i < maxLength; ++i)
		{
			SetTileDefInPoint(currentCoords, TileDefinition::GetTileDef(tileType));

			IntVec2 direction = directions[g_rng.RollRandomIntLessThan(4)];
			IntVec2 nextCoords = currentCoords + direction;
			if (IsTileInBounds(nextCoords) && (!IsTileInEdges(nextCoords)))
			{
				currentCoords = nextCoords;
			}
		}
	}
}

void Map::BuildEdges()
{
	for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
	{
		m_tiles[tileX].m_tileDef = TileDefinition::GetTileDef(m_mapDef.m_edgeTileType);
		m_tiles[tileX + (m_dimensions.y - 1) * m_dimensions.x].m_tileDef = TileDefinition::GetTileDef(m_mapDef.m_edgeTileType);
	}
	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		m_tiles[tileY * m_dimensions.x].m_tileDef = TileDefinition::GetTileDef(m_mapDef.m_edgeTileType);
		m_tiles[(tileY + 1) * m_dimensions.x - 1].m_tileDef = TileDefinition::GetTileDef(m_mapDef.m_edgeTileType);
	}
}

void Map::BuildStartArea()
{
	int startAreaSize = g_gameConfigBlackboard.GetValue("startAreaSize", DEFAULT_START_AREA);
	//SetTileTypeInRectangle(IntVec2(1, 1), IntVec2(6, 6), m_mapDef.m_startFloor);
	//SetTileTypeInRectangle(IntVec2(2, 4), IntVec2(5, 5), m_mapDef.m_startBunker);
	//SetTileTypeInRectangle(IntVec2(4, 2), IntVec2(5, 4), m_mapDef.m_startBunker);

	TileDefinition* startFloorDef = TileDefinition::GetTileDef(m_mapDef.m_startFloorTileType);
	TileDefinition* startBunkerDef = TileDefinition::GetTileDef(m_mapDef.m_startBunkerTileType);
	SetTileDefInRectangle(IntVec2(1, 1), IntVec2(1 + startAreaSize, 1 + startAreaSize), startFloorDef);
	SetTileDefInRectangle(IntVec2(2, startAreaSize - 1), IntVec2(startAreaSize, startAreaSize), startBunkerDef);
	SetTileDefInRectangle(IntVec2(startAreaSize - 1, 2), IntVec2(startAreaSize, startAreaSize - 1), startBunkerDef);

	// Map Entry
	SetTileDefInPoint(IntVec2(1, 1), TileDefinition::GetTileDef("MapEntry"));

}

void Map::BuildEndArea()
{
	int endAreaSize = g_gameConfigBlackboard.GetValue("endAreaSize", DEFAULT_END_AREA);
	//SetTileTypeInRectangle(m_dimensions - IntVec2(7, 7), m_dimensions - IntVec2(1, 1), m_mapDef.m_endFloor);
	//SetTileTypeInRectangle(m_dimensions - IntVec2(6, 6), m_dimensions - IntVec2(2, 5), m_mapDef.m_endBunker);
	//SetTileTypeInRectangle(m_dimensions - IntVec2(6, 5), m_dimensions - IntVec2(5, 2), m_mapDef.m_endBunker);

	TileDefinition* endFloorDef = TileDefinition::GetTileDef(m_mapDef.m_endFloorTileType);
	TileDefinition* endBunkerDef = TileDefinition::GetTileDef(m_mapDef.m_endBunkerTileType);
	SetTileDefInRectangle(m_dimensions - IntVec2(endAreaSize + 1, endAreaSize + 1), m_dimensions - IntVec2(1, 1), endFloorDef);
	SetTileDefInRectangle(m_dimensions - IntVec2(endAreaSize, endAreaSize), m_dimensions - IntVec2(2, endAreaSize - 1), endBunkerDef);
	SetTileDefInRectangle(m_dimensions - IntVec2(endAreaSize, endAreaSize - 1), m_dimensions - IntVec2(endAreaSize - 1, 2), endBunkerDef);
	// Map Exit
	SetTileDefInPoint(m_dimensions - IntVec2(2, 2), TileDefinition::GetTileDef("MapExit"));
}

void Map::SetTileDefInRectangle(IntVec2 const& mins, IntVec2 const& maxs, TileDefinition* tileDef)
{
	for (int tileY = mins.y; tileY < maxs.y; ++tileY)
	{
		for (int tileX = mins.x; tileX < maxs.x; ++tileX)
		{
			int tileIndex = GetTileIndexForCoords(tileX, tileY);
			m_tiles[tileIndex].m_tileDef = tileDef;
		}
	}
}

void Map::SetTileDefInPoint(IntVec2 const& tileCoords, TileDefinition* tileDef)
{
	// TODO if nullptr not set?
	int tileIndex = GetTileIndexForCoords(tileCoords);
	m_tiles[tileIndex].m_tileDef = tileDef;
}

void Map::SpawnInitialNPCs()
{
	SpawnInitialNPCsRandomlyByCount(EntityType::SCORPIO, EntityFaction::EVIL, m_mapDef.m_scorpioCount);
	SpawnInitialNPCsRandomlyByCount(EntityType::LEO, EntityFaction::EVIL, m_mapDef.m_leoCount);
	SpawnInitialNPCsRandomlyByCount(EntityType::ARIES, EntityFaction::EVIL, m_mapDef.m_ariesCount);
	SpawnInitialNPCsRandomlyByCount(EntityType::CAPRICORN, EntityFaction::EVIL, m_mapDef.m_capricornCount);
}

void Map::SpawnInitialNPCsRandomlyByCount(EntityType type, EntityFaction faction, int count)
{
	for (int i = 0; i < count; ++i)
	{
		int tries = 0;
		while (true)
		{
			int tileIndex = g_rng.RollRandomIntInRange(0, m_dimensions.x * m_dimensions.y - 1);
			Tile const& tile = m_tiles[tileIndex];
			bool isSolid = IsTileSolid(tile.m_tileCoords, true); // cannot spawn on water
			bool isInStartArea = IsTileInStartArea(tile.m_tileCoords);
			bool isInEndArea = IsTileInEndArea(tile.m_tileCoords);

			if (!isSolid && !isInStartArea && !isInEndArea)
			{
				SpawnNewEntity(type, faction, GetTileCenter(tile.m_tileCoords), 0.f);
				break;
			}
			tries++;
			if (tries > 100)
			{
				ERROR_AND_DIE("Cannot find place to Spawn NPC");
			}
		}
	}
}

bool Map::IsMapValid()
{
	UpdateDistanceMapFromStart();
	float value = m_distanceMapFromStart->GetValueAtCoords(m_dimensions - IntVec2(2, 2));
	return value != SPECIAL_VALUE;
}

void Map::FillNonSolidAndUnreachableTiles()
{
	TileHeatMap distMap(m_dimensions, SPECIAL_VALUE);
	PopulateDistanceField(distMap, IntVec2(1, 1), SPECIAL_VALUE, false);

	int numTiles = distMap.GetNumTiles();
	for (int tileIndex = 0; tileIndex < numTiles; ++tileIndex)
	{
		float value = distMap.GetValueAtIndex(tileIndex);
		bool isReachable = (value != SPECIAL_VALUE);
		bool isSolid = m_tiles[tileIndex].m_tileDef->m_isSolid;
		if (!isReachable && !isSolid)
		{
			m_tiles[tileIndex].m_tileDef = TileDefinition::GetTileDef(m_mapDef.m_edgeTileType);
		}
	}
}


#pragma endregion Initialization

#pragma region EntityManagement
Entity* Map::SpawnNewEntity(EntityType type, EntityFaction faction, Vec2 const& position, float orientationDegrees)
{
	Entity* entity = CreateNewEntity(type, faction);
	AddEntityToMap(entity, position, orientationDegrees);
	return entity;
}

void Map::AddEntityToMap(Entity* entity, Vec2 const& position, float orientationDegrees)
{
	entity->m_position = position;
	entity->m_orientationDegrees = orientationDegrees;
	entity->m_map = this;
	entity->Start();

	AddEntityToList(entity, m_allEntities);
	AddEntityToList(entity, m_entitiesByType[(int)entity->m_type]);
	if (IsAgent(entity))
	{
		AddEntityToList(entity, m_agentsByFaction[(int)entity->m_faction]);
	}
	if (IsBullet(entity))
	{
		AddEntityToList(entity, m_bulletsByFaction[(int)entity->m_faction]);
	}
	if (HasPhysics(entity))
	{
		AddEntityToList(entity, m_entitesWithPhysics);
	}
}

void Map::RemoveEntityFromMap(Entity* entity)
{
	entity->m_map = nullptr;
	RemoveEntityFromList(entity, m_allEntities);
	RemoveEntityFromList(entity, m_entitiesByType[(int)entity->m_type]);
	if (IsAgent(entity))
	{
		RemoveEntityFromList(entity, m_agentsByFaction[(int)entity->m_faction]);
	}
	if (IsBullet(entity))
	{
		RemoveEntityFromList(entity, m_bulletsByFaction[(int)entity->m_faction]);
	}
	if (HasPhysics(entity))
	{
		RemoveEntityFromList(entity, m_entitesWithPhysics);
	}
}

bool Map::IsBullet(Entity* entity) const
{
	if (entity == nullptr)
	{
		return false;
	}
	return entity->m_type == EntityType::BULLET || entity->m_type == EntityType::BOLT || entity->m_type == EntityType::MISSILE || entity->m_type == EntityType::FLAME;
}

bool Map::IsAgent(Entity* entity) const
{
	if (entity == nullptr)
	{
		return false;
	}
	EntityType const& entityType = entity->m_type;
	return (entityType == EntityType::ARIES || entityType == EntityType::LEO || entityType == EntityType::SCORPIO || entityType == EntityType::CAPRICORN || entityType == EntityType::PLAYERTANK);
}

bool Map::HasPhysics(Entity* entity) const
{
	if (entity == nullptr)
	{
		return false;
	}
	return (entity->m_isPushedByEntities) || (entity->m_doesPushEntities) || (entity->m_isPushedByWalls);
}

IntVec2 Map::GetDimensions()
{
	return m_dimensions;
}

IntVec2 Map::GenerateRandomTileCoordsInMap()
{
	int tileIndex = g_rng.RollRandomIntInRange(0, m_dimensions.x * m_dimensions.y - 1);
	Tile const& tile = m_tiles[tileIndex];
	return tile.m_tileCoords;
}

Entity* Map::CreateNewEntity(EntityType type, EntityFaction faction)
{
	switch (type)
	{
	case EntityType::PLAYERTANK:
		return new PlayerTank(faction);
		break;
	case EntityType::SCORPIO:
		return new Scorpio(faction);
		break;
	case EntityType::LEO:
		return new Leo(faction);
		break;
	case EntityType::ARIES:
		return new Aries(faction);
		break;
	case EntityType::CAPRICORN:
		return new Capricorn(faction);
		break;
	case EntityType::BULLET:
		return new Bullet(type, faction);
		break;
	case EntityType::BOLT:
		return new Bullet(type, faction);
		break;
	case EntityType::MISSILE:
		return new Bullet(type, faction);
		break;
	case EntityType::FLAME:
		return new Flame(type, faction);
		break;
	default:
		ERROR_AND_DIE(Stringf("Unknown Entity Type :#%i", type))
	}
	//return nullptr;
}

void Map::AddEntityToList(Entity* entityToAdd, EntityList& list)
{
	if (entityToAdd)
	{
		list.push_back(entityToAdd);
	}
}

void Map::RemoveEntityFromList(Entity* entityToRemove, EntityList& list)
{
	if (!entityToRemove)
	{
		return;
	}
	for (int i = 0; i < list.size(); ++i)
	{
		if (entityToRemove == list[i])
		{
			list[i] = nullptr;
			return;
		}
	}
	ERROR_AND_DIE("Cannot find the entity to remove from the list!");
}

#pragma endregion EntityManagement

#pragma region HeatMap

void Map::SwitchToNextTileRenderMode()
{
	m_currentTileRenderMode = static_cast<TileRenderMode>((static_cast<int>(m_currentTileRenderMode) + 1) % static_cast<int>(TileRenderMode::NUM));
}

void Map::SetSolidMapDirty()
{
	m_isSolidMapDirty = true;
}

void Map::PopulateDistanceField(TileHeatMap& out_distanceField, IntVec2 startCoords, float maxCost, bool treatWaterAsSolid)
{
	GUARANTEE_OR_DIE(out_distanceField.m_dimensions == m_dimensions, "TileHeatMap and Map did not match in dimensions!");
	out_distanceField.SetAllValues(maxCost);
	out_distanceField.SetValueAtCoords(startCoords, 0.f);

	IntVec2 directions[4] = { IntVec2(0,1), IntVec2(0,-1), IntVec2(1,0), IntVec2(-1,0) };
	// size_t size = sizeof(directions) / sizeof(directions[0]);
	float currentSearchValue = 0.f;

	bool isHeatSpreading = true;

	while (isHeatSpreading)
	{
		isHeatSpreading = false;

		for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
		{
			for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
			{
				IntVec2 const currentTileCoords = IntVec2(tileX, tileY);
				float currentValue = out_distanceField.GetValueAtCoords(currentTileCoords);
				if (currentValue == currentSearchValue)
				{
					isHeatSpreading = true; // or on neighbor value changed, one more round of search
					for (int i = 0; i < 4; ++i) // four direction
					{
						IntVec2 neighborTileCoords = currentTileCoords + directions[i];
						if (IsTileSolid(neighborTileCoords, treatWaterAsSolid))
						{
							continue;
						}
						float neighborValue = out_distanceField.GetValueAtCoords(neighborTileCoords);
						if (neighborValue <= (currentValue + 1.f)) // step is 1.f
						{
							continue;
						}
						out_distanceField.SetValueAtCoords(neighborTileCoords, currentValue + 1.f);
					}
				}
			}
		}

		currentSearchValue += 1.f;
	}
}

void Map::GenerateDistanceMapFromSolidMap(TileHeatMap& out_distanceField, IntVec2 startCoords, bool treatWaterAsSolid)
{
	GUARANTEE_OR_DIE(out_distanceField.m_dimensions == m_dimensions, "TileHeatMap and Map did not match in dimensions!");

	out_distanceField.SetAllValues(SPECIAL_VALUE);
	out_distanceField.SetValueAtCoords(startCoords, 0.f);

	IntVec2 directions[4] = { IntVec2(0,1), IntVec2(0,-1), IntVec2(1,0), IntVec2(-1,0) };

	TileHeatMap* solidMap = treatWaterAsSolid ? m_solidMapForLandBased : m_solidMapForAmphibians;
	float currentSearchValue = 0.f;
	bool isHeatSpreading = true;

	while (isHeatSpreading)
	{
		isHeatSpreading = false;

		for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
		{
			for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
			{
				IntVec2 const currentTileCoords = IntVec2(tileX, tileY);
				float currentValue = out_distanceField.GetValueAtCoords(currentTileCoords);
				if (currentValue == currentSearchValue)
				{
					isHeatSpreading = true; // or on neighbor value changed, one more round of search
					for (int i = 0; i < 4; ++i) // four direction
					{
						IntVec2 neighborTileCoords = currentTileCoords + directions[i];
						if (!solidMap->IsInBounds(neighborTileCoords))
						{
							continue;
						}
						if (solidMap->GetValueAtCoords(neighborTileCoords) == SOLID_MAP_TRUE_VALUE)
						{
							continue;
						}
						float neighborValue = out_distanceField.GetValueAtCoords(neighborTileCoords);
						if (neighborValue <= (currentValue + 1.f)) // step is 1.f
						{
							continue;
						}
						out_distanceField.SetValueAtCoords(neighborTileCoords, currentValue + 1.f);
					}
				}
			}
		}

		currentSearchValue += 1.f;
	}
}

void Map::GeneratePathToGoal(std::vector<IntVec2>& pathPoints, IntVec2 const& startPos, TileHeatMap const& distMap)
{
	pathPoints.clear();
	float currentHeat = distMap.GetValueAtCoords(startPos);
	if (currentHeat == SPECIAL_VALUE)
	{
		return; // no path to the goal
	}
	if (currentHeat <= 0.f)
	{
		pathPoints.push_back(startPos);
		return;
	}
	IntVec2 currentPos = startPos;
	IntVec2 directions[4] = { IntVec2(0,1), IntVec2(0,-1), IntVec2(1,0), IntVec2(-1,0) };
	while (true)
	{
		for (int i = 0; i < 4; ++i)
		{
			IntVec2 nextStepPos = currentPos + directions[i];
			if (!distMap.IsInBounds(nextStepPos))
			{
				continue;
			}
			float nextStepHeat = distMap.GetValueAtCoords(nextStepPos);
			if (nextStepHeat < currentHeat)
			{
				currentHeat = nextStepHeat;
				currentPos = nextStepPos;
				pathPoints.push_back(currentPos);
				break;
			}
		}
		if (currentHeat <= 0.f)
		{
			int n = (int)pathPoints.size();
			for (int i = 0; i < n / 2; ++i)
			{
				IntVec2 temp = pathPoints[i];
				pathPoints[i] = pathPoints[n - 1 - i];
				pathPoints[n - 1 - i] = temp;
			}
			return;
		}
	}
	//loop until the value is 0
}

void Map::UpdateDistanceMapFromStart()
{
	if (m_distanceMapFromStart)
	{
		delete m_distanceMapFromStart;
		m_distanceMapFromStart = nullptr;
	}

	m_distanceMapFromStart = new TileHeatMap(m_dimensions, SPECIAL_VALUE);
	PopulateDistanceField(*m_distanceMapFromStart, IntVec2(1, 1), SPECIAL_VALUE);
}

void Map::UpdateSolidMaps()
{
	if (m_isSolidMapDirty)
	{
		UpdateSolidMapForAmphibians();
		UpdateSolidMapForLandBased();
		m_isSolidMapDirty = false;
	}
}

void Map::UpdateSolidMapForAmphibians()
{
	if (m_solidMapForAmphibians)
	{
		delete m_solidMapForAmphibians;
		m_solidMapForAmphibians = nullptr;
	}

	m_solidMapForAmphibians = new TileHeatMap(m_dimensions, SOLID_MAP_FALSE_VALUE);

	for (int tileIndex = 0; tileIndex < (int)m_tiles.size(); ++tileIndex)
	{
		Tile const& tile = m_tiles[tileIndex];
		bool isSolid = IsTileSolid(tile.m_tileCoords, false);
		if (isSolid)
		{
			m_solidMapForAmphibians->SetValueAtIndex(tileIndex, SOLID_MAP_TRUE_VALUE);
		}
	}

	EntityList const& scorpioList = m_entitiesByType[static_cast<int>(EntityType::SCORPIO)];
	for (int index = 0; index < (int)scorpioList.size(); ++index)
	{
		Entity const* entity = scorpioList[index];
		if (entity)
		{
			IntVec2 tileCoords = GetTileCoordsForWorldPos(entity->m_position);
			if (!IsTileInBounds(tileCoords)) // skip potential out of bound entity
			{
				continue;
			}
			int tileIndex = GetTileIndexForCoords(tileCoords);
			m_solidMapForAmphibians->SetValueAtIndex(tileIndex, SOLID_MAP_TRUE_VALUE);
		}
	}
}

void Map::UpdateSolidMapForLandBased()
{
	if (m_solidMapForLandBased)
	{
		delete m_solidMapForLandBased;
		m_solidMapForLandBased = nullptr;
	}

	m_solidMapForLandBased = new TileHeatMap(m_dimensions, SOLID_MAP_FALSE_VALUE);

	for (int tileIndex = 0; tileIndex < (int)m_tiles.size(); ++tileIndex)
	{
		Tile const& tile = m_tiles[tileIndex];
		bool isSolid = IsTileSolid(tile.m_tileCoords, true);
		if (isSolid)
		{
			m_solidMapForLandBased->SetValueAtIndex(tileIndex, SOLID_MAP_TRUE_VALUE);
		}
	}

	EntityList const& scorpioList = m_entitiesByType[static_cast<int>(EntityType::SCORPIO)];
	for (int index = 0; index < (int)scorpioList.size(); ++index)
	{
		Entity const* entity = scorpioList[index];
		if (entity)
		{
			IntVec2 tileCoords = GetTileCoordsForWorldPos(entity->m_position);
			if (!IsTileInBounds(tileCoords)) // skip potential out of bound entity
			{
				continue;
			}
			int tileIndex = GetTileIndexForCoords(tileCoords);
			m_solidMapForLandBased->SetValueAtIndex(tileIndex, SOLID_MAP_TRUE_VALUE);
		}
	}
}

void Map::RenderTileRenderModeInfo() const
{
	std::string info = "";
	switch (m_currentTileRenderMode)
	{
	case TileRenderMode::DISTANCE_MAP_FROM_START:
		info = "Heat Map Debug: Distance Map from start (F6 for next mode)";
		break;
	case TileRenderMode::SOLID_MAP_FOR_AMPHIBIANS:
		info = "Heat Map Debug: Solid Map for amphibians (F6 for next mode)";
		break;
	case TileRenderMode::SOLID_MAP_FOR_LAND_BASED:
		info = "Heat Map Debug: Solid Map for land-based (F6 for next mode)";
		break;
	case TileRenderMode::DISTANCE_MAP_FOR_SELECTED:
		info = "(NOT DEVELOPED)Heat Map Debug: Distance Map to selected Entity's goal (F6 for next mode)";
		// TODO check if no entity is selected, show different text
		break;
	}

	BitmapFont* font = nullptr;
	font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	std::vector<Vertex_PCU> textVerts;
	font->AddVertsForText2D(textVerts, Vec2(2.f, SCREEN_SIZE_Y - 22.f), 20.f, info, Rgba8::GREEN, 0.6f);
	g_theRenderer->BindTexture(&font->GetTexture());
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(textVerts);

}

void Map::RenderDefaultMap() const
{
	std::vector<Vertex_PCU> tileVerts;
	constexpr int VERTS_PER_TILE = 6;
	tileVerts.reserve(m_tiles.size() * VERTS_PER_TILE);
	for (int tileIndex = 0; tileIndex < (int)m_tiles.size(); ++tileIndex)
	{
		Vec2 mins(static_cast<float>(m_tiles[tileIndex].m_tileCoords.x), static_cast<float>(m_tiles[tileIndex].m_tileCoords.y));
		Vec2 maxs = mins + Vec2::ONE;
		AABB2 tileAABB2(mins, maxs);
		TileDefinition const& tileDef = *(m_tiles[tileIndex].m_tileDef);
		AddVertsForAABB2D(tileVerts, tileAABB2, tileDef.m_tint, tileDef.m_UVs);
	}
	g_theRenderer->BindTexture(&g_terrainSpriteSheet->GetTexture());
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(tileVerts);
}

void Map::RenderDistanceMapFromStart() const
{
	if (!m_distanceMapFromStart)
	{
		// Warning
		return;
	}
	std::vector<Vertex_PCU> verts;
	m_distanceMapFromStart->AddVertsForDebugDraw(verts, AABB2(Vec2::ZERO, Vec2(m_dimensions)), 
		m_distanceMapFromStart->GetRangeOffValuesExcludingSpecial(SPECIAL_VALUE), 
		Rgba8(0, 0, 0), Rgba8::OPAQUE_WHITE, SPECIAL_VALUE, Rgba8::BLUE);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);

}

void Map::RenderSolidMapForAmphibians() const
{
	if (!m_solidMapForAmphibians)
	{
		// Warning
		return;
	}
	std::vector<Vertex_PCU> verts;
	m_solidMapForAmphibians->AddVertsForDebugDraw(verts, AABB2(Vec2::ZERO, Vec2(m_dimensions)), 
		FloatRange(SOLID_MAP_FALSE_VALUE, SOLID_MAP_TRUE_VALUE), Rgba8(0, 0, 0), Rgba8::OPAQUE_WHITE);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}

void Map::RenderSolidMapForLandBased() const
{
	if (!m_solidMapForLandBased)
	{
		return;
	}
	std::vector<Vertex_PCU> verts;
	m_solidMapForLandBased->AddVertsForDebugDraw(verts, AABB2(Vec2::ZERO, Vec2(m_dimensions)), 
		FloatRange(SOLID_MAP_FALSE_VALUE, SOLID_MAP_TRUE_VALUE), Rgba8(0, 0, 0), Rgba8::OPAQUE_WHITE);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}



#pragma endregion HeatMap


//-----------------------------------------------------------------------------------------------
// Ray Cast
//
RaycastResult2D Map::RaycastVsTiles(Ray2 const& ray, bool treatWaterAsSolid) const
{
	RaycastResult2D result;
	result.m_ray = ray;
	constexpr int STEPS_PER_UNIT = 100;
	constexpr float DIST_PER_STEP = 1 / static_cast<float>(STEPS_PER_UNIT);
	int numSteps = static_cast<int>(ray.m_maxLength * static_cast<float>(STEPS_PER_UNIT));

	// Take lots of little steps, until we run out of length OR hit something solid!
	IntVec2 prevTileCoords = GetTileCoordsForWorldPos(ray.m_startPos);
	for (int step = 0; step < numSteps; ++step)
	{
		float stepForwardDist = DIST_PER_STEP * static_cast<float>(step);
		Vec2 pos = ray.m_startPos + (ray.m_fwdNormal * stepForwardDist);
		IntVec2 currentTileCoords = GetTileCoordsForWorldPos(pos);

		// Impact!
		if (IsPointInSolid(pos, treatWaterAsSolid))
		{
			IntVec2 impactNormal = prevTileCoords - currentTileCoords;
			result.m_didImpact = true;
			result.m_impactPos = pos;
			result.m_impactDist = stepForwardDist;
			result.m_impactNormal = Vec2(impactNormal);
			return result;
		}
		prevTileCoords = currentTileCoords;
	}
	// Miss!
	result.m_impactDist = ray.m_maxLength;
	return result;
}

RaycastResult2D Map::FastVoxelRaycast(Ray2 const& ray, bool treatWaterAsSolid) const
{
	RaycastResult2D result;
	result.m_ray = ray; // original info
	int tileX = RoundDownToInt(ray.m_startPos.x);
	int tileY = RoundDownToInt(ray.m_startPos.y);

	if (IsTileSolid(IntVec2(tileX, tileY), treatWaterAsSolid))
	{
		result.m_didImpact = true;
		result.m_impactPos = ray.m_startPos;
		result.m_impactNormal = -ray.m_fwdNormal;
		return result;
	}

	float fwdDistPerXCrossing = 1.0f / fabsf(ray.m_fwdNormal.x); // 1 / cos theta
	int tileStepDirectionX = (ray.m_fwdNormal.x < 0.f) ? -1 : 1;
	float xAtFirstXCrossing = static_cast<float>(tileX) + static_cast<float>(tileStepDirectionX + 1) * 0.5f;
	float xDistToFirstXCrossing = xAtFirstXCrossing - ray.m_startPos.x;
	float fwdDistAtNextXCrossing = fabsf(xDistToFirstXCrossing) * fwdDistPerXCrossing; // forward direction distance


	float fwdDistPerYCrossing = 1.0f / fabsf(ray.m_fwdNormal.y); // 1 / sin theta
	int tileStepDirectionY = (ray.m_fwdNormal.y < 0.f) ? -1 : 1;
	float yAtFirstYCrossing = static_cast<float>(tileY) + static_cast<float>(tileStepDirectionY + 1) * 0.5f;
	float yDistToFirstYCrossing = yAtFirstYCrossing - ray.m_startPos.y;
	float fwdDistAtNextYCrossing = fabsf(yDistToFirstYCrossing) * fwdDistPerYCrossing; // forward direction distance

	while (true)
	{
		if (fwdDistAtNextXCrossing <= fwdDistAtNextYCrossing)
		{
			if (fwdDistAtNextXCrossing > ray.m_maxLength)
			{
				result.m_impactDist = ray.m_maxLength;
				return result;
			}
			tileX += tileStepDirectionX;
			if (IsTileSolid(IntVec2(tileX, tileY), treatWaterAsSolid))
			{
				result.m_didImpact = true;
				result.m_impactDist = fwdDistAtNextXCrossing;
				result.m_impactPos = ray.m_startPos + result.m_impactDist * ray.m_fwdNormal;
				result.m_impactNormal = Vec2(-static_cast<float>(tileStepDirectionX), 0.f);
				return result;
			}
			fwdDistAtNextXCrossing += fwdDistPerXCrossing;
		}
		else
		{
			if (fwdDistAtNextYCrossing > ray.m_maxLength)
			{
				result.m_impactDist = ray.m_maxLength;
				return result;
			}
			tileY += tileStepDirectionY;
			if (IsTileSolid(IntVec2(tileX, tileY), treatWaterAsSolid))
			{
				result.m_didImpact = true;
				result.m_impactDist = fwdDistAtNextYCrossing;
				result.m_impactPos = ray.m_startPos + result.m_impactDist * ray.m_fwdNormal;
				result.m_impactNormal = Vec2(-static_cast<float>(tileStepDirectionY), 0.f);
				return result;
			}
			fwdDistAtNextYCrossing += fwdDistPerYCrossing;
		}
	}
}

bool Map::HasLineOfSight(Vec2 const& startPos, Vec2 const& targetPos, float range, bool treatWaterAsSolid) const
{
	Vec2 disp = targetPos - startPos;
	float squaredDistance = disp.GetLengthSquared();
	if (squaredDistance >= range * range)
	{
		return false;
	}
	Vec2 fwdNormal = disp.GetNormalized();
	RaycastResult2D result = FastVoxelRaycast(Ray2(startPos, fwdNormal, range), treatWaterAsSolid);
	return squaredDistance < result.m_impactDist * result.m_impactDist; // Miss ImpactDist will be range
}

void Map::SpawnExplosion(float importance, Vec2 position)
{
	Entity* explosion = new Explosion(importance, position);
	m_explosions.push_back(explosion);
}

void Map::UpdateExplosions(float deltaSeconds)
{
	for (int entityIndex = 0; entityIndex < (int)m_explosions.size(); ++entityIndex)
	{
		Entity* entity = m_explosions[entityIndex];
		if (IsAlive(entity))
		{
			entity->Update(deltaSeconds);
		}
	}
}

void Map::RenderExplosions() const
{
	for (int entityIndex = 0; entityIndex < (int)m_explosions.size(); ++entityIndex)
	{
		Entity* entity = m_explosions[entityIndex];
		if (IsAlive(entity))
		{
			entity->Render();
		}
	}
}
