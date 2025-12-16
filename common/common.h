#pragma once

#include <string>
#include <atomic>

extern std::atomic<bool> g_running;

bool create_pipe(const std::string& pipe_name);
bool send_to_pipe(const std::string& pipe_name, const std::string& message);