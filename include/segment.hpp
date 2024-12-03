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

    struct Flags
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
    
    uint8_t *options;  

    uint8_t *payload;
    uint16_t payload_len; 
} __attribute__((packed));

const uint8_t FIN_FLAG = 0b00000001;
const uint8_t SYN_FLAG = 0b00000010;
const uint8_t ACK_FLAG = 0b00010000;
const uint8_t SYN_ACK_FLAG = SYN_FLAG | ACK_FLAG;
const uint8_t FIN_ACK_FLAG = FIN_FLAG | ACK_FLAG;

/**
 * Generate Segment that contain SYN packet
 */
Segment syn(uint32_t seqNum);

/**
 * Generate Segment that contain ACK packet
 */
Segment ack( uint32_t ackNum);

/**
 * Generate Segment that contain SYN-ACK packet
 */
Segment synAck(uint32_t seqNum, uint32_t ackNum);

/**
 * Generate Segment that contain FIN packet
 */
Segment fin();

/**
 * Generate Segment that contain FIN-ACK packet
 */
Segment finAck();

// update return type as needed
uint16_t calculateChecksum(Segment segment);

/**
 * Return a new segment with a calcuated checksum fields
 */
Segment updateChecksum(Segment segment);

/**
 * Check if a TCP Segment has a valid checksum
 */
bool isValidChecksum(Segment segment);


uint8_t extract_flags(const Segment::Flags &flags);

#endif