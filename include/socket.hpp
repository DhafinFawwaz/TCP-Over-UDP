#ifndef socket_h
#define socket_h

#include <string>
#include <functional>
#include <segment.hpp>
#include <segment_handler.hpp>
#include <arpa/inet.h>

using namespace std;

// for references
// https://maxnilz.com/docs/004-network/003-tcp-connection-state/
// You can use a different state enum if you'd like
enum TCPStatusEnum
{
    LISTEN = 0, // - represents waiting for a connection request from any remote TCP and port.
    SYN_SENT = 1, // - represents waiting for a matching connection request after having sent a connection request.
    SYN_RECEIVED = 2, // - represents waiting for a confirming connection request acknowledgment after having both received and sent a connection request.
    ESTABLISHED = 3, // - represents an open connection, data received can be delivered to the user. The normal state for the data transfer phase of the connection.
    FIN_WAIT_1 = 4, // - represents waiting for a connection termination request from the remote TCP, or an acknowledgment of the connection termination request previously sent.
    FIN_WAIT_2 = 5, // - represents waiting for a connection termination request from the remote TCP.
    CLOSE_WAIT = 6, // - represents waiting for a connection termination request from the local user.
    CLOSING = 7, // - represents waiting for a connection termination request acknowledgment from the remote TCP.
    LAST_ACK = 8, // - represents waiting for an acknowledgment of the connection termination request previously sent to the remote TCP (which includes an acknowledgment of its connection termination request).
    TIME_WAIT = 9, // - represents waiting for enough time to pass to be sure the remote TCP received the acknowledgment of its connection termination request.
    CLOSED = 10, // - represents no connection state at all, it is fictional because it represents the state when there is no TCB, and therefore, no connection
    FAILED = 11,
};



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


    TCPStatusEnum status = TCPStatusEnum::CLOSING;

    SegmentHandler segment_handler;
    void initSocket();
    int32_t recvAny(void* buffer, uint32_t length, sockaddr_in* addr, socklen_t* port);
    void sendAny(const char* ip, int32_t port, void* dataStream, uint32_t dataSize);

    void fin_send(const char* ip, int32_t port);
    void fin_recv(sockaddr_in* addr, socklen_t* len);
    bool isSocketBinded(int socket_fd);

    uint32_t calculateSegmentIndex(uint32_t seq_num, uint32_t initial_seq_num);

public:
    const uint32_t PAYLOAD_SIZE = 1460;
    const uint32_t SEGMENT_ONLY_SIZE = sizeof(Segment);
    const uint32_t MAX_SEGMENT_SIZE = PAYLOAD_SIZE + SEGMENT_ONLY_SIZE;

    TCPSocket(string& ip, int32_t port);
    void listen();
    void connect(string& server_ip, int32_t server_port);
    void send(const char* ip, int32_t port, void* dataStream, uint32_t dataSize);
    int32_t recv(void* buffer, uint32_t length, sockaddr_in* addr, socklen_t* port);
    void close();

    string getFormattedStatus();
    string getConnectedIP();
    int32_t getConnectedPort();
};

#endif