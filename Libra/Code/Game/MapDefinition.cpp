#include "Game/MapDefinition.hpp"
#include "Game/GameCommon.hpp"

std::vector<MapDefinition*> MapDefinition::s_mapDefs;

MapDefinition* MapDefinition::GetMapDef(std::string const& mapDefName)
{
	for (int i = 0; i < (int)s_mapDefs.size(); ++i)
	{
		if (s_mapDefs[i]->m_name == mapDefName)
		{
			return s_mapDefs[i];
		}
	}

	ERROR_AND_DIE(Stringf("Map is not in MapDefs: \"%s\"", mapDefName.c_str()));

	//return nullptr;
}


void MapDefinition::InitializeMapDefs()
{
	for (int i = 0; i < (int)s_mapDefs.size(); ++i)
	{
		delete s_mapDefs[i];
	}
	s_mapDefs.clear();

	XmlDocument mapDefsXml;
	char const* filePath = "Data/Definitions/MapDefinitions.xml";

	XmlResult result = mapDefsXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open required map defs file \"%s\"", filePath));

	XmlElement* rootElement = mapDefsXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, Stringf("No elements in map defs file \"%s\"", filePath));

	XmlElement* mapDefElement = rootElement->FirstChildElement();
	while (mapDefElement)
	{
		std::string elementName = mapDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "MapDefinition", Stringf("Root child element in %s was <%s>, must be <MapDefinition>!", filePath, elementName.c_str()));
		MapDefinition* newMapDef = new MapDefinition(*mapDefElement);
		s_mapDefs.push_back(newMapDef);

		mapDefElement = mapDefElement->NextSiblingElement();
	}
}

MapDefinition::MapDefinition(XmlElement const& mapDefElement)
{
	m_name			= ParseXmlAttribute(mapDefElement, "name", m_name);
	m_dimensions	= ParseXmlAttribute(mapDefElement, "dimensions", m_dimensions);
	m_fillTileType	= ParseXmlAttribute(mapDefElement, "fillTileType", m_fillTileType);
	m_edgeTileType	= ParseXmlAttribute(mapDefElement, "edgeTileType", m_edgeTileType);

	m_worm1TileType		= ParseXmlAttribute(mapDefElement, "worm1TileType", m_worm1TileType);
	m_worm1Count		= ParseXmlAttribute(mapDefElement, "worm1Count", m_worm1Count);
	m_worm1MaxLength	= ParseXmlAttribute(mapDefElement, "worm1MaxLength", m_worm1MaxLength);
	m_worm2TileType		= ParseXmlAttribute(mapDefElement, "worm2TileType", m_worm2TileType);
	m_worm2Count		= ParseXmlAttribute(mapDefElement, "worm2Count", m_worm2Count);
	m_worm2MaxLength	= ParseXmlAttribute(mapDefElement, "worm2MaxLength", m_worm2MaxLength);

	m_startFloorTileType	= ParseXmlAttribute(mapDefElement, "startFloorTileType", m_startFloorTileType);
	m_startBunkerTileType	= ParseXmlAttribute(mapDefElement, "startBunkerTileType", m_startBunkerTileType);
	m_endFloorTileType		= ParseXmlAttribute(mapDefElement, "endFloorTileType", m_endFloorTileType);
	m_endBunkerTileType		= ParseXmlAttribute(mapDefElement, "endBunkerTileType", m_endBunkerTileType);

	m_leoCount		= ParseXmlAttribute(mapDefElement, "leoCount", m_leoCount);
	m_ariesCount	= ParseXmlAttribute(mapDefElement, "ariesCount", m_ariesCount);
	m_scorpioCount	= ParseXmlAttribute(mapDefElement, "scorpioCount", m_scorpioCount);
	m_capricornCount = ParseXmlAttribute(mapDefElement, "capricornCount", m_capricornCount);
}
