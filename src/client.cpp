#include <client.hpp>
#include <iostream>
#include <ansi_code.hpp>
#include <fstream>
#include <segment_handler.hpp>
   
#define MAXLINE 1024 

Client::Client(string& host, int port) : Node(host, port) {}

void Client::setServerTarget(string server_ip, int server_port) {
    this->server_ip = server_ip;
    this->server_port = server_port;
}

void Client::run() {
    connection.listen();

    // ======== Handshake ========
    SegmentHandler segment_handler;
    uint32_t initial_seq_num = segment_handler.generateInitialSeqNum();
    cout << YEL << "[i] [Handshake] [S=" << initial_seq_num << "] Sending SYN request to " << this->server_ip << ":" << this->server_port << COLOR_RESET << endl;
    Segment syn_segment = syn(initial_seq_num);
    connection.send((char*)server_ip.c_str(), server_port, &syn_segment, MAXLINE);

    sockaddr_in addr; socklen_t len;
    Segment syn_ack_segment;
    while(true) {
        auto sync_ack_buffer_size = connection.recv(&syn_ack_segment, MAXLINE, &addr, &len);
        cout << syn_ack_segment.flags.syn << syn_ack_segment.flags.ack << syn_ack_segment.ack_num << endl;
        if(extract_flags(syn_ack_segment.flags) == SYN_ACK_FLAG && syn_ack_segment.seq_num == syn_segment.seq_num + 1) break;
    }
    cout << BLU << "[+] [Handshake] [S=" << syn_ack_segment.seq_num << "] [A=" << syn_ack_segment.ack_num << "] Received SYN-ACK request from " << this->server_ip << ":" << this->server_port << COLOR_RESET << endl;

    Segment ack_segment = ack(syn_ack_segment.ack_num + 1);
    cout << YEL << "[i] [Handshake] [A=" << ack_segment.ack_num << "] Sending ACK request to " << this->server_ip << ":" << this->server_port << COLOR_RESET << endl;
    connection.send((char*)server_ip.c_str(), server_port, &ack_segment, MAXLINE);


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
    cout << BLU << "[i] [Closing] Received FIN request from " << this->server_ip << ":" << this->server_port << COLOR_RESET << endl;

    cout << YEL << "[i] [Closing] Sending FIN-ACK request to " << this->server_ip << ":" << this->server_port << COLOR_RESET << endl;
    char fin_ack_buffer[MAXLINE]; ((char*)fin_ack_buffer)[0] = '\0';
    connection.send((char*)server_ip.c_str(), server_port, (void*)fin_ack_buffer, MAXLINE);

    char ack_buffer_closing[MAXLINE];
    auto ack_buffer_closing_size = connection.recv(ack_buffer_closing, MAXLINE, &addr, &len);
    cout << BLU << "[i] [Closing] Received FIN request from " << this->server_ip << ":" << this->server_port << COLOR_RESET << endl;

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