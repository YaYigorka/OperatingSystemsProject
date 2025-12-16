#include <iostream>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

#include "client.h"
#include "common.h"


Client::Client(const std::string& username) 
    : username(username), running(false), connected(false) {
    
    client_pipe = "/tmp/chat_client_" + std::to_string(getpid());
}

Client::~Client() {
    stop();
}

bool Client::connect_to_server() {
    if (!create_pipe(client_pipe)) {
        std::cerr << "Ошибка создания клиентского pipe" << std::endl;
        return false;
    }
    
    if (access(server_pipe.c_str(), F_OK) == -1) {
        std::cerr << "Сервер не запущен! Запустите сначала сервер." << std::endl;
        unlink(client_pipe.c_str());
        return false;
    }
    
    if (!send_to_pipe(server_pipe, "JOIN:" + username + ":" + client_pipe)) {
        std::cerr << "Не удалось подключиться к серверу" << std::endl;
        unlink(client_pipe.c_str());
        return false;
    }
    
    connected = true;
    return true;
}

void Client::disconnect_from_server() {
    if (connected) {
        send_to_pipe(server_pipe, "LEAVE:" + username);
        connected = false;
    }
}

void Client::cleanup() {
    unlink(client_pipe.c_str());
}

void Client::receive_messages() {
    int client_fd = open(client_pipe.c_str(), O_RDONLY | O_NONBLOCK);
    if (client_fd < 0) {
        std::cerr << "Ошибка открытия клиентского pipe" << std::endl;
        return;
    }
    
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = client_fd;
    fds[1].events = POLLIN;
    
    char buffer[1024];
    
    while (running && connected && g_running) {
        int ready = poll(fds, 2, 100);
        
        if (ready > 0) {
            if (fds[1].revents & POLLIN) {
                ssize_t bytes = read(client_fd, buffer, sizeof(buffer) - 1);
                if (bytes > 0) {
                    buffer[bytes] = '\0';
                    std::string message(buffer);
                    
                    if (!message.empty() && message.back() == '\n') {
                        message.pop_back();
                    }
                    
                    if (!message.empty()) {
                        size_t colon = message.find(':');
                        if (colon != std::string::npos) {
                            std::string sender = message.substr(0, colon);
                            std::string text = message.substr(colon + 1);
                            
                            if (sender == "SERVER") {
                                std::cout << "\n[СЕРВЕР]: " << text << std::endl;
                            } else {
                                std::cout << "\n[" << sender << "]: " << text << std::endl;
                            }
                            std::cout << "> " << std::flush;
                        }
                    }
                }
            }
            
            if (fds[0].revents & POLLIN) {
                process_input();
            }
        } else if (ready < 0) {
            break;
        }
    }
    
    close(client_fd);
}

void Client::process_input() {
    std::string input;
    std::getline(std::cin, input);
    
    input.erase(0, input.find_first_not_of(" \t"));
    input.erase(input.find_last_not_of(" \t") + 1);
    
    if (input.empty()) {
        return;
    }
    
    if (input == "/exit" || input == "/quit") {
        std::cout << "Выход из чата..." << std::endl;
        stop();
        return;
    }
    
    if (input == "/help") {
        std::cout << "\nПомощь:" << std::endl;
        std::cout << "  @имя сообщение  - отправить личное сообщение" << std::endl;
        std::cout << "  @all сообщение  - отправить сообщение всем" << std::endl;
        std::cout << "  /exit           - выйти из чата" << std::endl;
        std::cout << "  /help           - показать эту справку" << std::endl;
        return;
    }
    
    if (input[0] == '@') {
        size_t space = input.find(' ');
        if (space != std::string::npos && space > 1) {
            std::string receiver = input.substr(1, space - 1);
            std::string text = input.substr(space + 1);
            
            if (!text.empty()) {
                std::string actual_receiver = receiver;
                std::transform(receiver.begin(), receiver.end(), receiver.begin(), ::tolower);
                
                if (receiver == "all") {
                    actual_receiver = "ALL";
                }
                
                if (send_to_pipe(server_pipe, "MSG:" + username + ":" + actual_receiver + ":" + text)) {
                    if (actual_receiver == "ALL") {
                        std::cout << "Сообщение отправлено всем" << std::endl;
                    } else {
                        std::cout << "Сообщение отправлено " << actual_receiver << std::endl;
                    }
                } else {
                    std::cout << "Ошибка отправки сообщения" << std::endl;
                }
            } else {
                std::cout << "Сообщение не может быть пустым" << std::endl;
            }
        } else {
            std::cout << "Используйте: @получатель сообщение" << std::endl;
        }
    } else {
        std::cout << "Неизвестная команда. Введите /help для помощи" << std::endl;
    }
}

void Client::start() {
    if (running) return;
    
    std::cout << "=== Запуск клиента: " << username << " ===" << std::endl;
    
    if (!connect_to_server()) {
        return;
    }
    
    running = true;
    
    std::cout << "\nПодключено к чату!" << std::endl;
    std::cout << "Команды:" << std::endl;
    std::cout << "  @имя сообщение  - личное сообщение" << std::endl;
    std::cout << "  @all сообщение  - сообщение всем" << std::endl;
    std::cout << "  /exit           - выйти" << std::endl;
    std::cout << "  /help           - помощь" << std::endl;
    std::cout << std::endl;
    
    receive_messages();
    
    std::cout << "Клиент завершен" << std::endl;
}

void Client::stop() {
    if (!running) return;
    
    running = false;
    disconnect_from_server();
    cleanup();
}