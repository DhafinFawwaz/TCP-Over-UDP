#include <server.hpp>
#include <iostream>
#include <ansi_code.hpp>
#include <map>

#define PAYLOAD_SIZE 1460

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
    auto sync_buffer_size = connection.recv(&syn_segment, PAYLOAD_SIZE, &addr, &len);
    cout << YEL << "[i] " << connection.getFormattedStatus() << " [Handshake] [S=" << syn_segment.seq_num << "] Received SYN request from " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << endl << COLOR_RESET;

    bool retry = true;
    SegmentHandler segment_handler;
    Segment ack_segment;
    while (retry) {
        uint32_t initial_seq_num = segment_handler.generateInitialSeqNum();
        Segment syn_ack_segment = synAck(syn_segment.seq_num + 1, initial_seq_num);
        cout << BLU << "[i] " << connection.getFormattedStatus() << " [Handshake] [A=" << syn_ack_segment.ack_num << "] [S=" << syn_ack_segment.seq_num << "] Sending SYN-ACK request to " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << COLOR_RESET << endl;
        connection.send(inet_ntoa(addr.sin_addr), addr.sin_port, &syn_ack_segment, sync_buffer_size);

        while (true) {
            auto ack_buffer_size = connection.recv(&ack_segment, PAYLOAD_SIZE, &addr, &len);
            if(ack_buffer_size < 0) {
                cout << RED << "[-] " << connection.getFormattedStatus() << " [Handshake] Error, retrying" << COLOR_RESET << endl; // example case is timeout
                retry = true;
                break;
            }
            retry = false;
            if(extract_flags(ack_segment.flags) == ACK_FLAG && ack_segment.ack_num == syn_ack_segment.ack_num + 1) break;   
        }
        if(retry) continue;
        
        cout << YEL << "[i] " << connection.getFormattedStatus() << " [Handshake] [A=" << ack_segment.ack_num << "] Received ACK request from " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << COLOR_RESET << endl;
        
    }


    // ======== Established ========
    char* datastream = new char[response_buffer.size()]; // dont forget to delete this
    response_buffer.copy(datastream, response_buffer.size());
    segment_handler.setDataStream((uint8_t*)datastream, response_buffer.size());
    segment_handler.setInitialSeqNum(ack_segment.ack_num);
    segment_handler.generateSegments(this->port, addr.sin_port);

    cout << "[i] " << connection.getFormattedStatus() << " Sending input to " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << endl;

    // Sliding window stuff
    // bool done = false;
    // uint32_t sws, lar, lfs;
    // sws = 4*PAYLOAD_SIZE; 
    
    // // TODO: find out if this vector size is correct
    // vector<uint8_t> segments_map_state(response_buffer.size()/PAYLOAD_SIZE+1); // 0 = not received, 1 = received (queued), 2 = acked
    // map<uint32_t, time_t> timer_map; // initial send time
    // while (!done) {
    //     vector<Segment> subSegments;
    //     segment_handler.advanceWindow(sws/PAYLOAD_SIZE, subSegments);

    //     // while(lfs - lar <= sws) using this is probably easier and it means we're not using the advancedWindow
    //     for(uint8_t i = 0; i < subSegments.size(); i++) {
    //         connection.send(inet_ntoa(addr.sin_addr), addr.sin_port, &subSegments[i], PAYLOAD_SIZE);
    //         timer_map[subSegments[i].seq_num] = time(NULL);
    //         cout << BLU << "[i] " << connection.getFormattedStatus() << " [Established] [Seg 1] [S=" << subSegments[i].seq_num << "] Sent" << COLOR_RESET << endl;
    //     }
    //     lfs = subSegments[subSegments.size()-1].seq_num;
    //     cout << "[~] " << connection.getFormattedStatus() << " [Established] Waiting for segments to be ACKed" << endl;

    //     Segment ack_segment_sw;
    //     connection.recv(&ack_segment_sw, PAYLOAD_SIZE, &addr, &len);
    //     if(extract_flags(ack_segment_sw.flags) == ACK_FLAG) {
    //         if((ack_segment_sw.ack_num - ack_segment.ack_num) % PAYLOAD_SIZE == 0) {
    //             uint8_t idx = (ack_segment_sw.ack_num - ack_segment.ack_num) / PAYLOAD_SIZE;
    //             segments_map_state[idx] = 1;

    //             // find value from left to right in the map that is received (queued), state is 1. then set to acked, state is 2
    //             for(uint8_t i = lar; i < segments_map_state.size(); i++) {
    //                 if(segments_map_state[i] == 0) break;
    //                 else if(segments_map_state[i] == 1) {
    //                     segments_map_state[i] = 2;
    //                     lar = i*PAYLOAD_SIZE;
    //                     break;
    //                 }
    //             }

    //             cout << YEL << "[i] " << connection.getFormattedStatus() << " [Established] [Seg 1] [A=" << ack_segment_sw.ack_num << "] ACKed" << COLOR_RESET << endl;

    //         } else { // either its invalid or the last segment

    //         }
    //     }
        


    //     cout << YEL << "[i] [Established] [Seg 1] [A=3165500900] ACKed" << COLOR_RESET << endl;
    //     cout << YEL << "[i] [Established] [Seg 2] [A=3165502360] ACKed" << COLOR_RESET << endl;
    //     cout << YEL << "[i] [Established] [Seg 3] [A=3165503820] ACKed" << COLOR_RESET << endl;

    //     delete[] datastream;

    // }
    // ======== Closing ========
    cout << YEL << "[i] [Closing] Sending FIN request to " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << COLOR_RESET << endl;
    Segment fin_segment = fin();
    connection.send(inet_ntoa(addr.sin_addr), addr.sin_port, &fin_segment, sync_buffer_size);


    Segment fin_ack_segment;
    while (true) {
        auto fin_ack_buffer_size = connection.recv(&fin_ack_segment, PAYLOAD_SIZE, &addr, &len);
        if(fin_ack_buffer_size < 0) {
            cout << RED << "[-] " << connection.getFormattedStatus() << " [Closing] Error, retrying" << COLOR_RESET << endl; // example case is timeout
            continue;
        }
        if(extract_flags(fin_ack_segment.flags) == FIN_ACK_FLAG) break;
    }
    cout << BLU << "[+] [Closing] Received FIN-ACK request from " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << COLOR_RESET << endl;
    

    cout << BLU << "[i] [Closing] Sending ACK request to " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << COLOR_RESET << endl;
    Segment ack_buffer_closing = ack(fin_ack_segment.ack_num + 1);
    connection.send(inet_ntoa(addr.sin_addr), addr.sin_port, &ack_buffer_closing, sync_buffer_size);

    cout << GRN << "[i] Connection closed successfully" << COLOR_RESET << endl;
    
}


Server::~Server() {
    connection.close();
}
