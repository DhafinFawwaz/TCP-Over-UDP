#include <server.hpp>
#include <iostream>
#include <ansi_code.hpp>
#include <map>

Server::Server(string& host, int port) : Node(host, port) {}

void Server::SetResponseBuffer(string buffer) {
    this->response_buffer = buffer;
}

void Server::run() {
    this->connection.listen();
    this->connection.send(this->connection.getConnectedIP().c_str(), this->connection.getConnectedPort(), (char*)this->response_buffer.c_str(), this->response_buffer.size());
}


Server::~Server() {
    connection.close();
}
