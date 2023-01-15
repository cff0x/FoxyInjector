#include "launcher.h"
#include <fast-lzma2.h>
#include <cxxopts.hpp>

typedef NTSTATUS(WINAPI* lpNtCreateThreadEx)(
	OUT		PHANDLE				hThread,
	IN		ACCESS_MASK			DesiredAccess,
	IN		LPVOID				ObjectAttributes,
	IN		HANDLE				ProcessHandle,
	IN		LPVOID				lpStartAddress,
	IN		LPVOID				lpParameter,
	IN		ULONG				CreateSuspended,
	IN		SIZE_T				StackZeroBits,
	IN		SIZE_T				SizeOfStackCommit,
	IN		SIZE_T				SizeOfStackReserve,
	OUT		LPVOID				lpBytesBuffer
);

bool injectViaCreateRemoteThread(std::string dllPath, HANDLE processHandle, HANDLE processThreadHandle = nullptr) {
	std::filesystem::path fullDllPath{ dllPath };
	if (!std::filesystem::exists(fullDllPath)) {
		std::cerr << "Could not find library at \"" << std::filesystem::absolute(fullDllPath).string() << "\"!\n";
		return false;
	}

	std::cout << "-> Injecting library file \"" << std::filesystem::absolute(fullDllPath).string() << "\" into remote process...\n";

	size_t dllPathSize = std::filesystem::absolute(fullDllPath).string().length() + 1;

	// allocate a memory page for our LoadLibrary call
	std::cout << "-> Allocating memory in target process...\n";
	void* page = VirtualAllocEx(processHandle, nullptr, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (page == nullptr) {
		std::cerr << "Failed to allocate memory! Error code: 0x" << std::hex << GetLastError() << "\n";
		return false;
	}

	// write our module library path into the memory page we allocated before
	std::cout << "-> Writing library path into allocated memory...\n";
	if (WriteProcessMemory(processHandle, page, std::filesystem::absolute(fullDllPath).string().c_str(), dllPathSize, nullptr) == 0) {
		std::cerr << "Failed to write library path into the target process memory! Error code: 0x" << std::hex << GetLastError() << "\n";
		return false;
	}

	// inject our module library into the target process via LoadLibrary
	LPTHREAD_START_ROUTINE addrLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(LoadLibraryA("kernel32"), "LoadLibraryA");
	std::cout << "-> Found LoadLibraryA at 0x" << std::hex << addrLoadLibrary << "\n";
	
	// get ntdll.dll module handle
	HMODULE ntdllModule = GetModuleHandle("ntdll.dll");
	if (!ntdllModule) {
		std::cerr << "Failed to find module handle for ntdll.dll!\n";
		return false;
	}
	std::cout << "-> Found ntdll.dll at 0x" << std::hex << ntdllModule << "\n";
	
	// retrieve NtCreateThreadEx function address from ntdll.dll
	lpNtCreateThreadEx ntCreateThreadEx = (lpNtCreateThreadEx)GetProcAddress(ntdllModule, "NtCreateThreadEx");
	if (!ntCreateThreadEx) {
		std::cerr << "Failed to find function address for NtCreateThreadEx!\n";
		return false;
	}
	std::cout << "-> Found NtCreateThreadEx at 0x" << std::hex << ntCreateThreadEx << "\n";

	// create remote thread for LoadLibraryA via NtCreateThreadEx
	std::cout << "-> Creating remote thread in target process to call LoadLibraryA...\n";
	HANDLE remoteThreadHandle;
	NTSTATUS status = ntCreateThreadEx(&remoteThreadHandle,	THREAD_ALL_ACCESS, nullptr,	processHandle,
		(LPTHREAD_START_ROUTINE)addrLoadLibrary,	page, NULL, 0, 0, 0, nullptr);
	if (remoteThreadHandle == nullptr) {
		std::cerr << "Failed to create remote thread to call LoadLibrary! Error code: 0x" << std::hex << GetLastError() << "\n";
		return false;
	}

	// now we wait until DllMain from our module returns
	std::cout << "-> Waiting for execution of DllMain function...\n";
	if (WaitForSingleObject(remoteThreadHandle, INFINITE) == WAIT_FAILED) {
		std::cerr << "Failed to wait for DllMain execution! Error code: 0x" << std::hex << GetLastError() << "\n";
		return false;
	}

	// clean up
	std::cout << "-> Library injected successfully!\n";
	CloseHandle(remoteThreadHandle);

	return true;
	
}

int main(int argc, char** argv) {
	// command-line options
	cxxopts::Options options("FoxyInjector", "DLL injector");
	options.add_options()
		("launch", "Path of the executable file to launch", cxxopts::value<std::string>(), "Path")
		("pid", "PID of the process in which we want to inject our library", cxxopts::value<int>(), "PID")
		("name", "Name of the process in which we want to inject our library", cxxopts::value <std::string>(), "Name")
		("library", "Library file(s) to inject (separate each by commma)", cxxopts::value<std::vector<std::string>>(), "Path")
		("self", "Set this flag to inject the library into the launcher instead of another process")
		("help", "Print usage information");
	auto result = options.parse(argc, argv);

	if (result.count("help")) {
		std::cout << options.help() << "\n";
		exit(0);
	}

	// make sure only one of the target process options is given
	if (result.count("pid") && (result.count("launch") || result.count("name") || result.count("self")) ||
		result.count("launch") && (result.count("pid") || result.count("name") || result.count("self")) ||
		result.count("name") && (result.count("launch") || result.count("pid") || result.count("self")) ||
		result.count("self") && (result.count("launch") || result.count("pid") || result.count("name"))) {
		std::cout << "You may only use one of the options --launch, --pid, --name or --self!\n";
		std::cout << options.help() << "\n";
		exit(0);
	}

	// variables used to hold our handles
	HANDLE processThreadHandle = nullptr;
	HANDLE processHandle = nullptr;

	if (result.count("pid")) {
		std::cout << "-> Opening process with PID " << result["pid"].as<std::string>() << "...\n";
		processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, result["pid"].as<int>());
	} else if (result.count("name")) {
		std::cout << "-> Opening process named \"" << result["name"].as<std::string>() << "... (press CTRL+C to cancel)\n";

		// variable to hold process information
		PROCESSENTRY32 entry{};
		entry.dwSize = sizeof(PROCESSENTRY32);

		// we will search until we have found a match or you exited
		while (processHandle == nullptr) {
			// get a snapshot of the currently running processes
			HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

			// lets see if we can find a match
			if (Process32First(snapshot, &entry) == true)
			{
				while (Process32Next(snapshot, &entry) == true)
				{
					if (StringUtils::toLower(result["name"].as<std::string>()).compare(entry.szExeFile) == 0)
					{
						std::cout << "\n-> Found PID: " << (int)entry.th32ProcessID << "!\n";

						// we found or handle! now we can try to use it for injecting our module
						processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
						break;
					}
				}
			}

			// wanna indicate some sort of progress, idk
			std::cout << ".";

			// clean up
			CloseHandle(snapshot);
		}
	} else if (result.count("launch")) {
		STARTUPINFOA si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		si.cb = sizeof(STARTUPINFO);

		std::cout << "-> Launching \"" << result["launch"].as<std::string>() << "\" in suspended state...\n";

		if (!CreateProcessA(result["launch"].as<std::string>().c_str(), nullptr, nullptr, nullptr, false, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) {
			std::cerr << "Failed to create process! Error code: 0x" << std::hex << GetLastError() << "\n";
			return 1;
		}

		// set process handle value
		processHandle = pi.hProcess;
		processThreadHandle = pi.hThread;
	} else if (result.count("self")) {
		std::cout << "-> Injecting library into this launcher...\n";
		processHandle = GetCurrentProcess();
	} else {
		std::cerr << "You must specify a target process in which to inject the library!\n";
		std::cout << options.help() << "\n";
		exit(1);
	}

	// inject each library
	for (auto& lib : result["library"].as<std::vector<std::string>>()) {
		if (!injectViaCreateRemoteThread(lib, processHandle, processThreadHandle)) {
			std::cerr << "Failed to inject DLL \"" << lib << "\" into target process! Exiting...";
			exit(1);
		}
	}
	

	std::cout << "-> Done! Cleaning up...\n";

	// resume our target process now (only if we created our own process)
	if (processThreadHandle != nullptr) {
		std::cout << "-> Resuming target process thread...\n";
		if (ResumeThread(processThreadHandle) == -1) {
			std::cerr << "Failed to resume target process! Error code: 0x" << std::hex << GetLastError() << "\n";
			return 1;
		}

		CloseHandle(processThreadHandle);
	}

	// close process handle
	if (processHandle != nullptr) {
		CloseHandle(processHandle);
	}


	return 0;
}