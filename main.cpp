#include <iostream>
#include <string>
#include <algorithm>
#include <csignal>

#include "common.h"
#include "server.h"
#include "client.h"


void signal_handler(int signal) {
    std::cout << "\nЗавершение работы..." << std::endl;
    g_running = false;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    
    std::cout << "========================================" << std::endl;
    std::cout << "     ПРОСТОЙ МЕССЕНДЖЕР НА PIPE        " << std::endl;
    std::cout << "========================================" << std::endl;
    
    if (argc > 1 && std::string(argv[1]) == "--server") {
        Server server;
        server.start();
    } else {
        std::string username;
        if (argc > 1) {
            username = argv[1];
        } else {
            std::cout << "Введите ваше имя: ";
            std::getline(std::cin, username);
        }
        
        if (username.empty()) {
            std::cout << "Имя не может быть пустым!" << std::endl;
            return 1;
        }
        
        std::string lower_name = username;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        
        if (lower_name == "all" || lower_name == "server") {
            std::cout << "Имя '" << username << "' зарезервировано!" << std::endl;
            return 1;
        }
        
        Client client(username);
        client.start();
    }
    
    return 0;
}