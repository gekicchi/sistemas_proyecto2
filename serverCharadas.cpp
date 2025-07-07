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

// ------------------------ Función auxiliar ------------------------
string toLower(string str) {
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

// ------------------------ Clase derivada CharadasGame ------------------------
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
            // Nueva palabra
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

// ------------------------ Clase Servidor ------------------------
class Servidor {
    int sockServidor;
    sockaddr_in confServidor;
    vector<thread> clientes;

public:
    void iniciar(int cantidadClientes) {
        crearSocket();
        configurar();
        escuchar(cantidadClientes);

        for (int i = 0; i < cantidadClientes; ++i)
            aceptarCliente();

        for (auto& cliente : clientes)
            if (cliente.joinable()) cliente.join();

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
            return;
        }

        clientes.emplace_back([sockCliente]() {
            CharadasGame juego(sockCliente);
            juego.startGame();
        });
    }
};

// ------------------------ main() ------------------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <cantidad_clientes>" << endl;
        return 1;
    }

    int cantidadClientes = atoi(argv[1]);
    Servidor servidor;
    servidor.iniciar(cantidadClientes);
    return 0;
}
