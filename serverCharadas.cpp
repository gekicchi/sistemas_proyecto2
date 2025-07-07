// ServidorCharadas.cpp
#include <iostream>
#include <cstdlib>
#include <thread>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iomanip>


//realizado por Matias Oyarzun y Matias Peters
#define PUERTO 8002
#define BUFFERSIZE 1024
#define WORDQUANTITY 3

using namespace std;

// ------------------------ Clase SesionCliente ------------------------
class SesionCliente 
{
    int sockCliente;
    sockaddr_in confCliente;
    string posiblesPalabras[WORDQUANTITY] = {"frutilla\n", "gato\n", "conejito\n"};
    string pistas[WORDQUANTITY] = {"fruta roja con semillas en su exterior\n", 
        "animal domestico conocido por odiar a su amo\n", 
        "animal domestico de orejas grandes, si fuera mas pequeño\n"};

public:
    SesionCliente(int sock, sockaddr_in cliente) : sockCliente(sock), confCliente(cliente) {}

    void atender() 
    {
        char buffer[BUFFERSIZE] = {0};
        char nombre[BUFFERSIZE] = {0};
        int primerMensaje = 1;

        int random = rand() % WORDQUANTITY;
        string palabraSecreta = posiblesPalabras[random];
        string pista = "Pista: " + pistas[random];

        while (true) 
        {
            memset(buffer, 0, BUFFERSIZE);
            int valread = read(sockCliente, buffer, BUFFERSIZE);
            if (valread <= 0) break;

            if (primerMensaje) 
            {
                strcpy(nombre, buffer);
                string saludo = "Hola " + string(nombre) + "\nJugaremos a las Charadas" + "\nIntenta adivinar la palabra secreta\n";
                string mensajePrincipal = saludo + pista;
                send(sockCliente, mensajePrincipal.c_str(), mensajePrincipal.length(), 0);
                cout << mensajePrincipal << endl;
                primerMensaje = 0;
            } 
            else 
            {
                string palabra = buffer;
                if (comprobarPalabra(palabra,palabraSecreta)) 
                {
                    string mensajeFinal = "Correcto!";
                    send(sockCliente, mensajeFinal.c_str(), mensajeFinal.length(), 0);
                    cout << mensajeFinal << endl;
                    break;
                }

                cout << buffer;
                string respuesta = "Incorrecto, vuelve a intentarlo";

                send(sockCliente, respuesta.c_str(), respuesta.length(), 0);
            }
        }

        close(sockCliente);
    }

private:
    bool comprobarPalabra(const string& palabra, const string& correcta)
    {
        cout << "pruebas: " << palabra << " " << correcta;
        bool igual = palabra.compare(correcta);
        cout << igual << endl;
        return palabra.compare(correcta) == 0;
    }
};

// ------------------------ Clase Servidor ------------------------
class Servidor {
    int sockServidor;
    sockaddr_in confServidor;
    vector<thread> clientes;

public:
    void iniciar(int cantidadClientes) 
    {
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
    void crearSocket() 
    {
        if ((sockServidor = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        {
            perror("Error creando socket");
            exit(EXIT_FAILURE);
        }
    }

    void configurar() 
    {
        confServidor.sin_family = AF_INET;
        confServidor.sin_addr.s_addr = htonl(INADDR_ANY);
        confServidor.sin_port = htons(PUERTO);

        if (bind(sockServidor, (struct sockaddr*)&confServidor, sizeof(confServidor)) < 0) 
        {
            perror("Error en bind");
            exit(EXIT_FAILURE);
        }
    }

    void escuchar(int n) 
    {
        if (listen(sockServidor, n) < 0) 
        {
            perror("Error en listen");
            exit(EXIT_FAILURE);
        }
        cout << "Servidor escuchando en el puerto " << PUERTO << "..." << endl;
    }

    void aceptarCliente() 
    {
        sockaddr_in confCliente;
        socklen_t tamCliente = sizeof(confCliente);
        int sockCliente = accept(sockServidor, (struct sockaddr*)&confCliente, &tamCliente);
        if (sockCliente < 0) 
        {
            perror("Error en accept");
            exit(EXIT_FAILURE);
        }

        // Crear hebra para atender al cliente
        clientes.emplace_back(&SesionCliente::atender, SesionCliente(sockCliente, confCliente));
    }
};

// ------------------------ main() ------------------------
int main(int argc, char* argv[]) 
{
    if (argc < 2) 
    {
        cerr << "Uso: " << argv[0] << " <cantidad_clientes>" << endl;
        return 1;
    }

    int cantidadClientes = atoi(argv[1]);
    if (cantidadClientes <= 0) 
    {
        cerr << "Número inválido de clientes" << endl;
        return 1;
    }

    Servidor servidor;
    servidor.iniciar(cantidadClientes);

    return 0;
}
