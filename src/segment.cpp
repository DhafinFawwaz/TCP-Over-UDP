#include <segment.hpp>

Segment syn(uint32_t seqNum){
    Segment segment = {0};
    segment.flags.syn = 1;
    segment.seq_num = seqNum;
    return segment;
}

/**
 * Generate Segment that contain ACK packet
 */
Segment ack( uint32_t ackNum){
    Segment segment = {0};
    segment.flags.ack = 1;
    segment.ack_num = ackNum;
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
    return segment;
}

/**
 * Generate Segment that contain FIN packet
 */
Segment fin(){
    Segment segment = {0};
    segment.flags.fin = 1;
    return segment;
}

/**
 * Generate Segment that contain FIN-ACK packet
 */
Segment finAck(){
    Segment segment = {0};
    segment.flags.fin = 1;
    segment.flags.ack = 1;
    return segment;
}

// update return type as needed
uint16_t calculateChecksum(Segment segment){
    uint16_t new_sum = 0;
    uint16_t* ptr= (uint16_t*)segment.payload;
    for(uint16_t i = 0; i < 1460; i+=16){
        new_sum ^= *(ptr+i);
    }
    return ~new_sum;
}

/**
 * Return a new segment with a calcuated checksum fields
 */
Segment updateChecksum(Segment segment){
    Segment new_segment = segment;
    new_segment.checksum = calculateChecksum(new_segment);
    return new_segment;
}

/**
 * Check if a TCP Segment has a valid checksum
 */
bool isValidChecksum(Segment segment){
    uint16_t valid_sum = 0;
    uint16_t* ptr= (uint16_t*)segment.payload;
    for(uint16_t i = 0; i < 1460; i+=16){
        valid_sum ^= *(ptr+i);
    }
    valid_sum^=segment.checksum;
    return !(~valid_sum);
}