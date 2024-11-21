#include <client.hpp>
#include <iostream>
   
#define MAXLINE 1024 

Client::Client(string& host, int port) : Node(host, port) {}

void Client::setServerTarget(string server_ip, int server_port) {
    this->server_ip = server_ip;
    this->server_port = server_port;
}

void Client::handleMessage(void *buffer) {
    ((char*)buffer)[0] = '\0';
    connection.send((char*)server_ip.c_str(), server_port, buffer, MAXLINE);
    sockaddr_in* addr; 
    socklen_t* len;
    auto n = connection.recv(buffer, MAXLINE, addr, len);
    ((char*)buffer)[n] = '\0';
    cout << "Message from server: " << (char*)buffer << endl;
}

Client::~Client() {
    connection.close();
}