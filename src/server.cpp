#include <server.hpp>
#include <iostream>
#include <ansi_code.hpp>
#include <map>

#define PAYLOAD_SIZE 1460

Server::Server(string& host, int port) : Node(host, port) {}

void Server::SetResponseBuffer(string buffer) {
    this->response_buffer = buffer;
}

void Server::run() {
    this->connection.listen();    
}


Server::~Server() {
    connection.close();
}
