#ifndef segment_handler_h
#define segment_handler_h

#include <segment.hpp>
#include <vector>
#include <deque>
#include <map>
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

public:
    vector<Segment> segmentBuffer;
    map<uint32_t, Segment> segmentMap;
    ~SegmentHandler();

    void generateSegments(uint16_t sourcePort, uint16_t destPort);
    void generateSegmentsMap(uint16_t sourcePort, uint16_t destPort);
    uint32_t generateInitialSeqNum();
    void setDataStream(uint8_t *dataStream, uint32_t dataSize);
    uint8_t getWindowSize();
    void setWindowSize(uint8_t windowSize);
    uint8_t advanceWindow(deque<Segment> &subSegments);
    
    uint32_t getBufferSize();
    void setInitialSeqNum(uint32_t seqNum);
    void setInitialAckNum(uint32_t ackNum);
    uint32_t getInitialSeqNum();
    uint32_t getInitialAckNum();
};

#endif