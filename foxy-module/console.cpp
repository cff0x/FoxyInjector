#include "console.h"
#include "hekk.h"

#include <SDL.h>
#include <xmp.h>

#include "xm.h"

std::thread xmThread;
bool xmThreadRunning = false;
bool isAudioPaused = false;

static void fill_audio(void* udata, Uint8* stream, int len)
{
	if (xmp_play_buffer((xmp_context)udata, stream, len, 0) < 0)
		xmThreadRunning = false;
}

static int sdl_init(xmp_context ctx)
{
	SDL_AudioSpec a;

	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "sdl: can't initialize: %s\n", SDL_GetError());
		return -1;
	}

	a.freq = 44100;
	a.format = AUDIO_S16;
	a.channels = 2;
	a.samples = 2048;
	a.callback = fill_audio;
	a.userdata = ctx;

	if (SDL_OpenAudio(&a, NULL) < 0) {
		fprintf(stderr, "%s\n", SDL_GetError());
		return -1;
	}

	return 0;
}

static void sdl_deinit()
{
	SDL_CloseAudio();
}


// command function type
typedef bool(CommandFunction_t)(const std::string&, void*);
typedef CommandFunction_t* pCommandFunction;

// console commands
static const std::map<std::string, pCommandFunction> CONSOLE_COMMANDS =
{
	{ "test", consoleCommand_test },
	{ "clear", consoleCommand_clear },
	{ "quit", consoleCommand_quit },
	{ "setwindowpos", consoleCommand_windowTest },
	{ "uwu", consoleCommand_uwu }
};

// process console inputs
void updateConsole()
{
	// wait for user input from root
	std::string userInput;
	std::getline(std::cin, userInput);

	bool commandResult = false;
	bool commandFound = false;

	// handle input
	if (userInput.length() > 0)
	{

		// split command in parts (delimited by space)
		std::vector<std::string> arguments = StringUtils::splitString(userInput, ' ');
		std::string command = arguments[0];

		// remove first part (the actual command) from vector
		arguments.erase(arguments.begin());

		// look for the given command and execute it if existing
		for (const auto& kv : CONSOLE_COMMANDS) {
			if (kv.first != command)
				continue;

			// command has been found
			commandFound = true;

			// show error if command execution has been failed
			if (!kv.second(command, (arguments.size() == 0 ? nullptr : &arguments))) {
				std::cerr << "ERROR: command execution failed!\n";
			}
		}

		// show error if the command has not been found
		if (commandFound == false) {
			std::cerr << "ERROR: command not found!\n";
		}
	}

}

void musicThread()
{
	xmp_context ctx;
	struct xmp_module_info mi;
	struct xmp_frame_info fi;
	int i;

	ctx = xmp_create_context();

	if (sdl_init(ctx) < 0) {
		return;
	}

	xmp_load_module_from_memory(ctx, xmData, sizeof(xmData));

	if (xmp_start_player(ctx, 44100, 0) == 0) {

		/* Show module data */

		xmp_get_module_info(ctx, &mi);

		/* Play module */
		SDL_PauseAudio(0);

		while (true) {
			SDL_Delay(10);
			xmp_get_frame_info(ctx, &fi);

			if (isAudioPaused) {
				SDL_PauseAudio(1);

				while (isAudioPaused) {
					Sleep(10);
				}

				SDL_PauseAudio(0);
			}
		}
		xmp_end_player(ctx);
	}

	xmp_release_module(ctx);
}

bool consoleCommand_uwu(const std::string& cmd, void* args)
{
	if (!xmThreadRunning) {
		xmThreadRunning = true;
		xmThread = std::thread{ musicThread };

		std::cout << "UwU!!! :3\n";
		
	} else {

		isAudioPaused = !isAudioPaused;

		if (isAudioPaused) {
			std::cout << "QwQ...\n";
		} else {
			std::cout << "OwO OwO OwO!!!\n";
		}
	}
	
	
	return true;
}

bool consoleCommand_test(const std::string& cmd, void* args)
{
	std::cout << "You have executed the test command with the following input: " << cmd << ". Good job!\n";
	return true;
}

bool consoleCommand_clear(const std::string& cmd, void* args)
{
	std::cout << "Not yet implemented!\n";
	return true;
}

bool consoleCommand_quit(const std::string& cmd, void* args)
{
	bool confirmed = false;

	// make sure the user really wants to quit
	while (!confirmed) {
		std::cout << "Do you really want to exit?\n";
		std::cout << "All attached processes will be forcibly terminated! (y/n): ";
		
		std::string userInput;
		std::cin >> userInput;

		// compare user input
		userInput = StringUtils::toLower(userInput);
		if (userInput.compare("y") == 0) {
			confirmed = true;
		} else if (userInput.compare("n") == 0) {
			std::cout << "Aborting...\n";
			return true;
		} else {
			std::cout << "Invalid input!\n";
		}
	}

	// end process by stopping the thread
	quitMainLoop();
}

bool consoleCommand_windowTest(const std::string& cmd, void* args)
{
	std::vector<std::string>& arguments = *reinterpret_cast<std::vector<std::string>*>(args);
	std::cout << "command: " << cmd << "; arguments:\n";
	for (auto& arg : arguments) {
		std::cout << " - " << arg << "\n";
	}

	// arguments
	int posX, posY;
	int width, height;

	// usage: windowtest x y width height
	if (arguments.size() != 4) {
		std::cerr << "ERROR: wrong argument count! usage: windowtest x y width height\n";
		return false;
	}

	// set argument values
	posX = std::stoi(arguments[0]);
	posY = std::stoi(arguments[1]);
	width = std::stoi(arguments[2]);
	height = std::stoi(arguments[3]);

	// ask user for the window class
	std::string windowClass{};
	HWND selectedWindow = nullptr;
	while (windowClass.empty() || selectedWindow == nullptr) {
		std::cout << "-> Name of the window class to search for (or just press enter to abort): ";

		// get user input for searching the window (empty input aborts the command)
		std::cin >> windowClass;
		if (windowClass.empty()) {
			return true;
		}

		// search the given window class
		selectedWindow = FindWindow(windowClass.c_str(), nullptr);

		if (selectedWindow == nullptr) {

			// let the user try again if they failed
			std::cerr << "ERROR: could not find a window with this class name! Please try again or press enter to abort.\n";
			windowClass.erase();

		}
		else {

			// write a summary about the planned changes
			std::cout << "-> Window \"" << windowClass << "\" found (Handle: " << std::hex << selectedWindow << "!\n";
			std::cout << "-> Setting the following properties\n";
			std::cout << "   PosX: " << posX << ", PosY: " << posY << ", Width: " << width << ", Height: " << height << "\n";

			// ask for confirmation
			bool confirmedChanges = false;

			while (!confirmedChanges) {
				std::string userInput;
				std::cout << "-> Do you really want to continue? (y/n): ";
				std::cin >> userInput;

				// check answer
				userInput = StringUtils::toLower(userInput);
				if (userInput.compare("y") == 0) {
					confirmedChanges = true;
				}
				else if (userInput.compare("n") == 0) {
					std::cout << "-> Aborting!\n";
					return true;
				}
				else {
					std::cout << "-> Invalid input! Please choose either \"y\" or \"n\".\n";
				}
			}
		}
	}

	// set properties
	SetWindowPos(selectedWindow, nullptr, posX, posY, width, height, 0);
	std::cout << "-> Window has been repositioned/resized successfully!\n";

	return true;
}


