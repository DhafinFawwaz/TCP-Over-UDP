#include <segment_handler.hpp>
#include <deque>
#include <iostream>
#include <vector>
#include <string.h>
using namespace std;

#define PAYLOAD_SIZE 1460

SegmentHandler::SegmentHandler(){
    this->windowSize = 1;
    this->currentSeqNum = 0;
    this->currentAckNum = 0;
    this->dataStream = nullptr;
    this->dataSize = 0;
    this->dataIndex = 0;
    this->segmentMap = map<uint32_t, Segment>();
}

SegmentHandler::~SegmentHandler() {
    
}

uint32_t SegmentHandler::generateInitialSeqNum(){
    return rand() % 1000;
}
void SegmentHandler::generateSegmentsMap(uint16_t sourcePort, uint16_t destPort){
    if (dataSize == 0 || dataStream == nullptr) {
        return;
    }
    cout << 0 << endl;

    uint32_t segmentCount = (dataSize + PAYLOAD_SIZE - 1) / PAYLOAD_SIZE;
    uint32_t remainingData = dataSize;
    uint32_t currentSeqNum = this->currentSeqNum;
    cout << 1 << endl;
    
    // this->segmentMap.clear()
    // try {this->segmentMap.clear();}
    // catch(exception e){cout << e.what() << endl;}
    
    cout << 2 << endl;

    uint16_t currentIndex = 0;
    uint32_t seqNumBefore;
    uint32_t seqnum = currentSeqNum;
    for(uint16_t i = 0; i < segmentCount; i++){
        Segment newSegment = {0};
        cout << i << endl;
        uint16_t payloadSize = remainingData > PAYLOAD_SIZE ? PAYLOAD_SIZE : remainingData;
        newSegment.seq_num = seqnum;
        seqnum+= payloadSize;
        remainingData -= payloadSize;
        newSegment.sourcePort = sourcePort;
        newSegment.destPort = destPort;
        

        // if (currentIndex + payloadSize <= dataSize) {
        //     // uint32_t* from = (uint32_t*)dataStream + currentIndex;
        //     // uint32_t* to = (uint32_t*)dataStream + currentIndex + payloadSize;
        //     // vector<char> v(reinterpret_cast<char*>(from), reinterpret_cast<char*>(to));
        //     // newSegment.payload = v;
            
        // } else {
        //     uint32_t* from = (uint32_t*)dataStream + currentIndex;
        //     uint32_t* to = (uint32_t*)dataStream + dataSize - currentIndex;
        //     vector<char> v(reinterpret_cast<char*>(from), reinterpret_cast<char*>(to));
        //     newSegment.payload = v;
        // }
        if (currentIndex + payloadSize <= dataSize) {
            vector<char> v;
            for (uint16_t i = 0; i < payloadSize; i++) {
                v.push_back((reinterpret_cast<char*>(dataStream)[currentIndex + i]));
            }
            newSegment.payload = v;
        } else {
            vector<char> v;
            for (uint16_t i = 0; currentIndex + i < dataSize; i++) {
                v.push_back((reinterpret_cast<char*>(dataStream)[currentIndex + i]));
            }
            newSegment.payload = v;
        }


        newSegment.options = vector<char>(0);
        newSegment.window = windowSize;
        newSegment.data_offset = 5;
        cout << "calculate checksum:" << calculateSum(newSegment) << endl;
        newSegment.checksum = calculateChecksum(newSegment);
        cout << "calculate checksum:" << calculateSum(newSegment) << endl;
        cout << "checksum:" << newSegment.checksum << endl;
        cout << 11 << endl;
        cout << 12 << endl;
        // segmentMap[newSegment.seq_num] = newSegment;
       segmentMap.insert(std::make_pair(static_cast<uint32_t>(newSegment.seq_num), newSegment));
        cout << 13 << endl;
        currentIndex += PAYLOAD_SIZE;
        cout << 14 << endl;
    }
}

void SegmentHandler::generateSegments(uint16_t sourcePort, uint16_t destPort){

    // if (dataSize == 0 || dataStream == nullptr) {
    //     return;
    // }
    // uint32_t segmentCount = (dataSize + PAYLOAD_SIZE - 1) / PAYLOAD_SIZE;
    // uint32_t remainingData = dataSize;
    // uint32_t currentSeqNum = this->currentSeqNum;

    // segmentBuffer.clear();
    // segmentBuffer.reserve(segmentCount);

    // // Segment* segments  = new Segment[segmentCount];
    // segmentBuffer.clear();
    // uint16_t currentIndex = 0;
    // for(uint16_t i = 0; i < segmentCount; i++){
    //     segmentBuffer.push_back({0});

    //     uint16_t payloadSize = remainingData > PAYLOAD_SIZE ? PAYLOAD_SIZE : remainingData;

    //     segmentBuffer[i].sourcePort = sourcePort;
    //     segmentBuffer[i].destPort = destPort;

    //     if(i==0){
    //         segmentBuffer[i].seq_num = currentSeqNum;
    //         segmentBuffer[i].ack_num = currentAckNum;
    //     }else{
    //         segmentBuffer[i].seq_num = segmentBuffer[i-1].seq_num + payloadSize;
    //         segmentBuffer[i].ack_num = segmentBuffer[i-1].ack_num + payloadSize;
    //     }
        

    //     if (currentIndex + payloadSize <= dataSize) {
    //         // memcpy(segmentBuffer[i].payload, dataStream + currentIndex, payloadSize);
    //         segmentBuffer[i].payload = vector<char>(dataStream + currentIndex, dataStream + currentIndex + payloadSize);
    //     } else {
    //         segmentBuffer[i].payload = vector<char>(dataStream + currentIndex, dataStream + currentIndex + dataSize - currentIndex);
    //     }

    //     segmentBuffer[i].options = vector<char>(0);
    //     segmentBuffer[i].window = windowSize;
    //     segmentBuffer[i].checksum = calculateChecksum(segmentBuffer[i]);
        
    //     segmentBuffer[i].data_offset = 20;
        
    //     currentIndex += PAYLOAD_SIZE;
    // }

}

void SegmentHandler::setDataStream(void *dataStream, uint32_t dataSize){
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