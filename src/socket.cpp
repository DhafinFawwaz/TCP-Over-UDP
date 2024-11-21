#include <socket.hpp>


void TCPSocket::listen() {
    if ((socket = ::socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    }

    if (::bind(socket, (struct sockaddr *)&ip, sizeof(ip)) == -1) {
    }

    if (::listen(socket, 10) == -1) {
    }
}

void TCPSocket::send(string ip, int32_t port, void *dataStream, uint32_t dataSize) {

}

int32_t TCPSocket::recv(void *buffer, uint32_t length) {
    
}

void close() {

}