#include <SFML/Graphics.hpp>
#include <pthread.h>
#include <cmath>
#include <array>
#include <iostream>
#include <vector>

#include "globals.hpp"

using namespace std;
using namespace sf;

// Constants
pthread_mutex_t mutex;


const int MAZE_WIDTH = 21;
const int MAZE_HEIGHT = 21;
const int SCREEN_SIZE_FACTOR = 2;
const int TILE_SIZE = 30;
const int WINDOW_WIDTH = TILE_SIZE * MAZE_WIDTH;
const int WINDOW_HEIGHT = TILE_SIZE * MAZE_HEIGHT;
const int PACMAN_SPEED = 2;

const int FRAME = 16000;


// Mutex for thread synchronization


/*
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
*/

std::array<std::string, MAZE_HEIGHT> mazeMapping = {
		" ################### ",
		" #.................# ",
		" #o...............o# ",
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
		" #.##..............# ",
		" #o.#.....P........# ",
		" ##.#..............# ",
		" #.................# ",
		" #.######.#.######.# ",
		" #.................# ",
		" ################### "
	};

std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH> maze;

bool map_collision(bool i_collect_pellets, bool i_use_door, short i_x, short i_y, std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH>& maze)
{
	bool output = 0;

	//Getting the exact position.
	float cell_x = i_x / static_cast<float>(TILE_SIZE);
	float cell_y = i_y / static_cast<float>(TILE_SIZE);

	//A ghost/Pacman can intersect 4 cells at most.
	for (unsigned char a = 0; a < 4; a++)
	{
		short x = 0;
		short y = 0;

		switch (a)
		{
			case 0: //Top left cell
			{
				x = static_cast<short>(floor(cell_x));
				y = static_cast<short>(floor(cell_y));

				break;
			}
			case 1: //Top right cell
			{
				x = static_cast<short>(ceil(cell_x));
				y = static_cast<short>(floor(cell_y));

				break;
			}
			case 2: //Bottom left cell
			{
				x = static_cast<short>(floor(cell_x));
				y = static_cast<short>(ceil(cell_y));

				break;
			}
			case 3: //Bottom right cell
			{
				x = static_cast<short>(ceil(cell_x));
				y = static_cast<short>(ceil(cell_y));
			}
		}

		//Making sure that the position is inside the map.
		if (0 <= x && 0 <= y && MAZE_HEIGHT > y && MAZE_WIDTH > x)
		{
			if (0 == i_collect_pellets) //Here we only care about the walls.
			{
				if (Tile::Wall == maze[x][y])
				{
					output = 1;
				}
				else if (0 == i_use_door && Tile::Door == maze[x][y])
				{
					output = 1;
				}
			}
			else //Here we only care about the collectables.
			{
				if (Tile::Energizer == maze[x][y])
				{
					output = 1;

					maze[x][y] = Tile::Empty;
				}
				else if (Tile::Pellet == maze[x][y])
				{
					maze[x][y] = Tile::Empty;
				}
			}
		}
	}

	return output;
}

class Pacman {
    Position pos;
    char direction = 'r';
public:
    void draw(RenderWindow& window) {
        CircleShape pacman(TILE_SIZE / 2);
        pacman.setFillColor(Color(255, 255, 0));
        pacman.setPosition(pos.x, pos.y);

        window.draw(pacman);
    }

    Position getPostition() {
        return pos;
    }

    void setPosition(int x, int y) {
        pos = {x, y};
    }

    void update(std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH>& maze) {

        std::array<bool, 4> walls;
        walls[0] = map_collision(0, 0, PACMAN_SPEED + pos.x, pos.y, maze);
        walls[1] = map_collision(0, 0, pos.x, pos.y - PACMAN_SPEED, maze);
        walls[2] = map_collision(0, 0, pos.x - PACMAN_SPEED, pos.y, maze);
        walls[3] = map_collision(0, 0, pos.x, PACMAN_SPEED + pos.y, maze);

        int dir;
        if (direction == 'r') dir = 0;
        else if (direction == 'u') dir = 1;
        else if (direction == 'l') dir = 2;
        else if (direction == 'd') dir = 3;

        if (walls[dir] == 0) {
            switch (direction)
            {
            case 'u':
                pos.y -= PACMAN_SPEED;
                break;
            case 'd':
                pos.y += PACMAN_SPEED;
                break;
            case 'l':
                pos.x -= PACMAN_SPEED;
                break;
            case 'r':
                pos.x += PACMAN_SPEED;
                break;
            
            default:
                break;
            }
        }

        if (-TILE_SIZE >= pos.x)
        {
            pos.x = TILE_SIZE * MAZE_WIDTH - PACMAN_SPEED;
        }
        else if (TILE_SIZE * MAZE_WIDTH <= pos.x)
        {
            pos.x = PACMAN_SPEED - TILE_SIZE;
        }

    }

    void changeDirection(char newDirection) {
        // Lock mutex before changing Pacman's direction

        std::array<bool, 4> walls;
        walls[0] = map_collision(0, 0, PACMAN_SPEED + pos.x, pos.y, maze);
        walls[1] = map_collision(0, 0, pos.x, pos.y - PACMAN_SPEED, maze);
        walls[2] = map_collision(0, 0, pos.x - PACMAN_SPEED, pos.y, maze);
        walls[3] = map_collision(0, 0, pos.x, PACMAN_SPEED + pos.y, maze);

        if (newDirection == 'r')
        {
            if (0 == walls[0]) //You can't turn in this direction if there's a wall there.
            {
                direction = 'r';
            }
        }

        if (newDirection == 'u')
        {
            if (0 == walls[1])
            {
                direction = 'u';
            }
        }

        if (newDirection == 'l')
        {
            if (0 == walls[2])
            {
                direction = 'l';
            }
        }

        if (newDirection == 'd')
        {
            if (0 == walls[3])
            {
                direction = 'd';
            }
        }

        // Unlock mutex after changing Pacman's direction
    }
} pacman;


std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH> getMaze(const std::array<std::string, MAZE_HEIGHT>& i_map_sketch/*, std::array<Position, 4>& i_ghost_positions, */, Pacman& pacman)
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
				//Pacman!
				case 'P':
				{
					pacman.setPosition(TILE_SIZE * b, TILE_SIZE * a);
					break;
				}
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
    Clock clock;
    int accumulatedTime = 0;
    while (true) {
        // Calculate elapsed time since the last update
        int dt = clock.restart().asMicroseconds();
        accumulatedTime += dt;

        // Update game logic based on the fixed timestep
        while (accumulatedTime >= FRAME) {
            // Game logic
            pacman.update(maze);

            // Subtract the fixed timestep from accumulated time
            accumulatedTime -= FRAME;
        }
        
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

    

    maze = getMaze(mazeMapping, pacman);

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


            // i think this input getting part is wrong but for testing sake
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                pacman.changeDirection('u');
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                pacman.changeDirection('d');
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                pacman.changeDirection('l');
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                pacman.changeDirection('r');
            }
            // end of wrong part

            if (FRAME > delay) {
                // Rendering
                window.clear(sf::Color::Black);

                drawMap(maze, window);
                pacman.draw(window);

                

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

