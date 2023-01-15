#pragma once
#include <foxy.h>

bool APIENTRY DllMain(HMODULE dllHandle, DWORD reason, LPVOID reserved);
void mainLoop();

extern "C" __declspec(dllexport) bool checkIfRunning();
void quitMainLoop();
