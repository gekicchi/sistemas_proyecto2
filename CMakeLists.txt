cmake_minimum_required(VERSION 3.10)
project(Proyecto_Charadas)

set(CMAKE_CXX_STANDARD 17)

# Servidor
add_executable(serverCharadas serverCharadas.cpp)
target_link_libraries(serverCharadas pthread)

# Cliente
add_executable(clienteCharadas clienteCharadas.cpp)