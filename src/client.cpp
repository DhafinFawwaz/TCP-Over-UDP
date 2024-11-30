#include <client.hpp>
#include <iostream>
#include <ansi_code.hpp>
#include <fstream>
#include <segment_handler.hpp>
   
#define PAYLOAD_SIZE 1460 
#define SEGMENT_SIZE 200

Client::Client(string& host, int port) : Node(host, port) {}

void Client::setServerTarget(string server_ip, int server_port) {
    this->server_ip = server_ip;
    this->server_port = server_port;
}

void Client::run() {
    this->connection.connect(this->server_ip, this->server_port);
}


void Client::handleMessage(void* response, uint32_t size) {
    ofstream ofs("response");
    ofs.write((char*)response, size);
    ofs.close();
}


Client::~Client() {
    connection.close();
}