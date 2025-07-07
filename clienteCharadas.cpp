#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// CAMBIO AQUI: El puerto debe ser 8002 para que coincida con el servidor.
#define PUERTO 8002
#define BUFFERSIZE 1024

using namespace std;

class Cliente
{
    int sockCliente;
    sockaddr_in confServidor;
    string nombre;

public:
    Cliente(const string& nombreCliente) : nombre(nombreCliente) {}

    void conectar()
    {
        crearSocket();
        configurarServidor();
        cout << "*****Conectado al servidor****[" << nombre << "]" << endl;
    }

    void interactuar()
    {
        char buffer[BUFFERSIZE] = {0};

        send(sockCliente, nombre.c_str(), nombre.length(), 0);

        leerMensaje();

    while (true)
    {
        cout << "[Cliente] ";
        cout << flush;
        string entrada;
        getline(cin, entrada);

        entrada += "\n";
        send(sockCliente, entrada.c_str(), entrada.length(), 0);

        leerMensaje();
        if (entrada == "BYE\n")
            break;
    }

        close(sockCliente);
    }

private:
    void crearSocket()
    {
        if ((sockCliente = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Error creando socket");
            exit(EXIT_FAILURE);
        }
    }

    void configurarServidor()
    {
        confServidor.sin_family = AF_INET;
        confServidor.sin_port = htons(PUERTO);
        confServidor.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(sockCliente, (struct sockaddr*)&confServidor, sizeof(confServidor)) < 0)
        {
            perror("Error en la conexión (configurar servidor)");
            exit(EXIT_FAILURE);
        }
    }

    void leerMensaje()
{
    char buffer[BUFFERSIZE] = {0};
    int valread = read(sockCliente, buffer, BUFFERSIZE);

    if (valread > 0) {
        cout << string(buffer, valread) << endl;
        cout << flush;
    } else if (valread == 0) {
        cout << "[DEBUG leerMensaje] Servidor cerró la conexión." << endl;
    } else {
        perror("Error en read");
        exit(EXIT_FAILURE);
    }
}
};

int main(int argc, char const* argv[]) {
    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <nombre_cliente>" << endl;
        return 1;
    }

    Cliente cliente(argv[1]);
    cliente.conectar();
    cliente.interactuar();

    return 0;
}