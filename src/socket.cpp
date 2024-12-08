#include <socket.hpp>
#include <unistd.h> 
#include <ansi_code.hpp>
#include <iostream>
#include <string.h> // just for memcpy
#include <chrono>
#include <deque>
#include <algorithm>
#include <map>
#include <tuple>
#include <utility> // for pair
// #include <cstring> //memset

using namespace std;
using namespace std::chrono;

TCPSocket::TCPSocket(string& ip, int32_t port){
    this->status = TCPStatusEnum::CLOSED;
    this->ip = ip;
    this->port = port;

    if ((this->socket = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("[i] socket creation failed"); 
        exit(EXIT_FAILURE); 
    }
}

string TCPSocket::getConnectedIP() {
    return this->connected_ip;
}

int32_t TCPSocket::getConnectedPort() {
    return this->connected_port;
}

bool TCPSocket::isSocketBinded(int socket_fd) {
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    if (getsockname(socket_fd, (sockaddr*)&addr, &addr_len) == 0) {
        if (addr.sin_port != 0) {
            return true;
        }
    }
    return false;
}

void TCPSocket::initSocket() {
    if(isSocketBinded(this->socket)) {
        return;
    }

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
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt(this->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting socket timeout");
        exit(EXIT_FAILURE);
    }
}

// return size of received data, -1 if error
int32_t TCPSocket::recvAny(void* buffer, uint32_t length, sockaddr_in* addr, socklen_t* len) {
    uint32_t size = recvfrom(this->socket, buffer, length, MSG_WAITALL, (sockaddr*) addr, len); 

    // cout << "content received:" << endl;
    // for(int i = 0; i < 20; i++) {
    //     cout << ((char*)buffer)[i];
    // }
    // cout << endl;

    return size;
}
void TCPSocket::sendAny(const char* ip, int32_t port, void* dataStream, uint32_t dataSize) {
    sockaddr_in targetAddr; 
    targetAddr = {0};
    targetAddr.sin_family = AF_INET;
    if(memcmp(ip, "localhost", 9) == 0) 
        targetAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    else if (inet_pton(AF_INET, ip, &targetAddr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }
        
    targetAddr.sin_port = port;

    // cout << "content to send:" << endl;
    // for(int i = 0; i < 20; i++) {
    //     cout << ((char*)dataStream)[i];
    // }
    // cout << endl;

    sendto(this->socket, dataStream, dataSize, MSG_CONFIRM, (sockaddr*) &targetAddr, sizeof(targetAddr)); 
}

// Handshake
void TCPSocket::listen() {
    this->status = TCPStatusEnum::LISTEN;
    initSocket();
    while(this->status != TCPStatusEnum::ESTABLISHED) {
        cout << BLU << "[i] " << getFormattedStatus() << " Listening to the broadcast port for clients as " << this->ip << ":" << this->port << COLOR_RESET << endl;
        
        sockaddr_in addr; socklen_t len = sizeof(addr);
        Segment syn_segment, syn_ack_segment, ack_segment;
        int32_t sync_buffer_size;
        while (true) {
            sync_buffer_size = recvAny(&syn_segment, HEADER_ONLY_SIZE, &addr, &len);
            if(sync_buffer_size == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue; // no request received. just continue
            if(!isValidChecksum(syn_segment)) {
                cout << RED << "[i] " << getFormattedStatus() << " [Handshake] Invalid checksum, received corrupted packet" << COLOR_RESET << endl;
                continue;
            }
            if(sync_buffer_size >= 0) break; // request received
        }
        
        char* received_ip = inet_ntoa(addr.sin_addr);
        this->status = TCPStatusEnum::SYN_RECEIVED;
        cout << YEL << "[i] " << getFormattedStatus() << " [Handshake] [S=" << syn_segment.seq_num << "] Received SYN request from " << received_ip << ":" << addr.sin_port << endl << COLOR_RESET;

        auto send_time_syn = high_resolution_clock::now();
        chrono::seconds timeout_syn(12); // timelimit to retry sending syn ack again
        bool retry = true;
        while (retry) {
            if(high_resolution_clock::now() - send_time_syn > timeout_syn) {
                this->status = TCPStatusEnum::LISTEN;
                cout << RED << "[i] " << getFormattedStatus() << " [Established] Timeout" << COLOR_RESET << endl;
                break;
            }

            uint32_t initial_seq_num = segment_handler.generateInitialSeqNum();
            syn_ack_segment = synAck(syn_segment.seq_num + 1, initial_seq_num);
            cout << BLU << "[i] " << getFormattedStatus() << " [Handshake] [A=" << syn_ack_segment.ack_num << "] [S=" << syn_ack_segment.seq_num << "] Sending SYN-ACK request to " << received_ip << ":" << addr.sin_port << COLOR_RESET << endl;
            sendAny(received_ip, addr.sin_port, &syn_ack_segment, HEADER_ONLY_SIZE);

            auto send_time_syn_ack = high_resolution_clock::now();
            chrono::seconds timeout_syn_ack(5); // time limit to receive ack
            while (true) {
                auto ack_buffer_size = recvAny(&ack_segment, HEADER_ONLY_SIZE, &addr, &len);
                if(sync_buffer_size == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    if(high_resolution_clock::now() - send_time_syn_ack > timeout_syn_ack) {
                        this->status = TCPStatusEnum::LISTEN;
                        cout << RED << "[i] " << getFormattedStatus() << " [Established] Timeout" << COLOR_RESET << endl;
                        break;
                    } else continue;
                }
                if(!isValidChecksum(syn_segment)) {
                    cout << RED << "[i] " << getFormattedStatus() << " [Handshake] Invalid checksum, received corrupted packet" << COLOR_RESET << endl;
                    continue;
                }
                if(ack_buffer_size < 0) {
                    cout << RED << "[-] " << getFormattedStatus() << " [Handshake] Error, retrying" << COLOR_RESET << endl; // example case is timeout
                    retry = true;
                    break;
                }
                retry = false;
                if(extract_flags(ack_segment.flags) == ACK_FLAG && ack_segment.ack_num == syn_ack_segment.ack_num + 1) {
                    cout << YEL << "[i] " << getFormattedStatus() << " [Handshake] [A=" << ack_segment.ack_num << "] Received ACK request from " << received_ip << ":" << addr.sin_port << COLOR_RESET << endl;
                    this->status = TCPStatusEnum::ESTABLISHED;
                    break;  
                } 
            }
        }
        if(this->status == TCPStatusEnum::LISTEN) continue;

        cout << GRN << "[i] " << getFormattedStatus() << " [Established] Connection estabilished with "<< received_ip << ":" << addr.sin_port << COLOR_RESET << endl;
        this->segment_handler.setInitialSeqNum(syn_ack_segment.seq_num);
        this->segment_handler.setInitialAckNum(ack_segment.ack_num);

        this->connected_ip = string(received_ip);
        this->connected_port = addr.sin_port;
    }
}


uint32_t TCPSocket::calculateSegmentIndex(uint32_t seq_num, uint32_t initial_seq_num) {
    // return ceil((seq_num - initial_seq_num + 1) / static_cast<double>(PAYLOAD_SIZE)) - 1;
    return (seq_num - initial_seq_num + PAYLOAD_SIZE - 1) / PAYLOAD_SIZE + 1;
}

// Handshake
void TCPSocket::connect(string& server_ip, int32_t server_port) {
    initSocket();
    this->status = TCPStatusEnum::LISTEN;

    this->connected_ip = server_ip;
    this->connected_port = server_port;

    cout << YEL << "[+] " << getFormattedStatus() << " Trying to contact the sender at " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;


    bool retry = true;
    sockaddr_in addr; socklen_t len = sizeof(addr);
    Segment syn_segment, syn_ack_segment, ack_segment;
    while(retry) {
        uint32_t initial_seq_num = segment_handler.generateInitialSeqNum();
        this->status = TCPStatusEnum::SYN_SENT;
        cout << YEL << "[i] " << getFormattedStatus() << " [Handshake] [S=" << initial_seq_num << "] Sending SYN request to " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
        syn_segment = syn(initial_seq_num);
        sendAny(this->connected_ip.c_str(), this->connected_port, &syn_segment, HEADER_ONLY_SIZE);
        this->status = TCPStatusEnum::SYN_SENT;

        auto send_time_syn_ack = high_resolution_clock::now();
        chrono::seconds timeout_syn_ack(5); // time limit to receive syn ack
        while(true) {
            auto sync_ack_buffer_size = recvAny(&syn_ack_segment, HEADER_ONLY_SIZE, &addr, &len);
            if(sync_ack_buffer_size == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if(high_resolution_clock::now() - send_time_syn_ack > timeout_syn_ack) {
                    this->status = TCPStatusEnum::SYN_SENT;
                    cout << RED << "[i] " << getFormattedStatus() << " [Established] Timeout" << COLOR_RESET << endl;
                    break;
                } else continue;
            }

            if(sync_ack_buffer_size < 0) {
                cout << RED << "[-] " << getFormattedStatus() << " [Handshake] Error, retrying" << COLOR_RESET << endl; // example case it timeout
                // cout << errno << endl;
                retry = true;
                break;
            }
            retry = false;
            if(extract_flags(syn_ack_segment.flags) == SYN_ACK_FLAG && syn_ack_segment.seq_num == syn_segment.seq_num + 1) break;
        }
        if(retry) continue;

        cout << YEL << "[+] " << getFormattedStatus() << " [Handshake] [S=" << syn_ack_segment.seq_num << "] [A=" << syn_ack_segment.ack_num << "] Received SYN-ACK request from " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;

        ack_segment = ack(syn_ack_segment.ack_num + 1);
        cout << BLU << "[i] " << getFormattedStatus() << " [Handshake] [A=" << ack_segment.ack_num << "] Sending ACK request to " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
        sendAny(this->connected_ip.c_str(), this->connected_port, &ack_segment, HEADER_ONLY_SIZE);
    }
    this->status = TCPStatusEnum::ESTABLISHED;
    cout << GRN << "[i] " << getFormattedStatus() << " [Established] Connection estabilished with "<< this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;

    this->segment_handler.setInitialAckNum(ack_segment.ack_num);
    this->segment_handler.setInitialSeqNum(syn_ack_segment.seq_num);
}

// Sliding window with Go-Back-N
void TCPSocket::send(const char* ip, int32_t port, void* dataStream, uint32_t dataSize) {
    cout << MAG << "[i] " << getFormattedStatus() << " Sending input to " << ip << ":" << port << COLOR_RESET << endl;
    // for(uint32_t i = 0; i < dataSize; i++) {
    //     cout << dataStream[i];
    // }

    // TODO: find out how much should this value be
    const uint32_t SWS = (4 - 1) * PAYLOAD_SIZE; // Sender Window Size
    uint32_t initial_seq_num = segment_handler.getInitialSeqNum();
    uint32_t initial_ack_num = segment_handler.getInitialAckNum();
    uint32_t LAR = initial_seq_num; // Last Acknowledgment Received. -1 because if there's only 1 segment, initial_seq_num = last_seq_num.
    segment_handler.setDataStream(dataStream, dataSize);
    // segment_handler.generateSegments(this->port, port);
    // cout << "generate segment" << endl;
    segment_handler.generateSegmentsMap(this->port, port);
    // cout << "generate segment done" << endl;
    uint32_t LFS = min(initial_seq_num + SWS, prev(segment_handler.segmentMap.end())->second.seq_num); // Last Frame Sent


    auto send_time = high_resolution_clock::now();  
    chrono::seconds timeout(5);
    while(true) {
        // cout << "======================" << endl;
        // sleep(1);

        bool anything_sent = false;
        
        // cout << "LAR: " << LAR << endl;
        // cout << "LFS: " << LFS << endl;
        for(auto& [seq_num, segment] : segment_handler.segmentMap) {
            if(seq_num < LAR) continue;
            if(seq_num > LFS) break;
            anything_sent = true;
            
            // char payload[sizeof(Segment) + segment.payload_len];
            char* payload = new char[segment.data_offset*4 + segment.payload.size()];
            memcpy(payload, &segment, 20);
            // memcpy(payload+sizeof(Segment)-16, segment.options, 8);
            memcpy(payload+20, segment.options.data(), segment.options.size());
            memcpy(payload+segment.data_offset*4, segment.payload.data(), segment.payload.size());
            // cout << "sending: " << segment.payload.size() << endl;
            sendAny(ip, port, payload, segment.data_offset*4 + segment.payload.size());
            delete[] payload;
            uint32_t data_index = calculateSegmentIndex(seq_num, initial_seq_num);
            cout << BLU << "[i] " << getFormattedStatus() << " [Established] [Seg " << data_index << "] [S=" << segment.seq_num << "] Sent to " << this->connected_ip << ":" << this->connected_port << endl;
        }
        if(!anything_sent) break;

        Segment ack_segment;
        sockaddr_in addr; socklen_t len = sizeof(addr);
        // [~] [Established] Waiting for segments to be ACKed
        cout << MAG << "[~] " << getFormattedStatus() << " [Established] Waiting for segments to be ACKed" << COLOR_RESET << endl;
        while(true) {
            auto recv_size = recvAny(&ack_segment, HEADER_ONLY_SIZE, &addr, &len);
            if(!isValidChecksum(ack_segment)) {
                cout << RED << "[i] " << getFormattedStatus() << " [Handshake] Invalid checksum, received corrupted packet" << COLOR_RESET << endl;
                continue;
            }
            // cout << "recv_size: " << recv_size << endl;
            if(recv_size < 0) {
                if(high_resolution_clock::now() - send_time > timeout) {
                    this->status = TCPStatusEnum::FAILED;
                    cout << RED << "[i] " << getFormattedStatus() << " [Established] Timeout" << COLOR_RESET << endl;
                    break; // dont forget to change to break
                } else break;  // dont forget to change to continue
            }
            // cout << "LFS: " << LFS << endl;
            // cout << "LAR: " << LAR << endl;
            // cout << "if(!isValidChecksum(ack_segment))" << endl;
            if(!isValidChecksum(ack_segment)) {
                cout << RED << "[i] " << getFormattedStatus() << " [Established] Invalid checksum, received corrupted packet" << COLOR_RESET << endl;
                continue;
            }


            // cout << "ack: " << ack_segment.ack_num << endl;
            if(extract_flags(ack_segment.flags) == ACK_FLAG) {
                // while(LFS < SWS - LAR) LFS += PAYLOAD_SIZE;
                LFS = min(prev(segment_handler.segmentMap.end())->second.seq_num, LFS + ack_segment.ack_num - LAR);
                LAR = max(LAR, ack_segment.ack_num);

                uint32_t data_index = calculateSegmentIndex(ack_segment.ack_num, initial_seq_num);
                cout << YEL << "[i] " << getFormattedStatus() << " [Established] [Seg " << data_index-1 << "] [A=" << ack_segment.ack_num << "] ACKed from " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
                
                // cout << "LFS: " << LFS << endl;
                // cout << "LAR: " << LAR << endl;
                
                break;
            }
        }
    }

    if(this->status == TCPStatusEnum::FAILED) {
        cout << RED << "[-] " << getFormattedStatus() << " [Failed]" << COLOR_RESET << endl;
        return;
    }
    
    segment_handler.setInitialSeqNum(LFS); // so that the next request will have a new seq_num
    fin_send(ip, port);
    
}

void TCPSocket::fin_send(const char* ip, int32_t port) {
    
    Segment fin_segment = fin();
    sendAny(ip, port, &fin_segment, HEADER_ONLY_SIZE);
    cout << BLU << "[i] " << getFormattedStatus() << " [Closing] Sending FIN request to " << ip << ":" << port << COLOR_RESET << endl;
    this->status = TCPStatusEnum::FIN_WAIT_1;

    Segment fin_ack_segment; sockaddr_in addr; socklen_t len = sizeof(addr);
    while (true) {
        auto recv_size = recvAny(&fin_ack_segment, HEADER_ONLY_SIZE, &addr, &len);
        if(!isValidChecksum(fin_ack_segment)) {
            cout << RED << "[i] " << getFormattedStatus() << " [Handshake] Invalid checksum, received corrupted packet" << COLOR_RESET << endl;
            continue;
        }
        if (recv_size > 0 && extract_flags(fin_ack_segment.flags) == FIN_ACK_FLAG) {
            cout << YEL << "[+] " << getFormattedStatus() << " [Closing] Received FIN-ACK request from " << ip << ":" << port << COLOR_RESET << endl;
            this->status = TCPStatusEnum::CLOSING;
            break;
        }
    }

    Segment ack_segment = ack(fin_ack_segment.ack_num);
    sendAny(ip, port, &ack_segment, HEADER_ONLY_SIZE);
    this->status = TCPStatusEnum::TIME_WAIT;
    cout << BLU << "[i] " << getFormattedStatus() << " [Closing] Sending ACK request to " << ip << ":" << port << COLOR_RESET << endl;

    this->status = TCPStatusEnum::CLOSED;
    cout << GRN << "[i] " << getFormattedStatus() << " Connection closed successfully" << COLOR_RESET << endl;
}

// Sliding window with Go-Back-N
int32_t TCPSocket::recv(void* receive_buffer, uint32_t length, sockaddr_in* addr, socklen_t* len) {
    cout << MAG << "[i] " << getFormattedStatus() << " Ready to receive input from " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
    
    // TODO: find out how much should this value be
    const uint32_t RWS = 3 * PAYLOAD_SIZE; // Receiver Window Size
    uint32_t initial_seq_num = segment_handler.getInitialSeqNum();
    uint32_t initial_ack_num = segment_handler.getInitialAckNum();
    uint32_t LFR = initial_seq_num; // Last Frame Received
    uint32_t LAF = initial_seq_num + RWS + PAYLOAD_SIZE; // Largest Acceptable Frame
    uint32_t seq_num_ack = LFR;

    // cout << "Waiting for seq_num " << initial_seq_num << endl;

    map<uint32_t, Segment> buffers;
    auto send_time = high_resolution_clock::now();
    chrono::seconds timeout(10);
    char payload[DATA_OFFSET_MAX_SIZE + BODY_ONLY_SIZE];
    cout << MAG << "[~] " << getFormattedStatus() << " [Established] Waiting for segments to be sent" << COLOR_RESET << endl;
    while (true) {
        int recv_size = recvAny(&payload, DATA_OFFSET_MAX_SIZE + BODY_ONLY_SIZE, addr, len);
        
        // cout << "======================" << endl;
        // cout << "recv_size: " << recv_size << endl;
        // cout << "errno: " << errno << endl;

        Segment recv_segment;
        memcpy(&recv_segment, payload, HEADER_ONLY_SIZE);
        if(isValidChecksum(recv_segment) && extract_flags(recv_segment.flags) == FIN_FLAG) {
            break;
        }
        
        // cout << "not fin" << endl;

        uint32_t options_size = (payload + recv_segment.data_offset*4) - (payload + HEADER_ONLY_SIZE);
        uint32_t payload_size = recv_size - recv_segment.data_offset*4;
        // cout << "options_size: " << options_size << endl;
        // cout << "payload_size: " << payload_size << endl;
        
        if(options_size < 0) {
            uint32_t data_index = calculateSegmentIndex(recv_segment.seq_num, initial_seq_num);
            cout << RED << "[-] " << getFormattedStatus() << " [Established] [Seg=" << data_index << "] [A=" << recv_segment.seq_num << "] Invalid data offset" << COLOR_RESET << endl;
            continue;
        }
        if(payload_size < 0) {
            uint32_t data_index = calculateSegmentIndex(recv_segment.seq_num, initial_seq_num);
            cout << RED << "[-] " << getFormattedStatus() << " [Established] [Seg=" << data_index << "] [A=" << recv_segment.seq_num << "] Invalid payload size" << COLOR_RESET << endl;
            continue;
        }
        recv_segment.options = vector<char>(payload + HEADER_ONLY_SIZE, payload + recv_segment.data_offset*4);
        recv_segment.payload = vector<char>(payload + recv_segment.data_offset*4, payload + recv_size);

        if(!isValidChecksum(recv_segment)) {
            cout << RED << "[i] " << getFormattedStatus() << " [Established] Invalid checksum, received corrupted packet" << COLOR_RESET << endl;
            continue;
        }

        // cout << "checksum valid" << endl;

        // cout << "recv_segment.seq_num: " << recv_segment.seq_num << endl;
        // cout << "LFR: " << LFR << endl;
        // cout << "LAF: " << LAF << endl;
        // cout << "seq_num_ack: " << seq_num_ack << endl;

        if(recv_segment.seq_num < LFR || recv_segment.seq_num >= LAF) {
            if(recv_segment.seq_num >= initial_seq_num) { // meaning it has been acked but the ack is loss so the server resend it.
                // resend the ack
                uint32_t data_index = calculateSegmentIndex(seq_num_ack, initial_seq_num);
                Segment ack_segment = ack(seq_num_ack);
                sendAny(this->connected_ip.c_str(), this->connected_port, &ack_segment, HEADER_ONLY_SIZE);
                cout << BLU << "[+] " << getFormattedStatus() << " [Established] [Seg=" << data_index-1 << "] [A=" << seq_num_ack << "] Resent to " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
                continue;
            }
            continue; // discard
        }
        // cout << "in range" << endl;

        


        buffers[recv_segment.seq_num] = recv_segment;
        uint32_t data_index = calculateSegmentIndex(recv_segment.seq_num, initial_seq_num);
        cout << YEL << "[+] " << getFormattedStatus() << " [Established] [Seg=" << data_index << "] [S=" << recv_segment.seq_num << "] ACKed from " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;

        // cout << "seq_num_ack: " << seq_num_ack << endl;
        // cout << "recv_segment.seq_num: " << recv_segment.seq_num << endl;
        if(recv_segment.seq_num != seq_num_ack) {
            // cout << "buffered" << endl;
            continue; // save it for later
        }

        // cout << "got what we want" << endl;

        uint32_t total_received = 0;
        // iterate buffer from seq_num_ack to latest element of the map/buffer
        for(auto& [seq_num, segment] : buffers) {
            if(!seq_num == seq_num_ack) continue;
            total_received += segment.payload.size();
        }

        // cout << "(prev(buffers.end())->first: " << prev(buffers.end())->first << endl;
        // cout << "prev(buffers.end())->second.payload.size(): " << prev(buffers.end())->second.payload.size() << endl;
        // cout << "LFR: " << LFR << endl;
        // cout << "initial_seq_num: " << initial_seq_num << endl;
        // cout << "initial_acl_num: " << initial_ack_num << endl;
        // cout << "total_received: " << total_received << endl;
        if(!(prev(buffers.end())->first + prev(buffers.end())->second.payload.size() - initial_seq_num == total_received)) {
            // cout << "!(prev(buffers.end())->first + prev(buffers" << endl;
            continue;
        }


        // cout << "sending ack"<< endl;
        // cout << "prev(buffers.end())->first: " << prev(buffers.end())->first << endl;
        // cout << "prev(buffers.end())->second.payload.size(): " << prev(buffers.end())->second.payload.size() << endl;
        seq_num_ack = prev(buffers.end())->first + prev(buffers.end())->second.payload.size();
        LFR = seq_num_ack;
        LAF = LFR + RWS;
        Segment ack_segment = ack(seq_num_ack);
        sendAny(this->connected_ip.c_str(), this->connected_port, &ack_segment, HEADER_ONLY_SIZE);
        
        data_index = calculateSegmentIndex(seq_num_ack, initial_seq_num);
        cout << BLU << "[+] " << getFormattedStatus() << " [Established] [Seg=" << data_index-1 << "] [A=" << seq_num_ack << "] Sent to " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
    }
    if(this->status == TCPStatusEnum::FAILED) {
        cout << RED << "[-] " << getFormattedStatus() << " [Failed]" << COLOR_RESET << endl;
        return -1;
    }
    
    segment_handler.setInitialAckNum(seq_num_ack); // so that the next request will have a new ack_num

    uint32_t current = 0;
    for(auto& [seq_num, segment] : buffers) {
        memcpy(receive_buffer + current, segment.payload.data(), segment.payload.size());
        current += segment.payload.size();
    }

    fin_recv(addr, len);
    return current;
}

void TCPSocket::fin_recv(sockaddr_in* addr, socklen_t* len) {
    this->status = TCPStatusEnum::FIN_WAIT_1;
    cout << YEL << "[i] " << getFormattedStatus() << " [Closing] Received FIN request from " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;

    cout << BLU << "[i] " << getFormattedStatus() << " [Closing] Sending FIN-ACK request to " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
    Segment fin_ack_segment = finAck();
    sendAny(this->connected_ip.c_str(), this->connected_port, &fin_ack_segment, HEADER_ONLY_SIZE);

    Segment ack_segment;
    while(true) {
        int recv_size = recvAny(&ack_segment, HEADER_ONLY_SIZE, addr, len);
        if(!isValidChecksum(ack_segment)) {
            cout << RED << "[i] " << getFormattedStatus() << " [Established] Invalid checksum, received corrupted packet" << COLOR_RESET << endl;
            continue;
        }
        if (recv_size > 0 && extract_flags(ack_segment.flags) == ACK_FLAG) {
            cout << YEL << "[+] " << getFormattedStatus() << " [Closing] Received ACK request from " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
            break;
        } else {
            cout << RED << "[-] " << getFormattedStatus() << " [Closing] Error, retrying" << COLOR_RESET << endl;
        }
    }

    cout << GRN << "[i] " << getFormattedStatus() << " Connection closed successfully" << COLOR_RESET << endl;

}

void TCPSocket::close() {
    ::close(this->socket);
}

string TCPSocket::getFormattedStatus() {
    return "[" + to_string(this->status) + "]";
}
