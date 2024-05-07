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
const int GHOST_SPEED = 2;

const int FRAME = 16000;


// Mutex for thread synchronization


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
		"     . 1#   #2 .     ",
		"#####.# ##### #.#####",
		"    #.#   3   #.#    ",
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
                // here we are updating the maze which is the shared resource so we lock
                pthread_mutex_lock(&mutex);
				if (Tile::Energizer == maze[x][y])
				{
					output = 1;

					maze[x][y] = Tile::Empty;
				}
				else if (Tile::Pellet == maze[x][y])
				{
					maze[x][y] = Tile::Empty;
				}
                pthread_mutex_unlock(&mutex);
			}
		}
	}

	return output;
}

class Pacman {
protected:
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
        // Check collision with food pellets and removing them
        map_collision(1, 0, pos.x, pos.y, maze);

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


class Ghost: public Pacman {
    int id;
    bool movement_mode;
    
    Position home;
	//You can't stay in your house forever (sadly).
	Position home_exit;
	//Current target.
	Position target;
public:
    Ghost(int id) {
        this->id = id;
        setPosition(-100, -100);
    }
    void draw(RenderWindow& window) {
        CircleShape ghostHead(TILE_SIZE / 2);
        RectangleShape ghostBody(Vector2f(TILE_SIZE, TILE_SIZE / 2));
        
        switch (id)
        {
        case 0:
            ghostHead.setFillColor(Color(255, 0, 0));
            ghostBody.setFillColor(Color(255, 0, 0));
            break;
        case 1:
            ghostHead.setFillColor(Color(0, 255, 255));
            ghostBody.setFillColor(Color(0, 255, 255));
            break;
        case 2:
            ghostHead.setFillColor(Color(0, 255, 0));
            ghostBody.setFillColor(Color(0, 255, 0));
            break;
        case 3:
            ghostHead.setFillColor(Color(255, 0, 255));
            ghostBody.setFillColor(Color(255, 0, 255));
            break;
        default:
            break;
        }

        ghostHead.setPosition(pos.x, pos.y);
        ghostBody.setPosition(pos.x, pos.y + TILE_SIZE / 2);

        window.draw(ghostHead);
        window.draw(ghostBody);
    }
    
    void update(std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH>& maze) {
        int availableWays = 0;

        std::array<bool, 4> walls;
        walls[0] = map_collision(0, 0, GHOST_SPEED + pos.x, pos.y, maze);
        walls[1] = map_collision(0, 0, pos.x, pos.y - GHOST_SPEED, maze);
        walls[2] = map_collision(0, 0, pos.x - GHOST_SPEED, pos.y, maze);
        walls[3] = map_collision(0, 0, pos.x, GHOST_SPEED + pos.y, maze);

        int dir;
        if (direction == 'r') dir = 0;
        else if (direction == 'u') dir = 1;
        else if (direction == 'l') dir = 2;
        else if (direction == 'd') dir = 3;

        for (int i = 0; i < 4; i++) {
            if (i == (dir + 2) % 4) {
                continue;
            }
            else if (walls[i] == 0) {
                availableWays++;
            }
        }

        if (availableWays > 1) {
            int newDir = rand() % 4;
            if (walls[newDir] == 0 && newDir != (2 + direction) % 4) {
                if (newDir == 0) direction = 'r';
                else if (newDir == 1) direction = 'u';
                else if (newDir == 2) direction = 'l';
                else if (newDir == 3) direction = 'd';
            }
        }
        else if (walls[dir] == 1) {
            for (int i = 0; i < 4; i++) {
                if (walls[i] == 0 && i != (2 + dir) % 4) {
                    if (i == 0) direction = 'r';
                    else if (i == 1) direction = 'u';
                    else if (i == 2) direction = 'l';
                    else if (i == 3) direction = 'd';
                }
            }
        }

        if (walls[dir] == 0) {
            switch (direction)
            {
            case 'u':
                pos.y -= GHOST_SPEED;
                break;
            case 'd':
                pos.y += GHOST_SPEED;
                break;
            case 'l':
                pos.x -= GHOST_SPEED;
                break;
            case 'r':
                pos.x += GHOST_SPEED;
                break;
            
            default:
                break;
            }
        }

        if (-TILE_SIZE >= pos.x)
        {
            pos.x = TILE_SIZE * MAZE_WIDTH - GHOST_SPEED;
        }
        else if (TILE_SIZE * MAZE_WIDTH <= pos.x)
        {
            pos.x = GHOST_SPEED - TILE_SIZE;
        }

    }
};

class GhostManager {
    public:
    vector<Ghost> ghosts;
        GhostManager() {
            ghosts = {Ghost(0), Ghost(1), Ghost(2), Ghost(3)};
        }

        void draw(RenderWindow& window) {
            for (Ghost& ghost: ghosts) {
                ghost.draw(window);
            }
        }

} ghostManager;

std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH> getMaze(const std::array<std::string, MAZE_HEIGHT>& i_map_sketch, std::vector<Ghost>& ghosts, Pacman& pacman)
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
				case '0':
				{
					ghosts[0].setPosition(TILE_SIZE * b, TILE_SIZE * a);
					break;
				}
				//Pink ghost
				case '1':
				{
					ghosts[1].setPosition(TILE_SIZE * b, TILE_SIZE * a);

					break;
				}
				//Blue (cyan) ghost
				case '2':
				{
					ghosts[2].setPosition(TILE_SIZE * b, TILE_SIZE * a);

					break;
				}
				//Orange ghost
				case '3':
				{
					ghosts[3].setPosition(TILE_SIZE * b, TILE_SIZE * a);

					break;
				}
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
            else if (maze[i][j] == Tile::Pellet) {
                CircleShape pellet(TILE_SIZE / 10);
                pellet.setFillColor(Color::White);
                pellet.setPosition(TILE_SIZE * i + TILE_SIZE / 2 - pellet.getRadius(), TILE_SIZE * j + TILE_SIZE / 2 - pellet.getRadius());
                window.draw(pellet);
            }
            else if (maze[i][j] == Tile::Energizer) {
                CircleShape energizer(TILE_SIZE / 2);
                energizer.setFillColor(Color::Blue);
                energizer.setPosition(TILE_SIZE * i + TILE_SIZE / 2 - energizer.getRadius(), TILE_SIZE * j + TILE_SIZE / 2 - energizer.getRadius());
                window.draw(energizer);
            }
        }
    }
}


// Game Engine Thread Function
void* userInterfaceThread(void*) {
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


// Ghost Controller Thread Function
void* ghostControllerThread(void* arg) {
    int ghostId = *((int*)arg);
    // Ghost initialization

    Clock clock;
    int accumulatedTime = 0;
    while (true) {
        // Calculate elapsed time since the last update
        int dt = clock.restart().asMicroseconds();
        accumulatedTime += dt;

        // Update game logic based on the fixed timestep
        while (accumulatedTime >= FRAME) {
            
            
            // Game logic
            ghostManager.ghosts[ghostId].update(maze);
            

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

int main() {
    srand(time(NULL));
    // Initialize mutex
    pthread_mutex_init(&mutex, NULL);

    // Create window
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SFML Pacman");

    // Set thread attributes to create detached threads
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // Create user interface thread
    pthread_t userInterfaceThreadID;
    pthread_create(&userInterfaceThreadID, &attr, userInterfaceThread, NULL);

    pthread_t ghostThreads[4];
    // Create ghost controller threads
    for (int i = 0; i < 4; ++i) {
        int* id = new int(i);
        pthread_create(&ghostThreads[i], &attr, ghostControllerThread, id);
    }

    // Destroy thread attributes
    pthread_attr_destroy(&attr);


    maze = getMaze(mazeMapping, ghostManager.ghosts, pacman);

    // Main loop
    Clock clock; float dt; float delay = 0;
    while (window.isOpen()) {
        dt = clock.restart().asMicroseconds();
        delay += dt;

        while (FRAME <= delay) {
            delay -= FRAME;
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                pacman.changeDirection('u');
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                pacman.changeDirection('d');
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                pacman.changeDirection('l');
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                pacman.changeDirection('r');
            }

            if (FRAME > delay) {
                // Rendering
                window.clear(sf::Color::Black);

                drawMap(maze, window);
                ghostManager.draw(window);
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

