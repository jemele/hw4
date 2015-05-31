// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#ifndef _direction_h_
#define _direction_h_

// Please don't change these values. The direction_rotate function explicitly
// relies on the supplied encoding.
typedef enum {
    direction_left    = 0,
    direction_forward = 1,
    direction_right   = 2,
    direction_back    = 3,
    direction_count,
} direction_t;

// Handy for debugging.
const char *direction_t_to_string(direction_t v);

// Calculate the direction based on the delta. This assumes we're starting
// forward.
int direction_from_delta(int dx, int dy);

// Calculate the number and type of rotation needed to achieve the next
// direction from the current direction.
void direction_rotation(int current, int next, char *rotation, int *count);

#endif
