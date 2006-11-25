/* d_win32.c -- Win32 driver for FractInt */
#include <string.h>

#include "port.h"
#include "cmplx.h"
#include "fractint.h"
#include "drivers.h"
#include "externs.h"
#include "prototyp.h"

static int win32_init(int *argc, char *argv)
{
  return TRUE;
}

static void win32_terminate(void)
{
}

static void win32_flush(Driver *drv)
{
}

void win32_schedule_alarm(Driver *drv, int secs)
{
}

int win32_start_video(Driver *drv)
{
    return 0;
}

int win32_end_video(Driver *drv)
{
    return 0;
}

void win32_window(Driver *drv)
{
}

int win32_resize(Driver *drv)
{
    return 0;
}

void win32_redraw(Driver *drv)
{
}

int win32_read_palette(Driver *drv)
{
    return 0;
}

int win32_write_palette(Driver *drv)
{
    return 0;
}

int win32_read_pixel(Driver *drv, int x, int y)
{
    return 0;
}

void win32_write_pixel(Driver *drv, int x, int y, int color)
{
}

void win32_read_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
}

void win32_write_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
}

void win32_set_line_mode(Driver *drv, int mode)
{
}

void win32_draw_line(Driver *drv, int x1, int y1, int x2, int y2)
{
}

int win32_get_key(Driver *drv, int block)
{
    return 0;
}

void win32_shell(Driver *drv)
{
}

void win32_set_video_mode(Driver *drv, int ax, int bx, int cx, int dx)
{
}

void win32_put_string(Driver *drv, int row, int col, int attr, const char *msg)
{
}

void win32_set_for_text(Driver *drv)
{
}

void win32_set_for_graphics(Driver *drv)
{
}

void win32_set_clear(Driver *drv)
{
}

BYTE *win32_find_font(Driver *drv, int parm)
{
    return NULL;
}

void win32_move_cursor(Driver *drv, int row, int col)
{
}

void win32_hide_text_cursor(Driver *drv)
{
}

void win32_set_attr(Driver *drv, int row, int col, int attr, int count)
{
}

void win32_scroll_up(Driver *drv, int top, int bot)
{
}

void win32_stack_screen(Driver *drv)
{
}

void win32_unstack_screen(Driver *drv)
{
}

void win32_discard_screen(Driver *drv)
{
}

int win32_init_fm(Driver *drv)
{
    return 0;
}

void win32_buzzer(Driver *drv, int kind)
{
}

void win32_sound_on(Driver *drv, int frequency)
{
}

void win32_sound_off(Driver *drv)
{
}

int win32_diskp(Driver *drv)
{
    return 0;
}

/* new driver		    old fractint
   -------------------  ------------
   start_video		    startvideo
   end_video		    endvideo
   read_palette		    readvideopalette
   write_palette	    writevideopalette
   read_pixel		    readvideo
   write_pixel		    writevideo
   read_span		    readvideoline
   write_span		    writevideoline
   set_line_mode	    setlinemode
   draw_line		    drawline
   get_key		        getkey
   shell		        shell_to_dos
   set_video_mode	    setvideomode
   set_for_text		    setfortext
   set_for_graphics	    setforgraphics
   set_clear		    setclear
   find_font		    findfont
   move_cursor		    movecursor
   set_attr		        setattr
   scroll_up		    scrollup
   stack_screen		    stackscreen
   unstack_screen	    unstackscreen
   discard_screen	    discardscreen
   init_fm		        initfm
   buzzer		        buzzer
   sound_on		        sound_on
   sound_off		    sound_off
   diskp                diskp
*/
Driver win32_driver = STD_DRIVER_STRUCT(win32);
