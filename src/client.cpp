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
    uint32_t BUFFER_SIZE = DATA_OFFSET_MAX_SIZE + BODY_ONLY_SIZE*100;
    uint8_t buffer[BUFFER_SIZE]; sockaddr_in addr; socklen_t len = sizeof(addr);
    int32_t recv_size = this->connection.recv(buffer, BUFFER_SIZE, &addr, &len);
    handleMessage(buffer, recv_size);
}


void Client::handleMessage(void* response, uint32_t size) {
    cout << "Size: " << size << endl;
    for(int i = 0; i < 20; i++) {
        cout << ((char*)response)[i];
    }
    cout << endl;
    ofstream outFile("test/response", ios::binary);
    outFile.write(static_cast<const char*>(response), size);
    outFile.close();
}


Client::~Client() {
    connection.close();
}