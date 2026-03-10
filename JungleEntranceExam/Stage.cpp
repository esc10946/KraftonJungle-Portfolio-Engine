#include "Stage.h"
//Grid?

static const int COLS = 13;
static const float HALF_BW = 0.065;// (2.0f - a)/ 13.0f / 2.0f
static const float HALF_BH = 0.035;// HALF_BW * 0.5f +a
static const float STEP_X = HALF_BW * 2.0f;//-+a // Row gap
static const float STEP_Y = HALF_BH * 2.0f;//+a   // Col gap
static const float START_X = 2 * HALF_BW;// +a // first block center x
static const float START_Y = 2 * HALF_BH;// +a // first block center y

#define N(c) (c) 
#define H 100
#define I 200
#define _ 0

static EBlockColor ColorTable[]=
{ 
	EBlockColor::White,		//0
	EBlockColor::Orange,	//1
	EBlockColor::Cyan,		//2
	EBlockColor::Green,		//3
	EBlockColor::Red,		//4
	EBlockColor::Blue,		//5
	EBlockColor::Magenta,	//6
	EBlockColor::Yellow		//7
};

static int stage1[6][13] =
{
	H,	 H,	  H,   H,	H,	 H,	  H,   H,	H,	 H,	  H,   H,	H,		//Hard
	N(4),N(4),N(4),N(4),N(4),N(4),N(4),N(4),N(4),N(4),N(4),N(4),N(4),	//R
	N(7),N(7),N(7),N(7),N(7),N(7),N(7),N(7),N(7),N(7),N(7),N(7),N(7),	//Y
	N(5),N(5),N(5),N(5),N(5),N(5),N(5),N(5),N(5),N(5),N(5),N(5),N(5),	//B
	N(6),N(6),N(6),N(6),N(6),N(6),N(6),N(6),N(6),N(6),N(6),N(6),N(6),	//M
	N(3),N(3),N(3),N(3),N(3),N(3),N(3),N(3),N(3),N(3),N(3),N(3),N(3)	//G
};
static int stage2[13][13] =
{
};
static int stage3[13][13] =
{
};

#undef N
#undef H
#undef I
#undef _
static Block* MakeBlock(int code, float cx, float cy, int round)
{
	if(code == 0)
		return nullptr;
	EBlockType type;
	EBlockColor color;
	if (code == 200)
	{
		type = EBlockType::Immortal;
		color = EBlockColor::Gold;
	}
	else if(code == 100)
	{
		type = EBlockType::Hard;
		color = EBlockColor::Silver;
	}
	else
	{
		type = EBlockType::Normal;
		color = ColorTable[code];
	}
	Block* block = new Block(type, color, round);
	//block->Init(); grid 사용할것
	return block;
}
template <int ROWS>
static std::vector<Block*> BulidStage(int(&map)[ROWS][COLS], int round)
{
	std::vector<Block*> blocks;
	blocks.reserve(ROWS*COLS);

	for (int r = 0; r < ROWS; ++r)
	{
		for (int c = 0; c < COLS; ++c)
		{
			int code = map[r][c];
			if (code == 0) 
				continue;
			float cx = START_X + c * STEP_X;
			float cy = START_Y + r * STEP_Y;
			Block* b = MakeBlock(code, cx, cy, round);
			if(b)
				blocks.push_back(b);
		}
	}
	return blocks;
}
std::vector<Block*> CreateStage(int Round)
{
	switch (Round)
	{
	case 1:
		BulidStage(stage1, Round);
		break;
	case 2:
		BulidStage(stage2, Round);
		break;
	case 3:
		BulidStage(stage3, Round);
		break;
	}
    return std::vector<Block*>();
}
