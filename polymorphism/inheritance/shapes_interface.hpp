#pragma once
#include <iostream>

struct shape {
	virtual void draw()const = 0;
	virtual ~shape() {}
};
