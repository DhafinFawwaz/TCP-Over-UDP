#ifndef segment_h
#define segment_h

#include <cstdint>
#include <cmath>
#include <vector>
using namespace std;

struct Segment
{
    uint16_t sourcePort : 16;
    uint16_t destPort;

    uint32_t seq_num;
    uint32_t ack_num;

    struct
    {
        uint8_t  data_offset : 4;
        uint8_t  reserved : 4;
    } __attribute__((packed));

    struct Flags
    {
        uint8_t cwr : 1;
        uint8_t ece : 1;
        uint8_t urg : 1;
        uint8_t ack : 1;
        uint8_t psh : 1;
        uint8_t pst : 1;
        uint8_t syn : 1;
        uint8_t fin : 1;
    } __attribute__((packed)) flags;

    uint16_t checksum;
    uint16_t urg_point;

    uint16_t window;
    
    vector<char> options;  

    vector<char> payload;
} __attribute__((packed));

const uint8_t FIN_FLAG = 0b00000001;
const uint8_t SYN_FLAG = 0b00000010;
const uint8_t ACK_FLAG = 0b00010000;
const uint8_t SYN_ACK_FLAG = SYN_FLAG | ACK_FLAG;
const uint8_t FIN_ACK_FLAG = FIN_FLAG | ACK_FLAG;

const uint8_t HEADER_ONLY_SIZE = 20;
const uint8_t DATA_OFFSET_MAX_SIZE = 60;
const uint16_t BODY_ONLY_SIZE = 1460;

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

uint16_t calculateSum(Segment &segment);

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