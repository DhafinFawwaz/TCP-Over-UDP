#include <node.hpp>
#include <iostream>
#include <unistd.h>

Node::Node(string& host, int port) : host(host), port(port), connection(host, port) {}
