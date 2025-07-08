#include <iostream>
#include <cstdlib>
#include <thread>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <algorithm>
#include "GameBase.h"

#define PUERTO 8002
#define BUFFERSIZE 1024
#define WORDQUANTITY 3

using namespace std;

string toLower(string str) {
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

class CharadasGame : public GameBase {
    int sockCliente;
    string posiblesPalabras[WORDQUANTITY] = {"frutilla", "gato", "conejito"};
    string pistas[WORDQUANTITY] = {
        "Fruta roja con semillas en su exterior",
        "Animal doméstico conocido por odiar a su amo",
        "Animal de orejas grandes, si fuera más pequeño"};

public:
    CharadasGame(int sock) : sockCliente(sock) {}

    void startGame() override {
        char buffer[BUFFERSIZE] = {0};
        string nombre;
        int random;

        // Leer nombre del cliente
        int valread = read(sockCliente, buffer, BUFFERSIZE);
        if (valread <= 0) return;
        nombre = string(buffer, valread);
        nombre.erase(nombre.find_last_not_of("\n\r") + 1);

        sendLine("Hola " + nombre + ", jugaremos a las Charadas.");

        do {
            srand(time(nullptr) + sockCliente);
            random = rand() % WORDQUANTITY;
            string palabraSecreta = posiblesPalabras[random];
            string pista = "Pista: " + pistas[random];
            sendLine(pista);

            while (true) {
                memset(buffer, 0, BUFFERSIZE);
                int n = read(sockCliente, buffer, BUFFERSIZE);
                if (n <= 0) return;

                string intento = string(buffer, n);
                intento.erase(intento.find_last_not_of("\n\r") + 1);

                if (toLower(intento) == toLower(palabraSecreta)) {
                    sendLine("¡Correcto!");
                    break;
                } else {
                    sendLine("Incorrecto. Intenta nuevamente:");
                }
            }

            sendLine("¿Quieres jugar otra vez? (s/n)");
            memset(buffer, 0, BUFFERSIZE);
            int n = read(sockCliente, buffer, BUFFERSIZE);
            if (n <= 0) return;

            string respuesta(buffer, n);
            respuesta.erase(respuesta.find_last_not_of("\n\r") + 1);
            if (respuesta != "s" && respuesta != "S") break;

        } while (true);

        sendLine("Gracias por jugar. ¡Hasta la próxima!");
        close(sockCliente);
    }

private:
    void sendLine(const string& mensaje) {
        string msg = mensaje + "\n";
        send(sockCliente, msg.c_str(), msg.length(), 0);
    }
};

class Servidor {
    int sockServidor;
    sockaddr_in confServidor;

public:
    void iniciar() {
        crearSocket();
        configurar();
        escuchar();

        cout << "Servidor escuchando en el puerto " << PUERTO << "..." << endl;

        while (true) {
            aceptarCliente();
        }

        close(sockServidor);
    }

private:
    void crearSocket() {
        sockServidor = socket(AF_INET, SOCK_STREAM, 0);
        if (sockServidor < 0) {
            perror("Error creando socket");
            exit(EXIT_FAILURE);
        }
    }

    void configurar() {
        confServidor.sin_family = AF_INET;
        confServidor.sin_addr.s_addr = htonl(INADDR_ANY);
        confServidor.sin_port = htons(PUERTO);

        if (bind(sockServidor, (struct sockaddr*)&confServidor, sizeof(confServidor)) < 0) {
            perror("Error en bind");
            exit(EXIT_FAILURE);
        }
    }

    void escuchar() {
        if (listen(sockServidor, 10) < 0) {
            perror("Error en listen");
            exit(EXIT_FAILURE);
        }
    }

    void aceptarCliente() {
        sockaddr_in confCliente;
        socklen_t tamCliente = sizeof(confCliente);
        int sockCliente = accept(sockServidor, (struct sockaddr*)&confCliente, &tamCliente);
        cout << "Nuevo cliente conectado. Creando hebra..." << endl;
        if (sockCliente < 0) {
            perror("Error en accept");
            return;
        }

        // Crear y soltar hebra
        thread([sockCliente]() {
            CharadasGame juego(sockCliente);
            juego.startGame();
        }).detach();
    }
};

int main() {
    Servidor servidor;
    servidor.iniciar();
    return 0;
}
