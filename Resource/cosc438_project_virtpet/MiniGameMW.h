#pragma once
#include <string>

class MiniGameMW {
public:
	// Returns happiness points gained (0-40)
	static int playReactionTest();

private:
	static int runRound(HANDLE hOut);			// Returns reaction time in ms (-1 = false start)
	static int scoreToHappiness(int bestMs);
	static std::string getRating(int ms);
};