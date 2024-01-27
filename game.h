#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <vector>

//this could be used to make the indexing issues go away!
//we can bake it in there too if we want...
#define COORDS_TO_IDX(x, y, z, w) ((x) + n*((y) + n*((z) + n*(w))))

//did we win?
extern bool victory, defeat;
extern int remaining_safe;

uint32_t rng();
void generate(uint8_t *grid, unsigned n, int mines, int first_click_sq);
void reveal(uint8_t *grid, int n, int idx);
