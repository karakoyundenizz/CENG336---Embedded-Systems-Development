#ifndef MAP_H 
#define MAP_H

#include<stdint.h>

// The game map definition,
// You can change this to test your implementation.
// We will provide a fresh copy when testing your code.
#define MAP_SIZE 23

static const uint8_t SCENE_ARRAY[4][MAP_SIZE] = 
{
    {1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1},
    {0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0},
    {0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0}
};

#endif // MAP_H