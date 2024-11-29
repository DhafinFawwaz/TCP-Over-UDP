#ifndef segment_handler_h
#define segment_handler_h

#include <segment.hpp>
#include <vector>
using namespace std;

class SegmentHandler
{
private:
    uint8_t windowSize;
    uint32_t currentSeqNum;
    uint32_t currentAckNum;
    void *dataStream;
    uint32_t dataSize;
    uint32_t dataIndex;
    // Segment *segmentBuffer; // or use std vector if you like
    vector<Segment> segmentBuffer;

    void generateSegments(uint16_t sourcePort, uint16_t destPort);


public:
    uint32_t generateInitialSeqNum();
    void setDataStream(uint8_t *dataStream, uint32_t dataSize);
    uint8_t getWindowSize();
    Segment *advanceWindow(uint8_t size);
};

#endif