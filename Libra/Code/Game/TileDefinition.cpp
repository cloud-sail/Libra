#include "Game/TileDefinition.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Renderer.hpp"

//std::vector<TileDefinition> TileDefinition::s_definitions; // TODO remove

std::vector<TileDefinition*> TileDefinition::s_tileDefs;
IntVec2 TileDefinition::s_dimensions(8, 8);
const char* TileDefinition::s_tileImagePath = "Data/Images/Terrain_8x8.png";

TileDefinition* TileDefinition::GetTileDef(std::string const& tileDefName)
{	
	// Vector / Map to store tileDef* are both ok
	for (int i = 0; i < (int)s_tileDefs.size(); ++i)
	{
		if (s_tileDefs[i]->m_name == tileDefName)
		{
			return s_tileDefs[i];
		}
	}

	ERROR_AND_DIE(Stringf("Tile is not in tileDefs: \"%s\"", tileDefName.c_str()));

	//return nullptr;
}

void TileDefinition::InitializeTileDefs()
{
	for (int i = 0; i < (int)s_tileDefs.size(); ++i)
	{
		delete s_tileDefs[i];
	}
	s_tileDefs.clear();
	// TODO clear old tile defs in s_mapDefs after F8
	Texture* terrainTexture = g_theRenderer->CreateOrGetTextureFromFile(s_tileImagePath);
	// TODO delete old SpriteSheet and new One?
	g_terrainSpriteSheet = new SpriteSheet(*terrainTexture, s_dimensions);

	XmlDocument tileDefsXml;
	char const* filePath = "Data/Definitions/TileDefinitions.xml";
	XmlResult result = tileDefsXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open required tile defs file \"%s\"", filePath));

	XmlElement* rootElement = tileDefsXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, Stringf("No elements in tile defs file \"%s\"", filePath));

	XmlElement* tileDefElement = rootElement->FirstChildElement();
	while (tileDefElement)
	{
		std::string elementName = tileDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "TileDefinition", Stringf("Root child element in %s was <%s>, must be <TileDefinition>!", filePath, elementName.c_str()));
		TileDefinition* newTileDef = new TileDefinition(*tileDefElement);
		s_tileDefs.push_back(newTileDef);

		tileDefElement = tileDefElement->NextSiblingElement();
	}
}



TileDefinition::TileDefinition(XmlElement const& tileDefElement)
{
	m_name = ParseXmlAttribute(tileDefElement, "name", m_name);
	m_tint = ParseXmlAttribute(tileDefElement, "tint", m_tint);
	m_mapColor = ParseXmlAttribute(tileDefElement, "mapColor", m_mapColor); // ToFix Invalid?
	m_isSolid = ParseXmlAttribute(tileDefElement, "isSolid", m_isSolid);
	m_isWater = ParseXmlAttribute(tileDefElement, "isWater", m_isWater);
	
	IntVec2 spriteCoords = ParseXmlAttribute(tileDefElement, "spriteCoords", IntVec2(-1, -1));
	bool isSpriteCoordsValid =	(spriteCoords.x >= 0) && (spriteCoords.x < s_dimensions.x) &&
								(spriteCoords.y >= 0) && (spriteCoords.y < s_dimensions.y);
	GUARANTEE_OR_DIE(isSpriteCoordsValid, "Invalid SpriteCoords in TileDefinition: " + m_name);
	int spriteIndex = spriteCoords.x + (spriteCoords.y * s_dimensions.x);
	m_UVs = g_terrainSpriteSheet->GetSpriteUVs(spriteIndex);
}
