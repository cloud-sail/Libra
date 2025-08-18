#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Image.hpp"

//-----------------------------------------------------------------------------------------------
enum class GameState
{
	DEFAULT,
	ATTRACT,
	INGAME,
	GAVEOVER,
	VICTORY
};

// ----------------------------------------------------------------------------------------------
class Game
{
public:
	Game();
	~Game();
	void Update(float deltaSeconds);
	void Render() const;

	void PlayMusic(SoundID soundID);
	PlayerTank* GetPlayerTank();

	void MoveToNextMap();
	void OnPlayerDie();

public:
	Camera GetUICamera();

	float m_numTilesInViewVertically = 8.f;

private:
	void UpdateCameras(float deltaSeconds);


	void InitializeMaps();
	void InitializePlayerTank();
	void ResetTheGame();
	int GetMapIndex(Map* map);

//-----------------------------------------------------------------------------------------------
// Cheats and Debug Tools
private:
	void UpdateCommonDeveloperCheats();
	void UpdateInGameDeveloperCheats(float& deltaSeconds);
	void AdjustForPauseAndTimeDistortion(float& deltaSeconds);
	void SetPlayBackSpeed(float playBackSpeed);

public:
	bool m_isInvincible = false; // F2
	bool m_isNoClip = false; // F3
	bool m_isWholeMapView = false; // F4 Map View

private:
	bool m_isPaused = false;
	bool m_isSlowMo = false;
	bool m_isFastMo = false;
	bool m_isPausedAfterNextUpdate = false;  




private:
	Camera m_screenCamera;
	Map* m_currentMap = nullptr;
	std::vector<Map*> m_maps;
	PlayerTank* m_playerTank = nullptr;

	SoundPlaybackID m_musicPlaybackID = MISSING_SOUND_ID;

//-----------------------------------------------------------------------------------------------
// TODO: Animation, current timer, lerp, function of easing in/out and isLooped
private:
	void DrawScaledCircle() const;
	void DrawAttractScreen() const;
	void DrawGameOverScreen() const;
	void DrawVictoryScreen() const;
	void DrawPausedPanel() const;
	void DrawTestFont() const;
	void DrawTestTileHeatMap() const;
	void DrawTextInBox() const;
	void DrawSpriteAnimation() const;
	void DrawTestImage() const;

	void InitializeSpriteAnim();

private:
	SpriteAnimDefinition* m_spriteAnimOnce = nullptr;
	SpriteAnimDefinition* m_spriteAnimLoop = nullptr;
	SpriteAnimDefinition* m_spriteAnimPingPong = nullptr;
	SpriteAnimDefinition* m_testExplosionAnim = nullptr;

public:
	SpriteAnimDefinition* m_explosionAnim = nullptr;

//-----------------------------------------------------------------------------------------------
// Game State Transition
private:
	void UpdateCurrentState(float deltaSeconds);
	void RenderCurrentState() const;
	void ChangeState(GameState newState);
	void EnterState(GameState state);
	void ExitState(GameState state);

	void UpdateStateAttract(float deltaSeconds);
	void UpdateStateInGame(float deltaSeconds);
	void UpdateStateGameOver(float deltaSeconds);
	void UpdateStateVictory(float deltaSeconds);

	void RenderStateAttract() const;
	void RenderStateInGame() const;
	void RenderStateGameOver() const;
	void RenderStateVictory() const;

	bool CanTransitionTo(GameState newState) const;

	void CheckIfExitGame();

private:
	GameState m_currentState = GameState::DEFAULT;

	float m_gameOverTimer = 15.f;

//-----------------------------------------------------------------------------------------------
// Time/Clock
public:
	float GetUnscaledDeltaTime() const { return m_unscaledDeltaTime; }
	float GetElapsedTime() const { return m_elapsedTime; }

private:
	void UpdateClock(float deltaSeconds);

private:
	float m_unscaledDeltaTime = 0.f;
	float m_elapsedTime = 0.f;

//-----------------------------------------------------------------------------------------------
// Dev Console
private:
	void AddTextToDevConsole();

private:
	Image m_testImage = Image("Data/Images/EnemyBullet.png");
};
