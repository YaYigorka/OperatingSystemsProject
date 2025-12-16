#include <iostream>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "server.h"
#include "common.h"


Server::Server() : running(false) {}

Server::~Server() {
    stop();
}

void Server::send_to_client(const std::string& client_pipe, const std::string& message) {
    send_to_pipe(client_pipe, message);
}

void Server::broadcast(const std::string& sender, const std::string& message) {
    for (const auto& client : clients) {
        if (client.name != sender) {
            send_to_client(client.pipe, message);
        }
    }
}

void Server::process_connection(const std::string& name, const std::string& client_pipe) {
    clients.push_back({name, client_pipe});
    std::cout << "Подключился: " << name << std::endl;
    
    send_to_client(client_pipe, "SERVER:Добро пожаловать в чат!");
    
    for (const auto& client : clients) {
        if (client.name != name) {
            send_to_client(client.pipe, "SERVER:Пользователь " + name + " присоединился к чату");
        }
    }
}

void Server::process_message(const std::string& sender, const std::string& receiver, const std::string& text) {
    std::cout << "[" << sender << " -> " << receiver << "]: " << text << std::endl;
    
    if (receiver == "ALL") {
        for (const auto& client : clients) {
            if (client.name != sender) {
                send_to_client(client.pipe, sender + ":" + text);
            }
        }
    } else {
        for (const auto& client : clients) {
            if (client.name == receiver) {
                send_to_client(client.pipe, sender + ":" + text);
                break;
            }
        }
    }
}

void Server::process_disconnect(const std::string& name) {
    auto it = std::remove_if(clients.begin(), clients.end(),
        [&name](const ClientInfo& c) { return c.name == name; });
    clients.erase(it, clients.end());
    
    std::cout << "Отключился: " << name << std::endl;
    
    for (const auto& client : clients) {
        send_to_client(client.pipe, "SERVER:Пользователь " + name + " покинул чат");
    }
    
    unlink(("/tmp/chat_client_" + name).c_str());
}

void Server::cleanup() {
    for (const auto& client : clients) {
        unlink(client.pipe.c_str());
    }
    clients.clear();
}

void Server::start() {
    if (running) return;
    
    std::cout << "=== Запуск сервера ===" << std::endl;
    
    if (!create_pipe(server_pipe)) {
        std::cerr << "Ошибка создания серверного pipe" << std::endl;
        return;
    }
    
    std::cout << "Серверный pipe: " << server_pipe << std::endl;
    std::cout << "Ожидание подключений... (Ctrl+C для выхода)" << std::endl;
    
    int server_fd = open(server_pipe.c_str(), O_RDONLY);
    if (server_fd < 0) {
        std::cerr << "Ошибка открытия серверного pipe" << std::endl;
        return;
    }
    
    running = true;
    
    char buffer[1024];
    
    while (running && g_running) {
        ssize_t bytes = read(server_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string message(buffer);
            
            if (!message.empty() && message.back() == '\n') {
                message.pop_back();
            }
            
            if (message.empty()) continue;
            
            size_t colon1 = message.find(':');
            if (colon1 == std::string::npos) continue;
            
            std::string command = message.substr(0, colon1);
            std::string params = message.substr(colon1 + 1);
            
            if (command == "JOIN") {
                size_t colon2 = params.find(':');
                if (colon2 != std::string::npos) {
                    std::string name = params.substr(0, colon2);
                    std::string client_pipe = params.substr(colon2 + 1);
                    process_connection(name, client_pipe);
                }
            } else if (command == "MSG") {
                size_t colon2 = params.find(':');
                size_t colon3 = params.find(':', colon2 + 1);
                
                if (colon2 != std::string::npos && colon3 != std::string::npos) {
                    std::string sender = params.substr(0, colon2);
                    std::string receiver = params.substr(colon2 + 1, colon3 - colon2 - 1);
                    std::string text = params.substr(colon3 + 1);
                    process_message(sender, receiver, text);
                }
            } else if (command == "LEAVE") {
                std::string name = params;
                process_disconnect(name);
            }
        } else if (bytes == 0) {
            usleep(100000);
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                break;
            }
        }
    }
    
    close(server_fd);
    unlink(server_pipe.c_str());
    cleanup();
    
    std::cout << "Сервер остановлен" << std::endl;
}

void Server::stop() {
    running = false;
}