#include <SFML/Graphics.hpp>
#include <pthread.h>
#include <iostream>
#include <vector>

using namespace std;
using namespace sf;

// Constants
const int MAZE_WIDTH = 21;
const int MAZE_HEIGHT = 21;
const int TILE_SIZE = 15;
const int SCREEN_SIZE_FACTOR = 2;
const int WINDOW_WIDTH = TILE_SIZE * MAZE_WIDTH * SCREEN_SIZE_FACTOR;
const int WINDOW_HEIGHT = TILE_SIZE * MAZE_HEIGHT * SCREEN_SIZE_FACTOR;

const int FRAME = 16000;
// Mutex for thread synchronization
pthread_mutex_t mutex;

std::array<std::string, MAZE_HEIGHT> maze = {
		" ################### ",
		" #........#........# ",
		" #o##.###.#.###.##o# ",
		" #.................# ",
		" #.##.#.#####.#.##.# ",
		" #....#...#...#....# ",
		" ####.### # ###.#### ",
		"    #.#   0   #.#    ",
		"#####.# ##=## #.#####",
		"     .  #123#  .     ",
		"#####.# ##### #.#####",
		"    #.#       #.#    ",
		" ####.# ##### #.#### ",
		" #........#........# ",
		" #.##.###.#.###.##.# ",
		" #o.#.....P.....#.o# ",
		" ##.#.#.#####.#.#.## ",
		" #....#...#...#....# ",
		" #.######.#.######.# ",
		" #.................# ",
		" ################### "
	};

// Game Engine Thread Function
void* gameEngineThread(void*) {
    // Game initialization
    // Define the maze layout
    
    // Game loop
    while (true) {
        // Game logic

        // Rendering

        // Sleep or yield to give time to other threads
        // For simplicity, you can use usleep or sleep in POSIX systems
        // usleep(10000); // Sleep for 10 milliseconds
    }

    return nullptr;
}

// User Interface Thread Function
void* userInterfaceThread(void*) {
    // UI initialization

    // UI loop
    while (true) {
        // UI logic

        // Rendering

        // Sleep or yield to give time to other threads
        // For simplicity, you can use usleep or sleep in POSIX systems
        // usleep(10000); // Sleep for 10 milliseconds
    }

    return nullptr;
}

// Ghost Controller Thread Function
void* ghostControllerThread(void* arg) {
    int ghostId = *((int*)arg);
    // Ghost initialization

    // Ghost loop
    while (true) {
        // Ghost logic

        // Movement algorithm

        // Sleep or yield to give time to other threads
        // For simplicity, you can use usleep or sleep in POSIX systems
        // usleep(10000); // Sleep for 10 milliseconds
    }

    return nullptr;
}

int main() {
    // Initialize mutex
    pthread_mutex_init(&mutex, NULL);

    // Create window
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SFML Pacman");

    // Set thread attributes to create detached threads
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // Create game engine thread
    pthread_t gameEngineThreadID;
    pthread_create(&gameEngineThreadID, &attr, gameEngineThread, NULL);

    // Create user interface thread
    pthread_t userInterfaceThreadID;
    pthread_create(&userInterfaceThreadID, &attr, userInterfaceThread, NULL);

    // Create ghost controller threads
    std::vector<pthread_t> ghostThreads;
    for (int i = 0; i < 4; ++i) {
        pthread_t ghostThread;
        pthread_create(&ghostThread, &attr, ghostControllerThread, &i);
        ghostThreads.push_back(ghostThread);
    }

    // Destroy thread attributes
    pthread_attr_destroy(&attr);

    // Main loop
    Clock clock; float dt; float delay = 0;
    while (window.isOpen()) {
        dt = clock.getElapsedTime().asMicroseconds();
        delay += dt;

        while (FRAME <= delay) {
            delay -= FRAME;
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
            }

            // Rendering
            window.clear(sf::Color::Black);

            

            // Render game objects
            // Render UI
            window.display();
        }
    }

    // Joining detached threads is unnecessary and will be ignored

    // Destroy mutex
    pthread_mutex_destroy(&mutex);

    return 0;
}


