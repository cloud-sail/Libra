#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Tile.hpp"
#include "Game/Entity.hpp"
#include "Game/MapDefinition.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/Camera.hpp"
#include <vector>

// todo check entity is garbage or deleted show no entity is selected AFter garbage collection
// todo create a new entitylist contain entity with distance maps
// or add distance maps ptr to all entity
enum class TileRenderMode
{
	DEFAULT_MAP,
	DISTANCE_MAP_FROM_START,
	SOLID_MAP_FOR_AMPHIBIANS,
	SOLID_MAP_FOR_LAND_BASED,
	DISTANCE_MAP_FOR_SELECTED,
	NUM
};


//-----------------------------------------------------------------------------------------------
class Map
{
public:
	Map(MapDefinition const& mapDef);
	~Map();

	void Update(float deltaSeconds);
	void Render() const;

public:
	int GetTileIndexForCoords(IntVec2 const& tileCoords) const;
	int GetTileIndexForCoords(int coordX, int coordY);
	IntVec2 GetTileCoordsForWorldPos(Vec2 const& worldPos) const;
	AABB2 GetTileBounds(IntVec2 const& tileCoords) const;
	AABB2 GetTileBounds(int tileIndex) const;
	bool IsTileInBounds(IntVec2 const& tileCoords) const;
	bool IsTileInEdges(IntVec2 const& tileCoords) const;
	bool IsTileInStartArea(IntVec2 const& tileCoords) const;
	bool IsTileInEndArea(IntVec2 const& tileCoords) const;
	bool IsIntVec2InRectangle(IntVec2 const& mins, IntVec2 const& maxs, IntVec2 const& point) const;
	bool IsTileSolid(IntVec2 const& tileCoords, bool treatWaterAsSolid = false) const;
	bool IsPointInSolid(Vec2 const& worldPos, bool treatWaterAsSolid = false) const;
	Vec2 GetTileCenter(IntVec2 const& tileCoords) const;

	RaycastResult2D RaycastVsTiles(Ray2 const& ray, bool treatWaterAsSolid = false) const;
	RaycastResult2D FastVoxelRaycast(Ray2 const& ray, bool treatWaterAsSolid = false) const;
	// TODO make a ray cast with Bit Mask
	bool HasLineOfSight(Vec2 const& startPos, Vec2 const& targetPos, float range = 10.f, bool treatWaterAsSolid = false) const;

	Entity* SpawnNewEntity(EntityType type, EntityFaction faction, Vec2 const& position, float orientationDegrees);
	void AddEntityToMap(Entity* entity, Vec2 const& position, float orientationDegrees);
	void RemoveEntityFromMap(Entity* entity);

	bool IsAlive(Entity* entity) const;

	bool IsBullet(Entity* entity) const;
	bool IsAgent(Entity* entity) const;
	bool HasPhysics(Entity* entity) const;

	IntVec2 GetDimensions();
	IntVec2 GenerateRandomTileCoordsInMap();

private:
	// Update TODO
	void UpdateEntities(float deltaSeconds);
	void CheckIfPlayerReachExit();
	void PushOffBetweenEntities();
	void PushEntitiesOutofWalls();
	void CheckBulletVsEntities();
	void DeleteGarbageEntities(); // remove first then delete it

	bool IsGarbage(Entity* entity) const;
	void PushOffBetweenBothEntities(Entity& a, Entity& b);
	void PushEntityOutOfSolidTiles(Entity& entity);
	void CheckBulletListVsEntityList(EntityList const& bulletList, EntityList const& entityList);
	void HandleCollision(Entity& a, Entity& b);
	//void PushEntityOutOfTileIfSolid(Entity& entity, IntVec2 const& tileCoords);



	// Render
	void RenderTiles() const;
	void RenderEntities() const;
	void RenderDebugDraw() const;
	void RenderUI() const;
	void RenderHealthBar() const;

	// Camera
	void UpdateCameras(float deltaSeconds);
	AABB2 CalculateDebugCameraView();
	AABB2 CalculateFollowCameraView();
	void ClampCameraView(AABB2& cameraView);

	// Initialization
	void InitializeTiles();
	void FillTiles();
	void PopulateTiles(); // TODO return bool to indicate the map is ok or use another function to check
	void BuildWorms();
	void BuildWormTilesWithParameters(std::string tileType, int count, int maxLength);
	void BuildEdges();
	void BuildStartArea();
	void BuildEndArea();
	void SetTileDefInRectangle(IntVec2 const& mins, IntVec2 const& maxs, TileDefinition* tileDef);
	void SetTileDefInPoint(IntVec2 const& tileCoords, TileDefinition* tileDef);
	void SpawnInitialNPCs();
	void SpawnInitialNPCsRandomlyByCount(EntityType type, EntityFaction faction, int count);
	bool IsMapValid();
	void FillNonSolidAndUnreachableTiles();

	// Entity Management
	Entity* CreateNewEntity(EntityType type, EntityFaction faction);
	void AddEntityToList(Entity* entityToAdd, EntityList& list);
	void RemoveEntityFromList(Entity* entityToRemove, EntityList& list);



private:
	MapDefinition m_mapDef;
	IntVec2 m_dimensions;
	std::vector<Tile> m_tiles;
	EntityList m_allEntities;
	EntityList m_entitiesByType[static_cast<int>(EntityType::NUM)];
	EntityList m_agentsByFaction[static_cast<int>(EntityFaction::NUM)];
	EntityList m_bulletsByFaction[static_cast<int>(EntityFaction::NUM)];
	EntityList m_entitesWithPhysics;
	// physics_list


	Camera m_worldCamera;
	Vec2 m_exitPosition;

public:
	void SwitchToNextTileRenderMode();
	void SetSolidMapDirty();
	void GenerateDistanceMapFromSolidMap(TileHeatMap& out_distanceField, IntVec2 startCoords, bool treatWaterAsSolid);
	void GeneratePathToGoal(std::vector<IntVec2>& pathPoints, IntVec2 const& startPos, TileHeatMap const& distMap);

private:
	void PopulateDistanceField(TileHeatMap& out_distanceField, IntVec2 startCoords, float maxCost, bool treatWaterAsSolid = true);



	void UpdateDistanceMapFromStart();
	void UpdateSolidMaps();
	void UpdateSolidMapForAmphibians(); // only called by UpdateSolidMaps
	void UpdateSolidMapForLandBased(); // only called by UpdateSolidMaps


	void RenderTileRenderModeInfo() const;
	void RenderDefaultMap() const;
	void RenderDistanceMapFromStart() const;
	void RenderSolidMapForAmphibians() const;
	void RenderSolidMapForLandBased() const;

	// TODO check and change before render selected entity

private:
	TileRenderMode m_currentTileRenderMode = TileRenderMode::DEFAULT_MAP;

	TileHeatMap* m_distanceMapFromStart = nullptr;
	TileHeatMap* m_solidMapForAmphibians = nullptr;
	TileHeatMap* m_solidMapForLandBased = nullptr;
	bool m_isSolidMapDirty = true;



	//TileHeatMap* m_testHeatMap = nullptr;

public:
	void SpawnExplosion(float importance, Vec2 position);

private:
	void UpdateExplosions(float deltaSeconds);
	void RenderExplosions() const;
	EntityList m_explosions;

};


// typedef int TileIndex;
// typedef IntVec2 TileCoords;
// typedef std::vector<Entity*> EntityList;
// typedef std::vector<Vertex_PCU> Mesh;
