#include <socket.hpp>
#include <unistd.h> 
#include <ansi_code.hpp>
#include <iostream>
using namespace std;


TCPSocket::TCPSocket(string& ip, int32_t port){
    this->status = TCPStatusEnum::CLOSED;
    this->ip = ip;
    this->port = port;

    if ((this->socket = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("[i] socket creation failed"); 
        exit(EXIT_FAILURE); 
    }
}

void TCPSocket::initSocket() {
    if(this->socket < 0) {
        perror("[i] socket creation failed"); 
        exit(EXIT_FAILURE); 
    }

    sockaddr_in servaddr, cliaddr; 
    servaddr = {0};
    cliaddr = {0};
       
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = port; 

    if(bind(this->socket, (sockaddr*)&servaddr, sizeof(servaddr)) < 0) 
    { 
        perror("[i] bind failed"); 
        exit(EXIT_FAILURE); 
    }

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(this->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting socket timeout");
        exit(EXIT_FAILURE);
    }
}


// return size of received data, -1 if error
int32_t TCPSocket::recvAny(void* buffer, uint32_t length, sockaddr_in* addr, socklen_t* len) {
    return recvfrom(this->socket, buffer, length, MSG_WAITALL, (sockaddr*) addr, len); 
}
void TCPSocket::sendAny(const char* ip, int32_t port, void* dataStream, uint32_t dataSize) {
    sockaddr_in targetAddr; 
    targetAddr = {0};
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_addr.s_addr = INADDR_ANY; 
    targetAddr.sin_port = port;

    sendto(this->socket, dataStream, dataSize, MSG_CONFIRM, (sockaddr*) &targetAddr, sizeof(targetAddr)); 
}

// Handshake
void TCPSocket::listen() {
    this->status = TCPStatusEnum::LISTEN;
    initSocket();
    cout << BLU << "[i] " << getFormattedStatus() << " Listening to the broadcast port for clients." << COLOR_RESET << endl;
    
    
    sockaddr_in addr; socklen_t len = sizeof(addr);
    Segment syn_segment;
    int32_t sync_buffer_size;
    while (true) {
        sync_buffer_size = recvAny(&syn_segment, sizeof(syn_segment), &addr, &len);
        if(sync_buffer_size >= 0) break; // sync_buffer_size < 0 just means timeout, retry
    }
    
    char* received_ip = inet_ntoa(addr.sin_addr);
    this->status = TCPStatusEnum::SYN_RECEIVED;
    cout << YEL << "[i] " << getFormattedStatus() << " [Handshake] [S=" << syn_segment.seq_num << "] Received SYN request from " << received_ip << ":" << addr.sin_port << endl << COLOR_RESET;

    bool retry = true;
    Segment ack_segment;
    while (retry) {
        uint32_t initial_seq_num = segment_handler.generateInitialSeqNum();
        Segment syn_ack_segment = synAck(syn_segment.seq_num + 1, initial_seq_num);
        cout << BLU << "[i] " << getFormattedStatus() << " [Handshake] [A=" << syn_ack_segment.ack_num << "] [S=" << syn_ack_segment.seq_num << "] Sending SYN-ACK request to " << received_ip << ":" << addr.sin_port << COLOR_RESET << endl;
        sendAny(received_ip, addr.sin_port, &syn_ack_segment, sync_buffer_size);

        while (true) {
            auto ack_buffer_size = recvAny(&ack_segment, sizeof(ack_segment), &addr, &len);
            if(ack_buffer_size < 0) {
                cout << RED << "[-] " << getFormattedStatus() << " [Handshake] Error, retrying" << COLOR_RESET << endl; // example case is timeout
                retry = true;
                break;
            }
            retry = false;
            if(extract_flags(ack_segment.flags) == ACK_FLAG && ack_segment.ack_num == syn_ack_segment.ack_num + 1) break;   
        }
    }

    cout << YEL << "[i] " << getFormattedStatus() << " [Handshake] [A=" << ack_segment.ack_num << "] Received ACK request from " << received_ip << ":" << addr.sin_port << COLOR_RESET << endl;
    cout << GRN << "[i] " << getFormattedStatus() << " [Established] Connection estabilished with "<< received_ip << ":" << addr.sin_port << COLOR_RESET << endl;
}

// Handshake
void TCPSocket::connect(string& server_ip, int32_t server_port) {
    initSocket();
    this->status = TCPStatusEnum::LISTEN;

    this->server_ip = server_ip;
    this->server_port = server_port;

    cout << YEL << "[+] " << getFormattedStatus() << " Trying to contact the sender at " << this->server_ip << ":" << this->server_port << COLOR_RESET << endl;


    bool retry = true;
    sockaddr_in addr; socklen_t len = sizeof(addr);
    while(retry) {
        uint32_t initial_seq_num = segment_handler.generateInitialSeqNum();
        this->status = TCPStatusEnum::SYN_SENT;
        cout << YEL << "[i] " << getFormattedStatus() << " [Handshake] [S=" << initial_seq_num << "] Sending SYN request to " << this->server_ip << ":" << this->server_port << COLOR_RESET << endl;
        Segment syn_segment = syn(initial_seq_num);
        sendAny(this->server_ip.c_str(), this->server_port, &syn_segment, sizeof(syn_segment));
        Segment syn_ack_segment;

        while(true) {
            auto sync_ack_buffer_size = recvAny(&syn_ack_segment, sizeof(syn_ack_segment), &addr, &len);
            if(sync_ack_buffer_size < 0) {
                cout << RED << "[-] " << getFormattedStatus() << " [Handshake] Error, retrying" << COLOR_RESET << endl; // example case it timeout
                retry = true;
                break;
            }
            retry = false;
            if(extract_flags(syn_ack_segment.flags) == SYN_ACK_FLAG && syn_ack_segment.seq_num == syn_segment.seq_num + 1) break;
        }
        if(retry) continue;

        cout << BLU << "[+] " << getFormattedStatus() << " [Handshake] [S=" << syn_ack_segment.seq_num << "] [A=" << syn_ack_segment.ack_num << "] Received SYN-ACK request from " << this->server_ip << ":" << this->server_port << COLOR_RESET << endl;

        Segment ack_segment = ack(syn_ack_segment.ack_num + 1);
        cout << YEL << "[i] " << getFormattedStatus() << " [Handshake] [A=" << ack_segment.ack_num << "] Sending ACK request to " << this->server_ip << ":" << this->server_port << COLOR_RESET << endl;
        sendAny(this->server_ip.c_str(), this->server_port, &ack_segment, sizeof(ack_segment));
    }
    this->status = TCPStatusEnum::ESTABLISHED;
    cout << GRN << "[i] " << getFormattedStatus() << " [Established] Connection estabilished with "<< this->server_ip << ":" << this->server_port << COLOR_RESET << endl;
}

// Sliding window
void TCPSocket::send(const char* ip, int32_t port, void* dataStream, uint32_t dataSize) {
    cout << BLU << "[i] " << getFormattedStatus() << " [i] Sending input to " << ip << ":" << port << COLOR_RESET << endl;

    sockaddr_in targetAddr; 
    targetAddr = {0};
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_addr.s_addr = INADDR_ANY; 
    targetAddr.sin_port = port; 

    sendto(this->socket, dataStream, dataSize, MSG_CONFIRM, (sockaddr*) &targetAddr, sizeof(targetAddr)); 
}

// Sliding window
int32_t TCPSocket::recv(void* buffer, uint32_t length, sockaddr_in* addr, socklen_t* len) {
    cout << YEL << "[i] " << getFormattedStatus() << " [i] Ready to receive input from " << this->server_ip << ":" << this->server_port << COLOR_RESET << endl;
    
    return recvfrom(this->socket, buffer, length, MSG_WAITALL, (sockaddr*) addr, len); 
}

void TCPSocket::close() {
    ::close(this->socket);
}

string TCPSocket::getFormattedStatus() {
    return "[" + to_string(this->status) + "]";
}
