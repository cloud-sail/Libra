#pragma once
#include "Engine/Math/IntVec2.hpp"

//-----------------------------------------------------------------------------------------------
struct TileDefinition;

//-----------------------------------------------------------------------------------------------
struct Tile
{
public:
	Tile() {};

public:
	IntVec2 m_tileCoords;
	// cannot change the tileDefinition, but the pointer can be changed (point to another tileDefinition)
	TileDefinition const* m_tileDef = nullptr; 
};

