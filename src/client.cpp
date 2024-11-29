#include <client.hpp>
#include <iostream>
#include <ansi_code.hpp>
#include <fstream>
   
#define MAXLINE 1024 

Client::Client(string& host, int port) : Node(host, port) {}

void Client::setServerTarget(string server_ip, int server_port) {
    this->server_ip = server_ip;
    this->server_port = server_port;
}

void Client::run() {
    connection.listen();

    // ======== Handshake ========
    cout << YEL << "[i] [Handshake] [S=2266133599] Sending SYN request to " << this->server_ip << ":" << server_port << COLOR_RESET << endl;
    char sync_buffer[MAXLINE]; ((char*)sync_buffer)[0] = '\0';
    connection.send((char*)server_ip.c_str(), server_port, sync_buffer, MAXLINE);

    char sync_ack_buffer[MAXLINE]; sockaddr_in addr; socklen_t len;
    auto sync_ack_buffer_size = connection.recv(sync_ack_buffer, MAXLINE, &addr, &len);
    cout << BLU << "[+] [Handshake] [S=2266133600] [A=3165500899] Received SYN-ACK request from " << this->server_ip << ":" << this->server_port << COLOR_RESET << endl;

    cout << YEL << "[i] [Handshake] [A=3165500900] Sending ACK request to " << this->server_ip << ":" << server_port << COLOR_RESET << endl;
    char ack_buffer[MAXLINE]; ((char*)ack_buffer)[0] = '\0';
    connection.send((char*)server_ip.c_str(), server_port, ack_buffer, MAXLINE);


    // ======== Established ========
    cout << "[i] Ready to receive input from " << this->server_ip << ":" << server_port << endl;

    cout << "[~] [Established] Waiting for segments to be sent" << endl;


    char input_buffer[MAXLINE];
    auto input_buffer_size = connection.recv(input_buffer, MAXLINE, &addr, &len);
    cout << BLU << "[i] [Established] [Seg 1] [S=2266133600] ACKed" << COLOR_RESET << endl;
    cout << BLU << "[i] [Established] [Seg 2] [S=2266135060] ACKed" << COLOR_RESET << endl;
    cout << YEL << "[i] [Established] [Seg 1] [A=3165500900] Sent" << COLOR_RESET << endl;
    cout << BLU << "[i] [Established] [Seg 3] [S=2266136520] ACKed" << COLOR_RESET << endl;
    cout << YEL << "[i] [Established] [Seg 2] [A=3165502360] Sent" << COLOR_RESET << endl;
    cout << YEL << "[i] [Established] [Seg 3] [A=3165503820] Sent" << COLOR_RESET << endl;

    cout << "[~] [Established] Waiting for segments to be sent" << endl;


    // ======== Closing ========
    char fin_buffer[MAXLINE];
    auto fin_buffer_size = connection.recv(fin_buffer, MAXLINE, &addr, &len);
    cout << BLU << "[i] [Closing] Received FIN request from " << this->server_ip << ":" << server_port << COLOR_RESET << endl;

    cout << YEL << "[i] [Closing] Sending FIN-ACK request to " << this->server_ip << ":" << server_port << COLOR_RESET << endl;
    char fin_ack_buffer[MAXLINE]; ((char*)fin_ack_buffer)[0] = '\0';
    connection.send(inet_ntoa(addr.sin_addr), addr.sin_port, (void*)fin_ack_buffer, sync_ack_buffer_size);

    char ack_buffer_closing[MAXLINE];
    auto ack_buffer_closing_size = connection.recv(ack_buffer_closing, MAXLINE, &addr, &len);
    cout << BLU << "[i] [Closing] Received FIN request from " << this->server_ip << ":" << server_port << COLOR_RESET << endl;

    cout << GRN << "[i] Connection closed successfully" << COLOR_RESET << endl;

    handleMessage(input_buffer, input_buffer_size);
}


void Client::handleMessage(void* response, uint32_t size) {
    ofstream ofs("response");
    ofs.write((char*)response, size);
    ofs.close();
}


Client::~Client() {
    connection.close();
}