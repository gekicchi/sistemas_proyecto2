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
#include <iomanip>
#include <memory>

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

    enum class EstadoCliente {
        EsperandoNombre,
        EsperandoOpcionMenuInicial,
        EnJuego,
        EsperandoOpcionPostJuego
    };

    EstadoCliente estadoActual;

public:
    SesionCliente(int sock, sockaddr_in cliente) :
        sockCliente(sock), confCliente(cliente),
        currentGame(nullptr),
        clientName(""),
        estadoActual(EstadoCliente::EsperandoNombre)
    {}

    void atender() {
        char buffer[BUFFERSIZE] = {0};
        int valread;
        bool deberiaSalirSesion = false;

        while (!deberiaSalirSesion) {
            std::string entradaCliente;

            if (estadoActual == EstadoCliente::EsperandoNombre) {
                valread = read(sockCliente, buffer, BUFFERSIZE);
                if (valread <= 0) {
                    std::cerr << "Cliente desconectado o error al recibir nombre." << std::endl;
                    deberiaSalirSesion = true;
                    continue;
                }
                clientName = trimString(std::string(buffer, valread));
                std::cout << "Nuevo cliente conectado. IP: " << inet_ntoa(confCliente.sin_addr) << ", Puerto: " << ntohs(confCliente.sin_port) << std::endl;
                std::cout << "Nombre del cliente: " << clientName << std::endl;
                estadoActual = EstadoCliente::EsperandoOpcionMenuInicial;
            }
            else if (estadoActual == EstadoCliente::EsperandoOpcionMenuInicial) {
                std::string mensajeInicialCompleto =
                    "Hola " + clientName + "!\n" +
                    "Bienvenido al servidor de juegos.\n" +
                    "Elija una opcion:\n" +
                    "1. Jugar Charadas\n" +
                    "2. Salir\n" +
                    "Ingrese su opcion:\n";
                sendMessage(sockCliente, mensajeInicialCompleto);

                valread = read(sockCliente, buffer, BUFFERSIZE);
                if (valread <= 0) {
                    std::cerr << "Cliente " << clientName << " desconectado o error al leer opción del menú inicial." << std::endl;
                    deberiaSalirSesion = true;
                    continue;
                }
                entradaCliente = trimString(std::string(buffer, valread));

                if (entradaCliente == "1") {
                    currentGame = std::make_unique<CharadesGame>();
                    std::cout << "[DEPURACIÓN CharadesGame] Juego de Charadas inicializado para " << clientName << ". Nombre del juego: " << currentGame->getGameName() << std::endl;
                    currentGame->startGame(sockCliente);
                    estadoActual = EstadoCliente::EnJuego;
                } else if (entradaCliente == "2") {
                    sendMessage(sockCliente, "Saliendo del servidor. ¡Adios!\n");
                    deberiaSalirSesion = true;
                } else {
                    sendMessage(sockCliente, "Opcion invalida. Intente de nuevo.\n");
                }
            } else if (estadoActual == EstadoCliente::EnJuego) {
                valread = read(sockCliente, buffer, BUFFERSIZE);
                if (valread <= 0) {
                    std::cerr << "Cliente " << clientName << " desconectado o error al leer adivinanza." << std::endl;
                    deberiaSalirSesion = true;
                    continue;
                }
                entradaCliente = trimString(std::string(buffer, valread));

                if (currentGame) {
                    bool juegoTerminado = currentGame->processClientInput(sockCliente, entradaCliente);
                    if (juegoTerminado) {
                        estadoActual = EstadoCliente::EsperandoOpcionPostJuego;
                    }
                } else {
                    sendMessage(sockCliente, "Error: No hay juego activo. Saliendo.\n");
                    deberiaSalirSesion = true;
                }
            } else if (estadoActual == EstadoCliente::EsperandoOpcionPostJuego) {
                // Antes de leer, asegura que el menú post-juego se haya enviado.
                // Idealmente, esto lo hace processClientInput cuando el juego termina y gana/pierde.
                // Si llegamos aquí por una opción inválida, lo enviamos de nuevo.
                std::string mensajeMenuPostJuego =
                    "El juego de Charadas ha terminado.\n" +
                    "Opciones:\n" +
                    "1. Jugar otra Charada\n" +
                    "2. Salir del juego\n" +
                    "Ingrese su opcion:\n";
                sendMessage(sockCliente, mensajeMenuPostJuego);

                valread = read(sockCliente, buffer, BUFFERSIZE);
                if (valread <= 0) {
                    std::cerr << "Cliente " << clientName << " desconectado o error al leer opción del menú post-juego." << std::endl;
                    deberiaSalirSesion = true;
                    continue;
                }
                entradaCliente = trimString(std::string(buffer, valread));

                if (entradaCliente == "1") {
                    static_cast<CharadesGame*>(currentGame.get())->resetGame();
                    std::cout << "[DEPURACIÓN CharadesGame] Juego de Charadas reiniciado. Nueva palabra generada para " << clientName << "." << std::endl;
                    currentGame->startGame(sockCliente);
                    estadoActual = EstadoCliente::EnJuego;
                } else if (entradaCliente == "2") {
                    sendMessage(sockCliente, "Saliendo del juego. ¡Adios!\n");
                    deberiaSalirSesion = true;
                } else {
                    sendMessage(sockCliente, "Opcion invalida. Intente de nuevo.\n");
                    // Permanece en el mismo estado para que el menú post-juego se muestre de nuevo.
                }
            }
        }
        close(sockCliente);
        std::cout << "Sesión de cliente " << clientName << " finalizada y socket cerrado." << std::endl;
    }
};

class Servidor {
    int sockServidor;
    sockaddr_in confServidor;
    vector<thread> clientes;

public:
    void iniciar(int cantidadClientes) {
        crearSocket();
        configurar();
        escuchar(cantidadClientes);

        for (int i = 0; i < cantidadClientes; ++i) {
            aceptarCliente();
        }

        for (auto& cliente : clientes) {
            if (cliente.joinable()) {
                cliente.join();
            }
        }
        close(sockServidor);
    }

private:
    void crearSocket() {
        if ((sockServidor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Error creando socket");
            exit(EXIT_FAILURE);
        }
    }

    void configurar() {
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
    miServidor.iniciar(cantidadClientes);

    return 0;
}