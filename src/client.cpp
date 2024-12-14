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
    if(this->connection.status != TCPStatusEnum::ESTABLISHED) {
        cout << RED << "[-] Connecting to server failed" << COLOR_RESET << endl;
        return;
    }

    const uint32_t MAX_BUFFER_SIZE = 10 * 1024 * 1024; // 10mb
    vector<uint8_t> buffer(MAX_BUFFER_SIZE);
    sockaddr_in addr; 
    socklen_t len = sizeof(addr);

    int32_t recv_size = this->connection.recv(buffer.data(), buffer.size(), &addr, &len);
    
    if (recv_size > 0) {
        handleMessage(buffer.data(), recv_size);
    } else {
        cout << RED << "[-] Failed to receive data" << COLOR_RESET << endl;
    }
}


void Client::handleMessage(void* response, uint32_t size) {
    cout << "[+] Received: " << size << " byte" << endl;
    ofstream outFile("test/response", ios::binary);
    outFile.write(static_cast<const char*>(response), size);
    outFile.close();
    cout << "[+] Response written to 'test/response'" << endl;
}


Client::~Client() {
    connection.close(-1);
}