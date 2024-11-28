#include <segment_handler.hpp>


uint32_t SegmentHandler::generateInitialSeqNum(){
    return rand() % 1000;
}

void SegmentHandler::generateSegments(uint16_t sourcePort, uint16_t destPort){

    uint16_t segmentCount = ceil(dataSize / 1460);
    uint32_t remainingData = dataSize;

    Segment* segments  = new Segment[segmentCount];
    for(uint16_t i = 0; i < segmentCount; i++){
        Segment segment = {0};

        uint16_t payloadSize = remainingData > 1460 ? 1460 : remainingData;

        segment.sourcePort = sourcePort;
        segment.destPort = destPort;


        if(i==0){
            segment.flags.syn = 1;
            segment.seq_num = generateInitialSeqNum();
        }else{
            segment.flags.syn = 0;
            segment.seq_num = currentSeqNum;
        }
        currentSeqNum += payloadSize;

        segment.ack_num = currentAckNum;
        segment.flags.ack = 0;

        if(remainingData <= 1460){
            segment.flags.fin = 1;
        }else{
            segment.flags.fin = 0;
        }
        segment.payload = (uint8_t*)(dataStream + dataIndex);
        segment.options = NULL;
        segment.window = windowSize;
        segment.checksum = calculateChecksum(segment);
        
        segment.data_offset = 20;
        
        
        segments[i] = segment;
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