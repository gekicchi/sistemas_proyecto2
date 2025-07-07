#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PUERTO 8002
#define BUFFERSIZE 1024

using namespace std;

class Cliente {
    int sockCliente;
    sockaddr_in confServidor;
    string nombre;

public:
    Cliente(const string& nombreCliente) : nombre(nombreCliente) {}

    void conectar() {
        crearSocket();
        configurarServidor();
        cout << "***** Conectado al servidor ***** [" << nombre << "]\n" << endl;
    }

    void interactuar() {
        enviar(nombre);

        while (true) {
            if (!leerMensaje()) break;

            cout << "[Cliente] ";
            string entrada;
            getline(cin, entrada);

            entrada += "\n";
            enviar(entrada);

            if (entrada == "BYE\n") break;
        }

        close(sockCliente);
    }

private:
    void crearSocket() {
        sockCliente = socket(AF_INET, SOCK_STREAM, 0);
        if (sockCliente < 0) {
            perror("Error creando socket");
            exit(EXIT_FAILURE);
        }
    }

    void configurarServidor() {
        confServidor.sin_family = AF_INET;
        confServidor.sin_port = htons(PUERTO);
        confServidor.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(sockCliente, (struct sockaddr*)&confServidor, sizeof(confServidor)) < 0) {
            perror("Error en la conexión");
            exit(EXIT_FAILURE);
        }
    }

    bool leerMensaje() {
        char buffer[BUFFERSIZE] = {0};
        int valread = read(sockCliente, buffer, BUFFERSIZE);

        if (valread > 0) {
            cout << "[Servidor] " << string(buffer, valread);
            return true;
        } else {
            cout << "[Cliente] Conexión cerrada por el servidor." << endl;
            return false;
        }
    }

    void enviar(const string& msg) {
        send(sockCliente, msg.c_str(), msg.length(), 0);
    }
};

int main() {
    string nombreCliente;
    cout << "Ingrese su nombre: ";
    getline(cin, nombreCliente);

    Cliente cliente(nombreCliente);
    cliente.conectar();
    cliente.interactuar();

    return 0;
}
