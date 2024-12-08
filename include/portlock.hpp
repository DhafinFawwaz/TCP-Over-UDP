#ifndef PORTLOCK_HPP
#define PORTLOCK_HPP

#include <string>

class PortLock {
public:
    PortLock(int port);
    ~PortLock();

    bool isLocked() const;

private:
    int port;
    int lock_fd;
    std::string lock_file;
    bool locked;
};

#endif
