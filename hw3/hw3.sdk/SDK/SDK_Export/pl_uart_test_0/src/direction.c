// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdio.h>
#include <unistd.h>
#include "direction.h"

const char *direction_t_to_string(direction_t v) {
    static const char *s[] = {
    [direction_left]    "left",
    [direction_forward] "forward",
    [direction_right]   "right",
    [direction_back]    "back",
    };
    return s[v];
}

#define abs(x) ((x<0)?-x:x)
#define sign(x) ((x<0)?-1:1)

// Calculate the moves needed to go from one direction to another.
void direction_rotation(int current, int next, char *rotation, int *count)
{
    *count = 0;
    if (current == next) {
        return;
    }
    int delta = next - current;
    if (abs(delta) == 3) {
        delta = -sign(delta);
    }
    *rotation = ((delta < 0) ? 'L' : 'R');
    *count = abs(delta);
    printf("%s->%s: %d%c\n", direction_t_to_string(current),
            direction_t_to_string(next), *count, *rotation);
}

// Calculate the final orientation of the robot given dx and dy.  This assumes
// that dx and dy cannot *both* be set, i.e.., diagonal moves are not
// permitted.
int direction_from_delta(int dx, int dy)
{
    switch (dx) {
    case -1: return direction_left;
    case +1: return direction_right;
    }
    switch (dy) {
    case -1: return direction_back;
    case +1: return direction_forward;
    }

    // We should never get here
    printf("panic: unknown direction!\n");
    return direction_forward;
}

