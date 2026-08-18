#include "SSplitter.h"

SSplitter::~SSplitter()
{
	for (SWidget* Child : GetChildren())
	{
		delete Child;
	}
	Children.clear();
}