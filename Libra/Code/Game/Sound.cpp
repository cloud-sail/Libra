#include "Game/Sound.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Audio/AudioSystem.hpp"

SoundID Sound::ATTRACT_MUSIC = MISSING_SOUND_ID;
SoundID Sound::GAME_PLAY_MUSIC		= MISSING_SOUND_ID;

SoundID Sound::PAUSE	= MISSING_SOUND_ID;
SoundID Sound::UNPAUSE	= MISSING_SOUND_ID;
SoundID Sound::SELECT	= MISSING_SOUND_ID;


SoundID Sound::WIN	= MISSING_SOUND_ID;
SoundID Sound::LOSE = MISSING_SOUND_ID;
SoundID Sound::NEXT_LEVEL = MISSING_SOUND_ID;


SoundID Sound::PLAYER_SHOOT = MISSING_SOUND_ID;
SoundID Sound::PLAYER_HIT = MISSING_SOUND_ID;
SoundID Sound::PLAYER_DIE = MISSING_SOUND_ID;
SoundID Sound::PLAYER_REVIVE = MISSING_SOUND_ID;
SoundID Sound::ENEMY_SHOOT = MISSING_SOUND_ID;
SoundID Sound::ENEMY_HIT = MISSING_SOUND_ID;
SoundID Sound::ENEMY_DIE = MISSING_SOUND_ID;

SoundID Sound::BULLET_BOUNCE = MISSING_SOUND_ID;
SoundID Sound::BULLET_RICOCHET = MISSING_SOUND_ID;



void Sound::Init()
{
	ATTRACT_MUSIC			= g_theAudio->CreateOrGetSound("Data/Audio/AttractMusic.wav");
	GAME_PLAY_MUSIC			= g_theAudio->CreateOrGetSound("Data/Audio/GameplayMusic.wav");
	PAUSE					= g_theAudio->CreateOrGetSound("Data/Audio/Pause.wav");
	UNPAUSE					= g_theAudio->CreateOrGetSound("Data/Audio/Unpause.wav");
	SELECT					= g_theAudio->CreateOrGetSound("Data/Audio/Select.wav");
	WIN						= g_theAudio->CreateOrGetSound("Data/Audio/Win.wav");
	LOSE					= g_theAudio->CreateOrGetSound("Data/Audio/Lose.wav");
	NEXT_LEVEL				= g_theAudio->CreateOrGetSound("Data/Audio/NextLevel.wav");
	PLAYER_SHOOT			= g_theAudio->CreateOrGetSound("Data/Audio/PlayerShoot.wav");
	PLAYER_HIT				= g_theAudio->CreateOrGetSound("Data/Audio/PlayerHit.wav");
	PLAYER_DIE				= g_theAudio->CreateOrGetSound("Data/Audio/PlayerDie.wav");
	PLAYER_REVIVE			= g_theAudio->CreateOrGetSound("Data/Audio/PlayerRevive.wav");
	ENEMY_SHOOT				= g_theAudio->CreateOrGetSound("Data/Audio/EnemyShoot.wav");
	ENEMY_HIT				= g_theAudio->CreateOrGetSound("Data/Audio/EnemyHit.wav");
	ENEMY_DIE				= g_theAudio->CreateOrGetSound("Data/Audio/EnemyDie.wav");
	BULLET_BOUNCE			= g_theAudio->CreateOrGetSound("Data/Audio/BulletBounce.wav");
	BULLET_RICOCHET			= g_theAudio->CreateOrGetSound("Data/Audio/BulletRicochet2.wav");
}
