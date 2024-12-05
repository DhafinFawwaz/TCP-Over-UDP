#include <server.hpp>
#include <ansi_code.hpp>
#include <memory>
#include <string>
#include <map>

Server::Server(std::string& host, int port)
    : Node(host, port), port_lock(std::unique_ptr<PortLock>(new PortLock(port))) {
    
    if (!port_lock) {
        throw std::runtime_error("Failed to acquire port lock.");
    }
}

void Server::SetResponseBuffer(string buffer) {
    this->response_buffer = buffer;
}

void Server::run() {
    try {
        this->connection.listen();
        this->connection.send(this->connection.getConnectedIP().c_str(), 
                              this->connection.getConnectedPort(), 
                              (char*)this->response_buffer.c_str(), 
                              this->response_buffer.size());
    } catch (const std::exception& e) {
        std::cerr << "Error running server: " << e.what() << std::endl;
    }
}


Server::~Server() {
    connection.close();
}
