#include "app/game_progress.h"

#include <fstream>
#include <string>
#include <cstdio>

GameProgress::GameProgress(){

	Reset();
}

void GameProgress::Reset(){
	for (int i = 0; i < LEVELS; ++i) stars[i] = 0;
}

int GameProgress::TotalStars() const{

	int sum = 0;
	for (int i = 0; i < LEVELS; ++i) sum += (int)stars[i];
	return sum;

}

uint8 GameProgress::StarsFor(int levelIndex) const{

	if (levelIndex < 0 || levelIndex >= LEVELS) return 0;
	return stars[levelIndex];
}

bool GameProgress::SetStarsFor(int levelIndex, uint8 starsEarned){

	if (levelIndex < 0 || levelIndex >= LEVELS) return false;
	if (starsEarned > 3) starsEarned = 3;

	if (starsEarned > stars[levelIndex]) {
		stars[levelIndex] = starsEarned;
		return true;
	}

	return false;

}

int GameProgress::UnlockRequirement(int levelIndex) const{



	if (levelIndex <= 0) return 0;

	switch (levelIndex)
	{
		case 1: return 1;
		case 2: return 2;
		case 3: return 4;
		case 4: return 6;
		case 5: return 8;
		case 6: return 10;
		case 7: return 13;
		case 8: return 16;
		case 9: return 19;
		case 10: return 22;
		case 11: return 25;
		case 12: return 29;
		case 13: return 34;
		case 14: return 38;
	default:
		break;
	}
	return 3 * levelIndex;
}

bool GameProgress::IsLevelUnlocked(int levelIndex) const{

	if (levelIndex < 0 || levelIndex >= LEVELS) return false;
	if (levelIndex == 0) return true;
	return TotalStars() >= UnlockRequirement(levelIndex);

}

bool GameProgress::Load(const char* path){

	if (!path || !*path) path = "progress.cfg";

	Reset();

	std::ifstream f(path);
	if (!f) return false;
	std::string line;

	while (std::getline(f, line)) {

		if (line.empty()) continue;
		if (line[0] == '#') continue;

		int idx = 0;
		int val = 0;

		if (std::sscanf(line.c_str(), "star%d=%d", &idx, &val) == 2) {

			if (idx >= 1 && idx <= LEVELS) {
				if (val < 0) val = 0;
				if (val > 3) val = 3;
				stars[idx - 1] = (uint8)val;
			}
		}
	}
	return true;
}

bool GameProgress::Save(const char* path) const{

	if (!path || !*path) path = "progress.cfg";
	std::ofstream o(path, std::ios::out | std::ios::trunc);

	if (!o) return false;

	o << "# SandForge progress\n";

	for (int i = 0; i < LEVELS; ++i) {
		o << "star" << (i + 1) << "=" << (int)stars[i] << "\n";
	}

	return true;
}

