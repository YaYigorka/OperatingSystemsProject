#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"

std::atomic<bool> g_running(true);

bool create_pipe(const std::string& pipe_name) {
    unlink(pipe_name.c_str());
    return mkfifo(pipe_name.c_str(), 0666) == 0;
}

bool send_to_pipe(const std::string& pipe_name, const std::string& message) {
    int fd = open(pipe_name.c_str(), O_WRONLY);
    if (fd < 0) return false;
    
    std::string msg = message + "\n";
    bool success = write(fd, msg.c_str(), msg.size()) > 0;
    close(fd);
    return success;
}