// server.cpp

#include <iostream>
#include <cstdlib>
#include <thread>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <map>
#include <algorithm> 
#include <cctype>    

#include "GameBase.h"
#include "CharadesGame.h"

#define PUERTO 8002
#define BUFFERSIZE 1024

using namespace std;

void sendMessage(int sock, const string& msg) {
    send(sock, msg.c_str(), msg.length(), 0);
}


std::string trimString(const std::string& str) {
    size_t first = str.find_first_not_of(" \n\r\t");
    if (std::string::npos == first) {
        return ""; 
    }
    size_t last = str.find_last_not_of(" \n\r\t");
    return str.substr(first, (last - first + 1));
}


class SesionCliente {
    int sockCliente;
    sockaddr_in confCliente;
    std::unique_ptr<GameBase> currentGame;
    string clientName;

public:
    SesionCliente(int sock, sockaddr_in cliente) : sockCliente(sock), confCliente(cliente) {
        currentGame = std::make_unique<CharadesGame>();
    }

    void atender() {
        char buffer[BUFFERSIZE] = {0};
        bool shouldExitSession = false;

        int valread = read(sockCliente, buffer, BUFFERSIZE);
        if (valread <= 0) {
            close(sockCliente);
            return;
        }
        clientName = trimString(std::string(buffer, valread)); 

        sendMessage(sockCliente, "Hola " + clientName + "!\n");
        sendMessage(sockCliente, "Bienvenido al servidor de juegos.\n");

        
        string initialMenuPrompt = "Elija una opcion:\n1. Jugar Charadas\n2. Salir\nIngrese su opcion: ";
        sendMessage(sockCliente, initialMenuPrompt);

        valread = read(sockCliente, buffer, BUFFERSIZE);
        if (valread <= 0) {
            cout << "Cliente " << clientName << " desconectado durante la seleccion inicial." << endl;
            close(sockCliente);
            return;
        }
        string initialChoice = trimString(std::string(buffer, valread));

        if (initialChoice == "1") {
            sendMessage(sockCliente, "Ha elegido jugar Charadas. ¡Mucha suerte!\n");
        } else if (initialChoice == "2") {
            sendMessage(sockCliente, "Saliendo del juego. ¡Adios!\n");
            shouldExitSession = true;
        } else {
            sendMessage(sockCliente, "Opcion invalida. Saliendo del juego por defecto.\n");
            shouldExitSession = true;
        }

        
        while (!shouldExitSession) {
            if (shouldExitSession) { 
                break; 
            }
            
            currentGame->startGame(sockCliente);

            bool gameIsOver = false;
            while (!gameIsOver && !shouldExitSession) {
                valread = read(sockCliente, buffer, BUFFERSIZE);
                if (valread <= 0) {
                    cout << "Cliente " << clientName << " desconectado." << endl;
                    shouldExitSession = true;
                    break;
                }
                string clientInput = trimString(std::string(buffer, valread)); 

                if (clientInput == "BYE") { 
                    sendMessage(sockCliente, "Adios!\n");
                    shouldExitSession = true;
                    break;
                }
                
                gameIsOver = currentGame->processClientInput(sockCliente, clientInput);
                
                if (gameIsOver) {
                    sendMessage(sockCliente, "El juego de Charadas ha terminado.\n");
                    sendMessage(sockCliente, "Opciones:\n1. Jugar otra Charada\n2. Salir del juego\nIngrese su opcion: ");

                    valread = read(sockCliente, buffer, BUFFERSIZE);
                    if (valread <= 0) {
                        cout << "Cliente " << clientName << " desconectado mientras elegia opcion." << endl;
                        shouldExitSession = true;
                        break;
                    }
                    string choice = trimString(std::string(buffer, valread)); 

                    if (choice == "1") {
                        static_cast<CharadesGame*>(currentGame.get())->resetGame();
                        gameIsOver = false;
                        sendMessage(sockCliente, "Iniciando nueva Charada...\n");
                    } else if (choice == "2") {
                        sendMessage(sockCliente, "Saliendo del juego. ¡Adios!\n");
                        shouldExitSession = true;
                    } else {
                        sendMessage(sockCliente, "Opcion invalida. Saliendo del juego por defecto.\n");
                        shouldExitSession = true;
                    }
                }
            }
        }

        close(sockCliente);
        cout << "Sesion de cliente " << clientName << " finalizada y socket cerrado." << endl;
    }
};

class Servidor {
    int sockServidor;
    sockaddr_in confServidor;
    vector<thread> clientes;

public:
    Servidor() {
        crearSocket();
        configurarServidor();
    }

    ~Servidor() {
        close(sockServidor);
        for (thread& t : clientes) {
            if (t.joinable()) {
                t.join();
            }
        }
        cout << "Servidor cerrado." << endl;
    }

    void crearSocket() {
        if ((sockServidor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Error creando socket");
            exit(EXIT_FAILURE);
        }
    }

    void configurarServidor() {
        confServidor.sin_family = AF_INET;
        confServidor.sin_addr.s_addr = INADDR_ANY;
        confServidor.sin_port = htons(PUERTO);

        if (bind(sockServidor, (struct sockaddr*)&confServidor, sizeof(confServidor)) < 0) {
            perror("Error en bind");
            exit(EXIT_FAILURE);
        }
    }

    void escuchar(int n) {
        if (listen(sockServidor, n) < 0) {
            perror("Error en listen");
            exit(EXIT_FAILURE);
        }
        cout << "Servidor escuchando en el puerto " << PUERTO << "..." << endl;
    }

    void aceptarCliente() {
        sockaddr_in confCliente;
        socklen_t tamCliente = sizeof(confCliente);
        int sockCliente = accept(sockServidor, (struct sockaddr*)&confCliente, &tamCliente);
        if (sockCliente < 0) {
            perror("Error en accept");
            exit(EXIT_FAILURE);
        }

        clientes.emplace_back(&SesionCliente::atender, SesionCliente(sockCliente, confCliente));
        cout << "Nuevo cliente conectado. IP: " << inet_ntoa(confCliente.sin_addr) << ", Puerto: " << ntohs(confCliente.sin_port) << endl;
    }
};

int main(int argc, char* argv[]) {
    srand(time(0));

    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <cantidad_clientes>" << endl;
        return 1;
    }

    int cantidadClientes = atoi(argv[1]);
    if (cantidadClientes <= 0) {
        cerr << "La cantidad de clientes debe ser un numero positivo." << endl;
        return 1;
    }

    Servidor miServidor;
    miServidor.escuchar(cantidadClientes);

    if (cantidadClientes == 1) {
        miServidor.aceptarCliente();
    } else {
        cout << "Configurado para multiples clientes, pero la logica actual solo acepta uno." << endl;
        cout << "Para aceptar multiples, descomente el bucle de aceptacion." << endl;
    }

    return 0;
}