#include <socket.hpp>
#include <unistd.h> 
#include <ansi_code.hpp>
#include <iostream>
#include <string.h> // just for memcpy
#include <chrono>
#include <unordered_map>
#include <deque>
#include <algorithm>
#include <map>
#include <tuple>
#include <utility> // for pair
#include <fcntl.h>
#include "connection_info.hpp"
// #include <cstring> //memset

using namespace std;
using namespace std::chrono;


ConnectionInfo::ConnectionInfo() : is_packet_received(false), status(TCPStatusEnum::CLOSED) {}
ConnectionInfo::ConnectionInfo(sockaddr_in addr) : addr(addr), is_packet_received(false), status(TCPStatusEnum::CLOSED) {}
ConnectionInfo::ConnectionInfo(string& server, int32_t port): is_packet_received(false), status(TCPStatusEnum::CLOSED) {
    addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(server.c_str());
    addr.sin_port = htons(port);
}

string ConnectionInfo::get_ip() {
    return string(inet_ntoa(addr.sin_addr));
}

int32_t ConnectionInfo::get_port(){
    return addr.sin_port;   
}

string ConnectionInfo::get_host_port(){
    return string(inet_ntoa(addr.sin_addr)) + ":" + to_string(ntohs(addr.sin_port));
}
string ConnectionInfo::getFormattedStatus() {
    static const std::unordered_map<TCPStatusEnum, std::string> statusMap = {
        {TCPStatusEnum::LISTEN, "LISTEN"},
        {TCPStatusEnum::SYN_SENT, "SYN_SENT"},
        {TCPStatusEnum::SYN_RECEIVED, "SYN_RECEIVED"},
        {TCPStatusEnum::ESTABLISHED, "ESTABLISHED"},
        {TCPStatusEnum::FIN_WAIT_1, "FIN_WAIT_1"},
        {TCPStatusEnum::FIN_WAIT_2, "FIN_WAIT_2"},
        {TCPStatusEnum::CLOSE_WAIT, "CLOSE_WAIT"},
        {TCPStatusEnum::CLOSING, "CLOSING"},
        {TCPStatusEnum::LAST_ACK, "LAST_ACK"},
        {TCPStatusEnum::TIME_WAIT, "TIME_WAIT"},
        {TCPStatusEnum::CLOSED, "CLOSED"},
        {TCPStatusEnum::FAILED, "FAILED"},
    };

    std::string statusString = "[";
    statusString += std::to_string(this->status);
    statusString += "] ";
    statusString += "[" + statusMap.at(this->status) + "]";
    return statusString;
}


bool SockaddrInCompare::operator()(const struct sockaddr_in& a, const struct sockaddr_in& b) const {
    if (a.sin_addr.s_addr != b.sin_addr.s_addr) {
        return a.sin_addr.s_addr < b.sin_addr.s_addr;
    }
    
    return ntohs(a.sin_port) < ntohs(b.sin_port);
}