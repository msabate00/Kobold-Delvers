#pragma once
#include "app/defs.h"

class GameProgress

{

public:

	static const int LEVELS = 12;

	GameProgress();

	void Reset();

	int TotalStars() const;

	uint8 StarsFor(int levelIndex) const;
	bool SetStarsFor(int levelIndex, uint8 starsEarned);

	int MaxStarsFor(int levelIndex) const;
	int UnlockRequirement(int levelIndex) const;

	bool IsLevelUnlocked(int levelIndex) const;

	bool Load(const char* path = nullptr);

	bool Save(const char* path = nullptr) const;

private:

	uint8 stars[LEVELS]{ 0 };

};

