#include <socket.hpp>
#include <unistd.h> 
#include <ansi_code.hpp>
#include <iostream>
using namespace std;
#define PAYLOAD_SIZE 1460

TCPSocket::TCPSocket(string& ip, int32_t port){
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
    targetAddr.sin_addr.s_addr = inet_addr(ip);
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_addr.s_addr = INADDR_ANY; 
    targetAddr.sin_port = port; 

    sendto(this->socket, dataStream, dataSize, MSG_CONFIRM, (sockaddr*) &targetAddr, sizeof(targetAddr)); 
}

// Handshake
void TCPSocket::listen() {
    // this->status = TCPStatusEnum::LISTEN;
    cout << "hmmm" << endl;
    initSocket();
    
    
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    Segment syn_segment;
    int32_t sync_buffer_size;
    while (true) {
        sync_buffer_size = recvAny(&syn_segment, PAYLOAD_SIZE, &addr, &len);
        if(sync_buffer_size < 0) continue; // just means timeout, retry

        cout << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << endl;
        cout << "hmmm" << endl;
    }
    
    cout << YEL << "[i] " << getFormattedStatus() << " [Handshake] [S=" << syn_segment.seq_num << "] Received SYN request from " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << endl << COLOR_RESET;

    bool retry = true;
    SegmentHandler segment_handler;
    Segment ack_segment;
    while (retry) {
        uint32_t initial_seq_num = segment_handler.generateInitialSeqNum();
        Segment syn_ack_segment = synAck(syn_segment.seq_num + 1, initial_seq_num);
        cout << BLU << "[i] " << getFormattedStatus() << " [Handshake] [A=" << syn_ack_segment.ack_num << "] [S=" << syn_ack_segment.seq_num << "] Sending SYN-ACK request to " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << COLOR_RESET << endl;
        sendAny(inet_ntoa(addr.sin_addr), addr.sin_port, &syn_ack_segment, sync_buffer_size);

        while (true) {
            auto ack_buffer_size = recvAny(&ack_segment, PAYLOAD_SIZE, &addr, &len);
            if(ack_buffer_size < 0) {
                cout << RED << "[-] " << getFormattedStatus() << " [Handshake] Error, retrying" << COLOR_RESET << endl; // example case is timeout
                retry = true;
                break;
            }
            retry = false;
            if(extract_flags(ack_segment.flags) == ACK_FLAG && ack_segment.ack_num == syn_ack_segment.ack_num + 1) break;   
        }
        if(retry) continue;
        
        cout << YEL << "[i] " << getFormattedStatus() << " [Handshake] [A=" << ack_segment.ack_num << "] Received ACK request from " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << COLOR_RESET << endl;
        
        cout << GRN << "[i] " << getFormattedStatus() << " [Established] Connection estabilished with "<< inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << COLOR_RESET << endl;
    }
}

// Handshake
void TCPSocket::connect(string& server_ip, int32_t server_port) {
    initSocket();
    this->server_ip = server_ip;
    this->server_port = server_port;

    bool retry = true;
    sockaddr_in addr; socklen_t len;
    while(retry) {
        uint32_t initial_seq_num = segment_handler.generateInitialSeqNum();
        cout << YEL << "[i] " << getFormattedStatus() << " [Handshake] [S=" << initial_seq_num << "] Sending SYN request to " << this->server_ip << ":" << this->server_port << COLOR_RESET << endl;
        Segment syn_segment = syn(initial_seq_num);
        sendAny(this->server_ip.c_str(), this->server_port, &syn_segment, PAYLOAD_SIZE);

        Segment syn_ack_segment;
        while(true) {
            auto sync_ack_buffer_size = recv(&syn_ack_segment, PAYLOAD_SIZE, &addr, &len);
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
        sendAny(this->server_ip.c_str(), this->server_port, &ack_segment, PAYLOAD_SIZE);
    }
}


void TCPSocket::send(const char* ip, int32_t port, void* dataStream, uint32_t dataSize) {
    sockaddr_in targetAddr; 
    targetAddr = {0};
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_addr.s_addr = INADDR_ANY; 
    targetAddr.sin_port = port; 

    sendto(this->socket, dataStream, dataSize, MSG_CONFIRM, (sockaddr*) &targetAddr, sizeof(targetAddr)); 
}

int32_t TCPSocket::recv(void* buffer, uint32_t length, sockaddr_in* addr, socklen_t* len) {
    return recvfrom(this->socket, buffer, length, MSG_WAITALL, (sockaddr*) addr, len); 
}

void TCPSocket::close() {
    ::close(this->socket);
}

string TCPSocket::getFormattedStatus() {
    // return "[" + to_string(this->status) + "]";
    return "";
}
