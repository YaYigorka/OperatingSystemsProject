#pragma once

#include <string>
#include <atomic>

class Client {
private:
    std::string username;
    std::atomic<bool> running;
    std::atomic<bool> connected;
    
    const std::string server_pipe = "/tmp/chat_server";
    std::string client_pipe;
    
    bool connect_to_server();
    void disconnect_from_server();
    void receive_messages();
    void process_input();
    void cleanup();
    
public:
    Client(const std::string& username);
    ~Client();
    
    void start();
    void stop();
    bool is_connected() const { return connected; }
};