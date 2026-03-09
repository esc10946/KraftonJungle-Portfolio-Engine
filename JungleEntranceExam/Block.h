#pragma once


class Block
{
	enum Btype
	{
		normal,
		hard,
		immortal
	};

	Block();
	~Block();
	void Collider();

private:
	Btype type;
	int hp;
	int score;
};
