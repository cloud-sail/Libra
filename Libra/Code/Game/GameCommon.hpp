#pragma once
#include "Engine/Core/EngineCommon.hpp"

//-----------------------------------------------------------------------------------------------
class InputSystem;
class Window;
class Renderer;
class AudioSystem;
class App;
class Game;
class Map;
struct Tile;
class Entity;
class PlayerTank;
class RandomNumberGenerator;
class SpriteSheet;
class Texture;
class TileHeatMap; 
class SpriteAnimDefinition; 
class Explosion;
class Flame;
class Capricorn;

struct Vec2;
struct Rgba8;
struct AABB2;

//-----------------------------------------------------------------------------------------------
extern AudioSystem*		g_theAudio;
extern InputSystem*		g_theInput;
extern Renderer*		g_theRenderer;
extern Window*			g_theWindow;
extern App*				g_theApp;
extern RandomNumberGenerator g_rng;

//-----------------------------------------------------------------------------------------------
extern bool g_isDebugDraw;
extern Game* g_theGame;
extern SpriteSheet* g_terrainSpriteSheet;
extern SpriteSheet* g_explosionSpriteSheet;

//-----------------------------------------------------------------------------------------------
// Gameplay Constants
constexpr float SCREEN_SIZE_X = 1600.f;
constexpr float SCREEN_SIZE_Y = 800.f;

constexpr int TERRAIN_SHEET_SIZE_X = 8;
constexpr int TERRAIN_SHEET_SIZE_Y = 8;

constexpr float DEBUG_DRAW_THICKNESS = 0.03f;

constexpr int DEFAULT_START_AREA = 5;
constexpr int DEFAULT_END_AREA = 6;

constexpr float SOLID_MAP_TRUE_VALUE = 1.f;
constexpr float SOLID_MAP_FALSE_VALUE = 0.f;

constexpr float SPECIAL_VALUE = 9999.f;


//-----------------------------------------------------------------------------------------------
Vec2 GetPointOnSquare(float t);
