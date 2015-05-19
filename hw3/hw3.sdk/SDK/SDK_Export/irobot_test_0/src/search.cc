// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <iostream>
#include <algorithm>
#include <set>
#include <iterator>
#include <limits>

extern "C" {
#include "search.h"
}

// Provide ordering for the open set.
// This will cause the open set to return the cell with the smallest f first.
struct order_cell_by_f {
    bool operator()(const search_cell_t *lhs, const search_cell_t *rhs) const {
        return lhs->f < rhs->f;
    }
};

std::ostream& operator<<(std::ostream &os, const search_cell_t *c)
{
   return os << c->x << ',' << c->y;
}

search_cell_t* search_cell_at(search_map_t *map, int x, int y)
{
    return &map->cells[x + (map->dim_x *y)];
}

// The distance heuristic between two points.
// Because the actual path to the points isn't known, this is just a guess.
static int h_distance(search_cell_t *start, search_cell_t *goal)
{
    return std::abs(start->x-goal->x) + std::abs(start->y-goal->y);
}

int search_map_alloc(search_map_t *map, int dim_x, int dim_y)
{
    map->cells = (search_cell_t*)malloc(sizeof(search_cell_t)*dim_x*dim_y);
    if (!map->cells) {
        std::cout << "malloc failed" << std::endl;
        return 1;
    }
    map->dim_x = dim_x;
    map->dim_y = dim_y;
    return 0;
}

void search_map_free(search_map_t *map)
{
    if (map->cells) {
        free(map->cells);
        map->cells = 0;
    }
    map->dim_x = 0;
    map->dim_y = 0;
}

// Initialize the map.
void search_map_initialize(search_map_t *map)
{
    for (int i = 0; i < map->dim_x; ++i) {
        for (int j = 0; j < map->dim_y; ++j) {
            search_cell_t *current = search_cell_at(map,i,j);
            current->x = i;
            current->y = j;
            current->g = std::numeric_limits<int>::max();
            current->h = std::numeric_limits<int>::max();
            current->prev = 0;
            current->next = 0;
            current->open = false;
            current->blocked = false;
            current->closed = false;
        }
    }
}

void search_find(search_map_t *map, search_cell_t *start, search_cell_t *goal)
{
    // put the starting point on the open list.
    typedef std::set<search_cell_t*,order_cell_by_f> open_t;
    open_t open;
    start->g = 0;
    start->h = h_distance(start,goal);
    start->f = start->g + start->h;
    start->open = true;
    open.insert(start);

    std::cout << "start: " << start << std::endl
              << "goal: " << goal
              << std::endl;

    const int dim_x = map->dim_x;
    const int dim_y = map->dim_y;

    // While there are still nodes to process ...
    while (!open.empty() && !goal->closed) {

        // Process the next open.
        open_t::iterator i(open.begin());
        search_cell_t *current = *i;
        current->open = false;
        current->closed = true;
        open.erase(i);
 
        // Check all adjacent cells.
        // Ignore cells that are closed or unpassable.
        static const int p[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        for (unsigned i = 0; i < sizeof(p)/sizeof(*p); ++i) {
            const int x = current->x + p[i][0];
            const int y = current->y + p[i][1];
            if ((x < 0) || (y < 0) || (x >= dim_x) || (y >= dim_y)) {
                continue;
            }
            search_cell_t *adj = search_cell_at(map,x,y);
            if (adj->blocked || adj->closed) {
                continue;
            }

            // If adj is already open, check if the path through current is
            // better, and if so, use the path through current. Otherwise, use
            // the path through current and place adj on the open list.
            if (adj->open && ((current->g+1) < adj->g)) {
                adj->prev = current;
                adj->g = current->g + 1;
                adj->h = h_distance(adj,goal);
                adj->f = adj->g + adj->h;
            } else {
                adj->prev = current;
                adj->g = current->g + 1;
                adj->h = h_distance(adj,goal);
                adj->f = adj->g + adj->h;
                adj->open = true;
                open.insert(adj);
            }
        }
    }

    // If the goal was found and a path exists, build a forward path for
    // convenience.
    if (goal->closed) {
        search_cell_t *c;
        for (c = goal; c->prev; c = c->prev) {
            if (c->prev) {
                c->prev->next = c;
            }
        }
    }
}
