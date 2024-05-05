#include <SFML/Graphics.hpp>
#include <pthread.h>
#include <iostream>
#include <vector>

#include "globals.hpp"

using namespace std;
using namespace sf;

// Constants
const int MAZE_WIDTH = 21;
const int MAZE_HEIGHT = 21;
const int SCREEN_SIZE_FACTOR = 2;
const int TILE_SIZE = 30;
const int WINDOW_WIDTH = TILE_SIZE * MAZE_WIDTH;
const int WINDOW_HEIGHT = TILE_SIZE * MAZE_HEIGHT;

const int FRAME = 16000;
// Mutex for thread synchronization
pthread_mutex_t mutex;

std::array<std::string, MAZE_HEIGHT> mazeMapping = {
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

std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH> getMaze(const std::array<std::string, MAZE_HEIGHT>& i_map_sketch/*, std::array<Position, 4>& i_ghost_positions, *//*Pacman& i_pacman*/)
{
	//Is it okay if I put {} here? I feel like I'm doing something illegal.
	//But if I don't put it there, Visual Studio keeps saying "lOcAl vArIaBlE Is nOt iNiTiAlIzEd".
	std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH> output_map{};

	for (unsigned char a = 0; a < MAZE_HEIGHT; a++)
	{
		for (unsigned char b = 0; b < MAZE_WIDTH; b++)
		{
			//By default, every Tile is empty.
			output_map[b][a] = Tile::Empty;

			switch (i_map_sketch[a][b])
			{
				//#wall #obstacle #youcantgothroughme
				case '#':
				{
					output_map[b][a] = Tile::Wall;

					break;
				}
				case '=':
				{
					output_map[b][a] = Tile::Door;

					break;
				}
				case '.':
				{
					output_map[b][a] = Tile::Pellet;

					break;
				}
				//Red ghost
                /*
				case '0':
				{
					i_ghost_positions[0].x = TILE_SIZE * b;
					i_ghost_positions[0].y = TILE_SIZE * a;

					break;
				}
				//Pink ghost
				case '1':
				{
					i_ghost_positions[1].x = TILE_SIZE * b;
					i_ghost_positions[1].y = TILE_SIZE * a;

					break;
				}
				//Blue (cyan) ghost
				case '2':
				{
					i_ghost_positions[2].x = TILE_SIZE * b;
					i_ghost_positions[2].y = TILE_SIZE * a;

					break;
				}
				//Orange ghost
				case '3':
				{
					i_ghost_positions[3].x = TILE_SIZE * b;
					i_ghost_positions[3].y = TILE_SIZE * a;

					break;
				}
                */
				/*//Pacman!
				case 'P':
				{
					i_pacman.set_position(TILE_SIZE * b, TILE_SIZE * a);

					break;
				}
                */
				//This looks like a surprised face.
				case 'o':
				{
					output_map[b][a] = Tile::Energizer;
				}
			}
		}
	}

	return output_map;
}

void drawMap(std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH>& maze, RenderWindow& window) {
    RectangleShape tile(Vector2f(TILE_SIZE, TILE_SIZE));

    for (int i = 0; i < MAZE_WIDTH; i++) {
        for (int j = 0; j < MAZE_HEIGHT; j++) {
            tile.setPosition(TILE_SIZE * i, TILE_SIZE * j);
            if (maze[i][j] == Tile::Wall) {
                tile.setFillColor(Color(128, 128, 128));
                window.draw(tile);
            }
        }
    }
}

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

    std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH> maze;
    maze = getMaze(mazeMapping);

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

            if (FRAME > delay) {
                // Rendering
                window.clear(sf::Color::Black);

                drawMap(maze, window);
                

                // Render game objects
                // Render UI
                window.display();
            }
        }
    }

    // Joining detached threads is unnecessary and will be ignored

    // Destroy mutex
    pthread_mutex_destroy(&mutex);

    return 0;
}


