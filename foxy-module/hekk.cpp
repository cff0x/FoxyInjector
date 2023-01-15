#include "hekk.h"
#include "console.h"

static bool isRunning = true;
static bool quitApplication = false;
static bool mainThreadIsDone = false;

static HMODULE dllHandle = nullptr;
static DWORD processId = 0;
static HANDLE processHandle = nullptr;

static std::thread mainThread;


bool APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved)
{
	switch (reason) {
	case DLL_PROCESS_ATTACH:

		// spawn console and setup pipes for read/write
		AllocConsole();
		SetConsoleTitle("HEKK!");
		
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);

		// store some runtime process informations
		dllHandle = handle;
		processId = GetCurrentProcessId();

		// open a process handle with full access
		processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
		std::cout << "-> Attached to process ID " << (int)processId << "! (Process Handle: " << std::hex << processHandle << ", DLL Handle: " << dllHandle << ")\n";

		// create main thread
		mainThread = std::thread{ mainLoop };

		break;
	case DLL_PROCESS_DETACH:

		FreeConsole();
		fclose(stdin);
		fclose(stdout);

		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	
	return true;
}

void mainLoop()
{
	std::cout << "-> Main thread is now running!\n\n";
	std::cout << "Enter a command here! Type \"help\" to get a list of available commands.\n";

	// start main loop
	while (isRunning) {
		updateConsole();

		Sleep(10);
	}

	// terminate thread when loop is over
	std::terminate();
}


extern "C" __declspec(dllexport) bool checkIfRunning()
{
	return isRunning;
}

void quitMainLoop()
{
	isRunning = false;
}

