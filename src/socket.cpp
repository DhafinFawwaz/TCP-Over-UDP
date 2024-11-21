#include <socket.hpp>
#include <unistd.h> 
using namespace std;

TCPSocket::TCPSocket(char *ip, int32_t port){
    this->ip = ip;
    this->port = port;

    if ((this->socket = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("[i] socket creation failed"); 
        exit(EXIT_FAILURE); 
    }
}

void TCPSocket::listen() {
    if(this->socket < 0) {
        perror("[i] socket creation failed"); 
        exit(EXIT_FAILURE); 
    }

    sockaddr_in servaddr, cliaddr; 
    servaddr = {0};
    cliaddr = {0};
       
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = port; 

    if(bind(this->socket, (sockaddr*)&servaddr, sizeof(servaddr)) < 0) 
    { 
        perror("[i] bind failed"); 
        exit(EXIT_FAILURE); 
    }
}

void TCPSocket::send(char* ip, int32_t port, void* dataStream, uint32_t dataSize) {
    sockaddr_in targetAddr; 
    targetAddr = {0};
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_addr.s_addr = INADDR_ANY; 
    targetAddr.sin_port = port; 

    sendto(this->socket, dataStream, dataSize, MSG_CONFIRM, (sockaddr*) &targetAddr, sizeof(targetAddr)); 
}

int32_t TCPSocket::recv(void* buffer, uint32_t length, sockaddr_in* addr, socklen_t* len) {
    return recvfrom(this->socket, buffer, length, MSG_WAITALL, (sockaddr*) addr, len); 
}

void TCPSocket::close() {
    ::close(this->socket);
}