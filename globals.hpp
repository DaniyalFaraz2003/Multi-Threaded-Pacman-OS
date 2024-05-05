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

	bool operator==(const Position& i_position)
	{
		return this->x == i_position.x && this->y == i_position.y;
	}
};