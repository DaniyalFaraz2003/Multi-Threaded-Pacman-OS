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
pthread_mutex_t mutex1;

const int FONT_HEIGHT = 16;
const int MAZE_WIDTH = 21;
const int MAZE_HEIGHT = 21;
const int SCREEN_SIZE_FACTOR = 2;
const int TILE_SIZE = 16;
const int WINDOW_WIDTH = TILE_SIZE * MAZE_WIDTH;
const int WINDOW_HEIGHT = TILE_SIZE * MAZE_HEIGHT;
const int PACMAN_SPEED = 2;
const int GHOST_SPEED = 2;
const int ENERGIZER_DURATION = 512;
const int GHOST_FLASH_START = 64;
const int GHOST_ESCAPE_SPEED = 4;
const int GHOST_FRIGHTENED_SPEED = 3;
const int LONG_SCATTER_DURATION = 512;
const int SHORT_SCATTER_DURATION = 256;
const int SCREEN_RESIZE = 2;

const int GHOST_1_CHASE = 2;
const int GHOST_2_CHASE = 1;
const int GHOST_3_CHASE = 4;

const int FRAME = 16666;


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
    int energizer_timer = 0;
    bool dead = 0;
public:
    void draw(RenderWindow& window) {
        CircleShape pacman(TILE_SIZE / 2);
        pacman.setFillColor(Color(255, 255, 0));
        pacman.setPosition(pos.x, pos.y);

        window.draw(pacman);
    }



    char getDirection() {
        return this->direction;
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

        pthread_mutex_lock(&mutex);
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
        pthread_mutex_unlock(&mutex);


        pthread_mutex_lock(&mutex);
        if (-TILE_SIZE >= pos.x)
        {
            pos.x = TILE_SIZE * MAZE_WIDTH - PACMAN_SPEED;
        }
        else if (TILE_SIZE * MAZE_WIDTH <= pos.x)
        {
            pos.x = PACMAN_SPEED - TILE_SIZE;
        }
        pthread_mutex_unlock(&mutex);

        
        if (1 == map_collision(1, 0, pos.x, pos.y, maze)) //When Pacman eats an energizer...
        {
            //He becomes energized!
            pthread_mutex_lock(&mutex);
            energizer_timer = ENERGIZER_DURATION;
            pthread_mutex_unlock(&mutex);
        }
        else
        {
            pthread_mutex_lock(&mutex);
            energizer_timer = std::max(0, energizer_timer - 1);
            pthread_mutex_unlock(&mutex);
        }
        

    }

    Position getPosition () {
        return pos;
    }

    int get_energizer_timer()
    {
        return energizer_timer;
    }

    void changeDirection(char newDirection) {
        // Lock mutex before changing Pacman's direction

        std::array<bool, 4> walls;
        walls[0] = map_collision(0, 0, PACMAN_SPEED + pos.x, pos.y, maze);
        walls[1] = map_collision(0, 0, pos.x, pos.y - PACMAN_SPEED, maze);
        walls[2] = map_collision(0, 0, pos.x - PACMAN_SPEED, pos.y, maze);
        walls[3] = map_collision(0, 0, pos.x, PACMAN_SPEED + pos.y, maze);

        pthread_mutex_lock(&mutex);
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
        pthread_mutex_unlock(&mutex);
        // Unlock mutex after changing Pacman's direction
    }
} pacman;


class Ghost: public Pacman {
    int id;
    char movement_mode; // 's' for scatter and 'c' for chase
    bool use_door;
    Position home;
	//You can't stay in your house forever (sadly).
	Position home_exit;
	//Current target.
	Position target;
    int frightened_mode;
    int frightened_speed_timer;

public:
    Ghost(int id) {
        this->id = id;
    }

    void update_target(unsigned char i_pacman_direction, const Position& i_ghost_0_position, const Position& i_pacman_position) {
        if (1 == use_door)
        {
            if (pos == target)
            {
                if (home_exit == target) 
                {
                    use_door = 0; 
                }
                else if (home == target) 
                {
                    frightened_mode = 0; 

                    target = home_exit; 
                }
            }
        }
        else
        {
            if ('s' == movement_mode)
            {
                
                switch (id)
                {
                    case 0:
                    {
                        target = {TILE_SIZE * (MAZE_WIDTH - 1), 0};

                        break;
                    }
                    case 1:
                    {
                        target = {0, 0};

                        break;
                    }
                    case 2:
                    {
                        target = {TILE_SIZE * (MAZE_WIDTH - 1), TILE_SIZE * (MAZE_HEIGHT - 1)};

                        break;
                    }
                    case 3:
                    {
                        target = {0, TILE_SIZE * (MAZE_HEIGHT - 1)};
                    }
                }
            }
            else 
            {
                switch (id)
                {
                    case 0: 
                    {
                        target = i_pacman_position;

                        break;
                    }
                    case 1: 
                    {
                        target = i_pacman_position;

                        switch (i_pacman_direction)
                        {
                            case 0:
                            {
                                target.x += TILE_SIZE * GHOST_1_CHASE;

                                break;
                            }
                            case 1:
                            {
                                target.y -= TILE_SIZE * GHOST_1_CHASE;

                                break;
                            }
                            case 2:
                            {
                                target.x -= TILE_SIZE * GHOST_1_CHASE;

                                break;
                            }
                            case 3:
                            {
                                target.y += TILE_SIZE * GHOST_1_CHASE;
                            }
                        }

                        break;
                    }
                    case 2: 
                    {
                        target = i_pacman_position;

                        
                        switch (i_pacman_direction)
                        {
                            case 0:
                            {
                                target.x += TILE_SIZE * GHOST_2_CHASE;

                                break;
                            }
                            case 1:
                            {
                                target.y -= TILE_SIZE * GHOST_2_CHASE;

                                break;
                            }
                            case 2:
                            {
                                target.x -= TILE_SIZE * GHOST_2_CHASE;

                                break;
                            }
                            case 3:
                            {
                                target.y += TILE_SIZE * GHOST_2_CHASE;
                            }
                        }

                        //We're sending a vector from the red gohst to the second cell in front of Pacman.
                        //Then we're doubling it.
                        target.x += target.x - i_ghost_0_position.x;
                        target.y += target.y - i_ghost_0_position.y;

                        break;
                    }
                    case 3: //The orange gohst will chase Pacman until it gets close to him. Then it'll switch to the scatter mode.
                    {
                        //Using the Pythagoras' theorem again.
                        if (TILE_SIZE * GHOST_3_CHASE <= sqrt(pow(pos.x - i_pacman_position.x, 2) + pow(pos.y - i_pacman_position.y, 2)))
                        {
                            target = i_pacman_position;
                        }
                        else
                        {
                            target = {0, TILE_SIZE * (MAZE_HEIGHT - 1)};
                        }
                    }
                }
            }
        }
    }

    bool pacman_collision(const Position& i_pacman_position)
    {
        if (pos.x > i_pacman_position.x - TILE_SIZE && pos.x < TILE_SIZE + i_pacman_position.x)
        {
            if (pos.y > i_pacman_position.y - TILE_SIZE && pos.y < TILE_SIZE + i_pacman_position.y)
            {
                return 1;
            }
        }

        return 0;
    }

    void switchMovementMode() {
        if (movement_mode == 's') {
            movement_mode = 'c';
        } else {
            movement_mode = 's';
        }
    }

    void reset(const Position& i_home, const Position& i_home_exit) {
        
        movement_mode = 's';
        use_door = 0 < id;
        direction = 'r';
        home = i_home;
        home_exit = i_home_exit;
        target = i_home_exit;
        frightened_mode = 0;
	    frightened_speed_timer = 0;

    }
    float getTargetDistance(char dir) {
        short x = pos.x;
        short y = pos.y;

        switch (dir)
        {
            case 'r':
            {
                x += GHOST_SPEED;

                break;
            }
            case 'u':
            {
                y -= GHOST_SPEED;

                break;
            }
            case 'l':
            {
                x -= GHOST_SPEED;

                break;
            }
            case 'd':
            {
                y += GHOST_SPEED;
            }
        }
        return static_cast<float>(sqrt(pow(x - target.x, 2) + pow(y - target.y, 2)));
    }
    void draw(RenderWindow& window) {
        CircleShape ghostHead(TILE_SIZE / 2);
        RectangleShape ghostBody(Vector2f(TILE_SIZE, TILE_SIZE / 2));
        
        if (frightened_mode == 0) {
            switch (id)
            {
            case 0:
                ghostHead.setFillColor(Color(255, 0, 0));
                ghostBody.setFillColor(Color(255, 0, 0));
                break;
            case 1:
                ghostHead.setFillColor(Color(255, 192, 203));
                ghostBody.setFillColor(Color(255, 192, 203));
                break;
            case 2:
                ghostHead.setFillColor(Color(0, 255, 255));
                ghostBody.setFillColor(Color(0, 255, 255));
                
                break;
            case 3:
                ghostHead.setFillColor(Color(255, 165, 0));
                ghostBody.setFillColor(Color(255, 165, 0));
                break;
            default:
                break;

            
            }
        }
        else if (frightened_mode == 1) {
            ghostHead.setFillColor(Color(50, 76, 168));
            ghostBody.setFillColor(Color(50, 76, 168));

        }

        ghostHead.setPosition(pos.x, pos.y);
        ghostBody.setPosition(pos.x, pos.y + TILE_SIZE / 2);

        window.draw(ghostHead);
        window.draw(ghostBody);
    }
    
    int convertDirToInt(char dir) {
        switch (dir)
        {
        case 'r':
            return 0;
            break;
        case 'u':
            return 1;
            break;
        case 'l':
            return 2;
            break;
        case 'd':
            return 3;
            break;
        default:
            break;
        }
    }

    char convertDir(int dir) {
        if (dir == 0) return 'r';
        if (dir == 1) return 'u';
        if (dir == 2) return 'l';
        if (dir == 3) return 'd';
        return ' ';
    }

    void update(std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH>& maze, Ghost& i_ghost_0, Pacman& i_pacman) {
        int availableWays = 0;
        bool move = 0;

        int speed = GHOST_SPEED;
        
        //Here the gohst starts and stops being frightened.
        if (0 == frightened_mode && i_pacman.get_energizer_timer() == ENERGIZER_DURATION)
        {
            frightened_speed_timer = GHOST_FRIGHTENED_SPEED;
            frightened_mode = 1;
        }
        else if (0 == i_pacman.get_energizer_timer() && 1 == frightened_mode)
        {
            frightened_mode = 0;
        }

        
        //I used the modulo operator in case the gohst goes outside the grid.
        if (2 == frightened_mode && 0 == pos.x % GHOST_ESCAPE_SPEED && 0 == pos.y % GHOST_ESCAPE_SPEED)
        {
            speed = GHOST_ESCAPE_SPEED;
        }
        pthread_mutex_lock(&mutex1);
        update_target(i_pacman.getDirection(), i_ghost_0.getPostition(), i_pacman.getPostition());
        pthread_mutex_unlock(&mutex1);

        std::array<bool, 4> walls;
        walls[0] = map_collision(0, use_door, GHOST_SPEED + pos.x, pos.y, maze);
        walls[1] = map_collision(0, use_door, pos.x, pos.y - GHOST_SPEED, maze);
        walls[2] = map_collision(0, use_door, pos.x - GHOST_SPEED, pos.y, maze);
        walls[3] = map_collision(0, use_door, pos.x, GHOST_SPEED + pos.y, maze);

        int dir;
        if (direction == 'r') dir = 0;
        else if (direction == 'u') dir = 1;
        else if (direction == 'l') dir = 2;
        else if (direction == 'd') dir = 3;

        // here is the random ghost movement algorithm
        /*
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
        */

       if (1 != frightened_mode)
        {
            //I used 4 because using a number between 0 and 3 will make the gohst move in a direction it can't move.
            unsigned char optimal_direction = 4;

            //The gohst can move.
            move = 1;

            for (unsigned char a = 0; a < 4; a++)
            {
                //Gohsts can't turn back! (Unless they really have to)
                if (a == (2 + dir) % 4)
                {
                    continue;
                }
                else if (0 == walls[a])
                {
                    if (4 == optimal_direction)
                    {
                        optimal_direction = a;
                    }

                    availableWays++;

                    if (getTargetDistance(convertDir(a)) < getTargetDistance(convertDir(optimal_direction)))
                    {
                        //The optimal direction is the direction that's closest to the target.
                        optimal_direction = a;
                    }
                }
            }

            if (1 < availableWays)
            {
                //When the gohst is at the intersection, it chooses the optimal direction.
                direction = convertDir(optimal_direction);
            }
            else
            {
                if (4 == optimal_direction)
                {
                    //"Unless they have to" part.
                    direction = convertDir((2 + dir) % 4);
                }
                else
                {
                    direction = convertDir(optimal_direction);
                }
            }
        }
        else
        {
            //I used rand() because I figured that we're only using randomness here, and there's no need to use a whole library for it.
            unsigned char random_direction = rand() % 4;

            if (0 == frightened_speed_timer)
            {
                //The gohst can move after a certain number of frames.
                move = 1;

                frightened_speed_timer = GHOST_FRIGHTENED_SPEED;

                for (unsigned char a = 0; a < 4; a++)
                {
                    //They can't turn back even if they're frightened.
                    if (a == (2 + dir) % 4)
                    {
                        continue;
                    }
                    else if (0 == walls[a])
                    {
                        availableWays++;
                    }
                }

                if (0 < availableWays)
                {
                    while (1 == walls[random_direction] || random_direction == (2 + dir) % 4)
                    {
                        //We keep picking a random direction until we can use it.
                        random_direction = rand() % 4;
                    }

                    direction = convertDir(random_direction);
                }
                else
                {
                    //If there's no other way, it turns back.
                    direction = convertDir((2 + dir) % 4);
                }
            }
            else
            {
                frightened_speed_timer--;
            }
        }

        if (move == 1) {
            switch (direction)
            {
            case 'u':
                pos.y -= speed;
                break;
            case 'd':
                pos.y += speed;
                break;
            case 'l':
                pos.x -= speed;
                break;
            case 'r':
                pos.x += speed;
                break;
            default:
                break;
            }

            if (-TILE_SIZE >= pos.x)
            {
                pos.x = TILE_SIZE * MAZE_WIDTH - speed;
            }
            else if (pos.x >= TILE_SIZE * MAZE_WIDTH)
            {
                pos.x = speed - TILE_SIZE;
            }
        }

        pthread_mutex_lock(&mutex);
        if (1 == pacman_collision(i_pacman.getPosition()))
        {
            if (0 == frightened_mode) //When the gohst is not frightened and collides with Pacman, we kill Pacman.
            {
                // i_pacman.set_dead(1);
            }
            else //Otherwise, the gohst starts running towards the house.
            {
                use_door = 1;
                frightened_mode = 2;
                target = home;
            }
        }
        pthread_mutex_unlock(&mutex);
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

    void reset() {
        for (Ghost& ghost : ghosts) {
            //We use the blue ghost to get the location of the house and the red ghost to get the location of the exit.
            ghost.reset(ghosts[2].getPostition(), ghosts[0].getPostition());
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
                CircleShape pellet(TILE_SIZE / 8);
                pellet.setFillColor(Color::White);
                pellet.setPosition(TILE_SIZE * i + TILE_SIZE / 2 - pellet.getRadius(), TILE_SIZE * j + TILE_SIZE / 2 - pellet.getRadius());
                window.draw(pellet);
            }
            else if (maze[i][j] == Tile::Door) {
                tile.setFillColor(Color(255, 255, 255));
                window.draw(tile);
            }
            else if (maze[i][j] == Tile::Energizer) {
                CircleShape energizer(TILE_SIZE / 3);
                energizer.setFillColor(Color(230, 194, 124));
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
    Clock modeSwitchClock; int modeSwitchTime = 8;
    int accumulatedTime = 0;
    bool start = false;
    while (true) {
        // Calculate elapsed time since the last update
        int dt = clock.restart().asMicroseconds();
        accumulatedTime += dt;

        // Update game logic based on the fixed timestep
        while (accumulatedTime >= FRAME) {
            // Game logic
            if (modeSwitchClock.getElapsedTime().asSeconds() >= modeSwitchTime && !start) {
                ghostManager.ghosts[ghostId].switchMovementMode();
                start = true;
            }
            ghostManager.ghosts[ghostId].update(maze, ghostManager.ghosts[0], pacman);
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
    pthread_mutex_init(&mutex1, NULL);

    // Create window
    sf::RenderWindow window(sf::VideoMode(TILE_SIZE * MAZE_WIDTH * SCREEN_RESIZE, (FONT_HEIGHT + TILE_SIZE * MAZE_HEIGHT) * SCREEN_RESIZE), "Pac-Man", sf::Style::Close);
	//Resizing the window.
	window.setView(sf::View(sf::FloatRect(0, 0, TILE_SIZE * MAZE_WIDTH, FONT_HEIGHT + TILE_SIZE * MAZE_HEIGHT)));

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
    ghostManager.reset();
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
    pthread_mutex_destroy(&mutex1);

    return 0;
}

