#pragma once

#include <string>
#include <vector>
#include <atomic>

class Server {
private:
    struct ClientInfo {
        std::string name;
        std::string pipe;
    };
    
    std::vector<ClientInfo> clients;
    std::atomic<bool> running;
    const std::string server_pipe = "/tmp/chat_server";
    
    void process_connection(const std::string& name, const std::string& client_pipe);
    void process_message(const std::string& sender, const std::string& receiver, const std::string& text);
    void process_disconnect(const std::string& name);
    void send_to_client(const std::string& client_pipe, const std::string& message);
    void broadcast(const std::string& sender, const std::string& message);
    void cleanup();
    
public:
    Server();
    ~Server();
    
    void start();
    void stop();
    bool is_running() const { return running; }
};