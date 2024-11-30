#include <segment_handler.hpp>
#include <deque>
#include <iostream>
using namespace std;

#define PAYLOAD_SIZE 1460

uint32_t SegmentHandler::generateInitialSeqNum(){
    return rand() % 1000;
}

void SegmentHandler::generateSegments(uint16_t sourcePort, uint16_t destPort){

    uint16_t segmentCount = ceil(dataSize / PAYLOAD_SIZE);
    uint32_t remainingData = dataSize;

    // Segment* segments  = new Segment[segmentCount];
    segmentBuffer.clear();
    uint16_t currentIndex = 0;
    for(uint16_t i = 0; i < segmentCount; i++){
        segmentBuffer.push_back({0});

        uint16_t payloadSize = remainingData > PAYLOAD_SIZE ? PAYLOAD_SIZE : remainingData;

        segmentBuffer[i].sourcePort = sourcePort;
        segmentBuffer[i].destPort = destPort;

        if(i==0){
            segmentBuffer[i].seq_num = currentSeqNum;
            segmentBuffer[i].ack_num = currentAckNum;
        }else{
            segmentBuffer[i].seq_num = segmentBuffer[i-1].seq_num + payloadSize;
            segmentBuffer[i].ack_num = segmentBuffer[i-1].ack_num + payloadSize;
        }

        segmentBuffer[i].payload = (uint8_t*)(dataStream + currentIndex);
        segmentBuffer[i].options = NULL;
        segmentBuffer[i].window = windowSize;
        segmentBuffer[i].checksum = calculateChecksum(segmentBuffer[i]);
        
        segmentBuffer[i].data_offset = 20;
        
        currentIndex += PAYLOAD_SIZE;
    }

}

void SegmentHandler::setDataStream(uint8_t *dataStream, uint32_t dataSize){
    this->dataStream = dataStream;
    this->dataSize = dataSize;
}

uint8_t SegmentHandler::getWindowSize(){
    return this->windowSize;
}

void SegmentHandler::setWindowSize(uint8_t windowSize){
    this->windowSize = windowSize;
}

uint8_t SegmentHandler::advanceWindow(deque<Segment> &subSegments){
    if(subSegments.size() == 0) {
        dataIndex = 0;
        for(uint32_t i = 0; i < windowSize; i++) {
            subSegments.push_back(segmentBuffer[i]);
        }
    } else {
        dataIndex++;
        subSegments.pop_front();
        subSegments.push_back(segmentBuffer[dataIndex+windowSize-1]);
    }
}

void SegmentHandler::setInitialSeqNum(uint32_t seqNum){
    this->currentSeqNum = seqNum;
}

void SegmentHandler::setInitialAckNum(uint32_t ackNum){
    this->currentAckNum = ackNum;
}

uint32_t SegmentHandler::getInitialSeqNum(){
    return this->currentSeqNum;
}

uint32_t SegmentHandler::getInitialAckNum(){
    return this->currentAckNum;
}