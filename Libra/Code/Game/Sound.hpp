#pragma once
#include "Engine/Audio/AudioSystem.hpp"

class Sound
{
public:
	static SoundID ATTRACT_MUSIC;
	static SoundID GAME_PLAY_MUSIC;
	static SoundID PAUSE;
	static SoundID UNPAUSE;
	static SoundID SELECT;
	static SoundID WIN;
	static SoundID LOSE;
	static SoundID NEXT_LEVEL;
	static SoundID PLAYER_SHOOT;
	static SoundID PLAYER_HIT;
	static SoundID PLAYER_DIE;
	static SoundID PLAYER_REVIVE;
	static SoundID ENEMY_SHOOT;
	static SoundID ENEMY_HIT;
	static SoundID ENEMY_DIE;
	static SoundID BULLET_BOUNCE;
	static SoundID BULLET_RICOCHET;

public:
	static void Init();
};
