#ifndef connection_info_h
#define connection_info_h

#include <string>
#include <functional>
#include <segment.hpp>
#include <segment_handler.hpp>
#include <arpa/inet.h>
#include <tcp_status_enum.hpp>
#include <queue>
#include <map>
#include <utility>

using namespace std;


class ConnectionInfo {
public:
    string get_ip();
    int32_t get_port();
    string get_host_port();

    sockaddr_in addr;
    bool is_packet_received;
    TCPStatusEnum status;
    SegmentHandler segment_handler;
    ConnectionInfo();
    ConnectionInfo(sockaddr_in addr);
    ConnectionInfo(string& server, int32_t port);
    string getFormattedStatus();
};

struct SockaddrInCompare {
    bool operator()(const sockaddr_in &a, const sockaddr_in &b) const;
};

#endif