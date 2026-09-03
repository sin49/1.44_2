#include "ScoreManager.h"
#include <fstream>
#include <algorithm>
#include <cstring>

namespace ScoreManager
{
    ScoreEntry g_topRankings[(int)GameType::MaxCount][3];

    void Initialize() {
        for (int i = 0; i < (int)GameType::MaxCount; ++i) {
            for (int j = 0; j < 3; ++j) {
                strcpy_s(g_topRankings[i][j].initial, "---");
                g_topRankings[i][j].score = 0;
            }
        }

        std::ifstream file("ranking.txt");
        if (file.is_open()) {
            for (int i = 0; i < (int)GameType::MaxCount; ++i) {
                for (int j = 0; j < 3; ++j) {
                    file >> g_topRankings[i][j].initial >> g_topRankings[i][j].score;
                }
            }
            file.close();
        }
    }

    void SaveScores() {
        std::ofstream file("ranking.txt");
        if (file.is_open()) {
            for (int i = 0; i < (int)GameType::MaxCount; ++i) {
                for (int j = 0; j < 3; ++j) {
                    file << g_topRankings[i][j].initial << " " << g_topRankings[i][j].score << "\n";
                }
            }
            file.close();
        }
    }

    void AddScore(GameType game, int newScore, const char* initial) {
        int idx = (int)game;
        if (idx < 0 || idx >= (int)GameType::MaxCount) return;

        ScoreEntry temp[4];
        for (int j = 0; j < 3; ++j) {
            temp[j] = g_topRankings[idx][j];
        }
        strcpy_s(temp[3].initial, initial);
        temp[3].score = newScore;

        std::sort(temp, temp + 4, [](const ScoreEntry& a, const ScoreEntry& b) {
            return a.score > b.score;
        });

        for (int j = 0; j < 3; ++j) {
            g_topRankings[idx][j] = temp[j];
        }

        SaveScores();
    }

    ScoreEntry GetScore(GameType game, int rank) {
        int idx = (int)game;
        if (idx < 0 || idx >= (int)GameType::MaxCount) return { "---", 0 };
        if (rank < 0 || rank > 2) return { "---", 0 };

        return g_topRankings[idx][rank];
    }
    int g_currentRunScores[4] = { 0, 0, 0, 0 };

    // 각 게임이 끝날 때마다 점수를 임시 보관
    void RecordCurrentGameScore(GameType game, int score) {
        int idx = (int)game;
        if (idx >= 0 && idx < 4) {
            g_currentRunScores[idx] = score;
        }
    }

    // 게임 D까지 모두 끝나면 총점을 합산하고 랭킹을 갱신
    void FinalizeRelayAndSave(const char* initial) {
        int totalScore = 0;
        for (int i = 0; i < 4; ++i) {
            totalScore += g_currentRunScores[i];
            // 개별 게임 랭킹도 갱신
            AddScore((GameType)i, g_currentRunScores[i], initial);
        }
        // 최종 총합 랭킹 갱신
        AddScore(GameType::Total, totalScore, initial);
    }
}