#pragma once
namespace ScoreManager
{
    enum class GameType {
        GameA = 0,
        GameB,
        GameC,
        GameD,
        GameE,
        Total,
        MaxCount
    };

    struct ScoreEntry {
        char initial[4];
        int score;
    };

    extern int g_currentRunScores[5];

    void Initialize();
    void AddScore(GameType game, int newScore, const char* initial);
    ScoreEntry GetScore(GameType game, int rank);
    void SaveScores();

    void RecordCurrentGameScore(GameType game, int score);
    void FinalizeRelayAndSave(const char* initial);
}