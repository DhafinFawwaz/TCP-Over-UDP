#include <segment_handler.hpp>


uint32_t SegmentHandler::generateInitialSeqNum(){
    return rand() % 1000;
}

void SegmentHandler::generateSegments(uint16_t sourcePort, uint16_t destPort){

    uint16_t segmentCount = ceil(dataSize / 1460);
    uint32_t remainingData = dataSize;

    // Segment* segments  = new Segment[segmentCount];
    segmentBuffer = vector<Segment>(segmentCount);
    for(uint16_t i = 0; i < segmentCount; i++){
        segmentBuffer.push_back({0});

        uint16_t payloadSize = remainingData > 1460 ? 1460 : remainingData;

        segmentBuffer[i].sourcePort = sourcePort;
        segmentBuffer[i].destPort = destPort;


        if(i==0){
            segmentBuffer[i].flags.syn = 1;
            segmentBuffer[i].seq_num = generateInitialSeqNum();
        }else{
            segmentBuffer[i].flags.syn = 0;
            segmentBuffer[i].seq_num = currentSeqNum;
        }
        currentSeqNum += payloadSize;

        segmentBuffer[i].ack_num = currentAckNum;
        segmentBuffer[i].flags.ack = 0;

        if(remainingData <= 1460){
            segmentBuffer[i].flags.fin = 1;
        }else{
            segmentBuffer[i].flags.fin = 0;
        }
        segmentBuffer[i].payload = (uint8_t*)(dataStream + dataIndex);
        segmentBuffer[i].options = NULL;
        segmentBuffer[i].window = windowSize;
        segmentBuffer[i].checksum = calculateChecksum(segmentBuffer[i]);
        
        segmentBuffer[i].data_offset = 20;
        
        dataIndex += 1460;
    }

}

void SegmentHandler::setDataStream(uint8_t *dataStream, uint32_t dataSize){
    this->dataStream = dataStream;
    this->dataSize = dataSize;
}

uint8_t SegmentHandler::getWindowSize(){
    return this->windowSize;
}


Segment* SegmentHandler::advanceWindow(uint8_t size){

}