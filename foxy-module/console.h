#pragma once
#include <foxy.h>

void musicThread();
void updateConsole();
bool consoleCommand_uwu(const std::string& cmd, void* args);
bool consoleCommand_test(const std::string& cmd, void* args);
bool consoleCommand_clear(const std::string& cmd, void* args);
bool consoleCommand_quit(const std::string& cmd, void* args);
bool consoleCommand_windowTest(const std::string& cmd, void* args);