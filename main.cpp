#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "raylib.h"
#include "game.h"
#include "sound_data.h"

//screen size
int screenWidth = 1280;
int screenHeight = 720;

//globals
//display stuff
float tile_width = 50; //gets set at the start of main
float offset_x = 0;
float offset_y = 0;

bool left_click = false;
bool right_click = false;
bool menu = true; //are we on the main menu?

//4D Minesweeper grid, incrementally updated variables
unsigned n = 5; //go from 4 to 16
uint8_t grid[65536]; //pre-allocate (maximum size is 16^4, so this is good)
int mines = 20; //number of mines
//maximum mines: n^4 - 82 (81 => insta-win)
//good number:
//n = 4: [5; 10]
//n = 5: [15; 20]
//n = 6: ?????

//mouse position; updated in main, always before drawing
Vector2 mousepos;
//-1 if mouse not pointing at a square, the square's index otherwise
//NOTE: this is updated in the grid drawing routine
//old_mouse_square is for using mouse index inside draw routines
int mouse_square = -1, old_mouse_square = -1;

//sound effects
Sound snd_explosion, snd_flag, snd_win, snd_step;

//TODO: (better?) textures
void draw_square(int x, int y, int index, int neighbors, bool revealed, bool mine, bool flag, bool highlight)
{
    int scr_x = x * tile_width - offset_x;
    int scr_y = y * tile_width - offset_y;

    //square not on screen: don't spend precious time drawing it!
    if (scr_x < -tile_width || scr_y < -tile_width) return;
    if (scr_x > screenWidth || scr_y > screenHeight) return;

    //is the current rectangle getting hovered over?
    if (mousepos.x >= scr_x && mousepos.x < scr_x + tile_width
    && mousepos.y >= scr_y && mousepos.y < scr_y + tile_width)
        mouse_square = index;

    //calculate width/height at which we have to draw tile and avoid artifacts
    int draw_width = tile_width + ((x % (n + 1) == n - 1) ? 0 : tile_width / 2.);
    int draw_height = tile_width + ((y % (n + 1) == n - 1) ? 0 : tile_width / 2.);

    //fill of rectangle
    DrawRectangle(scr_x, scr_y, draw_width, draw_height,
        revealed ? (highlight ? (CLITERAL(Color) {179, 223, 255, 255}) : ((neighbors < 0) ? RED : LIGHTGRAY)) : (highlight ? SKYBLUE : DARKGRAY));
    //edges of rectangle (contained inside)
    if (tile_width >= 16)
        DrawRectangleLines(scr_x, scr_y, draw_width, draw_height, BLACK);

    if (revealed) //revealed: display mine (lost game) or number
    {
        if (mine)
        {
            //draw mine: circle and 4 straight lines
            DrawCircle(scr_x + tile_width * 0.5, scr_y + tile_width * 0.5, tile_width * 0.25, BLACK);
            DrawLine(scr_x + tile_width * 0.5, scr_y + tile_width * 0.125, scr_x + tile_width * 0.5, scr_y + tile_width * 0.875, BLACK);
            DrawLine(scr_x + tile_width * 0.125, scr_y + tile_width * 0.5, scr_x + tile_width * 0.875, scr_y + tile_width * 0.5, BLACK);
            DrawLine(scr_x + tile_width * 0.23483495705504465, scr_y + tile_width * 0.23483495705504465, scr_x + tile_width * 0.7651650429449554, scr_y + tile_width * 0.7651650429449554, BLACK);
            DrawLine(scr_x + tile_width * 0.23483495705504465, scr_y + tile_width * 0.7651650429449554, scr_x + tile_width * 0.7651650429449554, scr_y + tile_width * 0.23483495705504465, BLACK);
        }
        else if(neighbors != 255) //don't draw number when no neighbors (indicated by 255)
        {
            //not mine: draw number
            char number_text[] = "\0\0\0"; //4 characters (for example "-10")
            sprintf(number_text, "%d", neighbors);
            //TODO: color table!
            if (tile_width > 16.0f)
                DrawText(number_text, scr_x + (2 + (0 <= neighbors && neighbors < 10)) * tile_width / 8,
                    scr_y + tile_width / 4, tile_width * 0.5f,
                    (neighbors < 0) ? BLACK : RED);
        }
    }
    else if (flag) //there is a flag! (this would be better as a texture...)
    {
        //pole
        DrawRectangle(scr_x + tile_width * 0.265625, scr_y + tile_width * 0.375f,
        tile_width * 0.0625f, tile_width * 0.3875f, BLACK);
        //foot thing
        DrawRectangle(scr_x + tile_width * 0.203125, scr_y + tile_width * 0.75f,
        tile_width * 0.1875f, tile_width * 0.0625f, BLACK);

        Vector2 v1 = {scr_x + tile_width * 0.25f, scr_y + tile_width * 0.25f};
        Vector2 v2 = {scr_x + tile_width * 0.25f, scr_y + tile_width * 0.5f};
        Vector2 v3 = {scr_x + tile_width * 0.75f, scr_y + tile_width * 0.375f};
        //NOTE: the point order is important in this function!!!
        DrawTriangle(v1, v2, v3, RED);
    }
}

void draw_grid()
{
    old_mouse_square = mouse_square; //save it for display purposes
    //calculate mouse square coords
    unsigned mouse_a = mouse_square % n;
    mouse_square /= n;
    unsigned mouse_b = mouse_square % n;
    mouse_square /= n;
    unsigned mouse_c = mouse_square % n;
    mouse_square /= n;
    unsigned mouse_d = mouse_square % n;

    mouse_square = -1; //gets found inside this function

    for(unsigned a = 0; a < n; a++)
    {
        for(unsigned b = 0; b < n; b++)
        {
            for(unsigned c = 0; c < n; c++)
            {
                for(unsigned d = 0; d < n; d++)
                {
                    unsigned long x = a + (n+1)*c;
                    unsigned long y = b + (n+1)*d;
                    //82*3 = 246 possible states
                    //0 * 82 + ... = revealed
                    //1 * 82 + ... = hidden
                    //2 * 82 + ... = flagged
                    int idx = a + n*(b + n*(c + n*d));
                    uint8_t tile = grid[idx];
                    uint8_t tile_type = tile % 82; //0...80 for neighbors, 81 for mine

                    if (defeat) tile = tile_type; //reveal everything when lost

                    bool highlight = false;
                    if (old_mouse_square != -1)
                    {
                        //highlight if neighbor of mouse
                        if (abs((int)a - (int)mouse_a) <= 1
                        && abs((int)b - (int)mouse_b) <= 1
                        && abs((int)c - (int)mouse_c) <= 1
                        && abs((int)d - (int)mouse_d) <= 1
                        && idx != old_mouse_square)
                        highlight = true;
                    }

                    //stop clicking when game is over
                    if (idx == old_mouse_square && !victory && !defeat)
                    {
                        //flag/unflag
                        if (right_click)
                        {
                            if (tile - tile_type == 82)
                                grid[idx] += 82; //flag it!
                            if (tile - tile_type == 164)
                                grid[idx] -= 82; //unflag it!
 
                            if (tile >= 82) //not revealed tile
                                PlaySound(snd_flag); //flag sound
                        }
                        //reveal unflagged (and not already revealed) tile
                        if (left_click && tile - tile_type == 82)
                        {
                            if (remaining_safe == -1) //before first click
                                generate(grid, n, mines, idx);

                            reveal(grid, n, idx);

                            //play appropriate sound effect
                            if (defeat) //BOOM
                                PlaySound(snd_explosion);
                            else if (victory) //WIN!
                                PlaySound(snd_win);
                            else //not boom
                                PlaySound(snd_step);
                        }
                    }

                    //count neighboring flags (if cell is revealed only)
                    int neighboring_flags = 0;
                    for (int k = 0; tile < 82 && k < 81; k++)
                    {
                        int da = k % 3 - 1;
                        int db = (k/3) % 3 - 1;
                        int dc = (k/9) % 3 - 1;
                        int dd = (k/27) % 3 - 1;

                        //see if the square actually exists and we didn't wrap around an edge!!! (important!)
                        if ((int)a + da < 0 || (int)a + da >= (int)n) continue;
                        if ((int)b + db < 0 || (int)b + db >= (int)n) continue;
                        if ((int)c + dc < 0 || (int)c + dc >= (int)n) continue;
                        if ((int)d + dd < 0 || (int)d + dd >= (int)n) continue;

                        //calculate current square
                        //NOTE: all the (int)s are required, otherwise we're gonna be
                        //off by 2^32!!!
                        int target = idx + da + (int)n*(db + (int)n*(dc + (int)n*dd));

                        //if the square is flagged
                        if (grid[target] >= 164)
                            neighboring_flags++; //add 1 to the neighboring flag number
                    }
                    //distinguish between empty square and zero (due to delta)
                    if (victory)
                    {
                        neighboring_flags = -255 * (!tile_type);
                        tile_type = 0;
                    }
                    else if (neighboring_flags == 0 && tile_type == 0) neighboring_flags = -255;

                    draw_square(x, y, idx, tile_type - neighboring_flags,
                    tile / 82 == 0, tile_type == 81,
                    tile / 82 == 2 || victory, highlight);
                }
            }
        }
    }
}

void start_game()
{
    //reset globals
    victory = defeat = false; //reset win/loss bools
    remaining_safe = -1;
    mouse_square = old_mouse_square = -1;

    //calculate starting position and tile width
    //height of the board is n(n+1) - 1 tiles; we want 1 extra on each side
    tile_width = (float)screenHeight / (n*(n + 1) + 1);
    offset_y = -tile_width; //1 row above the entire thing!
    offset_x = offset_y - (screenWidth - screenHeight) / 2.;

    //fill with empty squares at first; generation will be done on first click
    for (unsigned i = 0; i < n*n*n*n; i++)
        grid[i] = 82; //x/82 == 1 => hidden (at the start everything is hidden)

    menu = false; //start the game
}

void game_loop()
{
    //go back to main menu if game over and space key pressed
    if ((victory || defeat) && IsKeyPressed(KEY_SPACE))
    {
        menu = true;
        //stop win/boom sounds
        StopSound(snd_win);
        StopSound(snd_explosion);
        return;
    }

    //drag stuff around
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        Vector2 delta = GetMouseDelta();
        offset_x -= delta.x;
        offset_y -= delta.y;
    }
    //move around with keys
    if (IsKeyDown(KEY_RIGHT)) offset_x += 200. * GetFrameTime();
    if (IsKeyDown(KEY_LEFT)) offset_x -= 200. * GetFrameTime();
    if (IsKeyDown(KEY_UP)) offset_y -= 200. * GetFrameTime();
    if (IsKeyDown(KEY_DOWN)) offset_y += 200. * GetFrameTime();

    //zoom
    if ((IsKeyDown(KEY_L) || GetMouseWheelMove() > 0.0)
    && tile_width * 16 < screenWidth) //L = zoom moar
    {
        //zoom into the center
        offset_x += screenWidth / 2.;
        offset_y += screenHeight / 2.;

        //multiply everything
        float p = pow(4.0, GetFrameTime());
        if (GetMouseWheelMove() > 0.0) p *= p * p; //more sensitivity with mouse wheel
        tile_width *= p;
        offset_x *= p;
        offset_y *= p;

        //get the thing back in place
        offset_x -= screenWidth / 2.;
        offset_y -= screenHeight / 2.;
    }
    if ((IsKeyDown(KEY_K) || GetMouseWheelMove() < -0.0)
    && tile_width * 512 > screenWidth) //K = zoom less
    {
        offset_x += screenWidth / 2.;
        offset_y += screenHeight / 2.;

        float p = pow(0.25, GetFrameTime());
        if (GetMouseWheelMove() < -0.0) p *= p * p; //more sensitivity with mouse wheel
        tile_width *= p;
        offset_x *= p;
        offset_y *= p;

        offset_x -= screenWidth / 2.;
        offset_y -= screenHeight / 2.;
    }

    // Draw
    BeginDrawing();
    ClearBackground(victory ? GREEN : (defeat ? RED : GRAY)); //victory: green

    draw_grid();

    // DrawFPS(10, 10);

    EndDrawing();
}

int main(void)
{
    //stop printing all that verbose info
    SetTraceLogLevel(LOG_WARNING);

    //start initializing audio device
    InitAudioDevice();

    //initialize random seed (for generation)
    srand(time(NULL));

    //load sounds
    Wave wave;
    wave.frameCount = SND_EXPLOSION_FRAME_COUNT;
    wave.channels = SND_EXPLOSION_CHANNELS;
    wave.sampleRate = SND_EXPLOSION_SAMPLE_RATE;
    wave.sampleSize = SND_EXPLOSION_SAMPLE_SIZE;
    wave.data = SND_EXPLOSION_DATA;
    snd_explosion = LoadSoundFromWave(wave);

    wave.frameCount = SND_FLAG_FRAME_COUNT;
    wave.channels = SND_FLAG_CHANNELS;
    wave.sampleRate = SND_FLAG_SAMPLE_RATE;
    wave.sampleSize = SND_FLAG_SAMPLE_SIZE;
    wave.data = SND_FLAG_DATA;
    snd_flag = LoadSoundFromWave(wave);

    wave.frameCount = SND_WIN_FRAME_COUNT;
    wave.channels = SND_WIN_CHANNELS;
    wave.sampleRate = SND_WIN_SAMPLE_RATE;
    wave.sampleSize = SND_WIN_SAMPLE_SIZE;
    wave.data = SND_WIN_DATA;
    snd_win = LoadSoundFromWave(wave);

    wave.frameCount = SND_STEP_FRAME_COUNT;
    wave.channels = SND_STEP_CHANNELS;
    wave.sampleRate = SND_STEP_SAMPLE_RATE;
    wave.sampleSize = SND_STEP_SAMPLE_SIZE;
    wave.data = SND_STEP_DATA;
    snd_step = LoadSoundFromWave(wave);

    // Initialization
    //TODO: improve this! get the monitor size automatically
    InitWindow(1280, 720, "4D Minesweeper");
    ToggleFullscreen(); //nothing works
    //set icon: SetWindowIcon

    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

    SetTargetFPS(60); // Set our game to run at 60 frames-per-second

    // Main loop (making an infinite loop => uncloseable, except for Ctrl+C)
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        //get mouse input
        left_click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        right_click = IsMouseButtonPressed(MOUSE_RIGHT_BUTTON);
        //mouse position
        mousepos = GetMousePosition();

        if (!menu)
        {
            //do game stuff
            game_loop();
        }
        else
        {
            //draw main menu
            const Rectangle n_textbox = { screenWidth/2.0f - 100, 180, 200, 50 };
            const Rectangle mines_textbox = { screenWidth/2.0f - 100, 280, 200, 50 };
            const Rectangle start_button = { screenWidth/2.0f - 100, screenHeight - 200.0f, 200, 50 };
            bool mouseOnNText = CheckCollisionPointRec(GetMousePosition(), n_textbox);
            bool mouseOnMinesText = CheckCollisionPointRec(GetMousePosition(), mines_textbox);

            if (mouseOnNText)
            {
                int key = GetCharPressed();
                while (key > 0)
                {
                    //only input numbers
                    if (key >= '0' && key <= '9')
                    {
                        //add new digit at the end
                        n = std::min(16, (int)n*10 + key - '0');
                    }

                    //NEXT!
                    key = GetCharPressed();
                }

                if (IsKeyPressed(KEY_BACKSPACE))
                {
                    n = n/10; //remove last digit
                    //clamp mine count (using 82 instead of 81, otherwise its insta-dub)
                    mines = std::min(std::max((int)0, (int)(n*n*n*n) - 82), (int)mines);
                }
            }
            else if (mouseOnMinesText)
            {
                int key = GetCharPressed();
                while (key > 0)
                {
                    //only input numbers
                    if (key >= '0' && key <= '9')
                    {
                        //add new digit at the end
                        //clamp mine count (using 82 instead of 81, otherwise its insta-dub)
                        mines = std::min(std::max((int)0, (int)(n*n*n*n) - 82), (int)mines*10 + key - '0');
                    }

                    //NEXT!
                    key = GetCharPressed();
                }

                if (IsKeyPressed(KEY_BACKSPACE))
                    mines = mines/10; //remove last digit
            }

            //do drawing stuff
            BeginDrawing();

            ClearBackground(LIGHTGRAY);

            //draw N textbox
            DrawText("Enter size of grid here:", n_textbox.x, 140, 20, GRAY);

            DrawRectangleRec(n_textbox, {160, 160, 160, 255});
            if (mouseOnNText) DrawRectangleLines((int)n_textbox.x, (int)n_textbox.y, (int)n_textbox.width, (int)n_textbox.height, RED);
            else DrawRectangleLines((int)n_textbox.x, (int)n_textbox.y, (int)n_textbox.width, (int)n_textbox.height, DARKGRAY);

            char tmp_text[7] = "\0";
            sprintf(tmp_text, "%d%c", n, '_' * (mouseOnNText * (int)(GetTime() * 2) % 2));
            DrawText(tmp_text, (int)n_textbox.x + 5, (int)n_textbox.y + 8, 40, DARKGRAY);

            //draw Mines textbox
            DrawText("Enter number of mines here:", mines_textbox.x, 240, 20, GRAY);

            DrawRectangleRec(mines_textbox, {160, 160, 160, 255});
            if (mouseOnMinesText) DrawRectangleLines((int)mines_textbox.x, (int)mines_textbox.y, (int)mines_textbox.width, (int)mines_textbox.height, RED);
            else DrawRectangleLines((int)mines_textbox.x, (int)mines_textbox.y, (int)mines_textbox.width, (int)mines_textbox.height, DARKGRAY);

            sprintf(tmp_text, "%d%c", mines, '_' * (mouseOnMinesText * (int)(GetTime() * 2) % 2));
            DrawText(tmp_text, (int)mines_textbox.x + 5, (int)mines_textbox.y + 8, 40, DARKGRAY);

            bool mouse_on_start = CheckCollisionPointRec(GetMousePosition(), start_button);
            if (mouse_on_start && mines > 0)
            {
                DrawRectangleRec(start_button, {220, 220, 220, 255});
                DrawText("START", (int)start_button.x + 30, (int)start_button.y + 8, 40, RED);
                DrawRectangleLines((int)start_button.x, (int)start_button.y, (int)start_button.width, (int)start_button.height, RED);

                //start game
                if (left_click)
                {
                    start_game();
                }
            }
            else
            {
                DrawRectangleRec(start_button, {160, 160, 160, 255});
                DrawText("START", (int)start_button.x + 30, (int)start_button.y + 8, 40, DARKGRAY);
                DrawRectangleLines((int)start_button.x, (int)start_button.y, (int)start_button.width, (int)start_button.height, DARKGRAY);
            }

            EndDrawing();
        }
    }

    CloseAudioDevice(); //close audio device
    CloseWindow(); // Close window and OpenGL context
    return 0;
}
