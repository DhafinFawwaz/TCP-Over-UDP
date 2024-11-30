#include <client.hpp>
#include <iostream>
#include <ansi_code.hpp>
#include <fstream>
#include <segment_handler.hpp>

Client::Client(string& host, int port) : Node(host, port) {}

void Client::setServerTarget(string server_ip, int server_port) {
    this->server_ip = server_ip;
    this->server_port = server_port;
}

void Client::run() {
    this->connection.connect(this->server_ip, this->server_port);
    
    uint32_t PAYLOAD_SIZE = 1460;
    uint32_t SEGMENT_ONLY_SIZE = sizeof(Segment);
    uint32_t MAX_SEGMENT_SIZE = PAYLOAD_SIZE + SEGMENT_ONLY_SIZE;
    uint32_t BUFFER_SIZE = MAX_SEGMENT_SIZE*10;
    
    uint8_t buffer[BUFFER_SIZE]; sockaddr_in addr; socklen_t len;
    int32_t recv_size = this->connection.recv(buffer, BUFFER_SIZE, &addr, &len);
    handleMessage(buffer, recv_size);
}


void Client::handleMessage(void* response, uint32_t size) {
    ofstream ofs("test/response.jpg", std::ios::binary);
    ofs.write(reinterpret_cast<char*>(response), size);
    ofs.close();
}


Client::~Client() {
    connection.close();
}