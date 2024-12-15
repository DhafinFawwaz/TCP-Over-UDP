#include <socket.hpp>
#include <unistd.h> 
#include <ansi_code.hpp>
#include <iostream>
#include <string.h> // just for memcpy
#include <chrono>
#include <unordered_map>
#include <deque>
#include <algorithm>
#include <map>
#include <tuple>
#include <utility> // for pair
#include <fcntl.h>
#include <thread>
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

    int broadcastEnable = 1;
    if (setsockopt(this->socket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        perror("Failed to enable broadcast");
    }

    sockaddr_in addr; 
    addr = {0};
       
    addr.sin_family = AF_INET;
    // addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port); 

    if(bind(this->socket, (sockaddr*)&addr, sizeof(addr)) < 0) 
    { 
        perror("[i] bind failed"); 
        exit(EXIT_FAILURE); 
    }

    // cout << "[i] [BIND] Socket binded to " << ip << ":" << port << endl;

    int flags = fcntl(this->socket, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl get failed");
        exit(EXIT_FAILURE); 
    }
    if (fcntl(this->socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl set failed");
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

// return size of received data, -1 if error
ssize_t TCPSocket::recvAny(void* buffer, uint32_t length, sockaddr_in* addr, socklen_t* len) {
    ssize_t size = recvfrom(this->socket, buffer, length, MSG_WAITALL, (sockaddr*) addr, len); 
    // if (size < 0) return -1;
    // cout << "size:" << size << endl;
    // cout << "content received:" << endl;
    // for(int i = 0; i < 20; i++) {
    //     cout << ((char*)buffer)[i];
    // }
    // cout << endl;

    return size;
}
void TCPSocket::sendAny(const char* ip, int32_t port, void* dataStream, uint32_t dataSize) {
    // cout << "sending to " << ip << ":" << port << endl;
    // cout << "content to send:" << endl;
    // for(int i = 0; i < 20; i++) {
    //     cout << ((char*)dataStream)[i];
    // }
    // cout << endl;
    
    sockaddr_in broadcastAddr = {0};
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(port);
    broadcastAddr.sin_addr.s_addr = inet_addr(ip);

    if (sendto(this->socket, dataStream, dataSize, 0, (sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) < 0) {
        perror("Failed to send broadcast message");
    }    
}

void TCPSocket::sendAddr(sockaddr_in& addr, void* dataStream, uint32_t dataSize) {
    // cout << "sending to:" << endl;
    // cout << inet_ntoa(addr.sin_addr) << endl;
    // cout << ntohs(addr.sin_port) << endl;

    if (sendto(this->socket, dataStream, dataSize, 0, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Failed to send broadcast message");
    }    
}

string TCPSocket::toHostPort(sockaddr_in addr) {
    return string(inet_ntoa(addr.sin_addr)) + ":" + to_string(ntohs(addr.sin_port));
}


void TCPSocket::recvHeaderLoop() {
    char buffer[DATA_OFFSET_MAX_SIZE + BODY_ONLY_SIZE];
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    while(true) {
        Segment segment;

        ssize_t size = recvAny(&segment, HEADER_ONLY_SIZE, &addr, &len);
        if(size < 0) continue;

        // else {
        //     cout << "size: " << size << endl;
        //     cout << "segment.flags: " << +extract_flags(segment.flags) << endl;
        // }

        if(!isValidChecksum(segment)) {
            cout << RED << "[i] " << getFormattedStatus() << " Invalid checksum, received corrupted packet" << COLOR_RESET << endl;
            continue;
        }

        if(extract_flags(segment.flags) == FIN_ACK_FLAG) {
            // cout << "FIN_ACK received" << endl;
            for(auto& c : this->connection_map) {
                if(c.second.addr.sin_addr.s_addr == addr.sin_addr.s_addr && c.second.addr.sin_port == addr.sin_port) {
                    ConnectionInfo& ci = c.second;
                    // cout << ci.status << endl;
            
                    if(ci.status == TCPStatusEnum::ESTABLISHED || ci.status == TCPStatusEnum::FIN_WAIT_1) {
                        this->received_buffer_queue[addr].push(segment);
                        break;
                    }

                    string hostPort = ci.get_host_port();
                    cout << YEL << "[+] " << ci.getFormattedStatus() << " Received FIN-ACK request again from " << hostPort << ". It's possible that the ACK packet is lost which makes the receiver resends it." << COLOR_RESET << endl;
                    handle_fin_ack(ci, segment.ack_num, hostPort);
                    break;
                }
            }
            continue;
        }

        this->received_buffer_queue[addr].push(segment);
    }
}



// Handshake
void TCPSocket::listen() {
    this->status = TCPStatusEnum::LISTEN;
    cout << BLU << "[i] [LISTEN] Listening to the broadcast port for clients as " << this->ip << ":" << this->port << COLOR_RESET << endl;
    
    // start thread to queue incoming handshake so it wont interfere when sending packets
    this->recv_threads.emplace_back([this]() { 
        this->recvHeaderLoop();
    });
}

bool TCPSocket::is_connected(sockaddr_in addr) {
    for(auto& [key, value] : this->connection_map) {
        if(value.addr.sin_addr.s_addr == addr.sin_addr.s_addr && value.addr.sin_port == addr.sin_port) return true;
    }
    return false;
}

bool TCPSocket::pop_in_queue_not_connected_with_flag(Segment& segment, sockaddr_in& clientAddr, uint8_t flag){
    for(auto& [key, queue] : this->received_buffer_queue) {
        if(queue.empty()) continue;
        if(is_connected(key)) continue;

        Segment& value = queue.front();
        // cout << "size: " << queue.size() << endl;
        // cout << "flag: " << +extract_flags(value.flags) << endl;
        this->received_buffer_queue[key].pop();
        if(extract_flags(value.flags) == flag) {
            segment = value;
            clientAddr = key;
            return true;
        }
    }
    return false;
}

bool TCPSocket::pop_in_queue_not_connected_with_condition(Segment& segment, sockaddr_in& clientAddr, function<bool(Segment, sockaddr_in)> condition){
    for(auto& [key, queue] : this->received_buffer_queue) {
        if(queue.empty()) continue;
        if(is_connected(key)) continue;

        Segment& value = queue.front();
        this->received_buffer_queue[key].pop();
        if(condition(value, key)) {
            segment = value;
            clientAddr = key;
            return true;
        }
    }
    return false;
}

string TCPSocket::create_retry_message(uint32_t retry_count) {
    if(retry_count == 0) return "";
    return " (Retry " + to_string(retry_count) + ")";
}

// handshake
int TCPSocket::accept(sockaddr_in* addr, socklen_t* len) {
    while(true) {

        // find any in map that is a syn request
        Segment syn_segment;
        sockaddr_in clientAddr;
        if(!pop_in_queue_not_connected_with_flag(syn_segment, clientAddr, SYN_FLAG)) continue;

        int client_socket_to_handshake = 0; if(!this->connection_map.empty()) client_socket_to_handshake = prev(this->connection_map.end())->first + 1;
        // this->connection_map[client_socket_to_handshake] = ConnectionInfo(clientAddr);
        // ConnectionInfo& ci = this->connection_map[client_socket_to_handshake];
        ConnectionInfo ci(clientAddr);
        ci.status = TCPStatusEnum::SYN_RECEIVED;

        string hostPort = toHostPort(clientAddr);
        cout << YEL << "[i] " << ci.getFormattedStatus() << " [S=" << syn_segment.seq_num << "] Received SYN request from " << hostPort << endl << COLOR_RESET;

        uint8_t retry_count_syn = 0;
        uint8_t max_retry_count_syn = 4;

        Segment syn_ack_segment;
        Segment ack_segment;
        while (true) {

            uint32_t initial_seq_num = segment_handler.generateInitialSeqNum();
            syn_ack_segment = synAck(syn_segment.seq_num + 1, initial_seq_num);
            cout << BLU << "[i] " << ci.getFormattedStatus() << " [A=" << syn_ack_segment.ack_num << "] [S=" << syn_ack_segment.seq_num << "] Sending SYN-ACK request to " << hostPort << create_retry_message(retry_count_syn) << COLOR_RESET << endl;
            ci.status = TCPStatusEnum::SYN_RECEIVED;

            sendAddr(clientAddr, &syn_ack_segment, HEADER_ONLY_SIZE);
            // cout << "clientAddr.sin_addr: " << inet_ntoa(clientAddr.sin_addr) << endl;
            // cout << "clientAddr.sin_port: " << ntohs(clientAddr.sin_port) << endl;
            // sendAny(inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), &syn_ack_segment, HEADER_ONLY_SIZE);

            auto send_time_ack = high_resolution_clock::now();
            chrono::seconds timeout_ack(1); // time limit to receive ack

            uint32_t ack_num_plus_1 = syn_ack_segment.ack_num + 1;
            while (true) {
                if(!pop_in_queue_not_connected_with_condition(ack_segment, clientAddr, 
                    [ack_num_plus_1](Segment segment, sockaddr_in addr) {
                        return extract_flags(segment.flags) == ACK_FLAG && segment.ack_num == ack_num_plus_1;
                    }
                )) {
                    if(high_resolution_clock::now() - send_time_ack < timeout_ack) continue; else {
                        this->status = TCPStatusEnum::LISTEN;
                        cout << RED << "[i] " << ci.getFormattedStatus() << " [A=" << ack_num_plus_1 << "]" << " Timeout waiting for ACK from " << hostPort << COLOR_RESET << endl;
                        break;
                    }
                }

                cout << YEL << "[i] " << ci.getFormattedStatus() << " [A=" << ack_segment.ack_num << "] Received ACK request from " << hostPort << COLOR_RESET << endl;
                ci.status = TCPStatusEnum::ESTABLISHED;
                cout << GRN << "[i] " << ci.getFormattedStatus() << " Connection estabilished with "<< hostPort << COLOR_RESET << endl;
                ci.segment_handler.setInitialSeqNum(syn_ack_segment.seq_num);
                ci.segment_handler.setInitialAckNum(ack_segment.ack_num);

                // this->segment_handler.setInitialSeqNum(syn_ack_segment.seq_num);
                // this->segment_handler.setInitialAckNum(ack_segment.ack_num);
                this->connection_map[client_socket_to_handshake] = ci;
                return client_socket_to_handshake;
            }

            retry_count_syn++;
            if(retry_count_syn >= max_retry_count_syn) {
                this->status = TCPStatusEnum::LISTEN;
                cout << RED << "[i] " << ci.getFormattedStatus() << " Retries exceeded limit. Timeout" << COLOR_RESET << endl;
                this->connection_map.erase(client_socket_to_handshake);
                break;
            }
        }
    }
}



uint32_t TCPSocket::calculateSegmentIndex(uint32_t seq_num, uint32_t initial_seq_num) {
    // return ceil((seq_num - initial_seq_num + 1) / static_cast<double>(PAYLOAD_SIZE)) - 1;
    return (seq_num - initial_seq_num + PAYLOAD_SIZE - 1) / PAYLOAD_SIZE + 1;
}

// Handshake
void TCPSocket::connect(string& server_ip, int32_t server_port) {
    this->status = TCPStatusEnum::LISTEN;

    // maybe start thread that will resend ack if it receives syn ack and the connection is estabilished
    // or just send it while sliding window is happening

    this->connected_ip = server_ip;
    this->connected_port = server_port;
    string hostPort = server_ip + ":" + to_string(server_port);

    cout << YEL << "[+] " << getFormattedStatus() << " Trying to contact the sender at " << hostPort << COLOR_RESET << endl;

    sockaddr_in addr; socklen_t len = sizeof(addr);
    Segment syn_segment, syn_ack_segment, ack_segment;

    uint32_t retry_count = 0;
    uint32_t max_retry_count = 4;
    while(true) {
        uint32_t initial_seq_num = segment_handler.generateInitialSeqNum();
        this->status = TCPStatusEnum::SYN_SENT;
        cout << BLU << "[i] " << getFormattedStatus() << " [S=" << initial_seq_num << "] Sending SYN request to " << hostPort << create_retry_message(retry_count) << COLOR_RESET << endl;
        syn_segment = syn(initial_seq_num);
        sendAny(server_ip.c_str(), server_port, &syn_segment, HEADER_ONLY_SIZE);

        auto send_time_syn_ack = high_resolution_clock::now();
        chrono::seconds timeout_syn_ack(1); // time limit to receive syn ack
        while(true) {
            auto sync_ack_buffer_size = recvAny(&syn_ack_segment, HEADER_ONLY_SIZE, &addr, &len);
            string hostPort = toHostPort(addr);
            if(sync_ack_buffer_size < 0) {
                if(high_resolution_clock::now() - send_time_syn_ack < timeout_syn_ack) continue; else {
                    cout << RED << "[i] " << getFormattedStatus() << " Timeout waiting for SYN-ACK from " << hostPort << COLOR_RESET << endl;
                    break;
                }
            }

            // cout << "syn_ack_segment.seq_num: " << syn_ack_segment.seq_num << endl;
            // cout << "syn_segment.seq_num + 1: " << syn_segment.seq_num + 1 << endl;
            // cout << "extract_flags(syn_ack_segment.flags) " << extract_flags(syn_ack_segment.flags) << endl;
             if(extract_flags(syn_ack_segment.flags) == SYN_ACK_FLAG && syn_ack_segment.seq_num == syn_segment.seq_num + 1) {
                cout << YEL << "[+] " << getFormattedStatus() << " [S=" << syn_ack_segment.seq_num << "] [A=" << syn_ack_segment.ack_num << "] Received SYN-ACK request from " << hostPort << COLOR_RESET << endl;

                ack_segment = ack(syn_ack_segment.ack_num + 1);
                cout << BLU << "[i] " << getFormattedStatus() << " [A=" << ack_segment.ack_num << "] Sending ACK request to " << hostPort << COLOR_RESET << endl;
                sendAny(this->connected_ip.c_str(), this->connected_port, &ack_segment, HEADER_ONLY_SIZE);

                
                this->status = TCPStatusEnum::ESTABLISHED;
                cout << GRN << "[i] " << getFormattedStatus() << " Connection estabilished with "<< hostPort << COLOR_RESET << endl;

                this->segment_handler.setInitialAckNum(ack_segment.ack_num);
                this->segment_handler.setInitialSeqNum(syn_ack_segment.seq_num);

                int client_socket_to_handshake = 0; if(!this->connection_map.empty()) client_socket_to_handshake = prev(this->connection_map.end())->first + 1;

                ConnectionInfo ci(addr);
                this->connection_map[client_socket_to_handshake] = ci;
                // this->connected_ip = ci.get_ip();
                // this->connected_port = ci.get_port();
                return;
             }
        }
        retry_count++;
        if(retry_count >= max_retry_count) {
            cout << RED << "[i] " << getFormattedStatus() << " Retries exceeded limit. Aborting" << COLOR_RESET << endl;
            break;
        }
    }
}

// Sliding window with Go-Back-N
void TCPSocket::send(int client_socket, void* dataStream, uint32_t dataSize) {
    ConnectionInfo ci = connection_map[client_socket];
    const char* ip = ci.get_ip().c_str();
    int32_t port = ci.get_port();
    string hostPort = ci.get_host_port();

    if(ci.status != TCPStatusEnum::ESTABLISHED) {
        cout << RED << "[-] " << ci.getFormattedStatus() << " Connection not established" << COLOR_RESET << endl;
        return;
    }
    cout << MAG << "[i] " << ci.getFormattedStatus() << " Sending input to " << ip << ":" << port << COLOR_RESET << endl;
    // for(uint32_t i = 0; i < dataSize; i++) {
    //     cout << dataStream[i];
    // }

    SegmentHandler& segment_handler = ci.segment_handler;

    // TODO: find out how much should this value be
    const uint32_t SWS = (4 - 1) * PAYLOAD_SIZE; // Sender Window Size
    uint32_t initial_seq_num = segment_handler.getInitialSeqNum();
    uint32_t initial_ack_num = segment_handler.getInitialAckNum();
    uint32_t LAR = initial_seq_num; // Last Acknowledgment Received.
    segment_handler.setDataStream(dataStream, dataSize);
    // segment_handler.generateSegments(this->port, port);
    // cout << "generate segment" << endl;
    segment_handler.generateSegmentsMap(this->port, port);
    // cout << "generate segment done" << endl;
    uint32_t LFS = initial_seq_num; // Last Frame Sent

    uint8_t max_retry_count = 4;
    uint8_t retry_count = 0;
    while(true) {
        // sleep(1);
        // cout << "LAR: " << LAR << endl;
        // cout << "LFS: " << LFS << endl;

        while(LFS - LAR <= SWS) {
            if(segment_handler.segmentMap.find(LFS) == segment_handler.segmentMap.end()) {
                cout << MAG << "[i] " << ci.getFormattedStatus() << " No more segments to send" << COLOR_RESET << endl;
                break;
            }
            Segment segment = segment_handler.segmentMap[LFS];
            uint64_t payload_size = segment.data_offset*4 + segment.payload.size();
            char* payload = new char[payload_size];
            memcpy(payload, &segment, HEADER_ONLY_SIZE);
            memcpy(payload+HEADER_ONLY_SIZE, segment.options.data(), segment.options.size());
            memcpy(payload+segment.data_offset*4, segment.payload.data(), segment.payload.size());
            sendAddr(ci.addr, payload, payload_size);
            delete[] payload;
            uint32_t data_index = calculateSegmentIndex(LFS, initial_seq_num);
            cout << BLU << "[i] " << ci.getFormattedStatus() << " [Seg " << data_index << "] [S=" << segment.seq_num << "] Sent to " << hostPort << create_retry_message(retry_count) << endl;
            LFS += segment.payload.size();
        }

        Segment ack_segment;
        sockaddr_in addr; socklen_t len = sizeof(addr);
        cout << MAG << "[~] " << ci.getFormattedStatus() << " Waiting for segments to be ACKed" << COLOR_RESET << endl;
        this_thread::sleep_for(chrono::milliseconds(10)); // collect all acks within 10ms, done by the other thread created by listen.

        auto recv_time = high_resolution_clock::now();
        chrono::seconds timeout_recv(2);
        while(true) {
            auto& queue = this->received_buffer_queue[ci.addr];
            if(queue.empty()) {
                if(high_resolution_clock::now() - recv_time < timeout_recv) continue; else {
                    cout << RED << "[i] " << ci.getFormattedStatus() << " Timeout waiting for ACK from " << hostPort << COLOR_RESET << endl;
                    LFS = LAR;
                    retry_count++;
                    break;
                }
            }
            ack_segment = queue.front();
            while(!queue.empty()) {
                Segment& segment = queue.front();
                if(extract_flags(segment.flags) == ACK_FLAG) {
                    if(ack_segment.ack_num < segment.ack_num) {
                        ack_segment = segment;
                    }
                }
                queue.pop();
            }

            retry_count = 0;

            // cout << "LFS: " << LFS << endl;
            // cout << "LAR: " << LAR << endl;
            // cout << "if(!isValidChecksum(ack_segment))" << endl;

            // cout << "ack: " << ack_segment.ack_num << endl;
            LAR = max(LAR, ack_segment.ack_num); // LAR if client send already acked value, maybe because of retries. ack_segment.ack_num to proceed forward. its the ack num that the client wants next

            uint32_t data_index = calculateSegmentIndex(ack_segment.ack_num, initial_seq_num);
            cout << YEL << "[i] " << ci.getFormattedStatus() << " [Seg " << data_index-1 << "] [A=" << ack_segment.ack_num << "] ACKed from " << hostPort << COLOR_RESET << endl;

            // if ack_segment is the last segment, then break
            Segment& last_segment = segment_handler.segmentMap.rbegin()->second;
            // cout << "ack_segment.ack_num: " << ack_segment.ack_num << endl;
            // cout << "last_segment.seq_num: " << last_segment.seq_num << endl;
            // cout << "last_segment.payload.size(): " << last_segment.payload.size() << endl;
            // cout << "last_segment.seq_num + last_segment.payload.size(): " << last_segment.seq_num + last_segment.payload.size() << endl;
            if(ack_segment.ack_num == last_segment.seq_num + last_segment.payload.size()) {
                cout << GRN << "[i] " << ci.getFormattedStatus() << " All segments ACKed from " << hostPort << COLOR_RESET << endl;
                segment_handler.setInitialSeqNum(LFS); // so that the next request will have a new seq_num
                return;
            }
            
            // cout << "LFS: " << LFS << endl;
            // cout << "LAR: " << LAR << endl;
            
            break;
        }
        if(retry_count >= max_retry_count) {
            cout << RED << "[i] " << ci.getFormattedStatus() << " Retries exceeded limit" << COLOR_RESET << endl;
            segment_handler.setInitialSeqNum(LFS); // so that the next request will have a new seq_num
            return;
        }
    }
    
    segment_handler.setInitialSeqNum(LFS); // so that the next request will have a new seq_num
}


void TCPSocket::handle_fin_ack(ConnectionInfo& ci, uint32_t ack_num, string& hostPort) {
    Segment ack_segment = ack(ack_num);
    sendAddr(ci.addr, &ack_segment, HEADER_ONLY_SIZE);
    ci.status = TCPStatusEnum::TIME_WAIT;
    cout << BLU << "[i] " << ci.getFormattedStatus() << " Sending ACK request to " << hostPort << COLOR_RESET << endl;
    ci.status = TCPStatusEnum::CLOSED;
    cout << GRN << "[i] " << ci.getFormattedStatus() << " Connection closed successfully" << COLOR_RESET << endl;
}

void TCPSocket::fin_send(ConnectionInfo& ci) {
    string ip = ci.get_ip();
    int32_t port = ci.get_port();
    string hostPort = ci.get_host_port();
    Segment fin_segment = fin();


    uint32_t max_retry_fin = 4;
    uint32_t retry_count_fin = 0;
    while(true) {
        ci.status = TCPStatusEnum::FIN_WAIT_1;
        cout << BLU << "[i] " << ci.getFormattedStatus() << " Sending FIN request to " << hostPort << create_retry_message(retry_count_fin) << COLOR_RESET << endl;
        sendAddr(ci.addr, &fin_segment, HEADER_ONLY_SIZE);
        
        Segment fin_ack_segment; sockaddr_in addr; socklen_t len = sizeof(addr);

        auto send_time_fin = high_resolution_clock::now();
        chrono::seconds timeout_fin(1);
        while (true) {
            auto& queue = this->received_buffer_queue[ci.addr];
            if(queue.empty()) {
                if(high_resolution_clock::now() - send_time_fin < timeout_fin) continue; else {
                    cout << RED << "[i] " << ci.getFormattedStatus() << " Timeout waiting for ACK from " << hostPort << COLOR_RESET << endl;
                    break;
                }
            }
            fin_ack_segment = queue.front();
            while(!queue.empty()) {
                Segment& segment = queue.front();
                if(extract_flags(segment.flags) == FIN_ACK_FLAG) {
                    fin_ack_segment = segment;
                    break;
                }
                queue.pop();
            }
            ci.status = TCPStatusEnum::FIN_WAIT_2;
            cout << YEL << "[+] " << ci.getFormattedStatus() << " Received FIN-ACK request from " << hostPort << COLOR_RESET << endl;
            handle_fin_ack(ci, fin_ack_segment.ack_num, hostPort); // automatically handled by the other thread created by listen
            return;
        }
        retry_count_fin++;
        if(retry_count_fin >= max_retry_fin) {
            cout << RED << "[i] " << ci.getFormattedStatus() << " Retries exceeded limit" << COLOR_RESET << endl;
            break;
        }
    }



}

// Sliding window with Go-Back-N
ssize_t TCPSocket::recv(void* receive_buffer, uint32_t length, sockaddr_in* addr, socklen_t* len) {
    if(this->status != TCPStatusEnum::ESTABLISHED) {
        cout << RED << "[-] " << getFormattedStatus() << " Connection not established" << COLOR_RESET << endl;
        return -1;
    }
    cout << MAG << "[i] " << getFormattedStatus() << " Ready to receive input from " << this->connected_ip << ":" << this->connected_port << COLOR_RESET << endl;
    cout << MAG << "[~] " << getFormattedStatus() << " Waiting for segments to be sent" << COLOR_RESET << endl;
    
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
    chrono::seconds timeout(5);
    char payload[DATA_OFFSET_MAX_SIZE + BODY_ONLY_SIZE];

    while (true) {
        int recv_size = recvAny(&payload, DATA_OFFSET_MAX_SIZE + BODY_ONLY_SIZE, addr, len);
        if(recv_size < 0) {
            if(high_resolution_clock::now() - send_time < timeout) continue; else {
                cout << RED << "[i] " << getFormattedStatus() << " Timeout waiting for data from " << toHostPort(*addr) << COLOR_RESET << endl;
                break;
            }
        }
        send_time = high_resolution_clock::now(); // reset
        
        // cout << "======================" << endl;
        // cout << "recv_size: " << recv_size << endl;
        // cout << "errno: " << errno << endl;

        Segment recv_segment;
        memcpy(&recv_segment, payload, HEADER_ONLY_SIZE);
        bool isHeaderOnlyChecksumValid = isValidChecksum(recv_segment);
        if(isHeaderOnlyChecksumValid && extract_flags(recv_segment.flags) == SYN_ACK_FLAG) {
            ConnectionInfo ci(*addr);
            cout << YEL << "[+] " << getFormattedStatus() << " [S=" << recv_segment.seq_num << "] [A=" << recv_segment.ack_num << "] Received SYN-ACK request again from " << ci.get_host_port() << ". Ack is probably lost which makes sender resends the SYN-ACK." << COLOR_RESET << endl;
            Segment ack_segment = ack(recv_segment.ack_num + 1);
            cout << BLU << "[i] " << getFormattedStatus() << " [A=" << ack_segment.ack_num << "] Resending ACK request to " << ci.get_host_port() << COLOR_RESET << endl;
            sendAny(this->connected_ip.c_str(), this->connected_port, &ack_segment, HEADER_ONLY_SIZE);
            continue;
        }
        if(isHeaderOnlyChecksumValid && extract_flags(recv_segment.flags) == FIN_FLAG) {
            fin_recv(addr, len);
            this->status = TCPStatusEnum::CLOSING;
            break;
        }
        
        // cout << "not fin" << endl;

        uint32_t options_size = uint32_t(0) + (uint32_t(recv_segment.data_offset)*uint32_t(4));
        uint32_t payload_size = recv_size - recv_segment.data_offset*uint32_t(4);
        // cout << "options_size: " << options_size << endl;
        // cout << "payload_size: " << payload_size << endl;
        
        if(options_size < 5 || options_size > DATA_OFFSET_MAX_SIZE) { // 20 - 60 bytes
            uint32_t data_index = calculateSegmentIndex(recv_segment.seq_num, initial_seq_num);
            cout << RED << "[-] " << getFormattedStatus() << " [Seg=" << data_index << "] [A=" << recv_segment.seq_num << "] Invalid data offset" << COLOR_RESET << endl;
            continue;
        }
        if(payload_size < 0 || payload_size > PAYLOAD_SIZE) {
            uint32_t data_index = calculateSegmentIndex(recv_segment.seq_num, initial_seq_num);
            cout << RED << "[-] " << getFormattedStatus() << " [Seg=" << data_index << "] [A=" << recv_segment.seq_num << "] Invalid payload size" << COLOR_RESET << endl;
            continue;
        }
        recv_segment.options = vector<char>(payload + HEADER_ONLY_SIZE, payload + recv_segment.data_offset*4);
        recv_segment.payload = vector<char>(payload + recv_segment.data_offset*4, payload + recv_size);

        if(!isValidChecksum(recv_segment)) {
            cout << RED << "[i] " << getFormattedStatus() << " Invalid checksum, received corrupted packet" << COLOR_RESET << endl;
            continue;
        }

        // cout << "checksum valid" << endl;

        // cout << "recv_segment.seq_num: " << recv_segment.seq_num << endl;
        // cout << "LFR: " << LFR << endl;
        // cout << "LAF: " << LAF << endl;
        // cout << "seq_num_ack: " << seq_num_ack << endl;

        string hostPort = toHostPort(*addr);

        if(recv_segment.seq_num < LFR || recv_segment.seq_num >= LAF) {
            if(recv_segment.seq_num >= initial_seq_num && recv_segment.seq_num < LFR) { // meaning it has been acked but the ack is loss so the server resend it.
            // only send ack if its smaller
                // resend the ack
                uint32_t data_index = calculateSegmentIndex(seq_num_ack, initial_seq_num);
                Segment ack_segment = ack(seq_num_ack);
                sendAny(this->connected_ip.c_str(), this->connected_port, &ack_segment, HEADER_ONLY_SIZE);
                cout << BLU << "[+] " << getFormattedStatus() << " [Seg=" << data_index-1 << "] [A=" << seq_num_ack << "] Resent to " << hostPort << COLOR_RESET << endl;
                continue;
            }
            continue; // discard
        }
        // cout << "in range" << endl;

        


        buffers[recv_segment.seq_num] = recv_segment;
        uint32_t data_index = calculateSegmentIndex(recv_segment.seq_num, initial_seq_num);
        cout << YEL << "[+] " << getFormattedStatus() << " [Seg=" << data_index << "] [S=" << recv_segment.seq_num << "] ACKed from " << hostPort << COLOR_RESET << endl;

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
            if(seq_num < seq_num_ack) continue;
            if(buffers.find(seq_num_ack + total_received) == buffers.end()) break; // break when we found a gap
            total_received += segment.payload.size();
        }

        // cout << "(prev(buffers.end())->first: " << prev(buffers.end())->first << endl;
        // cout << "prev(buffers.end())->second.payload.size(): " << prev(buffers.end())->second.payload.size() << endl;
        // cout << "LFR: " << LFR << endl;
        // cout << "initial_seq_num: " << initial_seq_num << endl;
        // cout << "initial_acl_num: " << initial_ack_num << endl;
        // cout << "total_received: " << total_received << endl;
        // if(!(prev(buffers.end())->first + prev(buffers.end())->second.payload.size() - initial_seq_num == total_received)) {
            // cout << "!(prev(buffers.end())->first + prev(buffers" << endl;
            // continue;
        // }


        // cout << "sending ack"<< endl;
        // cout << "prev(buffers.end())->first: " << prev(buffers.end())->first << endl;
        // cout << "prev(buffers.end())->second.payload.size(): " << prev(buffers.end())->second.payload.size() << endl;
        seq_num_ack = seq_num_ack + total_received;
        LFR = seq_num_ack;
        LAF = LFR + RWS;
        Segment ack_segment = ack(seq_num_ack);
        sendAny(this->connected_ip.c_str(), this->connected_port, &ack_segment, HEADER_ONLY_SIZE);
        
        data_index = calculateSegmentIndex(seq_num_ack, initial_seq_num);
        cout << BLU << "[+] " << getFormattedStatus() << " [Seg=" << data_index-1 << "] [A=" << seq_num_ack << "] Sent to " << hostPort << COLOR_RESET << endl;

        if(extract_flags(buffers.rbegin()->second.flags) == PSH_FLAG) {
            segment_handler.setInitialAckNum(seq_num_ack); // so that the next request will have a new ack_num

            uint32_t current = 0;
            for(auto& [seq_num, segment] : buffers) {
                memcpy(receive_buffer + current, segment.payload.data(), segment.payload.size());
                current += segment.payload.size();
            }
            return current;
        }
    }
    return -1;
}

void TCPSocket::fin_recv(sockaddr_in* addr, socklen_t* len) {
    string hostPort = toHostPort(*addr);
    this->status = TCPStatusEnum::FIN_WAIT_1;
    cout << YEL << "[i] " << getFormattedStatus() << " Received FIN request from " << hostPort << COLOR_RESET << endl;


    uint8_t max_retry_fin = 4;
    uint8_t retry_count_fin = 0;

    while(true) {
        this->status = TCPStatusEnum::CLOSING;
        cout << BLU << "[i] " << getFormattedStatus() << " Sending FIN-ACK request to " << hostPort << create_retry_message(retry_count_fin) << COLOR_RESET << endl;
        Segment fin_ack_segment = finAck();
        sendAddr(*addr, &fin_ack_segment, HEADER_ONLY_SIZE);
        
        Segment ack_segment;
        auto send_time_ack = high_resolution_clock::now();
        chrono::seconds timeout_ack(1);
        while(true) {
            int recv_size = recvAny(&ack_segment, HEADER_ONLY_SIZE, addr, len);
            if(recv_size < 0) {
                if(high_resolution_clock::now() - send_time_ack < timeout_ack) continue; else {
                    cout << RED << "[i] " << getFormattedStatus() << " Waiting for Ack timeout." << COLOR_RESET << endl;
                    break;
                }
            }
            if(!isValidChecksum(ack_segment)) {
                cout << RED << "[i] " << getFormattedStatus() << " Invalid checksum, received corrupted packet" << COLOR_RESET << endl;
                continue;
            }
            if (extract_flags(ack_segment.flags) == ACK_FLAG) {
                this->status = TCPStatusEnum::LAST_ACK;
                cout << YEL << "[+] " << getFormattedStatus() << " Received ACK request from " << hostPort << COLOR_RESET << endl;
                this->status = TCPStatusEnum::CLOSED;
                cout << GRN << "[i] " << getFormattedStatus() << " Connection closed successfully" << COLOR_RESET << endl;
                return;
            }
        }
        retry_count_fin++;
        if(retry_count_fin >= max_retry_fin) {
            cout << RED << "[i] " << getFormattedStatus() << " Retries exceeded limit" << COLOR_RESET << endl;
            break;
        }
    }
}

// client_socket = -1 to close self
void TCPSocket::close(int client_socket) {
    if(client_socket == -1) {
        ::close(this->socket);
        return;
    }
    
    if(this->connection_map.find(client_socket) != this->connection_map.end()) { // client/sender
        ConnectionInfo ci = this->connection_map[client_socket];
        fin_send(ci);
        this->connection_map.erase(client_socket);
    } else { // client/reciever
        // fin
        ::close(this->socket);
    }
}

string TCPSocket::getFormattedStatus() {
    static const std::unordered_map<TCPStatusEnum, std::string> statusMap = {
        {TCPStatusEnum::LISTEN, "LISTEN"},
        {TCPStatusEnum::SYN_SENT, "SYN_SENT"},
        {TCPStatusEnum::SYN_RECEIVED, "SYN_RECEIVED"},
        {TCPStatusEnum::ESTABLISHED, "ESTABLISHED"},
        {TCPStatusEnum::FIN_WAIT_1, "FIN_WAIT_1"},
        {TCPStatusEnum::FIN_WAIT_2, "FIN_WAIT_2"},
        {TCPStatusEnum::CLOSE_WAIT, "CLOSE_WAIT"},
        {TCPStatusEnum::CLOSING, "CLOSING"},
        {TCPStatusEnum::LAST_ACK, "LAST_ACK"},
        {TCPStatusEnum::TIME_WAIT, "TIME_WAIT"},
        {TCPStatusEnum::CLOSED, "CLOSED"},
    };

    std::string statusString = "[";
    statusString += std::to_string(this->status);
    statusString += "] ";
    statusString += "[" + statusMap.at(this->status) + "]";
    return statusString;
}
