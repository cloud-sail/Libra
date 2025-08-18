#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Tile.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vec2.hpp"
#include <vector>
#include <string>
//-----------------------------------------------------------------------------------------------
class SpriteSheet;

//-----------------------------------------------------------------------------------------------
struct TileDefinition
{
	static TileDefinition* GetTileDef(std::string const& tileDefName);

	static std::vector<TileDefinition*> s_tileDefs;

	static void InitializeTileDefs();
	static IntVec2 s_dimensions;
	static const char* s_tileImagePath;

	TileDefinition(XmlElement const& tileDefElement);

	std::string m_name;
	AABB2 m_UVs = AABB2::ZERO_TO_ONE; // Same effect as spriteIndex
	Rgba8 m_tint = Rgba8::OPAQUE_WHITE;
	Rgba8 m_mapColor = Rgba8::TRANSPARENT_BLACK;
	bool m_isSolid = false; // Do not access it directly outside of Map Function
	bool m_isWater = false; // Do not access it directly outside of Map Function


	// std::vector<AABB2> m_possibleUVs; // store variant of this type of tiles
	// std::vector<float> m_possibility;
	// m_tileType = UN
	// m_isOpaque = false;
	// std::string m_name
	// float m_flammability
	// m_health = 100
	// TileType m_typeToChangeToWhenDestroyed
	// SoundID soundWhenDrivingOver



};
