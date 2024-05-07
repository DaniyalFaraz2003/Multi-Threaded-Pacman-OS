enum Tile
{
	Door,
	Empty,
	Energizer,
	Pellet,
	Wall
};

struct Position
{
	int x;
	int y;

	Position(int x = 0, int y = 0): x(x), y(y) {}

	bool operator==(const Position& i_position)
	{
		return this->x == i_position.x && this->y == i_position.y;
	}
};