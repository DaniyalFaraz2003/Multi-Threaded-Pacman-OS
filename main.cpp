#include <SFML/Graphics.hpp>
#include <pthread.h>
#include <cmath>
#include <array>
#include <iostream>
#include <vector>
#include <semaphore.h> 
#include <unistd.h> 
#include <SFML/Audio.hpp>

#include "globals.hpp"

using namespace std;
using namespace sf;


pthread_mutex_t mutexPacmanPos;
pthread_mutex_t ghostRead;
int ghostReaders = 0;

pthread_mutex_t ghostVars[4];


pthread_mutex_t mutexScoreLives;
pthread_mutex_t gridLock;

pthread_mutex_t keyPermitMutex;
int key = 0;
int permit = 0;

pthread_mutex_t speedBoostMutex;
int speedBoost = 0;

pthread_mutex_t enrgizerMutex;
int energizersProduced = 4;



vector<Position> energizerPos = {
    {2, 2},
    {18, 2},
    {2, 15},
    {18, 15}
};



const int TILE_SIZE = 16;
Position SPEEDBOOST_POS1(TILE_SIZE * 2, TILE_SIZE * 5);
Position SPEEDBOOST_POS2(TILE_SIZE * 18, TILE_SIZE * 19);
Position KEY_POS(TILE_SIZE * 8, TILE_SIZE * 11);
Position PERMIT_POS(12 * TILE_SIZE, 9 * TILE_SIZE);
const int FONT_HEIGHT = 16;
const int MAZE_WIDTH = 21;
const int MAZE_HEIGHT = 21;
const int SCREEN_SIZE_FACTOR = 2;
const int WINDOW_WIDTH = TILE_SIZE * MAZE_WIDTH;
const int WINDOW_HEIGHT = TILE_SIZE * MAZE_HEIGHT;
const int PACMAN_SPEED = 2;
const int GHOST_SPEED = 1;
const int SPEED_UP = 2;
const int ENERGIZER_DURATION = 512;
const int GHOST_FLASH_START = 64;
const int GHOST_ESCAPE_SPEED = 4;
const int GHOST_FRIGHTENED_SPEED = 3;
const int LONG_SCATTER_DURATION = 512;
const int SHORT_SCATTER_DURATION = 256;
const int SCREEN_RESIZE = 2;
const int PACMAN_ANIMATION_FRAMES = 6;
const int PACMAN_ANIMATION_SPEED = 4;
const int PACMAN_DEATH_FRAMES = 12;

const int GHOST_1_CHASE = 2;
const int GHOST_2_CHASE = 1;
const int GHOST_3_CHASE = 4;

const int FRAME = 15000;


// Mutex for thread synchronization


std::array<std::string, MAZE_HEIGHT> mazeMapping = {
    " ################### ",
    " #........#........# ",
    " #o##.###.#.###.##o# ",
    " #.................# ",
    " #.##.#.#####.#.##.# ",
    " #....#...#...#....# ",
    " ####.### # ###.#### ",
    "    #.         .#    ",
    "#####.####=####.#####",
    "     .# 0  1  #.     ",
    "#####.#    2  #.#####",
    "    #.# 3     #.#    ",
    " ####.#########.#### ",
    " #.................# ",
    " #.##.###.#.###.##.# ",
    " #o.#.....P.....#.o# ",
    " ##.#.#.#####.#.#.## ",
    " #....#...#...#....# ",
    " #.######.#.######.# ",
    " #.................# ",
    " ################### "
};


std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH> maze;

bool map_collision(bool i_collect_pellets, bool i_use_door, short i_x, short i_y, std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH>& maze, bool& pelletEaten, int energizerTimer = 0)
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
			if (0 == i_collect_pellets) 
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
			else 
			{
                // here we are updating the maze which is the shared resource so we lock
                pthread_mutex_lock(&gridLock);
                pthread_mutex_lock(&enrgizerMutex);
                if (energizersProduced > 0) {

                    if (Tile::Energizer == maze[x][y] && energizerTimer == 0) {
                        output = 1;
					    maze[x][y] = Tile::Empty;
                        energizersProduced--;
                    }
                }
                pthread_mutex_unlock(&enrgizerMutex);
				if (Tile::Pellet == maze[x][y])
				{
					maze[x][y] = Tile::Empty;
                    pelletEaten = 1;
				}
                pthread_mutex_unlock(&gridLock);
			}
		}
	}

	return output;
}

class Key {
public:
    Sprite key;
    Texture t;
    Key() {
        t.loadFromFile("assets/key.png");
        key.setTexture(t);
        key.setScale(0.03125, 0.03125);
        key.setPosition(-100, -100);
    }

    void degenerate() {
        key.setPosition(-100, -100);
    }

    void generate() {
        key.setPosition(KEY_POS.x, KEY_POS.y);
    }

    Position getPos() {
        return Position(key.getPosition().x, key.getPosition().y);
    }

    void draw(RenderWindow& window) {
        window.draw(key);
    }
};

class Permit {
public:
    Sprite permit;
    Texture t;
    Permit() {
        t.loadFromFile("assets/check.png");
        permit.setTexture(t);
        permit.setScale(0.03125, 0.03125);
        permit.setPosition(-100, -100);
    }

    Position getPos() {
        return Position(permit.getPosition().x, permit.getPosition().y);
    }

    void degenerate() {
        permit.setPosition(-100, -100);
    }

    void generate() {
        permit.setPosition(PERMIT_POS.x, PERMIT_POS.y);
    }

    void draw(RenderWindow& window) {
        window.draw(permit);
    }
};

class SpeedBoost {
public:
    Sprite sb;
    Texture t;
    int id;
    SpeedBoost(int id) {
        t.loadFromFile("assets/speed.png");
        sb.setTexture(t);
        sb.setScale(0.03125, 0.03125);
        sb.setPosition(-100, -100);
        this->id = id;
    }

    Position getPos() {
        return Position(sb.getPosition().x, sb.getPosition().y);
    }

    void degenerate() {
        sb.setPosition(-100, -100);
    }

    void generate() {
        if (id == 1) {
            sb.setPosition(SPEEDBOOST_POS1.x, SPEEDBOOST_POS1.y);
        }
        if (id == 2) {
            sb.setPosition(SPEEDBOOST_POS2.x, SPEEDBOOST_POS2.y);
        }
    }

    void draw(RenderWindow& window) {
        window.draw(sb);
    }
};

SpeedBoost speedBoosts[] = {SpeedBoost(1), SpeedBoost(2)};
Permit permits[] = {Permit(), Permit()};
Key keys[] = {Key(), Key()};

class Pacman {
protected:
    int lives = 3;
    Position pos;
    char direction = 'r';
    int energizer_timer = 0;
    bool isDead = 0;
    int animationTimer = 0;
    bool animationOver = false;
    int score = 0;
public:

    int getScore() {
        return score;
    }

    void incScore(int val) {
        score += val;
    }
    void draw(RenderWindow& window) {

        if (1 == isDead)
        {
            if (animationTimer < PACMAN_DEATH_FRAMES * PACMAN_ANIMATION_SPEED)
            {
                animationTimer++;

                CircleShape pacman(TILE_SIZE / 2);
                pacman.setFillColor(Color(255, 0, 0));
                pacman.setPosition(pos.x, pos.y);

                window.draw(pacman);
            }
            else
            {
                
                animationOver = 1;
            }
        }
        else
        {
            CircleShape pacman(TILE_SIZE / 2);
            pacman.setFillColor(Color(255, 255, 0));
            pacman.setPosition(pos.x, pos.y);

            window.draw(pacman);
        }
    }

    bool getDeadState() {
        return isDead;
    }

    void setDeadState( bool state) {
        pthread_mutex_lock(&mutexScoreLives);
        isDead = state;
        if (state == 1) {
            animationTimer = 0;
            lives--;
        }
        pthread_mutex_unlock(&mutexScoreLives);
    }

    void reset () {
        pthread_mutex_lock(&mutexScoreLives);
        isDead = 0;
        direction = 'r';
        animationTimer = 0;
        animationOver = false;
        energizer_timer = 0;
        score = 0;
        pthread_mutex_unlock(&mutexScoreLives);
    }

    char getDirection() {
        return this->direction;
    }

    Position getPostition() {
        return pos;
    }

    void setPosition(int x, int y) {
        pthread_mutex_lock(&mutexPacmanPos);
        pos = {x, y};
        pthread_mutex_unlock(&mutexPacmanPos);
    }

    void update(std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH>& maze) {

        bool pelletEaten = 0;
        std::array<bool, 4> walls;
        walls[0] = map_collision(0, 0, PACMAN_SPEED + pos.x, pos.y, maze, pelletEaten);
        walls[1] = map_collision(0, 0, pos.x, pos.y - PACMAN_SPEED, maze, pelletEaten);
        walls[2] = map_collision(0, 0, pos.x - PACMAN_SPEED, pos.y, maze, pelletEaten);
        walls[3] = map_collision(0, 0, pos.x, PACMAN_SPEED + pos.y, maze, pelletEaten);


        int dir;
        if (direction == 'r') dir = 0;
        else if (direction == 'u') dir = 1;
        else if (direction == 'l') dir = 2;
        else if (direction == 'd') dir = 3;

        pthread_mutex_lock(&mutexPacmanPos);
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
        pthread_mutex_unlock(&mutexPacmanPos);


        pthread_mutex_lock(&mutexPacmanPos);
        if (-TILE_SIZE >= pos.x)
        {
            pos.x = TILE_SIZE * MAZE_WIDTH - PACMAN_SPEED;
        }
        else if (TILE_SIZE * MAZE_WIDTH <= pos.x)
        {
            pos.x = PACMAN_SPEED - TILE_SIZE;
        }
        pthread_mutex_unlock(&mutexPacmanPos);

        
        if (1 == map_collision(1, 0, pos.x, pos.y, maze, pelletEaten, energizer_timer)) //When Pacman eats an energizer...
        {
            //He becomes energized!
            pthread_mutex_lock(&mutexScoreLives);
            energizer_timer = ENERGIZER_DURATION;
            incScore(10);
            pthread_mutex_unlock(&mutexScoreLives);
        }
        else
        {
            pthread_mutex_lock(&mutexScoreLives);
            energizer_timer = std::max(0, energizer_timer - 1);
            if (pelletEaten) {
                incScore(1);
            }
            pthread_mutex_unlock(&mutexScoreLives);

        }
        

    }

    int getLives() {
        return lives;
    }

    bool getAnimationOver() {
        return animationOver;
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
        bool dummy = false;
        std::array<bool, 4> walls;
        walls[0] = map_collision(0, 0, PACMAN_SPEED + pos.x, pos.y, maze, dummy);
        walls[1] = map_collision(0, 0, pos.x, pos.y - PACMAN_SPEED, maze, dummy);
        walls[2] = map_collision(0, 0, pos.x - PACMAN_SPEED, pos.y, maze, dummy);
        walls[3] = map_collision(0, 0, pos.x, PACMAN_SPEED + pos.y, maze, dummy);

        pthread_mutex_lock(&mutexPacmanPos);
        if (newDirection == 'r')
        {
            if (0 == walls[0]) //since there is wall u cant turn in this direction
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
        pthread_mutex_unlock(&mutexPacmanPos);
        // Unlock mutex after changing Pacman's direction
    }
} pacman;


class Ghost: public Pacman {
    bool hasKey, hasPermit;
    int id;
    char movement_mode; 
    bool use_door;
    Position home;
	bool leftHome;
	Position home_exit = Position(160, 112);
	bool start;
	Position target;
    int frightened_mode;
    int frightened_speed_timer;
    bool canHaveSpeedBoost;
    bool hasSpeedBoost;

public:
    Ghost(int id) {
        this->id = id;
        if (id % 2) canHaveSpeedBoost = true;
        else canHaveSpeedBoost = false;
    }

    bool getStart() {
        return start;
    }

    void update_target(unsigned char i_pacman_direction, const Position& i_ghost_0_position, const Position& i_pacman_position) {
        if (1 == use_door)
        {
            if (pos == target)
            {
                if (home_exit == target) 
                {
                    use_door = 0; 
                    leftHome = true;                     
                }
                else if (home == target) 
                {
                    frightened_mode = 0; 
                    leftHome = 0;
                    use_door = 0;
                    hasKey = false;
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

                       
                        target.x += target.x - i_ghost_0_position.x;
                        target.y += target.y - i_ghost_0_position.y;

                        break;
                    }
                    case 3: 
                    {
                       
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

    bool speedBoostCollision(Position speedBoostPos) {
        if (pos.x > speedBoostPos.x - TILE_SIZE && pos.x < TILE_SIZE + speedBoostPos.x)
        {
            if (pos.y > speedBoostPos.y - TILE_SIZE && pos.y < TILE_SIZE + speedBoostPos.y)
            {
                return 1;
            }
        }
        return 0;
    }

    int keyPermitCollision(Position keyPos, Position permitPos) {
        if (pos.x > keyPos.x - TILE_SIZE && pos.x < TILE_SIZE + keyPos.x)
        {
            if (pos.y > keyPos.y - TILE_SIZE && pos.y < TILE_SIZE + keyPos.y)
            {
                return 1; // for key
            }
        }

        if (pos.x > permitPos.x - TILE_SIZE && pos.x < TILE_SIZE + permitPos.x)
        {
            if (pos.y > permitPos.y - TILE_SIZE && pos.y < TILE_SIZE + permitPos.y)
            {
                return 2; // for permit
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
        
        pthread_mutex_lock(&ghostVars[id]);
        hasKey = false; hasPermit = false;
        movement_mode = 's';
        leftHome = 0; // when this is 1 this means ghost behaviour outside home activated
        use_door = 0; // this is 1 means that ghost can go outside the house
        direction = 'r';
        home = i_home;
        target = home_exit;
        frightened_mode = 0;
	    frightened_speed_timer = 0;
        start = false;
        hasSpeedBoost = false;
        pthread_mutex_unlock(&ghostVars[id]);
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

        pthread_mutex_lock(&ghostVars[id]);
        ghostHead.setPosition(pos.x, pos.y);
        ghostBody.setPosition(pos.x, pos.y + TILE_SIZE / 2);
        pthread_mutex_unlock(&ghostVars[id]);

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

    void setStart(bool val) {
        start = val;
    }

    void update(std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH>& maze, Ghost& i_ghost_0, Pacman& i_pacman) {
        int availableWays = 0;
        bool move = 0;

        int speed = GHOST_SPEED;
        
        // begin read
        pthread_mutex_lock(&ghostRead);
        ghostReaders++;
        if (ghostReaders == 1) pthread_mutex_lock(&mutexPacmanPos);
        pthread_mutex_unlock(&ghostRead);


        //activating frightened-mode
        if (0 == frightened_mode && i_pacman.get_energizer_timer() == ENERGIZER_DURATION)
        {
            frightened_speed_timer = GHOST_FRIGHTENED_SPEED;
            frightened_mode = 1;
        }
        else if (0 == i_pacman.get_energizer_timer() && 1 == frightened_mode)
        {
            frightened_mode = 0;
        }
        // end read
        pthread_mutex_lock(&ghostRead);
        ghostReaders--;
        if (ghostReaders == 0) pthread_mutex_unlock(&mutexPacmanPos);
        pthread_mutex_unlock(&ghostRead);
        
       
        if (2 == frightened_mode && 0 == pos.x % GHOST_ESCAPE_SPEED && 0 == pos.y % GHOST_ESCAPE_SPEED)
        {
            speed = GHOST_ESCAPE_SPEED;
        }
        
        if (frightened_mode == 0 && hasSpeedBoost && 0 == pos.x % (SPEED_UP) && 0 == pos.y % (SPEED_UP)) {
            speed = SPEED_UP;
        }

        // begin read
        pthread_mutex_lock(&ghostRead);
        ghostReaders++;
        if (ghostReaders == 1) pthread_mutex_lock(&mutexPacmanPos);
        pthread_mutex_unlock(&ghostRead);
        
        update_target(i_pacman.getDirection(), i_ghost_0.getPostition(), i_pacman.getPostition());
        
        // end read
        pthread_mutex_lock(&ghostRead);
        ghostReaders--;
        if (ghostReaders == 0) pthread_mutex_unlock(&mutexPacmanPos);
        pthread_mutex_unlock(&ghostRead);
        

        bool dummy = false;
        std::array<bool, 4> walls;
        walls[0] = map_collision(0, use_door, GHOST_SPEED + pos.x, pos.y, maze, dummy);
        walls[1] = map_collision(0, use_door, pos.x, pos.y - GHOST_SPEED, maze, dummy);
        walls[2] = map_collision(0, use_door, pos.x - GHOST_SPEED, pos.y, maze, dummy);
        walls[3] = map_collision(0, use_door, pos.x, GHOST_SPEED + pos.y, maze, dummy);

        int dir;
        if (direction == 'r') dir = 0;
        else if (direction == 'u') dir = 1;
        else if (direction == 'l') dir = 2;
        else if (direction == 'd') dir = 3;


        if (1 != frightened_mode && leftHome)
        {
            unsigned char optimal_direction = 4;

            move = 1;

            for (unsigned char a = 0; a < 4; a++)
            {
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
                        optimal_direction = a;
                    }
                }
            }

            if (1 < availableWays)
            {
                pthread_mutex_lock(&ghostVars[id]);
                direction = convertDir(optimal_direction);
                pthread_mutex_unlock(&ghostVars[id]);
            }
            else
            {
                if (4 == optimal_direction)
                {
                    pthread_mutex_lock(&ghostVars[id]);
                    direction = convertDir((2 + dir) % 4);
                    pthread_mutex_unlock(&ghostVars[id]);
                }
                else
                {   pthread_mutex_lock(&ghostVars[id]);
                    direction = convertDir(optimal_direction);
                    pthread_mutex_unlock(&ghostVars[id]);
                }
            }
        }
        else
        {
            unsigned char random_direction = rand() % 4;
            if (!leftHome) 
            {
                move = 1;
                for (unsigned char a = 0; a < 4; a++)
                {
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
                        random_direction = rand() % 4;
                    }

                    pthread_mutex_lock(&ghostVars[id]);
                    direction = convertDir(random_direction);
                    pthread_mutex_unlock(&ghostVars[id]);
                }
                else
                {
                    pthread_mutex_lock(&ghostVars[id]);
                    direction = convertDir((2 + dir) % 4);
                    pthread_mutex_unlock(&ghostVars[id]);
                }
            } else {
                if (0 == frightened_speed_timer)
                {
                    move = 1;
                    frightened_speed_timer = GHOST_FRIGHTENED_SPEED;
                    for (unsigned char a = 0; a < 4; a++)
                    {
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
                            random_direction = rand() % 4;
                        }
                        pthread_mutex_lock(&ghostVars[id]);
                        direction = convertDir(random_direction);
                        pthread_mutex_unlock(&ghostVars[id]);
                    }
                    else
                    {
                        pthread_mutex_lock(&ghostVars[id]);
                        direction = convertDir((2 + dir) % 4);
                        pthread_mutex_unlock(&ghostVars[id]);
                    }
                }
                else
                {
                    frightened_speed_timer--;
                }
            }
        }

        
        if (move == 1) {
            pthread_mutex_lock(&ghostVars[id]);
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
            pthread_mutex_unlock(&ghostVars[id]);
        }

        // begin write
        pthread_mutex_lock(&ghostRead);
        ghostReaders++;
        if (ghostReaders == 1) pthread_mutex_lock(&mutexPacmanPos);
        pthread_mutex_unlock(&ghostRead);

        Position pacPos = i_pacman.getPosition();

        // end read
        pthread_mutex_lock(&ghostRead);
        ghostReaders--;
        if (ghostReaders == 0) pthread_mutex_unlock(&mutexPacmanPos);
        pthread_mutex_unlock(&ghostRead);

        for (int i = 0; i < 2; i++) {
            pthread_mutex_lock(&speedBoostMutex);
            if (speedBoostCollision(speedBoosts[i].getPos()) && canHaveSpeedBoost && !hasSpeedBoost) {
                if (speedBoost > 0) {
                    speedBoosts[speedBoost - 1].degenerate();
                    speedBoost--;
                    hasSpeedBoost = true;
                }
            }
            pthread_mutex_unlock(&speedBoostMutex);
        }

        for (int i = 0; i < 2; i++) {
            pthread_mutex_lock(&keyPermitMutex);
            int status = keyPermitCollision(keys[i].getPos(), permits[i].getPos());
            if (status == 1) { // for key;
                if (!hasKey) {
                    if (key > 0) {
                        keys[i].degenerate();
                        key--;
                        hasKey = true;
                    }
                }
            }
            pthread_mutex_unlock(&keyPermitMutex);
            pthread_mutex_lock(&keyPermitMutex);
            if (status == 2) { // for permit
                if (hasKey) {
                    if (permit > 0) {
                        permits[i].degenerate();
                        permit--;
                        use_door = 1;
                        leftHome = 1;
                        target = home_exit;
                    }
                }
            }
            pthread_mutex_unlock(&keyPermitMutex);
        }

        if (1 == pacman_collision(pacPos))
        {
            if (0 == frightened_mode) 
            {
                i_pacman.setDeadState(1);
            }
            else 
            {   
                pthread_mutex_lock(&mutexPacmanPos);
                use_door = 1;
                frightened_mode = 2;
                target = home;
                pthread_mutex_unlock(&mutexPacmanPos);
            }
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

    void reset() {
        for (Ghost& ghost : ghosts) {
            ghost.reset(ghosts[2].getPostition(), ghosts[0].getPostition());
        }
    } 

} ghostManager;

std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH> getMaze(const std::array<std::string, MAZE_HEIGHT>& i_map_sketch, std::vector<Ghost>& ghosts, Pacman& pacman)
{
	std::array<std::array<Tile, MAZE_HEIGHT>, MAZE_WIDTH> output_map{};
	for (unsigned char a = 0; a < MAZE_HEIGHT; a++)
	{
		for (unsigned char b = 0; b < MAZE_WIDTH; b++)
		{
			output_map[b][a] = Tile::Empty;

			switch (i_map_sketch[a][b])
			{
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
				//0 denotes Red ghost
				case '0':
				{
					ghosts[0].setPosition(TILE_SIZE * b, TILE_SIZE * a);
					break;
				}
				//1 denotes Pink ghost
				case '1':
				{
					ghosts[1].setPosition(TILE_SIZE * b, TILE_SIZE * a);

					break;
				}
				//2 denotes Blue (cyan) ghost
				case '2':
				{
					ghosts[2].setPosition(TILE_SIZE * b, TILE_SIZE * a);

					break;
				}
				//3 denotes orange ghost
				case '3':
				{
					ghosts[3].setPosition(TILE_SIZE * b, TILE_SIZE * a);

					break;
				}
				//P denotes  pacman
				case 'P':
				{
					pacman.setPosition(TILE_SIZE * b, TILE_SIZE * a);
					break;
				}
				//o denotes power pellet
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
    sf::Texture wallTexture;
    wallTexture.loadFromFile("assets/walls.jpeg");

     sf::Texture doorTexture;
    doorTexture.loadFromFile("assets/DoorTexture.jpeg");
      sf::Sprite doorSprite(doorTexture);
    doorSprite.setScale(TILE_SIZE / doorTexture.getSize().x, TILE_SIZE / doorTexture.getSize().y);
    
     sf::Sprite wallSprite(wallTexture);
     RectangleShape tile(Vector2f(TILE_SIZE, TILE_SIZE));
    
     float scale = 16.f / 512.f; 
   // tile.setScale(scale, scale);
    wallSprite.setScale(TILE_SIZE / wallTexture.getSize().x, TILE_SIZE / wallTexture.getSize().y);


    for (int i = 0; i < MAZE_WIDTH; i++) {
        for (int j = 0; j < MAZE_HEIGHT; j++) {
            tile.setPosition(TILE_SIZE * i, TILE_SIZE * j);
            if (maze[i][j] == Tile::Wall) {
                 tile.setTexture(&wallTexture); 
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
                tile.setTexture(&doorTexture); 
               // window.draw(doorSprite);
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
            if (!pacman.getDeadState()) pacman.update(maze);

            // Subtract the fixed timestep from accumulated time
            accumulatedTime -= FRAME;
        }
        // Rendering

       
    }

    return nullptr;
}


// Ghost Controller Thread Function
void* ghostControllerThread(void* arg) {
    int ghostId = *((int*)arg);

    Clock clock;
    Clock modeSwitchClock; int modeSwitchTime = 8;
    int accumulatedTime = 0;
    while (true) {
        // Calculate elapsed time since the last update
        int dt = clock.restart().asMicroseconds();
        accumulatedTime += dt;

        
        while (accumulatedTime >= FRAME) {
            // Game logic
            if (modeSwitchClock.getElapsedTime().asSeconds() >= modeSwitchTime && !ghostManager.ghosts[ghostId].getStart()) {
                
                ghostManager.ghosts[ghostId].switchMovementMode();
                ghostManager.ghosts[ghostId].setStart(true);
                
                modeSwitchClock.restart();
            }
            if (!pacman.getDeadState()) {
                
                ghostManager.ghosts[ghostId].update(maze, ghostManager.ghosts[0], pacman);
            }
            accumulatedTime -= FRAME;
        }
       
    }

    return nullptr;
}

void draw_text(unsigned short i_x, unsigned short i_y, const std::string& i_text, sf::RenderWindow& i_window)
{
	short character_x = i_x;
	short character_y = i_y;

	unsigned char character_width;

	sf::Sprite character_sprite;

	sf::Texture font_texture;
	font_texture.loadFromFile("assets/Font.png");

	character_width = font_texture.getSize().x / 96;

	character_sprite.setTexture(font_texture);

	for (std::string::const_iterator a = i_text.begin(); a != i_text.end(); a++)
	{
		if ('\n' == *a)
		{

			character_x = i_x;

			character_y += FONT_HEIGHT;

			continue;
		}

		character_sprite.setPosition(character_x, character_y);
		character_sprite.setTextureRect(sf::IntRect(character_width * (*a - 32), 0, character_width, FONT_HEIGHT));

		character_x += character_width;
		i_window.draw(character_sprite);
	}

    if ((TILE_SIZE * MAZE_WIDTH - 200) == i_x) {
        for (int i = 0; i < pacman.getLives(); i++) {
            CircleShape pacman(TILE_SIZE / 2);
            pacman.setFillColor(Color(255, 255, 0));
            pacman.setPosition(character_x + TILE_SIZE * i, character_y);

            i_window.draw(pacman);
        }
    }
}

void* produceKeyPermit(void*) {
    srand(time(NULL));
    sf::Clock clock;
    const float productionInterval = 5;
    while (true) {
    // Check if enough time has passed since the last production
        sf::Time elapsedTime = clock.getElapsedTime();
        if (elapsedTime.asSeconds() >= productionInterval) {
            // Generate random empty position within energizer locations
            int x = rand() % 2;

            if (x == 1) // generate key 
            {
                pthread_mutex_lock(&keyPermitMutex);
                if (key < 2) {
                    keys[key].generate();
                    key++;
                }
                pthread_mutex_unlock(&keyPermitMutex);
            }
            else { // generate permit
                pthread_mutex_lock(&keyPermitMutex);
                if (permit < 2) {
                    permits[permit].generate();
                    permit++;
                }
                pthread_mutex_unlock(&keyPermitMutex);
            }
            clock.restart();
        }

    }

    return NULL;
}

void* produceSpeedBoost(void*) {
    sf::Clock clock;
    const float productionInterval = 5;
    while (true) {
    // Check if enough time has passed since the last production
        sf::Time elapsedTime = clock.getElapsedTime();
        if (elapsedTime.asSeconds() >= productionInterval) {
            // Generate random empty position within energizer locations
            pthread_mutex_lock(&speedBoostMutex);   
            if (speedBoost < 2) {
                speedBoosts[speedBoost].generate();
                speedBoost++;
            }
            pthread_mutex_unlock(&speedBoostMutex);   
            clock.restart();
        }
    }

}

void* produceEnergizer(void*) {

    sf::Clock clock;
    const float productionInterval = 10;
    while (true) {
    // Check if enough time has passed since the last production
        sf::Time elapsedTime = clock.getElapsedTime();
        if (elapsedTime.asSeconds() >= productionInterval) {
            // Generate random empty position within energizer locations
            vector<bool> emptyPositionsState(4, false);
            vector<int> emptyPositions;
            for (int i = 0; i < 4; i++) {
                if (maze[energizerPos[i].x][energizerPos[i].y] == Tile::Empty) {
                    emptyPositionsState[i] = true;
                    emptyPositions.push_back(i);
                }
            }
            if (emptyPositions.size() == 0) {
                clock.restart();
                continue;
            }
            int randomPositionIndex = rand() % emptyPositions.size();
            int randomPos = emptyPositions[randomPositionIndex];
            Position randomPosition = energizerPos[randomPos];

            pthread_mutex_lock(&enrgizerMutex);
            if (energizersProduced != 4) {
                energizersProduced++;
                maze[randomPosition.x][randomPosition.y] = Tile::Energizer;
            }
            pthread_mutex_unlock(&enrgizerMutex);    
            clock.restart();
        }
    }

    return NULL;
}

int main() {
    srand(time(NULL));
    // Initialize mutex
    pthread_mutex_init(&mutexPacmanPos, NULL);
    pthread_mutex_init(&ghostRead, NULL);
    pthread_mutex_init(&mutexScoreLives, NULL);
    pthread_mutex_init(&gridLock, NULL);
    pthread_mutex_init(&enrgizerMutex, NULL);
    pthread_mutex_init(&keyPermitMutex, NULL);
    pthread_mutex_init(&speedBoostMutex, NULL);


    for (int i = 0; i < 4; i++) {
        pthread_mutex_init(&ghostVars[i], NULL);
    }


    sf::RenderWindow window(sf::VideoMode(TILE_SIZE * MAZE_WIDTH * SCREEN_RESIZE, (FONT_HEIGHT + TILE_SIZE * MAZE_HEIGHT) * SCREEN_RESIZE), "Pac-Man", sf::Style::Close);
	window.setView(sf::View(sf::FloatRect(0, 0, TILE_SIZE * MAZE_WIDTH, FONT_HEIGHT + TILE_SIZE * MAZE_HEIGHT)));

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    sf::Music music;
     music.openFromFile("assets/Thors Hammer - Ethan Meixsell.wav");
      music.play();
    
   

    pthread_t keyCardProducerID;
    pthread_create(&keyCardProducerID, &attr, produceKeyPermit, NULL);

    // Create user interface thread
    pthread_t userInterfaceThreadID;
    pthread_create(&userInterfaceThreadID, &attr, userInterfaceThread, NULL);
    
    pthread_t energizerProducerID;
    pthread_create(&energizerProducerID, &attr, produceEnergizer, NULL);

    pthread_t speedBoostProducerID;
    pthread_create(&speedBoostProducerID, &attr, produceSpeedBoost, NULL);


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

            if (pacman.getDeadState() && pacman.getAnimationOver()) {
                if (pacman.getLives() == 0) {
                    window.close();
                }
                maze = getMaze(mazeMapping, ghostManager.ghosts, pacman);
                pacman.reset();
                ghostManager.reset();
            }

            if (FRAME > delay) {
                window.clear(sf::Color::Black);

                if (!pacman.getDeadState()) {
                    drawMap(maze, window);

                    pthread_mutex_lock(&keyPermitMutex);
                    keys[0].draw(window); permits[0].draw(window);
                    keys[1].draw(window); permits[1].draw(window);
                    pthread_mutex_unlock(&keyPermitMutex);

                    pthread_mutex_lock(&speedBoostMutex);
                    speedBoosts[0].draw(window); speedBoosts[1].draw(window);
                    pthread_mutex_unlock(&speedBoostMutex);

                    ghostManager.draw(window);
                    draw_text(0, TILE_SIZE * MAZE_HEIGHT, "Score: " + std::to_string(pacman.getScore()), window);
                    draw_text(TILE_SIZE * MAZE_WIDTH - 200, TILE_SIZE * MAZE_HEIGHT, "Lives: ", window);
                }

                pacman.draw(window);
                window.display();
            }
        }
    }

    pthread_mutex_destroy(&mutexPacmanPos);
    pthread_mutex_destroy(&ghostRead);
    pthread_mutex_destroy(&mutexScoreLives);
    pthread_mutex_destroy(&gridLock);
    pthread_mutex_destroy(&enrgizerMutex);
    pthread_mutex_destroy(&keyPermitMutex);
    pthread_mutex_destroy(&speedBoostMutex);

    for (int i = 0; i < 4; i++) {
        pthread_mutex_destroy(&ghostVars[i]);
    }

    return 0;
}
