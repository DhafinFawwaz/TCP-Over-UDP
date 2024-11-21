#ifndef segment_h
#define segment_h

#include <cstdint>
#include <cmath>


struct Segment
{
    uint16_t sourcePort : 16;
    uint16_t destPort;

    uint32_t seq_num;
    uint32_t ack_num;

    struct
    {
        unsigned int data_offset : 4;
        unsigned int reserved : 4;
    };

    struct
    {
        unsigned int cwr : 1;
        unsigned int ece : 1;
        unsigned int urg : 1;
        unsigned int ack : 1;
        unsigned int psh : 1;
        unsigned int pst : 1;
        unsigned int syn : 1;
        unsigned int fin : 1;
    } flags;

    uint16_t checksum;
    uint16_t urg_point;

    uint16_t window;
    
    struct
    {
        uint64_t mem1;
        uint64_t mem2;
        uint64_t mem3;
        uint64_t mem4;
        uint64_t mem5; 
    } options;    

    uint8_t *payload;
} __attribute__((packed));

const uint8_t FIN_FLAG = 1;
const uint8_t SYN_FLAG = 2;
const uint8_t ACK_FLAG = 16;
const uint8_t SYN_ACK_FLAG = SYN_FLAG | ACK_FLAG;
const uint8_t FIN_ACK_FLAG = FIN_FLAG | ACK_FLAG;

/**
 * Generate Segment that contain SYN packet
 */
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

#endif