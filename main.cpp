#include <iostream>
#include <string>
#include <client.hpp>
#include <server.hpp>
#include <fstream>

using namespace std;

void cinLowercase(string& str) {
    cin >> str; for(auto& c : str) c = tolower(c);
}

string readFileContent(string& filePath) {
    ifstream ifs(filePath);
    if(!ifs) {
        cout << "[-] File not found" << endl;
        exit(1);
    }
    return string(std::istreambuf_iterator<char>(ifs >> std::skipws), std::istreambuf_iterator<char>());
}

int main(int argc, char *argv[]) {
    if(argc < 2 || argc > 3) {
        cout << "Usage: ./node [host] [port]" << endl;
        return 1;
    }

    string host;
    string port;

    if(argc == 2 && atoi(argv[1])) {
        host = "localhost";
        port = argv[1];
    } else if(argc == 3) {
        host = argv[1];
        port = argv[2];
    } else {
        cout << "Usage: ./node [host] [port]" << endl;
    }

    cout << "[i] Node started at " << host << ":" << port << endl;
    cout << "[?] Please choose the operating mode" << endl;
    cout << "[?] 1. Sender" << endl;
    cout << "[?] 2. Receiver" << endl;
    cout << "[?] Input: ";

    string mode; cinLowercase(mode);

    cout << endl;
    if(mode == "1" || mode == "sender") {
        cout << "[+] Node is now a sender" << endl;
        cout << "[?] Please choose the sending mode" << endl;
        cout << "[?] 1. User input" << endl;
        cout << "[?] 2. File input" << endl;
        cout << "Input: ";

        string sendingMode; cinLowercase(sendingMode);

        cout << endl;
        if(sendingMode == "1" || sendingMode == "user input") {
            cout << "[+] Input mode chosen, please enter your input: ";
            string input; cin >> input;

            cout << "[+] User input has been successfully received." << endl;
            cout << "[i] Listening to the broadcast port for clients." << endl;

            Server server(host, stoi(port));
            server.SetResponseBuffer(input);
            server.run();
        }
        else if(sendingMode == "2" || sendingMode == "file input") {
            cout << "[+] File mode chosen, please enter the file path: ";
            string filePath; cin >> filePath;
            string buffer = readFileContent(filePath);

            cout << "[+] File has been successfully read." << endl;
            cout << "[i] Listening to the broadcast port for clients." << endl;

            Server server(host, stoi(port));
            server.SetResponseBuffer(buffer);
            server.run();
        }
        else cout << "[-] Invalid mode" << endl;
    }
    else if(mode == "2" || mode == "receiver") {
        cout << "[+] Node is now a receiver" << endl;
        cout << "[?] Input the server program's port: ";
        string serverPort; cin >> serverPort;

        cout << "[+] Trying to contact the sender at " << host << ":" << serverPort << endl;
        
        Client client(host, stoi(port));
        client.setServerTarget(host, stoi(serverPort));
        client.run();
    }
    else cout << "[-] Invalid mode" << endl;

    return 0;
}