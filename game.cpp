#include "game.h"

//globals
int n_mines; //mines in grid
int remaining_safe = -1; //-1 = before first click; 0 = victory
bool victory = false; //did we win?
bool defeat = false; //did we go boom?

//TODO: improve potentially (use raylib rng? or sth else?)
uint32_t rng()
{
    //on linux RAND_MAX is INT32_MAX, on windows its INT16_MAX
    //so when n >= 14, n^4 > RAND_MAX, so we need sth like this
#if RAND_MAX < 65536
    return RAND_MAX * rand() + rand();
#else
    return rand();
#endif
}

//Generate a grid (WARNING: this is O(n^4), obviously), with zero mines surrounding first_click_sq
void generate(uint8_t *grid, unsigned n, int mines, int first_click_sq)
{
    unsigned n4 = n*n*n*n;

    //init globals
    n_mines = mines;
    remaining_safe = n4 - mines;

    for (unsigned i = 0; i < n4; i++)
    {
        //fill with empty squares at first
        grid[i] = 82; //x/82 == 1 => hidden (at the start everything is hidden)
    }

    //place mines
    for (int i = 0; i < mines; i++)
    {
        //recommended amount of mines is 1/31 of the whole grid
        //this means that in the worst case the average number of attempts is 1 + 1/30
        //so rejection sampling is acceptable
        int idx = rng() % n4;
        while (grid[idx] == 163) //avoid already placed mines
            idx = rng() % n4;

        //get coordinates of mine
        int tmp = idx;
        int x = tmp % n;
        tmp /= n;
        int y = tmp % n;
        tmp /= n;
        int z = tmp % n;
        tmp /= n;
        int w = tmp % n;

        //if we landed next to the first click, we have to go again!
        for (int k = 0; k < 81; k++)
        {
            int dx = k % 3 - 1;
            int dy = (k/3) % 3 - 1;
            int dz = (k/9) % 3 - 1;
            int dw = (k/27) % 3 - 1;

            //see if the square actually exists and we didn't wrap around an edge!!! (important!)
            if (x + dx < 0 || x + dx >= n) continue;
            if (y + dy < 0 || y + dy >= n) continue;
            if (z + dz < 0 || z + dz >= n) continue;
            if (w + dw < 0 || w + dw >= n) continue;

            //calculate current square
            //TODO: replace with COORDS_TO_IDX
            int target = idx + dx + n*(dy + n*(dz + n*dw));
            //if the square is next to the first click, we can't put a mine there!
            if (target == first_click_sq)
            {
                i--;
                tmp = -1; //error value to signal that we got out of this loop
                break;
            }
        }
        if (tmp == -1) continue; //and then repeat!

        //put a mine there
        grid[idx] = 163; //163 is code for a (hidden, unflagged) mine

        //update neighboring grid cell numbers
        for (int k = 0; k < 81; k++)
        {
            if (k == 40) continue; //we are on the current square! skip it!
            int dx = k % 3 - 1;
            int dy = (k/3) % 3 - 1;
            int dz = (k/9) % 3 - 1;
            int dw = (k/27) % 3 - 1;

            //see if the square actually exists and we didn't wrap around an edge!!! (important!)
            if (x + dx < 0 || x + dx >= n) continue;
            if (y + dy < 0 || y + dy >= n) continue;
            if (z + dz < 0 || z + dz >= n) continue;
            if (w + dw < 0 || w + dw >= n) continue;

            //calculate current square
            //TODO: replace with COORDS_TO_IDX
            int target = idx + dx + n*(dy + n*(dz + n*dw));
            //if the square is not a mine
            if (grid[target] != 163)
                grid[target]++; //add 1 to the neighboring mine number
        }
    }
}

//Reveal a square (by index)
void reveal(uint8_t *grid, int n, int start_idx)
{
    //breadth first search for zero spreading, to avoid stack overflow
    std::vector<unsigned> stack;
    stack.push_back(start_idx);

    while (!stack.empty())
    {
        //get last element
        unsigned idx = stack[stack.size() - 1];
        stack.pop_back();

        //make sure we don't decrease this for already cleared squares
        if (grid[idx] >= 82) remaining_safe--;
        else continue;

        grid[idx] %= 82; //x/82 == 0 => revealed
        if (grid[idx] == 81) //BOOM
            defeat = true;
        else //we cleared a safe square!
        {
            if(remaining_safe == 0) //victory check
                victory = true;
        }

        //we just got a zero: reveal everything else!
        if (grid[idx] == 0)
        {
            //get coordinates of current square
            int tmp = idx;
            int x = tmp % n;
            tmp /= n;
            int y = tmp % n;
            tmp /= n;
            int z = tmp % n;
            tmp /= n;
            int w = tmp % n;

            for (int k = 0; k < 81; k++)
            {
                if (k == 40) continue; //don't redo same square multiple times
                int dx = k % 3 - 1;
                int dy = (k/3) % 3 - 1;
                int dz = (k/9) % 3 - 1;
                int dw = (k/27) % 3 - 1;

                //see if the square actually exists and we didn't wrap around an edge!!! (important!)
                if (x + dx < 0 || x + dx >= n) continue;
                if (y + dy < 0 || y + dy >= n) continue;
                if (z + dz < 0 || z + dz >= n) continue;
                if (w + dw < 0 || w + dw >= n) continue;

                //calculate current square
                //TODO: replace with COORDS_TO_IDX
                int target = idx + dx + n*(dy + n*(dz + n*dw));
                //if the square is not already revealed (stops infinite recursion as well)
                if (grid[target] / 82 == 1) //don't reveal flagged squares at all!
                {
                    stack.push_back(target); //sweep all unrevealed neighbors
                    // printf("%d\n", target);
                }
            }
        }
    }
}
