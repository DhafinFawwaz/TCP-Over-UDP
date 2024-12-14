#ifndef socket_h
#define socket_h

#include <string>
#include <functional>
#include <segment.hpp>
#include <segment_handler.hpp>
#include <arpa/inet.h>
#include <tcp_status_enum.hpp>
#include <connection_info.hpp>
#include <queue>
#include <map>
#include <utility>
#include <thread>

using namespace std;

class TCPSocket
{
    // todo add tcp connection state?
private:
    /**
     * The ip address and port for the socket instance
     * Not to be confused with ip address and port of the connected connection
     */
    string ip;
    int32_t port;

    string connected_ip;
    int32_t connected_port;

    /**
     * Socket descriptor
     */
    int32_t socket;


    SegmentHandler segment_handler;
    ssize_t recvAny(void* buffer, uint32_t length, sockaddr_in* addr, socklen_t* port);
    void sendAny(const char* ip, int32_t port, void* dataStream, uint32_t dataSize);
    void sendAddr(sockaddr_in& addr, void* dataStream, uint32_t dataSize);

    void fin_send(const char* ip, int32_t port);
    void fin_recv(sockaddr_in* addr, socklen_t* len);
    bool isSocketBinded(int socket_fd);

    uint32_t calculateSegmentIndex(uint32_t seq_num, uint32_t initial_seq_num);

    void recvHeaderLoop();
    vector<thread> recv_threads;
    map<sockaddr_in, queue<Segment>, SockaddrInCompare> received_buffer_queue; // hostport: queue<Segment>
    string toHostPort(sockaddr_in addr);

    Segment pop_in_queue_with_flag(uint8_t flag);
    bool pop_in_queue_with_flag(Segment& segment, sockaddr_in& clientAddr, uint8_t flag);
    bool pop_in_queue_with_condition(Segment& segment, sockaddr_in& clientAddr, function<bool(Segment, sockaddr_in)> condition);
public:
    map<int, ConnectionInfo> connection_map;

    TCPStatusEnum status = TCPStatusEnum::CLOSING;
    const uint32_t PAYLOAD_SIZE = 1460;
    const uint32_t SEGMENT_ONLY_SIZE = sizeof(Segment);
    const uint32_t MAX_SEGMENT_SIZE = PAYLOAD_SIZE + SEGMENT_ONLY_SIZE;

    TCPSocket(string& ip, int32_t port);
    void listen();
    void connect(string& server_ip, int32_t server_port);
    int accept(sockaddr_in* addr, socklen_t* port);
    
    void send(int client_socket, void* dataStream, uint32_t dataSize);
    ssize_t recv(void* buffer, uint32_t length, sockaddr_in* addr, socklen_t* port);
    void close(int client_socket);

    string getFormattedStatus();
    string getConnectedIP();
    int32_t getConnectedPort();
};

#endif