#pragma once
#include "Game/Tile.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <vector>
#include <string>

struct MapDefinition
{

	static std::vector<MapDefinition*> s_mapDefs;
	static MapDefinition* GetMapDef(std::string const& mapDefName);
	static void InitializeMapDefs();
	MapDefinition(XmlElement const& mapDefElement);



	std::string m_name = "testMap";
	IntVec2 m_dimensions = IntVec2(48, 24);

	std::string m_fillTileType = "Grass";
	std::string m_edgeTileType = "RockWall";

	std::string m_worm1TileType = "DarkGrass";
	int m_worm1Count = 15;
	int m_worm1MaxLength = 12;

	std::string m_worm2TileType = "RockWall";
	int m_worm2Count = 60;
	int m_worm2MaxLength = 8;

	std::string m_startFloorTileType = "Concrete";
	std::string m_startBunkerTileType = "RockWall";
	std::string m_endFloorTileType = "Concrete";
	std::string m_endBunkerTileType = "StoneWall";


	int m_leoCount = 1;
	int m_ariesCount = 1;
	int m_scorpioCount = 1;
	int m_capricornCount = 1;
};

