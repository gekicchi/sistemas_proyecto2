// CharadesGame.h
#ifndef CHARADES_GAME_H
#define CHARADES_GAME_H

#include "GameBase.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

#define CHARADAS_WORD_QUANTITY 3

class CharadesGame : public GameBase {
private:
    std::string posiblesPalabras[CHARADAS_WORD_QUANTITY] = {"frutilla", "gato", "conejito"};
    std::string pistas[CHARADAS_WORD_QUANTITY] = {
        "fruta roja con semillas en su exterior",
        "animal domestico conocido por odiar a su amo",
        "animal domestico de orejas grandes, si fuera mas pequeño"
    };

    std::string palabraSecreta;
    std::string pistaActual;
    bool gameFinished;

    std::string trim(const std::string& str);

    std::string toLower(const std::string& str);

public:
    CharadesGame();

    std::string getGameName() const override;
    void startGame(int clientSocket) override;
    bool processClientInput(int clientSocket, const std::string& input) override;
    void resetGame();

};

#endif