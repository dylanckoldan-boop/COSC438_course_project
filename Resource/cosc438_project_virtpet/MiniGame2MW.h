#pragma once
#include <string>

class MiniGame2MW {
public:
	// Returns happiness points gained (0-40)
	static int playQuestionGame();

private:
	static int runRound2(HANDLE hOut);			// Returns reaction time in ms (-1 = false start)
	static int scoreToHappiness2(int bestMs);
	static std::string getRating2(int ms);
};