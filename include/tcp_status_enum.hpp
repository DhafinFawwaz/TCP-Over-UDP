#ifndef tcp_status_enum_h
#define tcp_status_enum_h

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

#endif
