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


// return xor of all bits on the segment except checksum
uint16_t calculateSum(Segment &segment){
    uint16_t new_sum = 0;
    new_sum ^= segment.sourcePort;
    new_sum ^= segment.destPort;
    new_sum ^= (uint16_t) (segment.seq_num);
    new_sum ^= (uint16_t) (segment.seq_num>>16);
    new_sum ^= (uint16_t) (segment.ack_num);
    new_sum ^= (uint16_t) (segment.ack_num>>16);
    new_sum ^= uint16_t(0) + segment.data_offset<<12 + segment.reserved<<8 + segment.flags.cwr<<7 + segment.flags.ece<<6 + segment.flags.urg<<5 + segment.flags.ack<<4 + segment.flags.psh<<3 + segment.flags.pst<<2 + segment.flags.syn<<1 + segment.flags.fin;

    new_sum ^= segment.urg_point;
    new_sum ^= segment.window;
    uint16_t* ptr= (uint16_t*)segment.options;
    if(ptr !=NULL){
        uint8_t length = sizeof(ptr)*8;
        for(uint16_t i = 0; i < length; i+=16){
            new_sum ^= *(ptr+i);
        }    
    }
    

    ptr= (uint16_t*)segment.payload;
    if(ptr != NULL){
        uint8_t payload_len = sizeof(ptr)*8;
        for(uint16_t i = 0; i < payload_len; i+=16){
            new_sum ^= *(ptr+i);
        }
    }
    return new_sum;
}

// update return type as needed
uint16_t calculateChecksum(Segment segment){
    return ~calculateSum(segment);
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
    uint16_t valid_sum = calculateSum(segment);
    valid_sum^=segment.checksum;
    return !(~valid_sum);
}