#pragma once
#include "UBlock.h"
#include <vector>

std::vector<UBlock*> CreateStage(int Round);

void GetStageInfo(int StageNum, int& outRow, int& outCol)