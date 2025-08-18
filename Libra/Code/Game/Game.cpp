#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/Sound.hpp"
#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/PlayerTank.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

//-----------------------------------------------------------------------------------------------
SpriteSheet* g_terrainSpriteSheet = nullptr;
SpriteSheet* g_explosionSpriteSheet = nullptr;

//-----------------------------------------------------------------------------------------------
Game::Game()
{
	TileDefinition::InitializeTileDefs();
	MapDefinition::InitializeMapDefs();

	InitializeMaps();
	InitializePlayerTank();
	InitializeSpriteAnim();

	ChangeState(GameState::ATTRACT);
}

Game::~Game()
{
	if (m_musicPlaybackID != MISSING_SOUND_ID)
	{
		g_theAudio->StopSound(m_musicPlaybackID);
	}
}

void Game::Update(float deltaSeconds)
{
	UpdateClock(deltaSeconds);
	UpdateCommonDeveloperCheats();
	UpdateCurrentState(deltaSeconds);
	UpdateCameras(deltaSeconds);

	if (g_theInput->WasKeyJustPressed(KEYCODE_Q))
	{
		AddTextToDevConsole();
	}
}

void Game::Render() const
{
	RenderCurrentState();
}


void Game::UpdateCameras(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	// Screen camera (for UI, HUD, Attract, etc.)
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}


void Game::AdjustForPauseAndTimeDistortion(float& deltaSeconds)
{
	float playBackSpeed = 1.f;
	XboxController const& controller = g_theInput->GetController(0);
	if ((g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE) || controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_BACK)) && !m_isPaused)
	{
		m_isPaused = true;
		g_theAudio->StartSound(Sound::PAUSE);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_P) || controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_START))
	{
		m_isPaused = !m_isPaused;
		if (m_isPaused)
		{
			g_theAudio->StartSound(Sound::PAUSE);
		}
		else
		{
			g_theAudio->StartSound(Sound::UNPAUSE);
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_O))
	{
		m_isPaused = false;
		m_isPausedAfterNextUpdate = true;
	}
	
	m_isSlowMo = g_theInput->IsKeyDown(KEYCODE_T) || controller.IsButtonDown(XboxButtonId::XBOX_BUTTON_LEFT_SHOULDER);
	m_isFastMo = g_theInput->IsKeyDown(KEYCODE_Y) || controller.GetLeftTrigger() > 0.f;


	if (m_isPaused)
	{
		deltaSeconds = 0.f;
		playBackSpeed = 0.f;
	}
	else
	{
		if (m_isPausedAfterNextUpdate)
		{
			deltaSeconds = 0.016f; // Press O Step Time
		}

		if (m_isFastMo && m_isSlowMo)
		{
			deltaSeconds *= 8.f;
			playBackSpeed = 2.f;
		}
		else if (m_isFastMo)
		{
			deltaSeconds *= 4.f;
			playBackSpeed = 1.5f;
		}
		else if (m_isSlowMo)
		{
			deltaSeconds *= 0.10f;
			playBackSpeed = 0.5f;
		}
	}

	SetPlayBackSpeed(playBackSpeed);

	if (m_isPausedAfterNextUpdate)
	{
		m_isPaused = true;
		m_isPausedAfterNextUpdate = false;
	}
}




void Game::InitializeMaps()
{
	for (int index = 0; index < (int)m_maps.size(); ++index)
	{
		delete m_maps[index];
	}
	m_maps.clear();

	std::string maps = g_gameConfigBlackboard.GetValue("maps", "Approach, Tunnel");
	Strings mapNames = SplitStringOnDelimiter(maps, ',');
	TrimSpaceInStrings(mapNames);

	for (int index = 0; index < (int)mapNames.size(); index++)
	{
		std::string mapName = mapNames[index];
		MapDefinition* mapDef = MapDefinition::GetMapDef(mapName);
		if (mapDef)
		{
			m_maps.push_back(new Map(*mapDef));
		}

	}

	m_currentMap = m_maps[0]; // current map is the first map
}

void Game::InitializePlayerTank()
{
	// DONT delete player Tank because map has delete it
	Entity* playerTank = m_currentMap->SpawnNewEntity(EntityType::PLAYERTANK, EntityFaction::GOOD, Vec2(1.5f, 1.5f), 30.f);
	m_playerTank = dynamic_cast<PlayerTank*>(playerTank);
	if (m_playerTank == nullptr)
	{
		ERROR_AND_DIE("Failed to initialize PlayerTank");
	}
}

void Game::ResetTheGame()
{
	InitializeMaps();
	InitializePlayerTank();
}

int Game::GetMapIndex(Map* map)
{
	for (int index = 0; index < (int)m_maps.size(); ++index)
	{
		if (map == m_maps[index])
		{
			return index;
		}
	}
	return -1;
}

void Game::PlayMusic(SoundID soundID)
{
	// TODO if musics are same, do nothing.
	if (m_musicPlaybackID != MISSING_SOUND_ID)
	{
		g_theAudio->StopSound(m_musicPlaybackID);
	}
	m_musicPlaybackID = g_theAudio->StartSound(soundID, true, 0.8f);
}

void Game::SetPlayBackSpeed(float playBackSpeed)
{
	if (m_musicPlaybackID == MISSING_SOUND_ID)
	{
		return;
	}
	g_theAudio->SetSoundPlaybackSpeed(m_musicPlaybackID, playBackSpeed);
}

void Game::UpdateCommonDeveloperCheats()
{
	XboxController const& controller = g_theInput->GetController(0);
	if (g_theInput->WasKeyJustPressed(KEYCODE_F1) || controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_UP))
	{
		g_isDebugDraw = !g_isDebugDraw;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F2) || controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_DOWN))
	{
		m_isInvincible = !m_isInvincible;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F3) || controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_LEFT))
	{
		m_isNoClip = !m_isNoClip;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F4) || controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_RIGHT))
	{
		m_isWholeMapView = !m_isWholeMapView;
	}
}

void Game::UpdateInGameDeveloperCheats(float& deltaSeconds)
{
	AdjustForPauseAndTimeDistortion(deltaSeconds);

	if (g_theInput->WasKeyJustPressed(KEYCODE_F9))
	{
		MoveToNextMap();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
	{
		m_currentMap->SwitchToNextTileRenderMode();
	}
}



PlayerTank* Game::GetPlayerTank()
{
	return m_playerTank;
}

void Game::MoveToNextMap()
{
	int currentMapIndex = GetMapIndex(m_currentMap);
	int nextMapIndex = currentMapIndex + 1;
	if (nextMapIndex >= (int)m_maps.size())
	{
		ChangeState(GameState::VICTORY);
		return;
	}
	g_theAudio->StartSound(Sound::NEXT_LEVEL);
	m_currentMap->RemoveEntityFromMap(m_playerTank);
	m_currentMap = m_maps[nextMapIndex];
	m_currentMap->AddEntityToMap(m_playerTank, Vec2(1.5f, 1.5f), 30.f);
}

void Game::OnPlayerDie()
{
	// check currentState?
	ChangeState(GameState::GAVEOVER);
}

Camera Game::GetUICamera()
{
	return m_screenCamera;
}

void Game::DrawScaledCircle() const
{
	// TODO use function pointer and some easing function to do animation for any texture, using OBB or AABB2 and rotate

	static constexpr float DEFAULT_RADIUS = 150.f;
	static constexpr float AMPLITUDE = 50.f;
	static constexpr float FREQUENCY = 360.f * 7.f / 6.f;
	std::vector<Vertex_PCU> verts;

	float radius = DEFAULT_RADIUS + AMPLITUDE * SinDegrees(FREQUENCY * GetElapsedTime());

	AddVertsForRing2D(verts, Vec2(800.f, 400.f), radius, 20.f, Rgba8::CYAN, 64);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}

void Game::DrawAttractScreen() const
{
	std::vector<Vertex_PCU> verts;
	AddVertsForAABB2D(verts, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), Rgba8::OPAQUE_WHITE);
	Texture* texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/AttractScreen.png");
	g_theRenderer->BindTexture(texture);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}

void Game::DrawGameOverScreen() const
{
	std::vector<Vertex_PCU> verts;
	AddVertsForAABB2D(verts, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), Rgba8::OPAQUE_WHITE);
	Texture* texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/YouDiedScreen.png");
	g_theRenderer->BindTexture(texture);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}

void Game::DrawVictoryScreen() const
{
	std::vector<Vertex_PCU> verts;
	AddVertsForAABB2D(verts, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), Rgba8::OPAQUE_WHITE);
	Texture* texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/VictoryScreen.jpg");
	g_theRenderer->BindTexture(texture);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}

void Game::DrawPausedPanel() const
{
	std::vector<Vertex_PCU> verts;
	AddVertsForAABB2D(verts, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), Rgba8(0, 0, 0, 128));
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}

void Game::DrawTestFont() const
{
	BitmapFont* testFont = nullptr;
	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	std::vector<Vertex_PCU> textVerts;
	testFont->AddVertsForText2D(textVerts, Vec2(350.f, 100.f), 30.f, "Hello, world");
	testFont->AddVertsForText2D(textVerts, Vec2(350.f, 700.f), 15.f, "It's nice to have options!", Rgba8::RED, 0.6f);
	g_theRenderer->BindTexture(&testFont->GetTexture());
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(textVerts);

}

void Game::DrawTestTileHeatMap() const
{
	IntVec2 const dimensions(32, 32);
	int const SIZE = dimensions.x * dimensions.y;
	TileHeatMap heatMap(dimensions);
	int currentIndex = RoundDownToInt(GetElapsedTime()* 100.f) % SIZE;
	for (int i = 0; i < SIZE; ++i)
	{
		if (i >= currentIndex)
		{
			heatMap.SetValueAtIndex(i, static_cast<float>(i - currentIndex));
		}
		else
		{
			heatMap.SetValueAtIndex(i, static_cast<float>(i - currentIndex + SIZE));
		}
	}
	std::vector<Vertex_PCU> verts;
	heatMap.AddVertsForDebugDraw(verts, AABB2(600.f, 200.f, 1000.f, 600.f), FloatRange(0.f, static_cast<float>(SIZE-1)), Rgba8(0,0,0,150), Rgba8(255, 0, 0, 150));
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);

}

void Game::DrawTextInBox() const
{
	AABB2 overrunBox;
	overrunBox.SetCenter(Vec2(1200.f, 400.f));
	overrunBox.SetDimensions(Vec2(300.f + 80.f * SinRadians(GetElapsedTime() * 0.6f), 100.f + 70.f * SinRadians(GetElapsedTime() * 0.7f)));

	AABB2 shrinkToFitBox;
	shrinkToFitBox.SetCenter(Vec2(400.f, 400.f));
	shrinkToFitBox.SetDimensions(Vec2(300.f + 80.f * SinRadians(GetElapsedTime() * 0.6f), 100.f + 70.f * SinRadians(GetElapsedTime() * 0.7f)));



	std::vector<Vertex_PCU> verts;
	AddVertsForAABB2D(verts, overrunBox, Rgba8(0, 0, 0));
	AddVertsForAABB2D(verts, shrinkToFitBox, Rgba8(0, 0, 0));
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);


	Vec2 alignment = GetPointOnSquare(GetElapsedTime());
	float cellAspect = 0.8f + 0.4f * SinRadians(GetElapsedTime());

	std::string text = Stringf("Text in box.\nalignment=(%.2f,%.2f)\n(overrun/shrink_to_fit)\ncellAspect=%.2f", alignment.x, alignment.y, cellAspect);

	int maxGlyphsToDraw = static_cast<int>(GetElapsedTime() * 10.f) % static_cast<int>(text.length());

	std::vector<Vertex_PCU> textVerts;
	BitmapFont* testFont = nullptr;
	testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	testFont->AddVertsForTextInBox2D(textVerts, text, overrunBox, 20.f, Rgba8(128,128,128), cellAspect, alignment, TextBoxMode::OVERRUN);
	testFont->AddVertsForTextInBox2D(textVerts, text, overrunBox, 20.f, Rgba8::YELLOW, cellAspect, alignment, TextBoxMode::OVERRUN, maxGlyphsToDraw);
	
	testFont->AddVertsForTextInBox2D(textVerts, text, shrinkToFitBox, 20.f, Rgba8(128, 128, 128), cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT);
	testFont->AddVertsForTextInBox2D(textVerts, text, shrinkToFitBox, 20.f, Rgba8::YELLOW, cellAspect, alignment, TextBoxMode::SHRINK_TO_FIT, maxGlyphsToDraw);
	g_theRenderer->BindTexture(&testFont->GetTexture());
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(textVerts);
}

void Game::DrawSpriteAnimation() const
{
	std::vector<Vertex_PCU> verts;
	SpriteDefinition spriteDefOnce		= m_spriteAnimOnce->GetSpriteDefAtTime(GetElapsedTime());
	SpriteDefinition spriteDefLoop		= m_spriteAnimLoop->GetSpriteDefAtTime(GetElapsedTime());
	SpriteDefinition spriteDefPingPong	= m_spriteAnimPingPong->GetSpriteDefAtTime(GetElapsedTime());
	AddVertsForAABB2D(verts, AABB2(0.f, 0.f, 150.f, 150.f), Rgba8::OPAQUE_WHITE, spriteDefOnce.GetUVs());
	AddVertsForAABB2D(verts, AABB2(150.f, 0.f, 300.f, 150.f), Rgba8::OPAQUE_WHITE, spriteDefLoop.GetUVs());
	AddVertsForAABB2D(verts, AABB2(300.f, 0.f, 450.f, 150.f), Rgba8::OPAQUE_WHITE, spriteDefPingPong.GetUVs());

	Texture* texture = &spriteDefPingPong.GetTexture();
	g_theRenderer->BindTexture(texture);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);

	std::vector<Vertex_PCU> explosionVerts;
	SpriteDefinition spriteDefExplosion = m_testExplosionAnim->GetSpriteDefAtTime(GetElapsedTime());
	AddVertsForAABB2D(explosionVerts, AABB2(450.f, 0.f, 600.f, 150.f), Rgba8::OPAQUE_WHITE, spriteDefExplosion.GetUVs());

	Texture* explosionTexture = &spriteDefExplosion.GetTexture();
	g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
	g_theRenderer->BindTexture(explosionTexture);
	g_theRenderer->DrawVertexArray(explosionVerts);
}

void Game::DrawTestImage() const
{
	AABB2 totalBounds = AABB2(1300.f, 650.f, 1600.f, 800.f);
	std::vector<Vertex_PCU> verts;
	IntVec2 dims = m_testImage.GetDimensions();
	verts.reserve(dims.x * dims.y * 6);

	int blurRadius = static_cast<int>(GetElapsedTime() * 5.f) % 5;
	Image testImage = m_testImage.GetBoxBlurred(blurRadius);

	for (int tileY = 0; tileY < dims.y; ++tileY)
	{
		for (int tileX = 0; tileX < dims.x; ++tileX)
		{
			float outMinX = RangeMap(static_cast<float>(tileX), 0.f, static_cast<float>(dims.x), totalBounds.m_mins.x, totalBounds.m_maxs.x);
			float outMaxX = RangeMap(static_cast<float>(tileX + 1), 0.f, static_cast<float>(dims.x), totalBounds.m_mins.x, totalBounds.m_maxs.x);
			float outMinY = RangeMap(static_cast<float>(tileY), 0.f, static_cast<float>(dims.y), totalBounds.m_mins.y, totalBounds.m_maxs.y);
			float outMaxY = RangeMap(static_cast<float>(tileY + 1), 0.f, static_cast<float>(dims.y), totalBounds.m_mins.y, totalBounds.m_maxs.y);

			AABB2 tileBounds = AABB2(outMinX, outMinY, outMaxX, outMaxY);
			AddVertsForAABB2D(verts, tileBounds, testImage.GetTexelColor(IntVec2(tileX, tileY)));
		}
	}

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->DrawVertexArray(verts);
}

void Game::InitializeSpriteAnim()
{
	int startIndex = 5;
	int endIndex = 15;
	float framesPerSecond = 2.f;
	Texture* spriteTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/TestSpriteSheet_4x4.png");
	SpriteSheet* testSpriteSheet = new SpriteSheet(*spriteTexture, IntVec2(4,4)); // Question: is it ok not to store the pointer? we store it in spritesheet spriteanim
	m_spriteAnimOnce = new SpriteAnimDefinition(*testSpriteSheet, startIndex, endIndex, framesPerSecond, SpriteAnimPlaybackType::ONCE);
	m_spriteAnimLoop = new SpriteAnimDefinition(*testSpriteSheet, startIndex, endIndex, framesPerSecond, SpriteAnimPlaybackType::LOOP);
	m_spriteAnimPingPong = new SpriteAnimDefinition(*testSpriteSheet, startIndex, endIndex, framesPerSecond, SpriteAnimPlaybackType::PINGPONG);

	Texture* explosionTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Explosion_5x5.png");
	SpriteSheet* explosionSpriteSheet = new SpriteSheet(*explosionTexture, IntVec2(5, 5));
	m_testExplosionAnim = new SpriteAnimDefinition(*explosionSpriteSheet, 0, 24, 50.f, SpriteAnimPlaybackType::LOOP);

	g_explosionSpriteSheet = new SpriteSheet(*explosionTexture, IntVec2(5, 5));
}

//-----------------------------------------------------------------------------------------------
// Game State Change
//
void Game::UpdateCurrentState(float deltaSeconds)
{
	switch (m_currentState)
	{
	case GameState::ATTRACT:
		UpdateStateAttract(deltaSeconds);
		break;
	case GameState::INGAME:
		UpdateStateInGame(deltaSeconds);
		break;
	case GameState::GAVEOVER:
		UpdateStateGameOver(deltaSeconds);
		break;
	case GameState::VICTORY:
		UpdateStateVictory(deltaSeconds);
		break;
	}
	
}

void Game::RenderCurrentState() const
{
	switch (m_currentState)
	{
	case GameState::ATTRACT:
		RenderStateAttract();
		break;
	case GameState::INGAME:
		RenderStateInGame();
		break;
	case GameState::GAVEOVER:
		RenderStateGameOver();
		break;
	case GameState::VICTORY:
		RenderStateVictory();
		break;
	}
}

void Game::ChangeState(GameState newState)
{
	ExitState(m_currentState);
	m_currentState = newState;
	EnterState(newState);
}

void Game::EnterState(GameState state)
{
	switch (state)
	{
	case GameState::ATTRACT:
		PlayMusic(Sound::ATTRACT_MUSIC);
		SetPlayBackSpeed(1.f); // Redundant setup
		break;
	case GameState::INGAME:
		PlayMusic(Sound::GAME_PLAY_MUSIC);
		m_isPaused = false; // Reset Paused
		break;
	case GameState::GAVEOVER:
		g_theAudio->StartSound(Sound::LOSE);
		SetPlayBackSpeed(0.f); // TODO choose a failure Music
		m_gameOverTimer = 3.f;
		break;
	case GameState::VICTORY:
		g_theAudio->StartSound(Sound::WIN);
		SetPlayBackSpeed(0.f); // TODO choose a victory Music
		break;
	}
}

void Game::ExitState(GameState state)
{
	switch (state)
	{
	case GameState::ATTRACT:
		break;
	case GameState::INGAME:
		break;
	case GameState::GAVEOVER:
		break;
	case GameState::VICTORY:
		ResetTheGame();
		break;
	}
}

void Game::UpdateStateAttract(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	CheckIfExitGame();

	if (CanTransitionTo(GameState::INGAME))
	{
		g_theAudio->StartSound(Sound::SELECT);
		ChangeState(GameState::INGAME);
		return;
	}
}

void Game::UpdateStateInGame(float deltaSeconds)
{
	if (CanTransitionTo(GameState::ATTRACT))
	{
		g_theAudio->StartSound(Sound::SELECT, false, 1.f, 0.f, 0.5f);
		ChangeState(GameState::ATTRACT);
		return;
	}
	// Transition To GameOver: Player Died
	// Transition To Victory: Player MoveTo the next map of the final one
	// Question: Check it every frame or changeState By other function?

	UpdateInGameDeveloperCheats(deltaSeconds);
	m_currentMap->Update(deltaSeconds);
}

void Game::UpdateStateGameOver(float deltaSeconds)
{
	if (CanTransitionTo(GameState::ATTRACT))
	{
		g_theAudio->StartSound(Sound::SELECT, false, 1.f, 0.f, 0.5f);
		ChangeState(GameState::ATTRACT);
		ResetTheGame();
		return;
	}
	if (CanTransitionTo(GameState::INGAME))
	{
		m_playerTank->Revive();
		ChangeState(GameState::INGAME);
	}

	if (m_gameOverTimer > 0.f)
	{
		m_gameOverTimer -= deltaSeconds;
	}

	if (m_gameOverTimer <= 0.f)
	{
		deltaSeconds = 0.f;
	}
	m_currentMap->Update(deltaSeconds);
}

void Game::UpdateStateVictory(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	if (CanTransitionTo(GameState::ATTRACT))
	{
		g_theAudio->StartSound(Sound::SELECT, false, 1.f, 0.f, 0.5f);
		ChangeState(GameState::ATTRACT);
	}

}

void Game::RenderStateAttract() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	DrawAttractScreen();
	DrawScaledCircle();
	//DrawTestFont();
	//DrawTestTileHeatMap();
	//DrawTextInBox();
	//DrawSpriteAnimation();
	//DrawTestImage();
	g_theRenderer->EndCamera(m_screenCamera);

}

void Game::RenderStateInGame() const
{
	// TODO move camera logic out of map
	m_currentMap->Render();

	g_theRenderer->BeginCamera(m_screenCamera);
	if (m_isPaused)
	{
		DrawPausedPanel();
	}
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderStateGameOver() const
{
	m_currentMap->Render();

	// UI
	g_theRenderer->BeginCamera(m_screenCamera);

	if (m_gameOverTimer <= 0.f)
	{
		DrawPausedPanel();
		DrawGameOverScreen();
	}

	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderStateVictory() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	DrawVictoryScreen();
	g_theRenderer->EndCamera(m_screenCamera);
}

bool Game::CanTransitionTo(GameState newState) const
{
	XboxController const& controller = g_theInput->GetController(0);
	GameState const& currentState = m_currentState;
	if (currentState == GameState::ATTRACT && newState == GameState::INGAME)
	{
		return (g_theInput->WasKeyJustPressed(KEYCODE_SPACE) ||
				g_theInput->WasKeyJustPressed(KEYCODE_N) ||
				g_theInput->WasKeyJustPressed(KEYCODE_P) ||
				controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_START) ||
				controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_A));
	}
	if (currentState == GameState::INGAME && newState == GameState::ATTRACT)
	{
		return (m_isPaused && 
				(g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE) ||
				controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_BACK)));
	}
	if (currentState == GameState::GAVEOVER && newState == GameState::ATTRACT)
	{
		return	(g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE) ||
				controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_BACK));
	}
	if (currentState == GameState::GAVEOVER && newState == GameState::INGAME)
	{
		return	(g_theInput->WasKeyJustPressed(KEYCODE_N) ||
				controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_START));
	}
	if (currentState == GameState::VICTORY && newState == GameState::ATTRACT)
	{
		return (g_theInput->WasKeyJustPressed(KEYCODE_SPACE) ||
			g_theInput->WasKeyJustPressed(KEYCODE_N) ||
			g_theInput->WasKeyJustPressed(KEYCODE_P) ||
			g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE) ||
			controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_START) ||
			controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_BACK) ||
			controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_A));
	}

	return false;
}

void Game::CheckIfExitGame()
{
	XboxController const& controller = g_theInput->GetController(0);
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE) ||
		controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_BACK))
	{
		g_theApp->HandleQuitRequested();
	}
}

void Game::UpdateClock(float deltaSeconds)
{
	m_unscaledDeltaTime = deltaSeconds;

	bool isSlowMo = g_theInput->IsKeyDown(KEYCODE_T);
	bool isFastMo = g_theInput->IsKeyDown(KEYCODE_Y);
	if (isFastMo && isSlowMo)
	{
		deltaSeconds *= 8.f;
	}
	else if (isFastMo)
	{
		deltaSeconds *= 4.f;
	}
	else if (isSlowMo)
	{
		deltaSeconds *= 0.10f;
	}
	m_elapsedTime += deltaSeconds;
}

void Game::AddTextToDevConsole()
{
	g_theDevConsole->AddText(DevConsole::INFO_MINOR, Stringf("Key Q Pressed, Time elapsed: %f", GetElapsedTime()));
	g_theDevConsole->AddText(DevConsole::ERROR, "LONG TEXT, LONG TEXT, LONG TEXT, LONG TEXT, LONG TEXT, LONG TEXT, LONG TEXT, LONG TEXT, LONG TEXT, LONG TEXT, LONG TEXT, LONG TEXT, LONG TEXT!");
	g_theDevConsole->Execute(Stringf(" Test   elapsedTime=%f   ", GetElapsedTime()));
}
