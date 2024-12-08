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
    timeout.tv_sec = 1;
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
    Segment syn_segment, syn_ack_segment, ack_segment;
    int32_t sync_buffer_size;
    while (true) {
        sync_buffer_size = recvAny(&syn_segment, HEADER_ONLY_SIZE, &addr, &len);
        if(sync_buffer_size >= 0) break; // sync_buffer_size < 0 just means timeout, retry
    }
    
    char* received_ip = inet_ntoa(addr.sin_addr);
    this->status = TCPStatusEnum::SYN_RECEIVED;
    cout << YEL << "[i] " << getFormattedStatus() << " [Handshake] [S=" << syn_segment.seq_num << "] Received SYN request from " << received_ip << ":" << addr.sin_port << endl << COLOR_RESET;

    bool retry = true;
    while (retry) {
        uint32_t initial_seq_num = segment_handler.generateInitialSeqNum();
        syn_ack_segment = synAck(syn_segment.seq_num + 1, initial_seq_num);
        cout << BLU << "[i] " << getFormattedStatus() << " [Handshake] [A=" << syn_ack_segment.ack_num << "] [S=" << syn_ack_segment.seq_num << "] Sending SYN-ACK request to " << received_ip << ":" << addr.sin_port << COLOR_RESET << endl;
        sendAny(received_ip, addr.sin_port, &syn_ack_segment, HEADER_ONLY_SIZE);

        while (true) {
            auto ack_buffer_size = recvAny(&ack_segment, HEADER_ONLY_SIZE, &addr, &len);
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
    this->segment_handler.setInitialSeqNum(syn_ack_segment.seq_num);
    this->segment_handler.setInitialAckNum(ack_segment.ack_num);

    this->connected_ip = string(received_ip);
    this->connected_port = addr.sin_port;
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

        while(true) {
            auto sync_ack_buffer_size = recvAny(&syn_ack_segment, HEADER_ONLY_SIZE, &addr, &len);
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

        cout << BLU << "[+] " << getFormattedStatus() << " [Handshake] [S=" << syn_ack_segment.seq_num << "] [A=" << syn_ack_segment.ack_num << "] Received SYN-ACK request from " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;

        ack_segment = ack(syn_ack_segment.ack_num + 1);
        cout << YEL << "[i] " << getFormattedStatus() << " [Handshake] [A=" << ack_segment.ack_num << "] Sending ACK request to " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
        sendAny(this->connected_ip.c_str(), this->connected_port, &ack_segment, HEADER_ONLY_SIZE);
    }
    this->status = TCPStatusEnum::ESTABLISHED;
    cout << GRN << "[i] " << getFormattedStatus() << " [Established] Connection estabilished with "<< this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;

    this->segment_handler.setInitialAckNum(ack_segment.ack_num);
    this->segment_handler.setInitialSeqNum(syn_ack_segment.seq_num);
}

// Sliding window with Go-Back-N
void TCPSocket::send(const char* ip, int32_t port, void* dataStream, uint32_t dataSize) {
    cout << BLU << "[i] " << getFormattedStatus() << " [i] Sending input to " << ip << ":" << port << COLOR_RESET << endl;
    // for(uint32_t i = 0; i < dataSize; i++) {
    //     cout << dataStream[i];
    // }

    // TODO: find out how much should this value be
    const uint32_t SWS = 4 * PAYLOAD_SIZE; // Sender Window Size
    uint32_t initial_seq_num = segment_handler.getInitialSeqNum();
    uint32_t initial_ack_num = segment_handler.getInitialAckNum();
    uint32_t LAR = initial_seq_num; // Last Acknowledgment Received. -1 because if there's only 1 segment, initial_seq_num = last_seq_num.
    segment_handler.setDataStream(dataStream, dataSize);
    // segment_handler.generateSegments(this->port, port);
    cout << "generate segment" << endl;
    segment_handler.generateSegmentsMap(this->port, port);
    cout << "generate segment done" << endl;
    uint32_t LFS = min(initial_seq_num + SWS, prev(segment_handler.segmentMap.end())->second.seq_num); // Last Frame Sent


    auto send_time = high_resolution_clock::now();  
    chrono::seconds timeout(5);
    while(true) {

        bool anything_sent = false;
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
            cout << "sending: " << segment.payload.size() << endl;
            sendAny(ip, port, payload, segment.data_offset*4 + segment.payload.size());
            delete[] payload;
            uint32_t data_index = ceil((seq_num - initial_seq_num) / PAYLOAD_SIZE);
            cout << BLU << "[i] " << getFormattedStatus() << " [Established] [Seg " << data_index+1 << "] [S=" << segment.seq_num << "] Sent to " << this->connected_ip << ":" << this->connected_port << endl;
        }

        // bool anything_sent = false;
        // for (auto& [seq_num, segment] : segment_handler.segmentMap) {
        //     if (seq_num < LAR) continue;
        //     if (seq_num > LFS) break;
        //     anything_sent = true;
            
        //     // Calculate total size needed for payload
        //     size_t header_size = sizeof(Segment) - sizeof(std::vector<char>) * 2; // Subtract sizes of dynamic vectors
        //     size_t total_size = header_size + segment.options.size() + segment.payload.size();
            
        //     // Allocate memory for full payload
        //     char* payload = new char[total_size];
            
        //     // Copy segment header
        //     memcpy(payload, &segment, header_size);
            
        //     // Copy options
        //     if (!segment.options.empty()) {
        //         memcpy(payload + header_size, segment.options.data(), segment.options.size());
        //     }
            
        //     // Copy payload
        //     if (!segment.payload.empty()) {
        //         memcpy(payload + header_size + segment.options.size(), 
        //             segment.payload.data(), 
        //             segment.payload.size());
        //     }
            
        //     // Debug print
        //     cout << "sending: " << segment.payload.size() << " bytes" << endl;
            
        //     // Send payload
        //     sendAny(ip, port, payload, total_size);
            
        //     // Free dynamically allocated memory
        //     // delete[] payload;
            
        //     // Calculate and print data index
        //     uint32_t data_index = ceil((seq_num - initial_seq_num) / PAYLOAD_SIZE);
        //     cout << BLU << "[i] " << getFormattedStatus() 
        //         << " [Established] [Seg " << data_index+1 
        //         << "] [S=" << segment.seq_num 
        //         << "] Sent to " << this->connected_ip << ":" << this->connected_port << endl;
        // }
        if(!anything_sent) break;

        Segment ack_segment;
        sockaddr_in addr; socklen_t len = sizeof(addr);
        while(true) {
            auto recv_size = recvAny(&ack_segment, HEADER_ONLY_SIZE, &addr, &len);
            cout << "recv_size: " << recv_size << endl;
            if(recv_size < 0) {
                if(high_resolution_clock::now() - send_time > timeout) {
                    this->status = TCPStatusEnum::FAILED;
                    cout << RED << "[i] " << getFormattedStatus() << " [Established] Timeout" << COLOR_RESET << endl;
                    break; // dont forget to change to break
                } else break;  // dont forget to change to continue
            }
            cout << "LFS: " << LFS << endl;
            cout << "LAR: " << LAR << endl;
            cout << "if(!isValidChecksum(ack_segment))" << endl;
            // if(!isValidChecksum(ack_segment)) continue;


            cout << "ack: " << ack_segment.ack_num << endl;
            if(extract_flags(ack_segment.flags) == ACK_FLAG) {
                // while(LFS < SWS - LAR) LFS += PAYLOAD_SIZE;
                LFS = min(prev(segment_handler.segmentMap.end())->second.seq_num, LFS + ack_segment.ack_num - LAR);
                LAR = max(LAR, ack_segment.ack_num);

                uint32_t data_index = ceil((LAR - initial_seq_num) / PAYLOAD_SIZE);
                cout << YEL << "[i] " << getFormattedStatus() << " [Established] [Seg " << data_index+1 << "] [A=" << ack_segment.ack_num << "] Received" << COLOR_RESET << endl;
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

    // segment_handler.segmentMap


    // uint8_t* data = (uint8_t*)(dataStream);
    // uint32_t last_seq_num = segment_handler.segmentBuffer[segment_handler.segmentBuffer.size() - 1].seq_num;
    
    // auto send_time = high_resolution_clock::now();
    // uint32_t data_index = 0; 
    // uint32_t offset = 0;
    // chrono::seconds timeout(5);
    // char payload[sizeof(Segment) + PAYLOAD_SIZE];
    // while (LAR < last_seq_num) {

    //     send_time = high_resolution_clock::now();
    //     while (LFS - LAR < SWS && LAR < last_seq_num) {
    //         // cout << LFS << " - " << LAR << " < " << SWS << " && " << LAR << " < " << last_seq_num << endl;
    //         offset = LFS - initial_seq_num;
    //         uint32_t payload_size = min(PAYLOAD_SIZE, dataSize - offset);
    //         if(payload_size == 0) break;

    //         data_index = offset / PAYLOAD_SIZE;
    //         if (data_index >= segment_handler.segmentBuffer.size()) break;
            
    //         cout << "LFS: " << LFS << endl;
    //         cout << "LAR: " << LAR << endl;
    //         cout << "SWS: " << SWS << endl;
    //         cout << "initial_seq_num: " << initial_seq_num << endl;
    //         cout << "last_seq_num: " << last_seq_num << endl;
    //         cout << "data_index: " << data_index << endl;
    //         cout << "payload_size: " << payload_size << endl;

    //         Segment& send_segment = segment_handler.segmentBuffer[data_index];
    //         memcpy(payload, &send_segment, sizeof(Segment));
    //         if (offset + payload_size <= dataSize) {
    //             memcpy(payload + sizeof(Segment), data + offset, payload_size);
    //         }
    //         // *((Segment*)payload) = send_segment;
    //         // *((uint8_t*)payload + sizeof(Segment)) = data[offset];



    //         for(auto s : segment_handler.segmentBuffer) {
    //             cout << s.seq_num << " ";
    //         }
    //         cout << endl;
    //         cout << "content:" << endl;
    //         for(uint32_t i = 0; i < payload_size; i++) {
    //             cout << data[i];
    //         }
    //         cout << endl;
    //         sendAny(ip, port, payload, sizeof(payload));
    //         cout << BLU << "[i] " << getFormattedStatus() << " [Established] [Seg " << data_index+1 << "] [S=" << send_segment.seq_num << "] Sent to " << this->connected_ip << ":" << this->connected_port << endl;
    //         LFS += payload_size;

    //     }

    //     sockaddr_in addr; socklen_t len = sizeof(addr);
    //     Segment ack_packet;
    //     while(true) {
    //         auto recv_size = recvAny(&ack_packet, sizeof(ack_packet), &addr, &len);
    //         if (recv_size < 0) {
    //             if(high_resolution_clock::now() - timeout > send_time) {
    //                 cout << RED << "[i] " << getFormattedStatus() << " [Established] Timeout, retrying..." << COLOR_RESET << endl;
    //                 // Reset window state
    //                 LFS = LAR;
    //                 data_index = (LAR - initial_seq_num) / PAYLOAD_SIZE;
    //                 offset = LAR - initial_seq_num;
    //                 // Reset timer
    //                 send_time = high_resolution_clock::now();
    //                 break;
    //             }
    //             continue;
    //         }

    //         uint32_t data_index = (ack_packet.ack_num - initial_ack_num) / PAYLOAD_SIZE;
    //         // cout << initial_ack_num << " " << data_index << endl;
    //         // cout << ack_packet.ack_num << " == " << segment_handler.segmentBuffer[data_index].ack_num << endl;

    //         if (extract_flags(ack_packet.flags) == ACK_FLAG && ack_packet.ack_num == segment_handler.segmentBuffer[data_index].ack_num) {
    //             LAR = max(LAR, segment_handler.segmentBuffer[data_index].seq_num);
    //             cout << YEL << "[i] " << getFormattedStatus() << " [Established] [Seg " << data_index+1 << "] [A=" << ack_packet.ack_num << "] Received" << COLOR_RESET << endl;
    //         } else {
    //             if(high_resolution_clock::now() - timeout > send_time) {
    //                 cout << RED << "[i] " << getFormattedStatus() << " [Established] Timeout, retrying..." << COLOR_RESET << endl;
    //                 LFS = LAR;
    //                 break;
    //             } else continue;
    //         }
    //     }

    // }
    
}

void TCPSocket::fin_send(const char* ip, int32_t port) {
    
    Segment fin_segment = fin();
    sendAny(ip, port, &fin_segment, HEADER_ONLY_SIZE);
    cout << BLU << "[i] " << getFormattedStatus() << " [Closing] Sending FIN request to " << ip << ":" << port << COLOR_RESET << endl;

    Segment fin_ack_segment; sockaddr_in addr; socklen_t len = sizeof(addr);
    while (true) {
        auto recv_size = recvAny(&fin_ack_segment, HEADER_ONLY_SIZE, &addr, &len);
        if (recv_size > 0 && extract_flags(fin_ack_segment.flags) == FIN_ACK_FLAG) {
            cout << YEL << "[+] " << getFormattedStatus() << " [Closing] Received FIN-ACK request from " << ip << ":" << port << COLOR_RESET << endl;
            break;
        }
    }

    Segment ack_segment = ack(fin_ack_segment.ack_num);
    sendAny(ip, port, &ack_segment, HEADER_ONLY_SIZE);
    cout << BLU << "[i] " << getFormattedStatus() << " [Closing] Sending ACK request to " << ip << ":" << port << COLOR_RESET << endl;

    cout << GRN << "[i] " << getFormattedStatus() << " Connection closed successfully" << COLOR_RESET << endl;
}

// Sliding window with Go-Back-N
int32_t TCPSocket::recv(void* receive_buffer, uint32_t length, sockaddr_in* addr, socklen_t* len) {
    cout << YEL << "[i] " << getFormattedStatus() << " [i] Ready to receive input from " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
    

    uint32_t received_buffer_size = 0;

    // TODO: find out how much should this value be
    const uint32_t RWS = 3 * PAYLOAD_SIZE; // Receiver Window Size
    uint32_t initial_seq_num = segment_handler.getInitialSeqNum();
    uint32_t initial_ack_num = segment_handler.getInitialAckNum();
    uint32_t LFR = initial_seq_num; // Last Frame Received
    uint32_t LAF = initial_seq_num + RWS + PAYLOAD_SIZE; // Largest Acceptable Frame
    uint32_t seq_num_ack = LFR;

    cout << "Waiting for seq_num " << initial_seq_num << endl;

    map<uint32_t, Segment> buffers;
    auto send_time = high_resolution_clock::now();
    chrono::seconds timeout(10);
    char payload[DATA_OFFSET_MAX_SIZE + BODY_ONLY_SIZE];
    while (true) {
        int recv_size = recvAny(&payload, DATA_OFFSET_MAX_SIZE + BODY_ONLY_SIZE, addr, len);
        cout << "recv_size: " << recv_size << endl;
        // cout << "errno: " << errno << endl;
        if(recv_size < 0) {
            if(high_resolution_clock::now() - send_time > timeout) {
                this->status = TCPStatusEnum::FAILED;
                continue; // dont forget to change to break
            } else continue;
        }
        // for(int i = 20; i < recv_size; i++) {
        //     cout << payload[i];
        // }
        // cout << endl;

        Segment recv_segment;
        memcpy(&recv_segment, payload, HEADER_ONLY_SIZE);
        if(extract_flags(recv_segment.flags) == FIN_FLAG) {
            break;
        }
        cout << "not fin" << endl;

        cout << +recv_segment.data_offset*4 << endl;
        cout << +HEADER_ONLY_SIZE << endl;
        cout << "options size: " << (payload + recv_segment.data_offset*4) - (payload + HEADER_ONLY_SIZE)  << endl;
        cout << "payload size: " << recv_size - recv_segment.data_offset*4 << endl;
        recv_segment.options = vector<char>(payload + HEADER_ONLY_SIZE, payload + recv_segment.data_offset*4);
        recv_segment.payload = vector<char>(payload + recv_segment.data_offset*4, payload + recv_size);
        if(!isValidChecksum(recv_segment)) {
            // cout<< "checksum:"<<recv_segment.checksum << endl;
            // cout<< "checksum dfdfd:"<<calculateSum(recv_segment) << endl;
            uint32_t data_index = ceil((LFR - initial_seq_num) / PAYLOAD_SIZE);
            cout << RED << "[+] " << getFormattedStatus() << " [Established] [Seg=" << data_index+1 << "] [A=" << recv_segment.seq_num << "] Receive Corrupted" << COLOR_RESET << endl;
            // must remove the element
            continue; // discard
        }
        cout << "checksum valid" << endl;

        cout << "recv_segment.seq_num: " << recv_segment.seq_num << endl;
        cout << "LFR: " << LFR << endl;
        cout << "LAF: " << LAF << endl;


        if(recv_segment.seq_num < LFR || recv_segment.seq_num >= LAF) {
            if(recv_segment.seq_num >= initial_seq_num) { // meaning it has been acked but the ack is loss so the server resend it.
                // resend the ack
                uint32_t data_index = ceil((LFR - initial_seq_num) / PAYLOAD_SIZE);
                Segment ack_segment = ack(seq_num_ack);
                sendAny(this->connected_ip.c_str(), this->connected_port, &ack_segment, 20);
                cout << BLU << "[+] " << getFormattedStatus() << " [Established] [Seg=" << data_index+1 << "] [A=" << seq_num_ack << "] Resent" << COLOR_RESET << endl;
                continue;
            }
            continue; // discard
        }
        cout << "in range" << endl;

        


        buffers[recv_segment.seq_num] = recv_segment;
        uint32_t data_index = ceil((LFR - initial_seq_num) / PAYLOAD_SIZE);
        cout << YEL << "[+] " << getFormattedStatus() << " [Established] [Seg=" << data_index+1 << "] [S=" << recv_segment.seq_num << "] Ack" << COLOR_RESET << endl;

        cout << "seq_num_ack: " << seq_num_ack << endl;
        cout << "recv_segment.seq_num: " << recv_segment.seq_num << endl;
        if(recv_segment.seq_num != seq_num_ack) {
            cout << "buffered" << endl;
            continue; // save it for later
        }

        cout << "got what we want" << endl;

        uint32_t total_received = 0;
        // iterate buffer from seq_num_ack to latest element of the map/buffer
        for(auto& [seq_num, segment] : buffers) {
            if(!seq_num == seq_num_ack) continue;
            total_received += segment.payload.size();
        }

        cout << "(prev(buffers.end())->first: " << prev(buffers.end())->first << endl;
        cout << "prev(buffers.end())->second.payload.size(): " << prev(buffers.end())->second.payload.size() << endl;
        cout << "LFR: " << LFR << endl;
        cout << "initial_seq_num: " << initial_seq_num << endl;
        cout << "initial_acl_num: " << initial_ack_num << endl;
        cout << "total_received: " << total_received << endl;
        if(!(prev(buffers.end())->first + prev(buffers.end())->second.payload.size() - initial_seq_num == total_received)) {
            cout << "!(prev(buffers.end())->first + prev(buffers" << endl;
            continue;
        }


        cout << "sending ack"<< endl;
        cout << "prev(buffers.end())->first: " << prev(buffers.end())->first << endl;
        cout << "prev(buffers.end())->second.payload.size(): " << prev(buffers.end())->second.payload.size() << endl;
        seq_num_ack = prev(buffers.end())->first + prev(buffers.end())->second.payload.size();
        LFR = seq_num_ack;
        Segment ack_segment = ack(seq_num_ack);
        sendAny(this->connected_ip.c_str(), this->connected_port, &ack_segment, 20);
        
        // uint32_t data_index = ceil((LFR - initial_seq_num) / PAYLOAD_SIZE);
        cout << BLU << "[+] " << getFormattedStatus() << " [Established] [Seg=" << data_index+1 << "] [S=" << seq_num_ack << "] Sent" << COLOR_RESET << endl;
    }
    if(this->status == TCPStatusEnum::FAILED) {
        cout << RED << "[-] " << getFormattedStatus() << " [Failed]" << COLOR_RESET << endl;
        return -1;
    }
    
    segment_handler.setInitialAckNum(seq_num_ack); // so that the next request will have a new ack_num

    uint32_t current = 0;
    for(auto& [seq_num, segment] : buffers) {
        memcpy(receive_buffer, segment.payload.data()+current, segment.payload.size());
        current += segment.payload.size();
    }

    fin_recv(addr, len);
    return current;


    // vector<string> buffers;
    // vector<pair<Segment, vector<char>>> buffers;
    // char payload[sizeof(Segment) + PAYLOAD_SIZE];
    // bool fin_received = false;
    // while (!fin_received) { // LAF - LFR <= RWS
    //     cout << "\nWindow State:" << endl;
    //     cout << "LFR: " << LFR << " (Last Frame Received)" << endl;
    //     cout << "LAF: " << LAF << " (Largest Acceptable Frame)" << endl;
    //     cout << "seq_num_ack: " << seq_num_ack << endl;
    //     cout << "-------------------" << endl;
    //     memset(addr, 0, sizeof(*addr));
    //     *len = sizeof(*addr);
    //     int recv_size = recvAny(&payload, sizeof(payload), addr, len);
    //     if(recv_size < 0) {
    //         if(errno == EAGAIN || errno == EWOULDBLOCK) {
    //             // Timeout case - just continue
    //             continue;
    //         }
    //         cout << "Receizve failed with error: " << strerror(errno) << endl;
    //         cout << "Current port: " << this->port << endl;
    //         cout << "Expected server: " << this->connected_ip << ":" << this->connected_port << endl;
    //         continue;
    //     }
    //     cout << recv_size << endl;
    //     if(recv_size < 0) continue; //  no need timeout message

    //     // cout << "content:" << endl;
    //     // for(int i = 0; i < recv_size; i++) {
    //     //     cout << payload[i+sizeof(Segment)];
    //     // }
    //     // cout << *(payload+sizeof(Segment)) << endl;

    //     // Segment recv_segment;
    //     // memccpy(&recv_segment, payload, 0, sizeof(Segment));
    //     Segment recv_segment = *((Segment*)payload);
    //     vector<char> content(payload + sizeof(Segment), payload + recv_size);
    //     cout << "Received seq_num " << recv_segment.seq_num << endl;
        
    //     // for(int i = 0; i < sizeof(Segment); i++) {
    //     //     cout << (int)((uint8_t*)payload)[i] << " ";
    //     // }
    //     // cout << endl;
    //     // cout << ((Segment*)payload)->flags.cwr << " " << ((Segment*)payload)->flags.ece << " " << ((Segment*)payload)->flags.urg << " " << ((Segment*)payload)->flags.ack << " " << ((Segment*)payload)->flags.psh << " " << ((Segment*)payload)->flags.pst << " " << ((Segment*)payload)->flags.syn << " " << ((Segment*)payload)->flags.fin << " " << endl;
        
    //     if(extract_flags(recv_segment.flags) == FIN_FLAG) {
    //         fin_received = true;
    //         break;
    //     }

    //     if(recv_segment.seq_num > LAF || recv_segment.seq_num < LFR) { // case outside window
    //         cout << RED << "[-] " << getFormattedStatus() << " [Established] [S=" << recv_segment.seq_num << "] Discarded" << COLOR_RESET << endl;
    //         continue;
    //     }

    //     if(recv_segment.seq_num == LFR) {
           
  
    //         memcpy((uint8_t*)receive_buffer + received_buffer_size, content.data(), content.size());
            
    //         // Update window
    //         LFR += content.size();
    //         LAF = LFR + RWS;
    //         seq_num_ack = LFR;
            
    //         // Send ACK
    //         Segment ack_segment = ack(initial_ack_num + received_buffer_size);
    //         received_buffer_size += content.size();
    //         sendAny(this->connected_ip.c_str(), this->connected_port, &ack_segment, sizeof(Segment));
            
    //         // Process any buffered segments that are now in order
    //         while(!buffers.empty()) {
    //             sort(buffers.begin(), buffers.end(), [](pair<Segment, vector<char>>& a, pair<Segment, vector<char>>& b) { 
    //             return a.first.seq_num < b.first.seq_num;
    //         });
                
    //             if(buffers[0].first.seq_num == LFR) {
    //                 auto& next_segment = buffers[0];
    //                 memcpy((uint8_t*)receive_buffer + received_buffer_size, 
    //                        next_segment.second.data(), next_segment.second.size());
                    
    //                 LFR += next_segment.second.size();
    //                 LAF = LFR + RWS;
    //                 seq_num_ack = LFR;
                    
    //                 Segment ack_segment = ack(initial_ack_num + received_buffer_size);
    //                 received_buffer_size += next_segment.second.size();
    //                 sendAny(this->connected_ip.c_str(), this->connected_port, 
    //                        &ack_segment, sizeof(Segment));
                    
    //                 buffers.erase(buffers.begin());
    //             } else {
    //                 break;
    //             }
    //         }
    //     } else {
           
    //         buffers.push_back(make_pair(recv_segment, content));
    //     }

    //     uint32_t data_index = (recv_segment.seq_num - initial_seq_num) / PAYLOAD_SIZE;
    //     cout << BLU << "[+] " << getFormattedStatus() << " [Established] [Seg " << data_index+1 << "] [S=" << recv_segment.seq_num << "] Received" << COLOR_RESET << endl;

    //     buffers.push_back(make_pair(recv_segment, content));
    //     cout << "payload:" << endl;
    //     for(int i = 0; i < min(recv_size, 10); i++) {
    //         cout << payload[i];
    //     }
    //     cout << "..." << endl;
    //     cout << "size: " << recv_size << endl;
    //     cout << "buffers.size(): " << buffers.size() << endl;

    //     if(buffers.size() == RWS/PAYLOAD_SIZE || fin_received) {
    //         sort(buffers.begin(), buffers.end(), [](pair<Segment, vector<char>>& a, pair<Segment, vector<char>>& b) { // sort by seq_num ascending
    //             return a.first.seq_num < b.first.seq_num;
    //         });

    //         // cout << "buffers:" << endl;
    //         // cout << (Segment*)buffers[0].c_str() << (Segment*)buffers[1].c_str() << (Segment*)buffers[2].c_str() << endl;
    //         // cout << buffers[0] << buffers[1] << buffers[2] << endl;
    //         // cout << buffers[0 + sizeof(Segment)] << buffers[1 + sizeof(Segment)] << buffers[2 + sizeof(Segment)] << endl;
    //         for (int i = 0; i < 3; i++) {
    //             cout << "Buffer[" << i << "] Seq=" << buffers[i].first.seq_num 
    //                 << " (LFR=" << LFR << ")" << endl;
    //         }
    //         cout << "Window State ----------------------" << endl;
            
    //         if(buffers[0].first.seq_num == LFR) {
    //             seq_num_ack = buffers[buffers.size() - 1].first.seq_num;
    //             LFR = seq_num_ack;
    //             LAF = LFR + RWS;
    //                 cout << "\nWindow Updated:" << endl;
    // cout << "New LFR: " << LFR << endl;
    // cout << "New LAF: " << LAF << endl;
    // cout << "New seq_num_ack: " << seq_num_ack << endl;
    // cout << "-------------------" << endl;

    //              for (const auto& segment : buffers) {
    //                 // memcpy((uint8_t*)receive_buffer + received_buffer_size, buffers[i].c_str() + sizeof(Segment), recv_size - sizeof(Segment));
    //                 // *((uint8_t*)receive_buffer + received_buffer_size) = *((uint8_t*)buffers[i].c_str() + sizeof(Segment));
    //                 // memcpy((uint8_t*)receive_buffer + received_buffer_size, (uint8_t*)buffers[i].c_str() + sizeof(Segment), recv_size - sizeof(Segment));
    //                 // cout << "copying" << endl;
    //                 // for(uint32_t i = 0; i < 100; i++) {
    //                 //     cout << buffers[i + sizeof(Segment)] << endl;
    //                 // }
    //                 memcpy((uint8_t*)receive_buffer + received_buffer_size, segment.second.data(), segment.second.size());

    //                 Segment ack_segment = ack(initial_ack_num + received_buffer_size);
    //                 received_buffer_size += segment.second.size();
    //                 uint32_t data_index = (ack_segment.ack_num - initial_ack_num) / PAYLOAD_SIZE;
    //                 cout << YEL << "[i] " << getFormattedStatus() << " [Established] [Seg " << data_index+1 << "] [A=" << ack_segment.ack_num << "] Sending ACK request to " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
    //                 sendAny(this->connected_ip.c_str(), this->connected_port, &ack_segment, sizeof(Segment));
    //             }
    //         }
    //         buffers.clear();
    //     }
    // }

    // cout << "final result" << endl;
    // for(uint32_t i = 0; i < received_buffer_size; i++) {
    //     cout << ((char*)receive_buffer)[i];
    // }
    // cout << endl;

    // segment_handler.setInitialAckNum(seq_num_ack); // so that the next request will have a new ack_num
    // fin_recv(addr, len);

    return received_buffer_size;
}

void TCPSocket::fin_recv(sockaddr_in* addr, socklen_t* len) {
    this->status = TCPStatusEnum::FIN_WAIT_1;
    cout << YEL << "[i] " << getFormattedStatus() << " [Closing] Received FIN request from " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;

    cout << YEL << "[i] " << getFormattedStatus() << " [Closing] Sending FIN-ACK request to " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
    Segment fin_ack_segment = finAck();
    sendAny(this->connected_ip.c_str(), this->connected_port, &fin_ack_segment, HEADER_ONLY_SIZE);

    Segment ack_segment;
    while(true) {
        int recv_size = recvAny(&ack_segment, HEADER_ONLY_SIZE, addr, len);
        if (recv_size > 0 && extract_flags(ack_segment.flags) == ACK_FLAG) {
            cout << GRN << "[+] " << getFormattedStatus() << " [Closing] Received ACK request from " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
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
