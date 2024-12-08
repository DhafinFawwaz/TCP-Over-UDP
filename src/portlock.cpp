#include "portlock.hpp"
#include <fcntl.h>  
#include <sys/file.h> 
#include <unistd.h>    
#include <iostream>

PortLock::PortLock(int port) : port(port), lock_fd(-1), locked(false) {
    lock_file = "/tmp/port_" + std::to_string(port) + ".lock";

    lock_fd = open(lock_file.c_str(), O_CREAT | O_RDWR, 0666);
    if (lock_fd == -1) {
        std::cerr << "[-] Failed to create/open lock file: " << lock_file << std::endl;
        return;
    }

    if (flock(lock_fd, LOCK_EX | LOCK_NB) == -1) {
        close(lock_fd);
        lock_fd = -1;
        return;
    }

    locked = true;
}

PortLock::~PortLock() {
    if (locked) {
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        locked = false;
    }
}

bool PortLock::isLocked() const {
    return locked;
}
