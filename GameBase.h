#ifndef GAME_BASE_H
#define GAME_BASE_H

#include <string> 
#include <vector> 


class GameBase {
public:
    virtual ~GameBase() = default;

    virtual std::string getGameName() const = 0;

    virtual void startGame(int clientSocket) = 0;

    virtual bool processClientInput(int clientSocket, const std::string& input) = 0;


protected:

};

#endif 