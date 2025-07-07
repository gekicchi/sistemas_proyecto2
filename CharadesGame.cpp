// CharadesGame.cpp
#include "CharadesGame.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sys/socket.h>
#include <algorithm>
#include <cctype>
#include <iomanip>

std::string CharadesGame::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \n\r\t");
    if (std::string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(" \n\r\t");
    return str.substr(first, (last - first + 1));
}

std::string CharadesGame::toLower(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return lowerStr;
}

CharadesGame::CharadesGame() : gameFinished(false) {
    resetGame();
}

std::string CharadesGame::getGameName() const {
    return "Charadas";
}

void CharadesGame::startGame(int clientSocket) {
    std::string initialMessage = "Jugaremos a las Charadas\nIntenta adivinar la palabra secreta\n" + pistaActual;
    send(clientSocket, initialMessage.c_str(), initialMessage.length(), 0);
}

bool CharadesGame::processClientInput(int clientSocket, const std::string& input) {
    if (gameFinished) {
        return true;
    }

    std::string cleanedInput = toLower(trim(input));
    std::string cleanedSecret = toLower(trim(palabraSecreta));

    // Mensaje de depuración traducido
    std::cout << "[DEPURACIÓN Juego Charadas] Cliente adivinó: '" << cleanedInput << "', Secreta: '" << cleanedSecret << "'" << std::endl;

    if (cleanedInput == cleanedSecret) {
        std::string finalMessage = "Correcto! Has adivinado la palabra.\n";
        send(clientSocket, finalMessage.c_str(), finalMessage.length(), 0);
        std::cout << "Cliente adivinó la palabra: " << cleanedSecret << std::endl;
        gameFinished = true;
        return true;
    } else {
        std::string response = "Incorrecto, vuelve a intentarlo.\n";
        send(clientSocket, response.c_str(), response.length(), 0);
        std::cout << "Cliente intentó '" << cleanedInput << "' - Incorrecto." << std::endl;
        return false;
    }
}

void CharadesGame::resetGame() {
    int random = rand() % CHARADAS_WORD_QUANTITY;
    palabraSecreta = posiblesPalabras[random];
    pistaActual = pistas[random];
    gameFinished = false; // Reiniciar el estado del juego
}