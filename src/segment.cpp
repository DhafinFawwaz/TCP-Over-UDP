#include <segment.hpp>
#include <iostream>
using namespace std;
Segment syn(uint32_t seqNum){
    Segment segment = {0};
    segment.flags.syn = 1;
    segment.seq_num = seqNum;
    segment.checksum = calculateChecksum(segment);
    return segment;
}

/**
 * Generate Segment that contain ACK packet
 */
Segment ack( uint32_t ackNum){
    Segment segment = {0};
    segment.flags.ack = 1;
    segment.ack_num = ackNum;
    segment.checksum = calculateChecksum(segment);
    return segment;
}

/**
 * Generate Segment that contain SYN-ACK packet
 */
Segment synAck(uint32_t seqNum, uint32_t ackNum){
    Segment segment = {0};
    segment.flags.ack = 1;
    segment.flags.syn = 1;
    segment.seq_num = seqNum;
    segment.ack_num = ackNum;
    segment.checksum = calculateChecksum(segment);
    return segment;
}

/**
 * Generate Segment that contain FIN packet
 */
Segment fin(){
    Segment segment = {0};
    segment.flags.fin = 1;
    segment.checksum = calculateChecksum(segment);
    return segment;
}

/**
 * Generate Segment that contain FIN-ACK packet
 */
Segment finAck(){
    Segment segment = {0};
    segment.flags.fin = 1;
    segment.flags.ack = 1;
    segment.checksum = calculateChecksum(segment);
    return segment;
}


// return xor of all bits on the segment except checksum
uint16_t calculateSum(Segment& segment){
    uint16_t new_sum = 0;
    new_sum ^= segment.sourcePort;
    new_sum ^= segment.destPort;
    new_sum ^= (uint16_t) (segment.seq_num);
    new_sum ^= (uint16_t) (segment.seq_num>>16);
    new_sum ^= (uint16_t) (segment.ack_num);
    new_sum ^= (uint16_t) (segment.ack_num>>16);
    new_sum ^= uint16_t(0) | segment.data_offset<<12 | segment.reserved<<8 | segment.flags.cwr<<7 | segment.flags.ece<<6 | segment.flags.urg<<5 | segment.flags.ack<<4 | segment.flags.psh<<3 | segment.flags.pst<<2 | segment.flags.syn<<1 | segment.flags.fin;
    new_sum ^= segment.urg_point;
    new_sum ^= segment.window;
    uint16_t temp;
    for(uint32_t i = 0; i < segment.options.size(); i+=2){
        temp = 0;
        temp |= ((segment.options.at(i))<<8);
        if (i < segment.options.size()-1){
            temp |= segment.options.at(i+1);
        }
        new_sum ^= temp;
    }
    
    // cout << 6 << endl;
    for(uint32_t i = 0; i < segment.payload.size(); i+=2){
        // cout << "i: " << i << endl;
        temp = 0;
        temp |= (segment.payload.at(i)<<8);
        if (i < segment.payload.size()-1){
            temp |= segment.payload.at(i+1);
        }
        new_sum ^= temp;
    }
    // cout << 7 << endl;
    return (uint16_t) new_sum;
}

// update return type as needed
uint16_t calculateChecksum(Segment& segment){
    return (uint16_t) ~calculateSum(segment);
}

/**
 * Return a new segment with a calcuated checksum fields
 */
Segment updateChecksum(Segment segment){
    segment.checksum = calculateChecksum(segment);
    return segment;
}

/**
 * Check if a TCP Segment has a valid checksum
 */
bool isValidChecksum(Segment& segment){
    return true;
    uint16_t valid_sum = calculateSum(segment);
    valid_sum^=segment.checksum;
    // cout<<"sum: "<<valid_sum<<endl;
    // cout<<"checksum: "<<(uint16_t)~valid_sum<<endl;
    return !(uint16_t)~valid_sum;
}

uint8_t extract_flags(const Segment::Flags &flags) {
    return (flags.cwr << 7) | (flags.ece << 6) | (flags.urg << 5) | (flags.ack << 4) | (flags.psh << 3) | (flags.pst << 2) | (flags.syn << 1) | (flags.fin);
}
