/*

Miscellaneous fractal-specific code (formerly in CALCFRAC.C)

*/
#include <algorithm>
#include <vector>

#include <limits.h>
#include <stdlib.h>
#include <string.h>

// see Fractint.c for a description of the "include"  hierarchy
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "targa_lc.h"
#include "drivers.h"

// routines in this module

static void set_Plasma_palette();
static U16 adjust(int xa, int ya, int x, int y, int xb, int yb);
static void subDivide(int x1, int y1, int x2, int y2);
static void verhulst();
static void Bif_Period_Init();
static bool Bif_Periodic(long time);
static void set_Cellular_palette();

U16(*getpix)(int, int)  = (U16(*)(int, int))getcolor;

typedef void (*PLOT)(int, int, int);

//**************** standalone engine for "test" *******************

int test()
{
    int startpass = 0;
    int startrow = startpass;
    if (g_resuming)
    {
        start_resume();
        get_resume(sizeof(startrow), &startrow, sizeof(startpass), &startpass, 0);
        end_resume();
    }
    if (teststart())   // assume it was stand-alone, doesn't want passes logic
    {
        return (0);
    }
    int numpasses = (g_std_calc_mode == '1') ? 0 : 1;
    for (int passes = startpass; passes <= numpasses ; passes++)
    {
        for (g_row = startrow; g_row <= g_i_y_stop; g_row = g_row+1+numpasses)
        {
            for (g_col = 0; g_col <= g_i_x_stop; g_col++)       // look at each point on screen
            {
                int color;
                g_init.x = g_dx_pixel();
                g_init.y = g_dy_pixel();
                if (driver_key_pressed())
                {
                    testend();
                    alloc_resume(20, 1);
                    put_resume(sizeof(g_row), &g_row, sizeof(passes), &passes, 0);
                    return (-1);
                }
                color = testpt(g_init.x, g_init.y, g_param_z1.x, g_param_z1.y, g_max_iterations, g_inside_color);
                if (color >= g_colors)
                {
                    // avoid trouble if color is 0
                    if (g_colors < 16)
                    {
                        color &= g_and_color;
                    }
                    else
                    {
                        color = ((color-1) % g_and_color) + 1; // skip color zero
                    }
                }
                (*g_plot)(g_col, g_row, color);
                if (numpasses && (passes == 0))
                {
                    (*g_plot)(g_col, g_row+1, color);
                }
            }
        }
        startrow = passes + 1;
    }
    testend();
    return (0);
}

//**************** standalone engine for "plasma" *******************

static int iparmx;      // iparmx = parm.x * 8
static int shiftvalue;  // shift based on #colors
static int recur1 = 1;
static int pcolors;
static int recur_level = 0;
U16 max_plasma;

// returns a random 16 bit value that is never 0
U16 rand16()
{
    U16 value;
    value = (U16)rand15();
    value <<= 1;
    value = (U16)(value + (rand15()&1));
    if (value < 1)
    {
        value = 1;
    }
    return (value);
}

void putpot(int x, int y, U16 color)
{
    if (color < 1)
    {
        color = 1;
    }
    g_put_color(x, y, (color >> 8) ? (color >> 8) : 1);  // don't write 0
    /* we don't write this if driver_diskp() because the above putcolor
          was already a "writedisk" in that case */
    if (!driver_diskp())
    {
        writedisk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, color >> 8);    // upper 8 bits
    }
    writedisk(x+g_logical_screen_x_offset, y+g_screen_y_dots+g_logical_screen_y_offset, color&255); // lower 8 bits
}

// fixes border
void putpotborder(int x, int y, U16 color)
{
    if ((x == 0) || (y == 0) || (x == g_logical_screen_x_dots-1) || (y == g_logical_screen_y_dots-1))
    {
        color = (U16)g_outside_color;
    }
    putpot(x, y, color);
}

// fixes border
void putcolorborder(int x, int y, int color)
{
    if ((x == 0) || (y == 0) || (x == g_logical_screen_x_dots-1) || (y == g_logical_screen_y_dots-1))
    {
        color = g_outside_color;
    }
    if (color < 1)
    {
        color = 1;
    }
    g_put_color(x, y, color);
}

U16 getpot(int x, int y)
{
    U16 color;

    color = (U16)readdisk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset);
    color = (U16)((color << 8) + (U16) readdisk(x+g_logical_screen_x_offset, y+g_screen_y_dots+g_logical_screen_y_offset));
    return (color);
}

static int plasma_check;                        // to limit kbd checking

static U16 adjust(int xa, int ya, int x, int y, int xb, int yb)
{
    S32 pseudorandom;
    pseudorandom = ((S32)iparmx)*((rand15()-16383));
    pseudorandom = pseudorandom * recur1;
    pseudorandom = pseudorandom >> shiftvalue;
    pseudorandom = (((S32)getpix(xa, ya)+(S32)getpix(xb, yb)+1) >> 1)+pseudorandom;
    if (max_plasma == 0)
    {
        if (pseudorandom >= pcolors)
        {
            pseudorandom = pcolors-1;
        }
    }
    else if (pseudorandom >= (S32)max_plasma)
    {
        pseudorandom = max_plasma;
    }
    if (pseudorandom < 1)
    {
        pseudorandom = 1;
    }
    g_plot(x, y, (U16)pseudorandom);
    return ((U16)pseudorandom);
}


static bool new_subD(int x1, int y1, int x2, int y2, int recur)
{
    int x, y;
    int nx1;
    int nx;
    int ny1, ny;
    S32 i, v;

    struct sub
    {
        BYTE t; // top of stack
        int v[16]; // subdivided value
        BYTE r[16];  // recursion level
    };

    static sub subx, suby;

    recur1 = (int)(320L >> recur);
    suby.t = 2;
    suby.v[0] = y2;
    ny   = suby.v[0];
    suby.v[2] = y1;
    ny1 = suby.v[2];
    suby.r[2] = 0;
    suby.r[0] = suby.r[2];
    suby.r[1] = 1;
    suby.v[1] = (ny1 + ny) >> 1;
    y = suby.v[1];

    while (suby.t >= 1)
    {
        if ((++plasma_check & 0x0f) == 1)
        {
            if (driver_key_pressed())
            {
                plasma_check--;
                return true;
            }
        }
        while (suby.r[suby.t-1] < (BYTE)recur)
        {
            //     1.  Create new entry at top of the stack
            //     2.  Copy old top value to new top value.
            //            This is largest y value.
            //     3.  Smallest y is now old mid point
            //     4.  Set new mid point recursion level
            //     5.  New mid point value is average
            //            of largest and smallest

            suby.t++;
            suby.v[suby.t] = suby.v[suby.t-1];
            ny1  = suby.v[suby.t];
            ny   = suby.v[suby.t-2];
            suby.r[suby.t] = suby.r[suby.t-1];
            suby.v[suby.t-1]   = (ny1 + ny) >> 1;
            y    = suby.v[suby.t-1];
            suby.r[suby.t-1]   = (BYTE)(std::max(suby.r[suby.t], suby.r[suby.t-2])+1);
        }
        subx.t = 2;
        subx.v[0] = x2;
        nx  = subx.v[0];
        subx.v[2] = x1;
        nx1 = subx.v[2];
        subx.r[2] = 0;
        subx.r[0] = subx.r[2];
        subx.r[1] = 1;
        subx.v[1] = (nx1 + nx) >> 1;
        x = subx.v[1];

        while (subx.t >= 1)
        {
            while (subx.r[subx.t-1] < (BYTE)recur)
            {
                subx.t++; // move the top ofthe stack up 1
                subx.v[subx.t] = subx.v[subx.t-1];
                nx1  = subx.v[subx.t];
                nx   = subx.v[subx.t-2];
                subx.r[subx.t] = subx.r[subx.t-1];
                subx.v[subx.t-1]   = (nx1 + nx) >> 1;
                x    = subx.v[subx.t-1];
                subx.r[subx.t-1]   = (BYTE)(std::max(subx.r[subx.t], subx.r[subx.t-2])+1);
            }

            i = getpix(nx, y);
            if (i == 0)
            {
                i = adjust(nx, ny1, nx, y , nx, ny);
            }
            // cppcheck-suppress AssignmentIntegerToAddress
            v = i;
            i = getpix(x, ny);
            if (i == 0)
            {
                i = adjust(nx1, ny, x , ny, nx, ny);
            }
            v += i;
            if (getpix(x, y) == 0)
            {
                i = getpix(x, ny1);
                if (i == 0)
                {
                    i = adjust(nx1, ny1, x , ny1, nx, ny1);
                }
                v += i;
                i = getpix(nx1, y);
                if (i == 0)
                {
                    i = adjust(nx1, ny1, nx1, y , nx1, ny);
                }
                v += i;
                g_plot(x, y, (U16)((v + 2) >> 2));
            }

            if (subx.r[subx.t-1] == (BYTE)recur)
            {
                subx.t = (BYTE)(subx.t - 2);
            }
        }

        if (suby.r[suby.t-1] == (BYTE)recur)
        {
            suby.t = (BYTE)(suby.t - 2);
        }
    }
    return false;
}

static void subDivide(int x1, int y1, int x2, int y2)
{
    int x, y;
    S32 v, i;
    if ((++plasma_check & 0x7f) == 1)
    {
        if (driver_key_pressed())
        {
            plasma_check--;
            return;
        }
    }
    if (x2-x1 < 2 && y2-y1 < 2)
    {
        return;
    }
    recur_level++;
    recur1 = (int)(320L >> recur_level);

    x = (x1+x2) >> 1;
    y = (y1+y2) >> 1;
    v = getpix(x, y1);
    if (v == 0)
    {
        v = adjust(x1, y1, x , y1, x2, y1);
    }
    i = v;
    v = getpix(x2, y);
    if (v == 0)
    {
        v = adjust(x2, y1, x2, y , x2, y2);
    }
    i += v;
    v = getpix(x, y2);
    if (v == 0)
    {
        v = adjust(x1, y2, x , y2, x2, y2);
    }
    i += v;
    v = getpix(x1, y);
    if (v == 0)
    {
        v = adjust(x1, y1, x1, y , x1, y2);
    }
    i += v;

    if (getpix(x, y) == 0)
    {
        g_plot(x, y, (U16)((i+2) >> 2));
    }

    subDivide(x1, y1, x , y);
    subDivide(x , y1, x2, y);
    subDivide(x , y , x2, y2);
    subDivide(x1, y , x , y2);
    recur_level--;
}


int plasma()
{
    U16 rnd[4];
    bool OldPotFlag = false;
    bool OldPot16bit = false;
    plasma_check = 0;

    if (g_colors < 4)
    {
        stopmsg(STOPMSG_NONE,
                "Plasma Clouds can currently only be run in a 4-or-more-color video\n"
                "mode (and color-cycled only on VGA adapters [or EGA adapters in their\n"
                "640x350x16 mode]).");
        return (-1);
    }
    iparmx = (int)(g_params[0] * 8);
    if (g_param_z1.x <= 0.0)
    {
        iparmx = 0;
    }
    if (g_param_z1.x >= 100)
    {
        iparmx = 800;
    }
    g_params[0] = (double)iparmx / 8.0;  // let user know what was used
    if (g_params[1] < 0)
    {
        g_params[1] = 0;  // limit parameter values
    }
    if (g_params[1] > 1)
    {
        g_params[1] = 1;
    }
    if (g_params[2] < 0)
    {
        g_params[2] = 0;  // limit parameter values
    }
    if (g_params[2] > 1)
    {
        g_params[2] = 1;
    }
    if (g_params[3] < 0)
    {
        g_params[3] = 0;  // limit parameter values
    }
    if (g_params[3] > 1)
    {
        g_params[3] = 1;
    }

    if (!g_random_seed_flag && g_params[2] == 1)
    {
        --g_random_seed;
    }
    if (g_params[2] != 0 && g_params[2] != 1)
    {
        g_random_seed = (int)g_params[2];
    }
    max_plasma = (U16)g_params[3];  // max_plasma is used as a flag for potential

    if (max_plasma != 0)
    {
        if (pot_startdisk() >= 0)
        {
            max_plasma = 0xFFFF;
            if (g_outside_color >= COLOR_BLACK)
            {
                g_plot    = (PLOT)putpotborder;
            }
            else
            {
                g_plot    = (PLOT)putpot;
            }
            getpix =  getpot;
            OldPotFlag = g_potential_flag;
            OldPot16bit = g_potential_16bit;
        }
        else
        {
            max_plasma = 0;        // can't do potential (startdisk failed)
            g_params[3]   = 0;
            if (g_outside_color >= COLOR_BLACK)
            {
                g_plot    = putcolorborder;
            }
            else
            {
                g_plot    = g_put_color;
            }
            getpix  = (U16(*)(int, int))getcolor;
        }
    }
    else
    {
        if (g_outside_color >= COLOR_BLACK)
        {
            g_plot    = putcolorborder;
        }
        else
        {
            g_plot    = g_put_color;
        }
        getpix  = (U16(*)(int, int))getcolor;
    }
    srand(g_random_seed);
    if (!g_random_seed_flag)
    {
        ++g_random_seed;
    }

    if (g_colors == 256)                     // set the (256-color) palette
    {
        set_Plasma_palette();             // skip this if < 256 colors
    }

    if (g_colors > 16)
    {
        shiftvalue = 18;
    }
    else
    {
        if (g_colors > 4)
        {
            shiftvalue = 22;
        }
        else
        {
            if (g_colors > 2)
            {
                shiftvalue = 24;
            }
            else
            {
                shiftvalue = 25;
            }
        }
    }
    if (max_plasma != 0)
    {
        shiftvalue = 10;
    }

    if (max_plasma == 0)
    {
        pcolors = std::min(g_colors, 256);
        for (auto &elem : rnd)
        {
            elem = (U16)(1+(((rand15()/pcolors)*(pcolors-1)) >> (shiftvalue-11)));
        }
    }
    else
    {
        for (auto &elem : rnd)
        {
            elem = rand16();
        }
    }
    if (g_debug_flag == debug_flags::prevent_plasma_random)
    {
        for (auto &elem : rnd)
        {
            elem = 1;
        }
    }

    g_plot(0,      0,  rnd[0]);
    g_plot(g_logical_screen_x_dots-1,      0,  rnd[1]);
    g_plot(g_logical_screen_x_dots-1, g_logical_screen_y_dots-1,  rnd[2]);
    g_plot(0, g_logical_screen_y_dots-1,  rnd[3]);

    int n;
    recur_level = 0;
    if (g_params[1] == 0)
    {
        subDivide(0, 0, g_logical_screen_x_dots-1, g_logical_screen_y_dots-1);
    }
    else
    {
        int i = 1;
        int k = 1;
        recur1 = 1;
        while (new_subD(0, 0, g_logical_screen_x_dots-1, g_logical_screen_y_dots-1, i) == 0)
        {
            k = k * 2;
            if (k  >(int)std::max(g_logical_screen_x_dots-1, g_logical_screen_y_dots-1))
            {
                break;
            }
            if (driver_key_pressed())
            {
                n = 1;
                goto done;
            }
            i++;
        }
    }
    if (!driver_key_pressed())
    {
        n = 0;
    }
    else
    {
        n = 1;
    }
done:
    if (max_plasma != 0)
    {
        g_potential_flag = OldPotFlag;
        g_potential_16bit = OldPot16bit;
    }
    g_plot    = g_put_color;
    getpix  = (U16(*)(int, int))getcolor;
    return (n);
}

static void set_Plasma_palette()
{
    static BYTE const Red[3]   = { 63, 0, 0 };
    static BYTE const Green[3] = { 0, 63, 0 };
    static BYTE const Blue[3]  = { 0,  0, 63 };

    if (g_map_specified || g_colors_preloaded)
    {
        return;    // map= specified
    }

    g_dac_box[0][0] = 0;
    g_dac_box[0][1] = 0;
    g_dac_box[0][2] = 0;
    for (int i = 1; i <= 85; i++)
    {
        g_dac_box[i][0] = (BYTE)((i*Green[0] + (86-i)*Blue[0])/85);
        g_dac_box[i][1] = (BYTE)((i*Green[1] + (86-i)*Blue[1])/85);
        g_dac_box[i][2] = (BYTE)((i*Green[2] + (86-i)*Blue[2])/85);

        g_dac_box[i+85][0] = (BYTE)((i*Red[0] + (86-i)*Green[0])/85);
        g_dac_box[i+85][1] = (BYTE)((i*Red[1] + (86-i)*Green[1])/85);
        g_dac_box[i+85][2] = (BYTE)((i*Red[2] + (86-i)*Green[2])/85);
        g_dac_box[i+170][0] = (BYTE)((i*Blue[0] + (86-i)*Red[0])/85);
        g_dac_box[i+170][1] = (BYTE)((i*Blue[1] + (86-i)*Red[1])/85);
        g_dac_box[i+170][2] = (BYTE)((i*Blue[2] + (86-i)*Red[2])/85);
    }
    spindac(0, 1);
}

//**************** standalone engine for "diffusion" *******************

#define RANDOM(x)  (rand()%(x))

int diffusion()
{
    int xmax, ymax, xmin, ymin;     // Current maximum coordinates
    int border;   // Distance between release point and fractal
    int mode;     // Determines diffusion type:  0 = central (classic)
    //                             1 = falling particles
    //                             2 = square cavity
    int colorshift; // If zero, select colors at random, otherwise shift the color every colorshift points
    int colorcount, currentcolor;
    double cosine, sine, angle;
    int x, y;
    float r, radius;

    if (driver_diskp())
    {
        notdiskmsg();
    }

    y = -1;
    x = y;
    g_bit_shift = 16;
    g_fudge_factor = 1L << 16;

    border = (int)g_params[0];
    mode = (int)g_params[1];
    colorshift = (int)g_params[2];

    colorcount = colorshift; // Counts down from colorshift
    currentcolor = 1;  // Start at color 1 (color 0 is probably invisible)

    if (mode > 2)
    {
        mode = 0;
    }

    if (border <= 0)
    {
        border = 10;
    }

    srand(g_random_seed);
    if (!g_random_seed_flag)
    {
        ++g_random_seed;
    }

    if (mode == 0)
    {
        xmax = g_logical_screen_x_dots / 2 + border;  // Initial box
        xmin = g_logical_screen_x_dots / 2 - border;
        ymax = g_logical_screen_y_dots / 2 + border;
        ymin = g_logical_screen_y_dots / 2 - border;
    }
    if (mode == 1)
    {
        xmax = g_logical_screen_x_dots / 2 + border;  // Initial box
        xmin = g_logical_screen_x_dots / 2 - border;
        ymin = g_logical_screen_y_dots - border;
    }
    if (mode == 2)
    {
        if (g_logical_screen_x_dots > g_logical_screen_y_dots)
        {
            radius = (float)(g_logical_screen_y_dots - border);
        }
        else
        {
            radius = (float)(g_logical_screen_x_dots - border);
        }
    }
    if (g_resuming) // restore worklist, if we can't the above will stay in place
    {
        start_resume();
        if (mode != 2)
        {
            get_resume(sizeof(xmax), &xmax, sizeof(xmin), &xmin, sizeof(ymax), &ymax,
                       sizeof(ymin), &ymin, 0);
        }
        else
        {
            get_resume(sizeof(xmax), &xmax, sizeof(xmin), &xmin, sizeof(ymax), &ymax,
                       sizeof(radius), &radius, 0);
        }
        end_resume();
    }

    switch (mode)
    {
    case 0: // Single seed point in the center
        g_put_color(g_logical_screen_x_dots / 2, g_logical_screen_y_dots / 2, currentcolor);
        break;
    case 1: // Line along the bottom
        for (int i = 0; i <= g_logical_screen_x_dots; i++)
        {
            g_put_color(i, g_logical_screen_y_dots-1, currentcolor);
        }
        break;
    case 2: // Large square that fills the screen
        if (g_logical_screen_x_dots > g_logical_screen_y_dots)
        {
            for (int i = 0; i < g_logical_screen_y_dots; i++)
            {
                g_put_color(g_logical_screen_x_dots/2-g_logical_screen_y_dots/2 , i , currentcolor);
                g_put_color(g_logical_screen_x_dots/2+g_logical_screen_y_dots/2 , i , currentcolor);
                g_put_color(g_logical_screen_x_dots/2-g_logical_screen_y_dots/2+i , 0 , currentcolor);
                g_put_color(g_logical_screen_x_dots/2-g_logical_screen_y_dots/2+i , g_logical_screen_y_dots-1 , currentcolor);
            }
        }
        else
        {
            for (int i = 0; i < g_logical_screen_x_dots; i++)
            {
                g_put_color(0 , g_logical_screen_y_dots/2-g_logical_screen_x_dots/2+i , currentcolor);
                g_put_color(g_logical_screen_x_dots-1 , g_logical_screen_y_dots/2-g_logical_screen_x_dots/2+i , currentcolor);
                g_put_color(i , g_logical_screen_y_dots/2-g_logical_screen_x_dots/2 , currentcolor);
                g_put_color(i , g_logical_screen_y_dots/2+g_logical_screen_x_dots/2 , currentcolor);
            }
        }
        break;
    }

    while (true)
    {
        switch (mode)
        {
        case 0: // Release new point on a circle inside the box
            angle = 2*(double)rand()/(RAND_MAX/PI);
            FPUsincos(&angle, &sine, &cosine);
            x = (int)(cosine*(xmax-xmin) + g_logical_screen_x_dots);
            y = (int)(sine  *(ymax-ymin) + g_logical_screen_y_dots);
            x = x >> 1; // divide by 2
            y = y >> 1;
            break;
        case 1: /* Release new point on the line ymin somewhere between xmin
                 and xmax */
            y = ymin;
            x = RANDOM(xmax-xmin) + (g_logical_screen_x_dots-xmax+xmin)/2;
            break;
        case 2: /* Release new point on a circle inside the box with radius
                 given by the radius variable */
            angle = 2*(double)rand()/(RAND_MAX/PI);
            FPUsincos(&angle, &sine, &cosine);
            x = (int)(cosine*radius + g_logical_screen_x_dots);
            y = (int)(sine  *radius + g_logical_screen_y_dots);
            x = x >> 1;
            y = y >> 1;
            break;
        }

        // Loop as long as the point (x,y) is surrounded by color 0
        // on all eight sides

        while ((getcolor(x+1, y+1) == 0) && (getcolor(x+1, y) == 0) &&
                (getcolor(x+1, y-1) == 0) && (getcolor(x  , y+1) == 0) &&
                (getcolor(x  , y-1) == 0) && (getcolor(x-1, y+1) == 0) &&
                (getcolor(x-1, y) == 0) && (getcolor(x-1, y-1) == 0))
        {
            // Erase moving point
            if (g_show_orbit)
            {
                g_put_color(x, y, 0);
            }

            if (mode == 0)
            {
                // Make sure point is inside the box
                if (x == xmax)
                {
                    x--;
                }
                else if (x == xmin)
                {
                    x++;
                }
                if (y == ymax)
                {
                    y--;
                }
                else if (y == ymin)
                {
                    y++;
                }
            }

            if (mode == 1) /* Make sure point is on the screen below ymin, but
                    we need a 1 pixel margin because of the next random step.*/
            {
                if (x >= g_logical_screen_x_dots-1)
                {
                    x--;
                }
                else if (x <= 1)
                {
                    x++;
                }
                if (y < ymin)
                {
                    y++;
                }
            }

            // Take one random step
            x += RANDOM(3) - 1;
            y += RANDOM(3) - 1;

            // Check keyboard
            if ((++plasma_check & 0x7f) == 1)
            {
                if (check_key())
                {
                    alloc_resume(20, 1);
                    if (mode != 2)
                    {
                        put_resume(sizeof(xmax), &xmax, sizeof(xmin), &xmin,
                                   sizeof(ymax), &ymax, sizeof(ymin), &ymin, 0);
                    }
                    else
                    {
                        put_resume(sizeof(xmax), &xmax, sizeof(xmin), &xmin,
                                   sizeof(ymax), &ymax, sizeof(radius), &radius, 0);
                    }

                    plasma_check--;
                    return 1;
                }
            }

            // Show the moving point
            if (g_show_orbit)
            {
                g_put_color(x, y, RANDOM(g_colors-1)+1);
            }

        } // End of loop, now fix the point

        /* If we're doing colorshifting then use currentcolor, otherwise
           pick one at random */
        g_put_color(x, y, colorshift?currentcolor:RANDOM(g_colors-1)+1);

        // If we're doing colorshifting then check to see if we need to shift
        if (colorshift)
        {
            if (!--colorcount)
            {
                // If the counter reaches zero then shift
                currentcolor++;      // Increase the current color and wrap
                currentcolor %= g_colors;  // around skipping zero
                if (!currentcolor)
                {
                    currentcolor++;
                }
                colorcount = colorshift;  // and reset the counter
            }
        }

        /* If the new point is close to an edge, we may need to increase
           some limits so that the limits expand to match the growing
           fractal. */

        switch (mode)
        {
        case 0:
            if (((x+border) > xmax) || ((x-border) < xmin)
                    || ((y-border) < ymin) || ((y+border) > ymax))
            {
                // Increase box size, but not past the edge of the screen
                ymin--;
                ymax++;
                xmin--;
                xmax++;
                if ((ymin == 0) || (xmin == 0))
                {
                    return 0;
                }
            }
            break;
        case 1: // Decrease ymin, but not past top of screen
            if (y-border < ymin)
            {
                ymin--;
            }
            if (ymin == 0)
            {
                return 0;
            }
            break;
        case 2: /* Decrease the radius where points are released to stay away
                 from the fractal.  It might be decreased by 1 or 2 */
            r = sqr((float)x-g_logical_screen_x_dots/2) + sqr((float)y-g_logical_screen_y_dots/2);
            if (r <= border*border)
            {
                return 0;
            }
            while ((radius-border)*(radius-border) > r)
            {
                radius--;
            }
            break;
        }
    }
}



//*********** standalone engine for "bifurcation" types **************

//*************************************************************
// The following code now forms a generalised Fractal Engine
// for Bifurcation fractal typeS.  By rights it now belongs in
// CALCFRACT.C, but it's easier for me to leave it here !

// Besides generalisation, enhancements include Periodicity
// Checking during the plotting phase (AND halfway through the
// filter cycle, if possible, to halve calc times), quicker
// floating-point calculations for the standard Verhulst type,
// and new bifurcation types (integer bifurcation, f.p & int
// biflambda - the real equivalent of complex Lambda sets -
// and f.p renditions of bifurcations of r*sin(Pi*p), which
// spurred Mitchel Feigenbaum on to discover his Number).

// To add further types, extend the fractalspecific[] array in
// usual way, with Bifurcation as the engine, and the name of
// the routine that calculates the next bifurcation generation
// as the "orbitcalc" routine in the fractalspecific[] entry.

// Bifurcation "orbitcalc" routines get called once per screen
// pixel column.  They should calculate the next generation
// from the doubles Rate & Population (or the longs lRate &
// lPopulation if they use integer math), placing the result
// back in Population (or lPopulation).  They should return 0
// if all is ok, or any non-zero value if calculation bailout
// is desirable (e.g. in case of errors, or the series tending
// to infinity).                Have fun !
//*************************************************************

#define DEFAULTFILTER 1000     /* "Beauty of Fractals" recommends using 5000
                               (p.25), but that seems unnecessary. Can
                               override this value with a nonzero param1 */

#define SEED 0.66               // starting value for population

static std::vector<int> verhulst_array;
unsigned long filter_cycles;
static bool half_time_check = false;
static long   lPopulation, lRate;
double Population,  Rate;
static bool mono = false;
static int outside_x = 0;
static long   LPI;

int Bifurcation()
{
    int x = 0;
    if (g_resuming)
    {
        start_resume();
        get_resume(sizeof(x), &x, 0);
        end_resume();
    }
    bool resized = false;
    try
    {
        verhulst_array.resize(g_i_y_stop + 1);
        resized = true;
    }
    catch (std::bad_alloc const&)
    {
    }
    if (!resized)
    {
        stopmsg(STOPMSG_NONE, "Insufficient free memory for calculation.");
        return (-1);
    }

    LPI = (long)(PI * g_fudge_factor);

    for (int y = 0; y <= g_i_y_stop; y++)   // should be iystop
    {
        verhulst_array[y] = 0;
    }

    mono = false;
    if (g_colors == 2)
    {
        mono = true;
    }
    if (mono)
    {
        if (g_inside_color != COLOR_BLACK)
        {
            outside_x = 0;
            g_inside_color = 1;
        }
        else
        {
            outside_x = 1;
        }
    }

    filter_cycles = (g_param_z1.x <= 0) ? DEFAULTFILTER : (long)g_param_z1.x;
    half_time_check = false;
    if (g_periodicity_check && (unsigned long)g_max_iterations < filter_cycles)
    {
        filter_cycles = (filter_cycles - g_max_iterations + 1) / 2;
        half_time_check = true;
    }

    if (g_integer_fractal)
    {
        g_l_init.y = g_l_y_max - g_i_y_stop*g_l_delta_y;            // Y-value of
    }
    else
    {
        g_init.y = (double)(g_y_max - g_i_y_stop*g_delta_y); // bottom pixels
    }

    while (x <= g_i_x_stop)
    {
        if (driver_key_pressed())
        {
            verhulst_array.clear();
            alloc_resume(10, 1);
            put_resume(sizeof(x), &x, 0);
            return (-1);
        }

        if (g_integer_fractal)
        {
            lRate = g_l_x_min + x*g_l_delta_x;
        }
        else
        {
            Rate = (double)(g_x_min + x*g_delta_x);
        }
        verhulst();        // calculate array once per column

        for (int y = g_i_y_stop; y >= 0; y--) // should be iystop & >=0
        {
            int color;
            color = verhulst_array[y];
            if (color && mono)
            {
                color = g_inside_color;
            }
            else if ((!color) && mono)
            {
                color = outside_x;
            }
            else if (color>=g_colors)
            {
                color = g_colors-1;
            }
            verhulst_array[y] = 0;
            (*g_plot)(x, y, color); // was row-1, but that's not right?
        }
        x++;
    }
    verhulst_array.clear();
    return (0);
}

static void verhulst()          // P. F. Verhulst (1845)
{
    unsigned int pixel_row;

    if (g_integer_fractal)
    {
        lPopulation = (g_param_z1.y == 0) ? (long)(SEED*g_fudge_factor) : (long)(g_param_z1.y*g_fudge_factor);
    }
    else
    {
        Population = (g_param_z1.y == 0) ? SEED : g_param_z1.y;
    }

    g_overflow = false;

    for (unsigned long counter = 0UL; counter < filter_cycles ; counter++)
    {
        if (curfractalspecific->orbitcalc())
        {
            return;
        }
    }
    if (half_time_check) // check for periodicity at half-time
    {
        Bif_Period_Init();
        unsigned long counter;
        for (counter = 0; counter < (unsigned long)g_max_iterations ; counter++)
        {
            if (curfractalspecific->orbitcalc())
            {
                return;
            }
            if (g_periodicity_check && Bif_Periodic(counter))
            {
                break;
            }
        }
        if (counter >= (unsigned long)g_max_iterations)   // if not periodic, go the distance
        {
            for (counter = 0; counter < filter_cycles ; counter++)
            {
                if (curfractalspecific->orbitcalc())
                {
                    return;
                }
            }
        }
    }

    if (g_periodicity_check)
    {
        Bif_Period_Init();
    }
    for (unsigned long counter = 0UL; counter < (unsigned long)g_max_iterations ; counter++)
    {
        if (curfractalspecific->orbitcalc())
        {
            return;
        }

        // assign population value to Y coordinate in pixels
        if (g_integer_fractal)
        {
            pixel_row = g_i_y_stop - (int)((lPopulation - g_l_init.y) / g_l_delta_y); // iystop
        }
        else
        {
            pixel_row = g_i_y_stop - (int)((Population - g_init.y) / g_delta_y);
        }

        // if it's visible on the screen, save it in the column array
        if (pixel_row <= (unsigned int)g_i_y_stop)
        {
            verhulst_array[ pixel_row ] ++;
        }
        if (g_periodicity_check && Bif_Periodic(counter))
        {
            if (pixel_row <= (unsigned int)g_i_y_stop)
            {
                verhulst_array[ pixel_row ] --;
            }
            break;
        }
    }
}
static  long    lBif_closenuf, lBif_savedpop;   // poss future use
static  double   Bif_closenuf,  Bif_savedpop;
static  int      Bif_savedinc;
static  long     Bif_savedand;

static void Bif_Period_Init()
{
    Bif_savedinc = 1;
    Bif_savedand = 1;
    if (g_integer_fractal)
    {
        lBif_savedpop = -1;
        lBif_closenuf = g_l_delta_y / 8;
    }
    else
    {
        Bif_savedpop = -1.0;
        Bif_closenuf = (double)g_delta_y / 8.0;
    }
}

// Bifurcation Population Periodicity Check
// Returns : true if periodicity found, else false
static bool Bif_Periodic(long time)
{
    if ((time & Bif_savedand) == 0)      // time to save a new value
    {
        if (g_integer_fractal)
        {
            lBif_savedpop = lPopulation;
        }
        else
        {
            Bif_savedpop =  Population;
        }
        if (--Bif_savedinc == 0)
        {
            Bif_savedand = (Bif_savedand << 1) + 1;
            Bif_savedinc = 4;
        }
    }
    else                         // check against an old save
    {
        if (g_integer_fractal)
        {
            if (labs(lBif_savedpop-lPopulation) <= lBif_closenuf)
            {
                return true;
            }
        }
        else
        {
            if (fabs(Bif_savedpop-Population) <= Bif_closenuf)
            {
                return true;
            }
        }
    }
    return false;
}

//********************************************************************
/*                                                                                                    */
// The following are Bifurcation "orbitcalc" routines...
/*                                                                                                    */
//********************************************************************
#if defined(XFRACT) || defined(_WIN32)
int BifurcLambda() // Used by lyanupov
{
    Population = Rate * Population * (1 - Population);
    return (fabs(Population) > BIG);
}
#endif

#define LCMPLXtrig0(arg, out) Arg1->l = (arg); ltrig0(); (out) = Arg1->l
#define  CMPLXtrig0(arg, out) Arg1->d = (arg); dtrig0(); (out) = Arg1->d

int BifurcVerhulstTrig()
{
    //  Population = Pop + Rate * fn(Pop) * (1 - fn(Pop))
    g_tmp_z.x = Population;
    g_tmp_z.y = 0;
    CMPLXtrig0(g_tmp_z, g_tmp_z);
    Population += Rate * g_tmp_z.x * (1 - g_tmp_z.x);
    return (fabs(Population) > BIG);
}

int LongBifurcVerhulstTrig()
{
#if !defined(XFRACT)
    g_l_temp.x = lPopulation;
    g_l_temp.y = 0;
    LCMPLXtrig0(g_l_temp, g_l_temp);
    g_l_temp.y = g_l_temp.x - multiply(g_l_temp.x, g_l_temp.x, g_bit_shift);
    lPopulation += multiply(lRate, g_l_temp.y, g_bit_shift);
#endif
    return (g_overflow);
}

int BifurcStewartTrig()
{
    //  Population = (Rate * fn(Population) * fn(Population)) - 1.0
    g_tmp_z.x = Population;
    g_tmp_z.y = 0;
    CMPLXtrig0(g_tmp_z, g_tmp_z);
    Population = (Rate * g_tmp_z.x * g_tmp_z.x) - 1.0;
    return (fabs(Population) > BIG);
}

int LongBifurcStewartTrig()
{
#if !defined(XFRACT)
    g_l_temp.x = lPopulation;
    g_l_temp.y = 0;
    LCMPLXtrig0(g_l_temp, g_l_temp);
    lPopulation = multiply(g_l_temp.x, g_l_temp.x, g_bit_shift);
    lPopulation = multiply(lPopulation, lRate,      g_bit_shift);
    lPopulation -= g_fudge_factor;
#endif
    return (g_overflow);
}

int BifurcSetTrigPi()
{
    g_tmp_z.x = Population * PI;
    g_tmp_z.y = 0;
    CMPLXtrig0(g_tmp_z, g_tmp_z);
    Population = Rate * g_tmp_z.x;
    return (fabs(Population) > BIG);
}

int LongBifurcSetTrigPi()
{
#if !defined(XFRACT)
    g_l_temp.x = multiply(lPopulation, LPI, g_bit_shift);
    g_l_temp.y = 0;
    LCMPLXtrig0(g_l_temp, g_l_temp);
    lPopulation = multiply(lRate, g_l_temp.x, g_bit_shift);
#endif
    return (g_overflow);
}

int BifurcAddTrigPi()
{
    g_tmp_z.x = Population * PI;
    g_tmp_z.y = 0;
    CMPLXtrig0(g_tmp_z, g_tmp_z);
    Population += Rate * g_tmp_z.x;
    return (fabs(Population) > BIG);
}

int LongBifurcAddTrigPi()
{
#if !defined(XFRACT)
    g_l_temp.x = multiply(lPopulation, LPI, g_bit_shift);
    g_l_temp.y = 0;
    LCMPLXtrig0(g_l_temp, g_l_temp);
    lPopulation += multiply(lRate, g_l_temp.x, g_bit_shift);
#endif
    return (g_overflow);
}

int BifurcLambdaTrig()
{
    //  Population = Rate * fn(Population) * (1 - fn(Population))
    g_tmp_z.x = Population;
    g_tmp_z.y = 0;
    CMPLXtrig0(g_tmp_z, g_tmp_z);
    Population = Rate * g_tmp_z.x * (1 - g_tmp_z.x);
    return (fabs(Population) > BIG);
}

int LongBifurcLambdaTrig()
{
#if !defined(XFRACT)
    g_l_temp.x = lPopulation;
    g_l_temp.y = 0;
    LCMPLXtrig0(g_l_temp, g_l_temp);
    g_l_temp.y = g_l_temp.x - multiply(g_l_temp.x, g_l_temp.x, g_bit_shift);
    lPopulation = multiply(lRate, g_l_temp.y, g_bit_shift);
#endif
    return (g_overflow);
}

#define LCMPLXpwr(arg1, arg2, out)    Arg2->l = (arg1); Arg1->l = (arg2);\
         lStkPwr(); Arg1++; Arg2++; (out) = Arg2->l

long beta;

int BifurcMay()
{
    /* X = (lambda * X) / (1 + X)^beta, from R.May as described in Pickover,
            Computers, Pattern, Chaos, and Beauty, page 153 */
    g_tmp_z.x = 1.0 + Population;
    g_tmp_z.x = pow(g_tmp_z.x, -beta); // pow in math.h included with mpmath.h
    Population = (Rate * Population) * g_tmp_z.x;
    return (fabs(Population) > BIG);
}

int LongBifurcMay()
{
#if !defined(XFRACT)
    g_l_temp.x = lPopulation + g_fudge_factor;
    g_l_temp.y = 0;
    g_l_param2.x = beta * g_fudge_factor;
    LCMPLXpwr(g_l_temp, g_l_param2, g_l_temp);
    lPopulation = multiply(lRate, lPopulation, g_bit_shift);
    lPopulation = divide(lPopulation, g_l_temp.x, g_bit_shift);
#endif
    return (g_overflow);
}

bool BifurcMaySetup()
{

    beta = (long)g_params[2];
    if (beta < 2)
    {
        beta = 2;
    }
    g_params[2] = (double)beta;

    timer(0, curfractalspecific->calctype);
    return false;
}

// Here Endeth the Generalised Bifurcation Fractal Engine

// END Phil Wilson's Code (modified slightly by Kev Allen et. al. !)


//****************** standalone engine for "popcorn" *******************

int popcorn()   // subset of std engine
{
    int start_row;
    start_row = 0;
    if (g_resuming)
    {
        start_resume();
        get_resume(sizeof(start_row), &start_row, 0);
        end_resume();
    }
    g_keyboard_check_interval = g_max_keyboard_check_interval;
    g_plot = noplot;
    g_l_temp_sqr_x = 0;
    g_temp_sqr_x = g_l_temp_sqr_x;
    for (g_row = start_row; g_row <= g_i_y_stop; g_row++)
    {
        g_reset_periodicity = true;
        for (g_col = 0; g_col <= g_i_x_stop; g_col++)
        {
            if (standard_fractal() == -1) // interrupted
            {
                alloc_resume(10, 1);
                put_resume(sizeof(g_row), &g_row, 0);
                return (-1);
            }
            g_reset_periodicity = false;
        }
    }
    g_calc_status = calc_status_value::COMPLETED;
    return (0);
}

//****************** standalone engine for "lyapunov" ********************
//** save_release behavior:                                             **
//**    1730 & prior: ignores inside=, calcmode='1', (a,b)->(x,y)       **
//**    1731: other calcmodes and inside=nnn                            **
//**    1732: the infamous axis swap: (b,a)->(x,y),                     **
//**            the order parameter becomes a long int                  **
//************************************************************************
int lyaLength, lyaSeedOK;
int lyaRxy[34];

#define WES 1   // define WES to be 0 to use Nick's lyapunov.obj
#if WES
int lyapunov_cycles(double, double);
#else
int lyapunov_cycles(int, double, double, double);
#endif

int lyapunov_cycles_in_c(long, double, double);

int lyapunov()
{
    double a, b;

    if (driver_key_pressed())
    {
        return -1;
    }
    g_overflow = false;
    if (g_params[1] == 1)
    {
        Population = (1.0+rand())/(2.0+RAND_MAX);
    }
    else if (g_params[1] == 0)
    {
        if (fabs(Population)>BIG || Population == 0 || Population == 1)
        {
            Population = (1.0+rand())/(2.0+RAND_MAX);
        }
    }
    else
    {
        Population = g_params[1];
    }
    (*g_plot)(g_col, g_row, 1);
    if (g_invert != 0)
    {
        invertz2(&g_init);
        a = g_init.y;
        b = g_init.x;
    }
    else
    {
        a = g_dy_pixel();
        b = g_dx_pixel();
    }
#if !defined(XFRACT) && !defined(_WIN32)
    /*  the assembler routines don't work for a & b outside the
        ranges 0 < a < 4 and 0 < b < 4. So, fall back on the C
        routines if part of the image sticks out.
        */
#if WES
    color = lyapunov_cycles(a, b);
#else
    if (lyaSeedOK && a > 0 && b > 0 && a <= 4 && b <= 4)
    {
        color = lyapunov_cycles(filter_cycles, Population, a, b);
    }
    else
    {
        color = lyapunov_cycles_in_c(filter_cycles, a, b);
    }
#endif
#else
    g_color = lyapunov_cycles_in_c(filter_cycles, a, b);
#endif
    if (g_inside_color > COLOR_BLACK && g_color == 0)
    {
        g_color = g_inside_color;
    }
    else if (g_color>=g_colors)
    {
        g_color = g_colors-1;
    }
    (*g_plot)(g_col, g_row, g_color);
    return g_color;
}


bool lya_setup()
{
    /* This routine sets up the sequence for forcing the Rate parameter
        to vary between the two values.  It fills the array lyaRxy[] and
        sets lyaLength to the length of the sequence.

        The sequence is coded in the bit pattern in an integer.
        Briefly, the sequence starts with an A the leading zero bits
        are ignored and the remaining bit sequence is decoded.  The
        sequence ends with a B.  Not all possible sequences can be
        represented in this manner, but every possible sequence is
        either represented as itself, as a rotation of one of the
        representable sequences, or as the inverse of a representable
        sequence (swapping 0s and 1s in the array.)  Sequences that
        are the rotation and/or inverses of another sequence will generate
        the same lyapunov exponents.

        A few examples follow:
            number    sequence
                0       ab
                1       aab
                2       aabb
                3       aaab
                4       aabbb
                5       aabab
                6       aaabb (this is a duplicate of 4, a rotated inverse)
                7       aaaab
                8       aabbbb  etc.
         */

    long i;

    filter_cycles = (long)g_params[2];
    if (filter_cycles == 0)
    {
        filter_cycles = g_max_iterations/2;
    }
    lyaSeedOK = g_params[1] > 0 && g_params[1] <= 1 && g_debug_flag != debug_flags::force_standard_fractal;
    lyaLength = 1;

    i = (long)g_params[0];
    lyaRxy[0] = 1;
    int t;
    for (t = 31; t >= 0; t--)
    {
        if (i & (1 << t))
        {
            break;
        }
    }
    for (; t >= 0; t--)
    {
        lyaRxy[lyaLength++] = (i & (1<<t)) != 0;
    }
    lyaRxy[lyaLength++] = 0;
    if (g_inside_color < COLOR_BLACK)
    {
        stopmsg(STOPMSG_NONE,
            "Sorry, inside options other than inside=nnn are not supported by the lyapunov");
        g_inside_color = 1;
    }
    if (g_user_std_calc_mode == 'o')
    {
        // Oops,lyapunov type
        g_user_std_calc_mode = '1';  // doesn't use new & breaks orbits
        g_std_calc_mode = '1';
    }
    return true;
}

int lyapunov_cycles_in_c(long filter_cycles, double a, double b)
{
    int color, lnadjust;
    double total;
    double temp;
    // e10=22026.4657948  e-10=0.0000453999297625

    total = 1.0;
    lnadjust = 0;
    long i;
    for (i = 0; i < filter_cycles; i++)
    {
        for (int count = 0; count < lyaLength; count++)
        {
            Rate = lyaRxy[count] ? a : b;
            if (curfractalspecific->orbitcalc())
            {
                g_overflow = true;
                goto jumpout;
            }
        }
    }
    for (i = 0; i < g_max_iterations/2; i++)
    {
        for (int count = 0; count < lyaLength; count++)
        {
            Rate = lyaRxy[count] ? a : b;
            if (curfractalspecific->orbitcalc())
            {
                g_overflow = true;
                goto jumpout;
            }
            temp = fabs(Rate-2.0*Rate*Population);
            total *= temp;
            if (total == 0)
            {
                g_overflow = true;
                goto jumpout;
            }
        }
        while (total > 22026.4657948)
        {
            total *= 0.0000453999297625;
            lnadjust += 10;
        }
        while (total < 0.0000453999297625)
        {
            total *= 22026.4657948;
            lnadjust -= 10;
        }
    }

jumpout:
    if (g_overflow || total <= 0 || (temp = log(total) + lnadjust) > 0)
    {
        color = 0;
    }
    else
    {
        double lyap;
        if (g_log_map_flag)
        {
            lyap = -temp/((double) lyaLength*i);
        }
        else
        {
            lyap = 1 - exp(temp/((double) lyaLength*i));
        }
        color = 1 + (int)(lyap * (g_colors-1));
    }
    return color;
}


//****************** standalone engine for "cellular" *******************

#define BAD_T         1
#define BAD_MEM       2
#define STRING1       3
#define STRING2       4
#define TABLEK        5
#define TYPEKR        6
#define RULELENGTH    7
#define INTERUPT      8

#define CELLULAR_DONE 10

static std::vector<BYTE> cell_array[2];

S16 r, k_1, rule_digits;
bool lstscreenflag = false;

void abort_cellular(int err, int t)
{
    int i;
    switch (err)
    {
    case BAD_T:
    {
        char msg[30];
        sprintf(msg, "Bad t=%d, aborting\n", t);
        stopmsg(STOPMSG_NONE, msg);
    }
    break;
    case BAD_MEM:
    {
        stopmsg(STOPMSG_NONE, "Insufficient free memory for calculation");
    }
    break;
    case STRING1:
    {
        stopmsg(STOPMSG_NONE, "String can be a maximum of 16 digits");
    }
    break;
    case STRING2:
    {
        static char msg[] = {"Make string of 0's through  's" };
        msg[27] = (char)(k_1 + 48); // turn into a character value
        stopmsg(STOPMSG_NONE, msg);
    }
    break;
    case TABLEK:
    {
        static char msg[] = {"Make Rule with 0's through  's" };
        msg[27] = (char)(k_1 + 48); // turn into a character value
        stopmsg(STOPMSG_NONE, msg);
    }
    break;
    case TYPEKR:
    {
        stopmsg(STOPMSG_NONE,
            "Type must be 21, 31, 41, 51, 61, 22, 32, 42, 23, 33, 24, 25, 26, 27");
    }
    break;
    case RULELENGTH:
    {
        static char msg[] = {"Rule must be    digits long" };
        i = rule_digits / 10;
        if (i == 0)
        {
            msg[14] = (char)(rule_digits + 48);
        }
        else
        {
            msg[13] = (char)(i+48);
            msg[14] = (char)((rule_digits % 10) + 48);
        }
        stopmsg(STOPMSG_NONE, msg);
    }
    break;
    case INTERUPT:
    {
        stopmsg(STOPMSG_NONE, "Interrupted, can't resume");
    }
    break;
    case CELLULAR_DONE:
        break;
    }
}

int cellular()
{
    S16 start_row;
    S16 filled, notfilled;
    U16 cell_table[32];
    U16 init_string[16];
    U16 kr, k;
    U32 lnnmbr;
    U16 twor;
    S16 t, t2;
    S32 randparam;
    double n;
    char buf[30];

    set_Cellular_palette();

    randparam = (S32)g_params[0];
    lnnmbr = (U32)g_params[3];
    kr = (U16)g_params[2];
    switch (kr)
    {
    case 21:
    case 31:
    case 41:
    case 51:
    case 61:
    case 22:
    case 32:
    case 42:
    case 23:
    case 33:
    case 24:
    case 25:
    case 26:
    case 27:
        break;
    default:
        abort_cellular(TYPEKR, 0);
        return -1;
    }

    r = (S16)(kr % 10); // Number of nearest neighbors to sum
    k = (U16)(kr / 10); // Number of different states, k=3 has states 0,1,2
    k_1 = (S16)(k - 1); // Highest state value, k=3 has highest state value of 2
    rule_digits = (S16)((r * 2 + 1) * k_1 + 1); // Number of digits in the rule

    if (!g_random_seed_flag && randparam == -1)
    {
        --g_random_seed;
    }
    if (randparam != 0 && randparam != -1)
    {
        n = g_params[0];
        sprintf(buf, "%.16g", n); // # of digits in initial string
        t = (S16)strlen(buf);
        if (t>16 || t <= 0)
        {
            abort_cellular(STRING1, 0);
            return -1;
        }
        for (auto &elem : init_string)
        {
            elem = 0; // zero the array
        }
        t2 = (S16)((16 - t)/2);
        for (int i = 0; i < t; i++)
        {
            // center initial string in array
            init_string[i+t2] = (U16)(buf[i] - 48); // change character to number
            if (init_string[i+t2]>(U16)k_1)
            {
                abort_cellular(STRING2, 0);
                return -1;
            }
        }
    }

    srand(g_random_seed);
    if (!g_random_seed_flag)
    {
        ++g_random_seed;
    }

    // generate rule table from parameter 1
#if !defined(XFRACT)
    n = g_params[1];
#else
    // gcc can't manage to convert a big double to an unsigned long properly.
    if (g_params[1]>0x7fffffff)
    {
        n = (g_params[1]-0x7fffffff);
        n += 0x7fffffff;
    }
    else
    {
        n = g_params[1];
    }
#endif
    if (n == 0)
    {
        // calculate a random rule
        n = rand()%(int)k;
        for (int i = 1; i < rule_digits; i++)
        {
            n *= 10;
            n += rand()%(int)k;
        }
        g_params[1] = n;
    }
    sprintf(buf, "%.*g", rule_digits , n);
    t = (S16)strlen(buf);
    if (rule_digits < t || t < 0)
    {
        // leading 0s could make t smaller
        abort_cellular(RULELENGTH, 0);
        return -1;
    }
    for (int i = 0; i < rule_digits; i++)   // zero the table
    {
        cell_table[i] = 0;
    }
    for (int i = 0; i < t; i++)
    {
        // reverse order
        cell_table[i] = (U16)(buf[t-i-1] - 48); // change character to number
        if (cell_table[i]>(U16)k_1)
        {
            abort_cellular(TABLEK, 0);
            return -1;
        }
    }


    start_row = 0;
    bool resized = false;
    try
    {
        cell_array[0].resize(g_i_x_stop+1);
        cell_array[1].resize(g_i_x_stop+1);
        resized = true;
    }
    catch (std::bad_alloc const&)
    {
    }
    if (!resized)
    {
        abort_cellular(BAD_MEM, 0);
        return (-1);
    }

    // nxtscreenflag toggled by space bar in fractint.c, true for continuous
    // false to stop on next screen

    filled = 0;
    notfilled = (S16)(1-filled);
    if (g_resuming && !g_cellular_next_screen && !lstscreenflag)
    {
        start_resume();
        get_resume(sizeof(start_row), &start_row, 0);
        end_resume();
        get_line(start_row, 0, g_i_x_stop, &cell_array[filled][0]);
    }
    else if (g_cellular_next_screen && !lstscreenflag)
    {
        start_resume();
        end_resume();
        get_line(g_i_y_stop, 0, g_i_x_stop, &cell_array[filled][0]);
        g_params[3] += g_i_y_stop + 1;
        start_row = -1; // after 1st iteration its = 0
    }
    else
    {
        if (g_random_seed_flag || randparam == 0 || randparam == -1)
        {
            for (g_col = 0; g_col <= g_i_x_stop; g_col++)
            {
                cell_array[filled][g_col] = (BYTE)(rand()%(int)k);
            }
        } // end of if random

        else
        {
            for (g_col = 0; g_col <= g_i_x_stop; g_col++)
            {
                // Clear from end to end
                cell_array[filled][g_col] = 0;
            }
            int i = 0;
            for (g_col = (g_i_x_stop-16)/2; g_col < (g_i_x_stop+16)/2; g_col++)
            {
                // insert initial
                cell_array[filled][g_col] = (BYTE)init_string[i++];    // string
            }
        } // end of if not random
        lstscreenflag = lnnmbr != 0;
        put_line(start_row, 0, g_i_x_stop, &cell_array[filled][0]);
    }
    start_row++;

    // This section calculates the starting line when it is not zero
    // This section can't be resumed since no screen output is generated
    // calculates the (lnnmbr - 1) generation
    if (lstscreenflag)   // line number != 0 & not resuming & not continuing
    {
        for (U32 big_row = (U32)start_row; big_row < lnnmbr; big_row++)
        {
            thinking(1, "Cellular thinking (higher start row takes longer)");
            if (g_random_seed_flag || randparam == 0 || randparam == -1)
            {
                // Use a random border
                for (int i = 0; i <= r; i++)
                {
                    cell_array[notfilled][i] = (BYTE)(rand()%(int)k);
                    cell_array[notfilled][g_i_x_stop-i] = (BYTE)(rand()%(int)k);
                }
            }
            else
            {
                // Use a zero border
                for (int i = 0; i <= r; i++)
                {
                    cell_array[notfilled][i] = 0;
                    cell_array[notfilled][g_i_x_stop-i] = 0;
                }
            }

            t = 0; // do first cell
            twor = (U16)(r+r);
            for (int i = 0; i <= twor; i++)
            {
                t = (S16)(t + (S16)cell_array[filled][i]);
            }
            if (t > rule_digits || t < 0)
            {
                thinking(0, nullptr);
                abort_cellular(BAD_T, t);
                return (-1);
            }
            cell_array[notfilled][r] = (BYTE)cell_table[t];

            // use a rolling sum in t
            for (g_col = r+1; g_col < g_i_x_stop-r; g_col++)
            {
                // now do the rest
                t = (S16)(t + cell_array[filled][g_col+r] - cell_array[filled][g_col-r-1]);
                if (t > rule_digits || t < 0)
                {
                    thinking(0, nullptr);
                    abort_cellular(BAD_T, t);
                    return (-1);
                }
                cell_array[notfilled][g_col] = (BYTE)cell_table[t];
            }

            filled = notfilled;
            notfilled = (S16)(1-filled);
            if (driver_key_pressed())
            {
                thinking(0, nullptr);
                abort_cellular(INTERUPT, 0);
                return -1;
            }
        }
        start_row = 0;
        thinking(0, nullptr);
        lstscreenflag = false;
    }

    // This section does all the work
contloop:
    for (g_row = start_row; g_row <= g_i_y_stop; g_row++)
    {
        if (g_random_seed_flag || randparam == 0 || randparam == -1)
        {
            // Use a random border
            for (int i = 0; i <= r; i++)
            {
                cell_array[notfilled][i] = (BYTE)(rand()%(int)k);
                cell_array[notfilled][g_i_x_stop-i] = (BYTE)(rand()%(int)k);
            }
        }
        else
        {
            // Use a zero border
            for (int i = 0; i <= r; i++)
            {
                cell_array[notfilled][i] = 0;
                cell_array[notfilled][g_i_x_stop-i] = 0;
            }
        }

        t = 0; // do first cell
        twor = (U16)(r+r);
        for (int i = 0; i <= twor; i++)
        {
            t = (S16)(t + (S16)cell_array[filled][i]);
        }
        if (t > rule_digits || t < 0)
        {
            thinking(0, nullptr);
            abort_cellular(BAD_T, t);
            return (-1);
        }
        cell_array[notfilled][r] = (BYTE)cell_table[t];

        // use a rolling sum in t
        for (g_col = r+1; g_col < g_i_x_stop-r; g_col++)
        {
            // now do the rest
            t = (S16)(t + cell_array[filled][g_col+r] - cell_array[filled][g_col-r-1]);
            if (t > rule_digits || t < 0)
            {
                thinking(0, nullptr);
                abort_cellular(BAD_T, t);
                return (-1);
            }
            cell_array[notfilled][g_col] = (BYTE)cell_table[t];
        }

        filled = notfilled;
        notfilled = (S16)(1-filled);
        put_line(g_row, 0, g_i_x_stop, &cell_array[filled][0]);
        if (driver_key_pressed())
        {
            abort_cellular(CELLULAR_DONE, 0);
            alloc_resume(10, 1);
            put_resume(sizeof(g_row), &g_row, 0);
            return -1;
        }
    }
    if (g_cellular_next_screen)
    {
        g_params[3] += g_i_y_stop + 1;
        start_row = 0;
        goto contloop;
    }
    abort_cellular(CELLULAR_DONE, 0);
    return 1;
}

bool CellularSetup()
{
    if (!g_resuming)
    {
        g_cellular_next_screen = false; // initialize flag
    }
    timer(0, curfractalspecific->calctype);
    return false;
}

static void set_Cellular_palette()
{
    static BYTE const Red[3]    = { 42, 0, 0 };
    static BYTE const Green[3]  = { 10, 35, 10 };
    static BYTE const Blue[3]   = { 13, 12, 29 };
    static BYTE const Yellow[3] = { 60, 58, 18 };
    static BYTE const Brown[3]  = { 42, 21, 0 };

    if (g_map_specified && g_color_state != 0)
    {
        return;       // map= specified
    }

    g_dac_box[0][0] = 0;
    g_dac_box[0][1] = 0;
    g_dac_box[0][2] = 0;

    g_dac_box[1][0] = Red[0];
    g_dac_box[1][1] = Red[1];
    g_dac_box[1][2] = Red[2];
    g_dac_box[2][0] = Green[0];
    g_dac_box[2][1] = Green[1];
    g_dac_box[2][2] = Green[2];
    g_dac_box[3][0] = Blue[0];
    g_dac_box[3][1] = Blue[1];
    g_dac_box[3][2] = Blue[2];
    g_dac_box[4][0] = Yellow[0];
    g_dac_box[4][1] = Yellow[1];
    g_dac_box[4][2] = Yellow[2];
    g_dac_box[5][0] = Brown[0];
    g_dac_box[5][1] = Brown[1];
    g_dac_box[5][2] = Brown[2];

    spindac(0, 1);
}

// frothy basin routines

#define FROTH_BITSHIFT      28
#define FROTH_D_TO_L(x)     ((long)((x)*(1L<<FROTH_BITSHIFT)))
#define FROTH_CLOSE         1e-6      // seems like a good value
#define FROTH_LCLOSE        FROTH_D_TO_L(FROTH_CLOSE)
#define SQRT3               1.732050807568877193
#define FROTH_SLOPE         SQRT3
#define FROTH_LSLOPE        FROTH_D_TO_L(FROTH_SLOPE)
#define FROTH_CRITICAL_A    1.028713768218725  // 1.0287137682187249127
#define froth_top_x_mapping(x)  ((x)*(x)-(x)-3*fsp.fl.f.a*fsp.fl.f.a/4)


struct froth_double_struct
{
    double a;
    double halfa;
    double top_x1;
    double top_x2;
    double top_x3;
    double top_x4;
    double left_x1;
    double left_x2;
    double left_x3;
    double left_x4;
    double right_x1;
    double right_x2;
    double right_x3;
    double right_x4;
};

struct froth_long_struct
{
    long a;
    long halfa;
    long top_x1;
    long top_x2;
    long top_x3;
    long top_x4;
    long left_x1;
    long left_x2;
    long left_x3;
    long left_x4;
    long right_x1;
    long right_x2;
    long right_x3;
    long right_x4;
};

struct froth_struct
{
    int repeat_mapping;
    int altcolor;
    int attractors;
    int shades;
    union
    {
        froth_double_struct f;
        froth_long_struct l;
    } fl;
};

namespace
{
froth_struct fsp;
}

// color maps which attempt to replicate the images of James Alexander.
static void set_Froth_palette()
{
    char const *mapname;

    if (g_color_state != 0)   // 0 means g_dac_box matches default
    {
        return;
    }
    if (g_colors >= 16)
    {
        if (g_colors >= 256)
        {
            if (fsp.attractors == 6)
            {
                mapname = "froth6.map";
            }
            else
            {
                mapname = "froth3.map";

            }
        }
        else // colors >= 16
        {
            if (fsp.attractors == 6)
            {
                mapname = "froth616.map";
            }
            else
            {
                mapname = "froth316.map";

            }
        }
        if (ValidateLuts(mapname))
        {
            return;
        }
        g_color_state = 0; // treat map as default
        spindac(0, 1);
    }
}

bool froth_setup()
{
    double sin_theta;
    double cos_theta;

    sin_theta = SQRT3/2; // sin(2*PI/3)
    cos_theta = -0.5;    // cos(2*PI/3)

    // for the all important backwards compatibility
    if (g_params[0] != 2)
    {
        g_params[0] = 1;
    }
    fsp.repeat_mapping = (int)g_params[0] == 2;
    if (g_params[1] != 0)
    {
        g_params[1] = 1;
    }
    fsp.altcolor = (int)g_params[1];
    fsp.fl.f.a = g_params[2];

    fsp.attractors = fabs(fsp.fl.f.a) <= FROTH_CRITICAL_A ? (!fsp.repeat_mapping ? 3 : 6)
                         : (!fsp.repeat_mapping ? 2 : 3);

    // new improved values
    // 0.5 is the value that causes the mapping to reach a minimum
    double x0 = 0.5;
    // a/2 is the value that causes the y value to be invariant over the mappings
    fsp.fl.f.halfa = fsp.fl.f.a/2;
    double y0 = fsp.fl.f.halfa;
    fsp.fl.f.top_x1 = froth_top_x_mapping(x0);
    fsp.fl.f.top_x2 = froth_top_x_mapping(fsp.fl.f.top_x1);
    fsp.fl.f.top_x3 = froth_top_x_mapping(fsp.fl.f.top_x2);
    fsp.fl.f.top_x4 = froth_top_x_mapping(fsp.fl.f.top_x3);

    // rotate 120 degrees counter-clock-wise
    fsp.fl.f.left_x1 = fsp.fl.f.top_x1 * cos_theta - y0 * sin_theta;
    fsp.fl.f.left_x2 = fsp.fl.f.top_x2 * cos_theta - y0 * sin_theta;
    fsp.fl.f.left_x3 = fsp.fl.f.top_x3 * cos_theta - y0 * sin_theta;
    fsp.fl.f.left_x4 = fsp.fl.f.top_x4 * cos_theta - y0 * sin_theta;

    // rotate 120 degrees clock-wise
    fsp.fl.f.right_x1 = fsp.fl.f.top_x1 * cos_theta + y0 * sin_theta;
    fsp.fl.f.right_x2 = fsp.fl.f.top_x2 * cos_theta + y0 * sin_theta;
    fsp.fl.f.right_x3 = fsp.fl.f.top_x3 * cos_theta + y0 * sin_theta;
    fsp.fl.f.right_x4 = fsp.fl.f.top_x4 * cos_theta + y0 * sin_theta;

    // if 2 attractors, use same shades as 3 attractors
    fsp.shades = (g_colors-1) / std::max(3, fsp.attractors);

    // rqlim needs to be at least sq(1+sqrt(1+sq(a))),
    // which is never bigger than 6.93..., so we'll call it 7.0
    if (g_magnitude_limit < 7.0)
    {
        g_magnitude_limit = 7.0;
    }
    set_Froth_palette();
    // make the best of the .map situation
    g_orbit_color = fsp.attractors != 6 && g_colors >= 16 ? (fsp.shades<<1)+1 : g_colors-1;

    if (g_integer_fractal)
    {
        froth_long_struct tmp_l;

        tmp_l.a        = FROTH_D_TO_L(fsp.fl.f.a);
        tmp_l.halfa    = FROTH_D_TO_L(fsp.fl.f.halfa);

        tmp_l.top_x1   = FROTH_D_TO_L(fsp.fl.f.top_x1);
        tmp_l.top_x2   = FROTH_D_TO_L(fsp.fl.f.top_x2);
        tmp_l.top_x3   = FROTH_D_TO_L(fsp.fl.f.top_x3);
        tmp_l.top_x4   = FROTH_D_TO_L(fsp.fl.f.top_x4);

        tmp_l.left_x1  = FROTH_D_TO_L(fsp.fl.f.left_x1);
        tmp_l.left_x2  = FROTH_D_TO_L(fsp.fl.f.left_x2);
        tmp_l.left_x3  = FROTH_D_TO_L(fsp.fl.f.left_x3);
        tmp_l.left_x4  = FROTH_D_TO_L(fsp.fl.f.left_x4);

        tmp_l.right_x1 = FROTH_D_TO_L(fsp.fl.f.right_x1);
        tmp_l.right_x2 = FROTH_D_TO_L(fsp.fl.f.right_x2);
        tmp_l.right_x3 = FROTH_D_TO_L(fsp.fl.f.right_x3);
        tmp_l.right_x4 = FROTH_D_TO_L(fsp.fl.f.right_x4);

        fsp.fl.l = tmp_l;
    }
    return true;
}

void froth_cleanup()
{
}


// Froth Fractal type
int calcfroth()   // per pixel 1/2/g, called with row & col set
{
    int found_attractor = 0;

    if (check_key())
    {
        return -1;
    }

    g_orbit_save_index = 0;
    g_color_iter = 0;
    if (g_show_dot >0)
    {
        (*g_plot)(g_col, g_row, g_show_dot %g_colors);
    }
    if (!g_integer_fractal) // fp mode
    {
        if (g_invert != 0)
        {
            invertz2(&g_tmp_z);
            g_old_z = g_tmp_z;
        }
        else
        {
            g_old_z.x = g_dx_pixel();
            g_old_z.y = g_dy_pixel();
        }

        g_temp_sqr_x = sqr(g_old_z.x);
        g_temp_sqr_y = sqr(g_old_z.y);
        while (!found_attractor
                && (g_temp_sqr_x + g_temp_sqr_y < g_magnitude_limit)
                && (g_color_iter < g_max_iterations))
        {
            // simple formula: z = z^2 + conj(z*(-1+ai))
            // but it's the attractor that makes this so interesting
            g_new_z.x = g_temp_sqr_x - g_temp_sqr_y - g_old_z.x - fsp.fl.f.a*g_old_z.y;
            g_old_z.y += (g_old_z.x+g_old_z.x)*g_old_z.y - fsp.fl.f.a*g_old_z.x;
            g_old_z.x = g_new_z.x;
            if (fsp.repeat_mapping)
            {
                g_new_z.x = sqr(g_old_z.x) - sqr(g_old_z.y) - g_old_z.x - fsp.fl.f.a*g_old_z.y;
                g_old_z.y += (g_old_z.x+g_old_z.x)*g_old_z.y - fsp.fl.f.a*g_old_z.x;
                g_old_z.x = g_new_z.x;
            }

            g_color_iter++;

            if (g_show_orbit)
            {
                if (driver_key_pressed())
                {
                    break;
                }
                plot_orbit(g_old_z.x, g_old_z.y, -1);
            }

            if (fabs(fsp.fl.f.halfa-g_old_z.y) < FROTH_CLOSE
                    && g_old_z.x >= fsp.fl.f.top_x1 && g_old_z.x <= fsp.fl.f.top_x2)
            {
                if ((!fsp.repeat_mapping && fsp.attractors == 2)
                        || (fsp.repeat_mapping && fsp.attractors == 3))
                {
                    found_attractor = 1;
                }
                else if (g_old_z.x <= fsp.fl.f.top_x3)
                {
                    found_attractor = 1;
                }
                else if (g_old_z.x >= fsp.fl.f.top_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 1;
                    }
                    else
                    {
                        found_attractor = 2;
                    }
                }
            }
            else if (fabs(FROTH_SLOPE*g_old_z.x - fsp.fl.f.a - g_old_z.y) < FROTH_CLOSE
                     && g_old_z.x <= fsp.fl.f.right_x1 && g_old_z.x >= fsp.fl.f.right_x2)
            {
                if (!fsp.repeat_mapping && fsp.attractors == 2)
                {
                    found_attractor = 2;
                }
                else if (fsp.repeat_mapping && fsp.attractors == 3)
                {
                    found_attractor = 3;
                }
                else if (g_old_z.x >= fsp.fl.f.right_x3)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 2;
                    }
                    else
                    {
                        found_attractor = 4;
                    }
                }
                else if (g_old_z.x <= fsp.fl.f.right_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 3;
                    }
                    else
                    {
                        found_attractor = 6;
                    }
                }
            }
            else if (fabs(-FROTH_SLOPE*g_old_z.x - fsp.fl.f.a - g_old_z.y) < FROTH_CLOSE
                     && g_old_z.x <= fsp.fl.f.left_x1 && g_old_z.x >= fsp.fl.f.left_x2)
            {
                if (!fsp.repeat_mapping && fsp.attractors == 2)
                {
                    found_attractor = 2;
                }
                else if (fsp.repeat_mapping && fsp.attractors == 3)
                {
                    found_attractor = 2;
                }
                else if (g_old_z.x >= fsp.fl.f.left_x3)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 3;
                    }
                    else
                    {
                        found_attractor = 5;
                    }
                }
                else if (g_old_z.x <= fsp.fl.f.left_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 2;
                    }
                    else
                    {
                        found_attractor = 3;
                    }
                }
            }
            g_temp_sqr_x = sqr(g_old_z.x);
            g_temp_sqr_y = sqr(g_old_z.y);
        }
    }
    else // integer mode
    {
        if (g_invert != 0)
        {
            invertz2(&g_tmp_z);
            g_l_old_z.x = (long)(g_tmp_z.x * g_fudge_factor);
            g_l_old_z.y = (long)(g_tmp_z.y * g_fudge_factor);
        }
        else
        {
            g_l_old_z.x = g_l_x_pixel();
            g_l_old_z.y = g_l_y_pixel();
        }

        g_l_temp_sqr_x = lsqr(g_l_old_z.x);
        g_l_temp_sqr_y = lsqr(g_l_old_z.y);
        g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
        while (!found_attractor && (g_l_magnitude < g_l_magnitude_limit)
                && (g_l_magnitude >= 0) && (g_color_iter < g_max_iterations))
        {
            // simple formula: z = z^2 + conj(z*(-1+ai))
            // but it's the attractor that makes this so interesting
            g_l_new_z.x = g_l_temp_sqr_x - g_l_temp_sqr_y - g_l_old_z.x - multiply(fsp.fl.l.a, g_l_old_z.y, g_bit_shift);
            g_l_old_z.y += (multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift)<<1) - multiply(fsp.fl.l.a, g_l_old_z.x, g_bit_shift);
            g_l_old_z.x = g_l_new_z.x;
            if (fsp.repeat_mapping)
            {
                g_l_magnitude = (g_l_temp_sqr_x = lsqr(g_l_old_z.x)) + (g_l_temp_sqr_y = lsqr(g_l_old_z.y));
                if ((g_l_magnitude > g_l_magnitude_limit) || (g_l_magnitude < 0))
                {
                    break;
                }
                g_l_new_z.x = g_l_temp_sqr_x - g_l_temp_sqr_y - g_l_old_z.x - multiply(fsp.fl.l.a, g_l_old_z.y, g_bit_shift);
                g_l_old_z.y += (multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift)<<1) - multiply(fsp.fl.l.a, g_l_old_z.x, g_bit_shift);
                g_l_old_z.x = g_l_new_z.x;
            }
            g_color_iter++;

            if (g_show_orbit)
            {
                if (driver_key_pressed())
                {
                    break;
                }
                iplot_orbit(g_l_old_z.x, g_l_old_z.y, -1);
            }

            if (labs(fsp.fl.l.halfa-g_l_old_z.y) < FROTH_LCLOSE
                    && g_l_old_z.x > fsp.fl.l.top_x1 && g_l_old_z.x < fsp.fl.l.top_x2)
            {
                if ((!fsp.repeat_mapping && fsp.attractors == 2)
                        || (fsp.repeat_mapping && fsp.attractors == 3))
                {
                    found_attractor = 1;
                }
                else if (g_l_old_z.x <= fsp.fl.l.top_x3)
                {
                    found_attractor = 1;
                }
                else if (g_l_old_z.x >= fsp.fl.l.top_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 1;
                    }
                    else
                    {
                        found_attractor = 2;
                    }
                }
            }
            else if (labs(multiply(FROTH_LSLOPE, g_l_old_z.x, g_bit_shift)-fsp.fl.l.a-g_l_old_z.y) < FROTH_LCLOSE
                     && g_l_old_z.x <= fsp.fl.l.right_x1 && g_l_old_z.x >= fsp.fl.l.right_x2)
            {
                if (!fsp.repeat_mapping && fsp.attractors == 2)
                {
                    found_attractor = 2;
                }
                else if (fsp.repeat_mapping && fsp.attractors == 3)
                {
                    found_attractor = 3;
                }
                else if (g_l_old_z.x >= fsp.fl.l.right_x3)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 2;
                    }
                    else
                    {
                        found_attractor = 4;
                    }
                }
                else if (g_l_old_z.x <= fsp.fl.l.right_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 3;
                    }
                    else
                    {
                        found_attractor = 6;
                    }
                }
            }
            else if (labs(multiply(-FROTH_LSLOPE, g_l_old_z.x, g_bit_shift)-fsp.fl.l.a-g_l_old_z.y) < FROTH_LCLOSE)
            {
                if (!fsp.repeat_mapping && fsp.attractors == 2)
                {
                    found_attractor = 2;
                }
                else if (fsp.repeat_mapping && fsp.attractors == 3)
                {
                    found_attractor = 2;
                }
                else if (g_l_old_z.x >= fsp.fl.l.left_x3)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 3;
                    }
                    else
                    {
                        found_attractor = 5;
                    }
                }
                else if (g_l_old_z.x <= fsp.fl.l.left_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 2;
                    }
                    else
                    {
                        found_attractor = 3;
                    }
                }
            }
            g_l_temp_sqr_x = lsqr(g_l_old_z.x);
            g_l_temp_sqr_y = lsqr(g_l_old_z.y);
            g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
        }
    }
    if (g_show_orbit)
    {
        scrub_orbit();
    }

    g_real_color_iter = g_color_iter;
    if ((g_keyboard_check_interval -= abs((int)g_real_color_iter)) <= 0)
    {
        if (check_key())
        {
            return (-1);
        }
        g_keyboard_check_interval = g_max_keyboard_check_interval;
    }

    // inside - Here's where non-palette based images would be nice.  Instead,
    // we'll use blocks of (colors-1)/3 or (colors-1)/6 and use special froth
    // color maps in attempt to replicate the images of James Alexander.
    if (found_attractor)
    {
        if (g_colors >= 256)
        {
            if (!fsp.altcolor)
            {
                if (g_color_iter > fsp.shades)
                {
                    g_color_iter = fsp.shades;
                }
            }
            else
            {
                g_color_iter = fsp.shades * g_color_iter / g_max_iterations;
            }
            if (g_color_iter == 0)
            {
                g_color_iter = 1;
            }
            g_color_iter += fsp.shades * (found_attractor-1);
        }
        else if (g_colors >= 16)
        {
            // only alternate coloring scheme available for 16 colors
            long lshade;

            // Trying to make a better 16 color distribution.
            // Since their are only a few possiblities, just handle each case.
            // This is a mostly guess work here.
            lshade = (g_color_iter<<16)/g_max_iterations;
            if (fsp.attractors != 6) // either 2 or 3 attractors
            {
                if (lshade < 2622)         // 0.04
                {
                    g_color_iter = 1;
                }
                else if (lshade < 10486)     // 0.16
                {
                    g_color_iter = 2;
                }
                else if (lshade < 23593)     // 0.36
                {
                    g_color_iter = 3;
                }
                else if (lshade < 41943L)     // 0.64
                {
                    g_color_iter = 4;
                }
                else
                {
                    g_color_iter = 5;
                }
                g_color_iter += 5 * (found_attractor-1);
            }
            else // 6 attractors
            {
                if (lshade < 10486)        // 0.16
                {
                    g_color_iter = 1;
                }
                else
                {
                    g_color_iter = 2;
                }
                g_color_iter += 2 * (found_attractor-1);
            }
        }
        else   // use a color corresponding to the attractor
        {
            g_color_iter = found_attractor;
        }
        g_old_color_iter = g_color_iter;
    }
    else   // outside, or inside but didn't get sucked in by attractor.
    {
        g_color_iter = 0;
    }

    g_color = abs((int)(g_color_iter));

    (*g_plot)(g_col, g_row, g_color);

    return g_color;
}

/*
These last two froth functions are for the orbit-in-window feature.
Normally, this feature requires standard_fractal, but since it is the
attractor that makes the frothybasin type so unique, it is worth
putting in as a stand-alone.
*/

int froth_per_pixel()
{
    if (!g_integer_fractal) // fp mode
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
        g_temp_sqr_x = sqr(g_old_z.x);
        g_temp_sqr_y = sqr(g_old_z.y);
    }
    else  // integer mode
    {
        g_l_old_z.x = g_l_x_pixel();
        g_l_old_z.y = g_l_y_pixel();
        g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
        g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    }
    return 0;
}

int froth_per_orbit()
{
    if (!g_integer_fractal) // fp mode
    {
        g_new_z.x = g_temp_sqr_x - g_temp_sqr_y - g_old_z.x - fsp.fl.f.a*g_old_z.y;
        g_new_z.y = 2.0*g_old_z.x*g_old_z.y - fsp.fl.f.a*g_old_z.x + g_old_z.y;
        if (fsp.repeat_mapping)
        {
            g_old_z = g_new_z;
            g_new_z.x = sqr(g_old_z.x) - sqr(g_old_z.y) - g_old_z.x - fsp.fl.f.a*g_old_z.y;
            g_new_z.y = 2.0*g_old_z.x*g_old_z.y - fsp.fl.f.a*g_old_z.x + g_old_z.y;
        }

        g_temp_sqr_x = sqr(g_new_z.x);
        g_temp_sqr_y = sqr(g_new_z.y);
        if (g_temp_sqr_x + g_temp_sqr_y >= g_magnitude_limit)
        {
            return 1;
        }
        g_old_z = g_new_z;
    }
    else  // integer mode
    {
        g_l_new_z.x = g_l_temp_sqr_x - g_l_temp_sqr_y - g_l_old_z.x - multiply(fsp.fl.l.a, g_l_old_z.y, g_bit_shift);
        g_l_new_z.y = g_l_old_z.y + (multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift)<<1) - multiply(fsp.fl.l.a, g_l_old_z.x, g_bit_shift);
        if (fsp.repeat_mapping)
        {
            g_l_temp_sqr_x = lsqr(g_l_new_z.x);
            g_l_temp_sqr_y = lsqr(g_l_new_z.y);
            if (g_l_temp_sqr_x + g_l_temp_sqr_y >= g_l_magnitude_limit)
            {
                return 1;
            }
            g_l_old_z = g_l_new_z;
            g_l_new_z.x = g_l_temp_sqr_x - g_l_temp_sqr_y - g_l_old_z.x - multiply(fsp.fl.l.a, g_l_old_z.y, g_bit_shift);
            g_l_new_z.y = g_l_old_z.y + (multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift)<<1) - multiply(fsp.fl.l.a, g_l_old_z.x, g_bit_shift);
        }
        g_l_temp_sqr_x = lsqr(g_l_new_z.x);
        g_l_temp_sqr_y = lsqr(g_l_new_z.y);
        if (g_l_temp_sqr_x + g_l_temp_sqr_y >= g_l_magnitude_limit)
        {
            return 1;
        }
        g_l_old_z = g_l_new_z;
    }
    return 0;
}
