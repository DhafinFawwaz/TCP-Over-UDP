#include <server.hpp>
#include <ansi_code.hpp>
#include <iostream>
#include <ostream>
#include <memory>
#include <string>
#include <map>
#include <thread>

Server::Server(std::string& host, int port)
    : Node(host, port), port_lock(std::unique_ptr<PortLock>(new PortLock(port))) {
    
    if (!port_lock) {
        throw std::runtime_error("Failed to acquire port lock.");
    }
}

void Server::SetResponseBuffer(vector<char>& buffer) {
    this->response_buffer = buffer;
}

void Server::handle_client(int client_socket) {
    sockaddr_in addr; socklen_t len = sizeof(addr);
    this->connection.send(client_socket, (char*)this->response_buffer.data(), this->response_buffer.size());
    this->connection.close(client_socket);
}

void Server::run() {
    try {
        this->connection.listen();
        vector<thread> threads;
        while(true) {
            sockaddr_in addr; socklen_t len = sizeof(addr);
            int client_socket = this->connection.accept(&addr, &len);
            threads.emplace_back([this, client_socket]() {
                this->handle_client(client_socket);
            });
        }
    } catch (const std::exception& e) {
        std::cerr << "Error running server: " << e.what() << std::endl;
    }
}


Server::~Server() {
    for (auto& [key, value] : this->connection.connection_map) {
        this->connection.close(key);
    }
}
