#include <node.hpp>
#include <iostream>
#include <unistd.h>

Node::Node(string& host, int port) : host(host), port(port), connection((char*)host.c_str(), port) {}
