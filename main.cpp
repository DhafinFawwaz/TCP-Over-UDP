#include <iostream>
#include <string>

#include <client.hpp>
#include <server.hpp>

using namespace std;

void cinLowercase(string& str) {
    cin >> str; for(auto& c : str) c = tolower(c);
}
void readFileContent(const char* filePath, char* buffer) {
    FILE* file = fopen(filePath, "r");
    if(file == NULL) {
        cout << "[-] File not found" << endl;
        return;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    fread(buffer, 1, fileSize, file);
    fclose(file);
}

int main(int argc, char *argv[]) {
    if(argc < 2 || argc > 3) {
        cout << "Usage: ./node [host] [port]" << endl;
        return 1;
    }

    char* host;
    char* port;

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
        cout << "Input: " << endl;

        string sendingMode; cinLowercase(sendingMode);

        cout << endl;
        if(sendingMode == "1" || sendingMode == "user input") {
            cout << "[+] Input mode chosen, please enter your input: ";
            char* input; cin >> input;

            cout << "[+] User input has been successfully received." << endl;
            cout << "[i] Listening to the broadcast port for clients." << endl;

            Client client(host, port);
            client.handleMessage(input);
            client.run();
            
        }
        else if(sendingMode == "2" || sendingMode == "file input") {
            cout << "[+] File mode chosen, please enter the file path: ";
            char* filePath; cin >> filePath;
            char* buffer;
            readFileContent(filePath, buffer);

            cout << "[+] File has been successfully read." << endl;
            cout << "[i] Listening to the broadcast port for clients." << endl;

            Client client(host, port);
            client.handleMessage(buffer);
            client.run();
        }
        else cout << "[-] Invalid mode" << endl;
    }
    else if(mode == "2" || mode == "receiver") {
        cout << "[+] Node is now a receiver" << endl;
        cout << "[?] Input the server program's port: ";
        char* serverPort; cin >> serverPort;

        cout << "[+] Trying to contact the sender at " << host << ":" << serverPort << endl;
        
        Server server(host, serverPort);
        // server.handleMessage();
        server.run();
    }
    else cout << "[-] Invalid mode" << endl;

    return 0;
}