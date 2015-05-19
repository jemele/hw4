// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#ifndef _search_h_
#define _search_h_

// Information about cell needed to support search (A*).
struct search_cell {

    // the location of the cell.
    // this should uniquely identify all cells.
    int x,y;

    // used to trace the path.
    struct search_cell *parent;

    // this cell is on the open list.
    int open, blocked, closed;

    // cost metrics.
    int g;
    int h;
    int f;

};
typedef struct search_cell search_cell_t;

// A collection of cells, and their dimension.
struct search_map {
    int dim_x, dim_y;
    search_cell_t * cells;
};
typedef struct search_map search_map_t;

// Allocate internal structures for the map from the heap. This is needed
// because there's not enough stack space to store everything we need.
int search_map_alloc(search_map_t *map, int dim_x, int dim_y);

// Initialize an allocated search map to defaults. This resets all cell fields
// to meaningful defaults. This should be called before using search_find.
void search_map_initialize(search_map_t *map);

// Return a reference to the cell in the map at the specified location.
search_cell_t *search_cell_at(search_map_t *map, int x, int y);

// Find the goal given the map and start cell. The map must be initialized with
// search_map_initialize before calling this function.
void search_find(search_map_t *map, search_cell_t *start, search_cell_t *goal);

// Free any dynamic memory associated with the map.
void search_map_free(search_map_t *map);
#endif
