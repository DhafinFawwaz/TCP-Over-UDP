#include <server.hpp>
#include <iostream>
#include <ansi_code.hpp>

#define MAXLINE 1024

Server::Server(string& host, int port) : Node(host, port) {}

void Server::SetResponseBuffer(string buffer) {
    this->response_buffer = buffer;
}

void Server::run() {
    connection.listen();

    // ======== Handshake ========
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    Segment syn_segment;
    auto sync_buffer_size = connection.recv(&syn_segment, MAXLINE, &addr, &len);
    cout << YEL << "[i] [Handshake] [S=" << syn_segment.seq_num << "] Received SYN request from " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << endl << COLOR_RESET;

    bool retry = true;
    while (retry) {
        SegmentHandler segment_handler;
        uint32_t initial_seq_num = segment_handler.generateInitialSeqNum();
        Segment syn_ack_segment = synAck(syn_segment.seq_num + 1, initial_seq_num);
        cout << BLU << "[i] [Handshake] [A=" << syn_ack_segment.ack_num << "] [S=" << syn_ack_segment.seq_num << "] Sending SYN-ACK request to " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << COLOR_RESET << endl;
        connection.send(inet_ntoa(addr.sin_addr), addr.sin_port, &syn_ack_segment, sync_buffer_size);

        Segment ack_segment;
        while (true) {
            auto ack_buffer_size = connection.recv(&ack_segment, MAXLINE, &addr, &len);
            if(ack_buffer_size < 0) {
                cout << RED << "[-] [Handshake] Timeout, retrying" << COLOR_RESET << endl;
                retry = true;
                break;
            }
            retry = false;
            if(extract_flags(ack_segment.flags) == ACK_FLAG && ack_segment.ack_num == syn_ack_segment.ack_num + 1) break;   
        }
        if(retry) continue;
        
        cout << YEL << "[i] [Handshake] [A=" << ack_segment.ack_num << "] Received ACK request from " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << COLOR_RESET << endl;
        

        
        // ======== Established ========
        cout << "[i] Sending input to " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << endl;

        connection.send(inet_ntoa(addr.sin_addr), addr.sin_port, (void*)response_buffer.c_str(), response_buffer.size());
        cout << BLU << "[i] [Established] [Seg 1] [S=2266133600] Sent" << COLOR_RESET << endl;
        cout << BLU << "[i] [Established] [Seg 2] [S=2266135060] Sent" << COLOR_RESET << endl;
        cout << BLU << "[i] [Established] [Seg 3] [S=2266136520] Sent" << COLOR_RESET << endl;


        cout << "[~] [Established] Waiting for segments to be ACKed" << endl;

        cout << YEL << "[i] [Established] [Seg 1] [A=3165500900] ACKed" << COLOR_RESET << endl;
        cout << YEL << "[i] [Established] [Seg 2] [A=3165502360] ACKed" << COLOR_RESET << endl;
        cout << YEL << "[i] [Established] [Seg 3] [A=3165503820] ACKed" << COLOR_RESET << endl;


        // ======== Closing ========
        cout << YEL << "[i] [Closing] Sending FIN request to " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << COLOR_RESET << endl;
        char fin_buffer[MAXLINE]; ((char*)fin_buffer)[0] = '\0';
        connection.send(inet_ntoa(addr.sin_addr), addr.sin_port, (void*)fin_buffer, sync_buffer_size);

        char fin_ack_buffer[MAXLINE];
        auto fin_ack_buffer_size = connection.recv(fin_ack_buffer, MAXLINE, &addr, &len);
        cout << BLU << "[+] [Closing] Received FIN-ACK request from " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << COLOR_RESET << endl;

        cout << BLU << "[i] [Closing] Sending ACK request to " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << COLOR_RESET << endl;
        char ack_buffer_closing[MAXLINE]; ((char*)ack_buffer_closing)[0] = '\0';
        connection.send(inet_ntoa(addr.sin_addr), addr.sin_port, (void*)ack_buffer_closing, sync_buffer_size);

        cout << GRN << "[i] Connection closed successfully" << COLOR_RESET << endl;
    }
    
}


Server::~Server() {
    connection.close();
}
