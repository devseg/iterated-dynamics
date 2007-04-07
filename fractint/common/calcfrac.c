/*
	CALCFRAC.C contains the high level ("engine") code for calculating the
	fractal images (well, SOMEBODY had to do it!).
	Original author Tim Wegner, but just about ALL the authors have contributed
	SOME code to this routine at one time or another, or contributed to one of
	the many massive restructurings.
	The following modules work very closely with CALCFRAC.C:
	  FRACTALS.C    the fractal-specific code for escape-time fractals.
	  FRACSUBR.C    assorted subroutines belonging mainly to calcfrac.
	  CALCMAND.ASM  fast Mandelbrot/Julia integer implementation
	Additional fractal-specific modules are also invoked from CALCFRAC:
	  LORENZ.C      engine level and fractal specific code for attractors.
	  JB.C          julibrot logic
	  PARSER.C      formula fractals
	  and more
 -------------------------------------------------------------------- */
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "targa_lc.h"
#include "drivers.h"

#define SHOWDOT_SAVE    1
#define SHOWDOT_RESTORE 2

#define JUST_A_POINT 0
#define LOWER_RIGHT  1
#define UPPER_RIGHT  2
#define LOWER_LEFT   3
#define UPPER_LEFT   4

#define DEM_BAILOUT 535.5  /* (pb: not sure if this is special or arbitrary) */
#define MAX_Y_BLOCK 7    /* MAX_X_BLOCK*MAX_Y_BLOCK*2 <= 4096, the size of "prefix" */
#define MAX_X_BLOCK 202  /* each maxnblk is oversize by 2 for a "border" */
							/* MAX_X_BLOCK defn must match fracsubr.c */

/***** vars for new btm *****/
enum direction
{
	North, East, South, West
};

/* variables exported from this file */
int g_orbit_draw_mode = ORBITDRAW_RECTANGLE;
_LCMPLX g_init_orbit_l = { 0, 0 };
long g_magnitude_l = 0;
long g_limit_l = 0;
long g_limit2_l = 0;
long g_close_enough_l = 0;
_CMPLX g_initial_z = { 0, 0 };
_CMPLX g_old_z = { 0, 0 };
_CMPLX g_new_z = { 0, 0 };
_CMPLX g_temp_z = { 0, 0 };
int g_color = 0;
long g_color_iter;
long g_old_color_iter;
long g_real_color_iter;
int g_row;
int g_col;
int g_passes;
int g_invert;
double g_f_radius;
double g_f_x_center;
double g_f_y_center; /* for inversion */
void (_fastcall *g_put_color)(int, int, int) = putcolor_a;
void (_fastcall *g_plot_color)(int, int, int) = putcolor_a;
double g_magnitude;
double g_rq_limit;
double g_rq_limit2;
int g_no_magnitude_calculation = FALSE;
int g_use_old_periodicity = FALSE;
int g_use_old_distance_test = FALSE;
int g_old_demm_colors = FALSE;
int (*g_calculate_type)(void) = NULL;
int (*g_calculate_type_temp)(void);
int g_quick_calculate = FALSE;
double g_proximity = 0.01;
double g_close_enough;
unsigned long g_magnitude_limit;               /* magnitude limit (CALCMAND) */
/* ORBIT variables */
int g_show_orbit;                     /* flag to turn on and off */
int g_orbit_index;                      /* pointer into g_save_orbit array */
int g_orbit_color = 15;                 /* XOR color */
int g_x_stop;
int g_y_stop;							/* stop here */
int g_symmetry;          /* symmetry flag */
int g_reset_periodicity; /* nonzero if escape time pixel rtn to reset */
int g_input_counter;
int g_max_input_counter;    /* avoids checking keyboard too often */
char *g_resume_info = NULL;                    /* resume info if allocated */
int g_resuming;                           /* nonzero if resuming after interrupt */
int g_num_work_list;                       /* resume g_work_list for standard engine */
WORKLIST g_work_list[MAXCALCWORK];
int g_xx_start;
int g_xx_stop;
int g_xx_begin;             /* these are same as g_work_list, */
int g_yy_start;
int g_yy_stop;
int g_yy_begin;             /* declared as separate items  */
VOIDPTR g_type_specific_work_area = NULL;
/* variables which must be visible for tab_display */
int g_got_status; /* -1 if not, 0 for 1or2pass, 1 for ssg, */
			  /* 2 for btm, 3 for 3d, 4 for tesseral, 5 for diffusion_scan */
              /* 6 for orbits */
int g_current_pass;
int g_total_passes;
int g_current_row;
int g_current_col;
/* vars for diffusion scan */
unsigned g_bits = 0; 		/* number of bits in the counter */
unsigned long g_diffusion_counter; 	/* the diffusion counter */
unsigned long g_diffusion_limit; 	/* the diffusion counter */
int g_three_pass;
int g_next_screen_flag; /* for cellular next screen generation */
int     g_num_attractors;                 /* number of finite attractors  */
_CMPLX  g_attractors[N_ATTR];       /* finite attractor vals (f.p)  */
_LCMPLX g_attractors_l[N_ATTR];      /* finite attractor vals (int)  */
int    g_attractor_period[N_ATTR];          /* period of the finite attractor */
int g_periodicity_check;
/* next has a skip bit for each s_max_block unit;
	1st pass sets bit  [1]... off only if block's contents guessed;
	at end of 1st pass [0]... bits are set if any surrounding block not guessed;
	bits are numbered [..][y/16 + 1][x + 1]&(1<<(y&15)) */
typedef int (*TPREFIX)[2][MAX_Y_BLOCK][MAX_X_BLOCK];
/* size of next puts a limit of MAXPIXELS pixels across on solid guessing logic */
BYTE g_stack[4096];              /* common temp, two put_line calls */
/* For periodicity testing, only in standard_fractal() */
int g_next_saved_incr;
int g_first_saved_and;
int g_atan_colors = 180;
long (*g_calculate_mandelbrot_asm_fp)(void);

/* routines in this module      */
static void perform_work_list(void);
static int  one_or_two_pass(void);
static int  _fastcall standard_calculate(int);
static int  _fastcall potential(double, long);
static void decomposition(void);
static int  boundary_trace_main(void);
static void step_col_row(void);
static int  solid_guess(void);
static int  _fastcall guess_row(int, int, int);
static void _fastcall plot_block(int, int, int, int);
static void _fastcall setsymmetry(int, int);
static int  _fastcall x_symmetry_split(int, int);
static int  _fastcall y_symmetry_split(int, int);
static void _fastcall put_truecolor_disk(int, int, int);
static int diffusion_engine(void);
static int draw_orbits(void);
static int tesseral(void);
static int _fastcall tesseral_check_column(int, int, int);
static int _fastcall tesseral_check_row(int, int, int);
static int _fastcall tesseral_column(int, int, int);
static int _fastcall tesseral_row(int, int, int);
static int diffusion_scan(void);
/* added for testing automatic_log_map() */
static long automatic_log_map(void);

/* lookup tables to avoid too much bit fiddling : */
static char s_diffusion_la[] =
{
	0, 8, 0, 8,4,12,4,12,0, 8, 0, 8,4,12,4,12, 2,10, 2,10,6,14,6,14,2,10,
	2,10, 6,14,6,14,0, 8,0, 8, 4,12,4,12,0, 8, 0, 8, 4,12,4,12,2,10,2,10,
	6,14, 6,14,2,10,2,10,6,14, 6,14,1, 9,1, 9, 5,13, 5,13,1, 9,1, 9,5,13,
	5,13, 3,11,3,11,7,15,7,15, 3,11,3,11,7,15, 7,15, 1, 9,1, 9,5,13,5,13,
	1, 9, 1, 9,5,13,5,13,3,11, 3,11,7,15,7,15, 3,11, 3,11,7,15,7,15,0, 8,
	0, 8, 4,12,4,12,0, 8,0, 8, 4,12,4,12,2,10, 2,10, 6,14,6,14,2,10,2,10,
	6,14, 6,14,0, 8,0, 8,4,12, 4,12,0, 8,0, 8, 4,12, 4,12,2,10,2,10,6,14,
	6,14, 2,10,2,10,6,14,6,14, 1, 9,1, 9,5,13, 5,13, 1, 9,1, 9,5,13,5,13,
	3,11, 3,11,7,15,7,15,3,11, 3,11,7,15,7,15, 1, 9, 1, 9,5,13,5,13,1, 9,
	1, 9, 5,13,5,13,3,11,3,11, 7,15,7,15,3,11, 3,11, 7,15,7,15
};

static char s_diffusion_lb[] =
{
	0, 8, 8, 0, 4,12,12, 4, 4,12,12, 4, 8, 0, 0, 8, 2,10,10, 2, 6,14,14,
	6, 6,14,14, 6,10, 2, 2,10, 2,10,10, 2, 6,14,14, 6, 6,14,14, 6,10, 2,
	2,10, 4,12,12, 4, 8, 0, 0, 8, 8, 0, 0, 8,12, 4, 4,12, 1, 9, 9, 1, 5,
	13,13, 5, 5,13,13, 5, 9, 1, 1, 9, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,
	11, 3, 3,11, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 5,13,13,
	5, 9, 1, 1, 9, 9, 1, 1, 9,13, 5, 5,13, 1, 9, 9, 1, 5,13,13, 5, 5,13,
	13, 5, 9, 1, 1, 9, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 3,
	11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 5,13,13, 5, 9, 1, 1, 9,
	9, 1, 1, 9,13, 5, 5,13, 2,10,10, 2, 6,14,14, 6, 6,14,14, 6,10, 2, 2,
	10, 4,12,12, 4, 8, 0, 0, 8, 8, 0, 0, 8,12, 4, 4,12, 4,12,12, 4, 8, 0,
	0, 8, 8, 0, 0, 8,12, 4, 4,12, 6,14,14, 6,10, 2, 2,10,10, 2, 2,10,14,
	6, 6,14
};

static double s_dem_delta, s_dem_width;     /* distance estimator variables */
static double s_dem_too_big;
static int s_dem_mandelbrot;
/* static vars for solid_guess & its subroutines */
static int s_max_block, s_half_block;
static int s_guess_plot;                   /* paint 1st pass row at a time?   */
static int s_right_guess, s_bottom_guess;
static _CMPLX s_saved_z;
static double s_rq_limit_save;
static int s_pixel_pi; /* value of pi in pixels */
static int s_ix_start;
static int s_iy_start;						/* start here */
static int s_work_pass;
static int s_work_sym;                   /* for the sake of calculate_mandelbrot    */
static enum direction s_going_to;
static int s_trail_row;
static int s_trail_col;
static unsigned int s_t_prefix[2][MAX_Y_BLOCK][MAX_X_BLOCK]; /* common temp */
static BYTE *s_save_dots = NULL;
static BYTE *s_fill_buffer;
static int s_save_dots_len;
static int s_show_dot_color;
static int s_show_dot_width = 0;

/* FMODTEST routine. */
/* Makes the test condition for the FMOD coloring type
	that of the current bailout method. 'or' and 'and'
	methods are not used - in these cases a normal
	modulus test is used
*/
static double fmod_test(void)
{
	double result;
	if (inside == FMODI && save_release <= 2000) /* for backwards compatibility */
	{
		result = (g_magnitude == 0.0 || !g_no_magnitude_calculation || integerfractal) ?
			sqr(g_new_z.x) + sqr(g_new_z.y) : g_magnitude;
		return result;
	}

	switch (g_bail_out_test)
	{
	case Mod:
		result = (g_magnitude == 0.0 || !g_no_magnitude_calculation || integerfractal) ?
			sqr(g_new_z.x) + sqr(g_new_z.y) : g_magnitude;
		break;
	case Real:
		result = sqr(g_new_z.x);
		break;
	case Imag:
		result = sqr(g_new_z.y);
		break;
	case Or:
		{
			double tmpx, tmpy;
			tmpx = sqr(g_new_z.x);
			tmpy = sqr(g_new_z.y);
			result = (tmpx > tmpy) ? tmpx : tmpy;
		}
		break;
	case Manh:
		result = sqr(fabs(g_new_z.x) + fabs(g_new_z.y));
		break;
	case Manr:
		result = sqr(g_new_z.x + g_new_z.y);
		break;
	default:
		result = sqr(g_new_z.x) + sqr(g_new_z.y);
		break;
	}
	return result;
}

/*
	The sym_fill_line() routine was pulled out of the boundary tracing
	code for re-use with showdot. It's purpose is to fill a line with a
	solid color. This assumes that BYTE *str is already filled
	with the color. The routine does write the line using symmetry
	in all cases, however the symmetry logic assumes that the line
	is one color; it is not general enough to handle a row of
	pixels of different colors.
*/
static void sym_fill_line(int row, int left, int right, BYTE *str)
{
	int i, j, k, length;
	length = right-left + 1;
	put_line(row, left, right, str);
	/* here's where all the symmetry goes */
	if (g_plot_color == g_put_color)
	{
		g_input_counter -= length >> 4; /* seems like a reasonable value */
	}
	else if (g_plot_color == symplot2) /* X-axis symmetry */
	{
		i = g_yy_stop-(row-g_yy_start);
		if (i > g_y_stop && i < ydots)
		{
			put_line(i, left, right, str);
			g_input_counter -= length >> 3;
		}
	}
	else if (g_plot_color == symplot2Y) /* Y-axis symmetry */
	{
		put_line(row, g_xx_stop-(right-g_xx_start), g_xx_stop-(left-g_xx_start), str);
		g_input_counter -= length >> 3;
	}
	else if (g_plot_color == symplot2J)  /* Origin symmetry */
	{
		i = g_yy_stop-(row-g_yy_start);
		j = min(g_xx_stop-(right-g_xx_start), xdots-1);
		k = min(g_xx_stop-(left -g_xx_start), xdots-1);
		if (i > g_y_stop && i < ydots && j <= k)
		{
			put_line(i, j, k, str);
		}
		g_input_counter -= length >> 3;
	}
	else if (g_plot_color == symplot4) /* X-axis and Y-axis symmetry */
	{
		i = g_yy_stop-(row-g_yy_start);
		j = min(g_xx_stop-(right-g_xx_start), xdots-1);
		k = min(g_xx_stop-(left -g_xx_start), xdots-1);
		if (i > g_y_stop && i < ydots)
		{
			put_line(i, left, right, str);
			if (j <= k)
			{
				put_line(i, j, k, str);
			}
		}
		if (j <= k)
		{
			put_line(row, j, k, str);
		}
		g_input_counter -= length >> 2;
	}
	else    /* cheap and easy way out */
	{
		for (i = left; i <= right; i++)  /* DG */
		{
			(*g_plot_color)(i, row, str[i-left]);
		}
		g_input_counter -= length >> 1;
	}
}

/*
  The sym_put_line() routine is the symmetry-aware version of put_line().
  It only works efficiently in the no symmetry or XAXIS symmetry case,
  otherwise it just writes the pixels one-by-one.
*/
static void sym_put_line(int row, int left, int right, BYTE *str)
{
	int length, i;
	length = right-left + 1;
	put_line(row, left, right, str);
	if (g_plot_color == g_put_color)
	{
		g_input_counter -= length >> 4; /* seems like a reasonable value */
	}
	else if (g_plot_color == symplot2) /* X-axis symmetry */
	{
		i = g_yy_stop-(row-g_yy_start);
		if (i > g_y_stop && i < ydots)
		{
			put_line(i, left, right, str);
		}
		g_input_counter -= length >> 3;
	}
	else
	{
		for (i = left; i <= right; i++)  /* DG */
		{
			(*g_plot_color)(i, row, str[i-left]);
		}
		g_input_counter -= length >> 1;
	}
}

static void show_dot_save_restore(int startx, int stopx, int starty, int stopy, int direction, int action)
{
	int j, ct;
	ct = 0;
	if (direction != JUST_A_POINT)
	{
		if (s_save_dots == NULL)
		{
			stopmsg(0, "s_save_dots NULL");
			exit(0);
		}
		if (s_fill_buffer == NULL)
		{
			stopmsg(0, "s_fill_buffer NULL");
			exit(0);
		}
	}
	switch (direction)
	{
	case LOWER_RIGHT:
		for (j = starty; j <= stopy; startx++, j++)
		{
			if (action == SHOWDOT_SAVE)
			{
				get_line(j, startx, stopx, s_save_dots + ct);
				sym_fill_line(j, startx, stopx, s_fill_buffer);
			}
			else
			{
				sym_put_line(j, startx, stopx, s_save_dots + ct);
			}
			ct += stopx-startx + 1;
		}
		break;
	case UPPER_RIGHT:
		for (j = starty; j >= stopy; startx++, j--)
		{
			if (action == SHOWDOT_SAVE)
			{
				get_line(j, startx, stopx, s_save_dots + ct);
				sym_fill_line(j, startx, stopx, s_fill_buffer);
			}
			else
			{
				sym_put_line(j, startx, stopx, s_save_dots + ct);
			}
			ct += stopx-startx + 1;
		}
		break;
	case LOWER_LEFT:
		for (j = starty; j <= stopy; stopx--, j++)
		{
			if (action == SHOWDOT_SAVE)
			{
				get_line(j, startx, stopx, s_save_dots + ct);
				sym_fill_line(j, startx, stopx, s_fill_buffer);
			}
			else
			{
				sym_put_line(j, startx, stopx, s_save_dots + ct);
			}
			ct += stopx-startx + 1;
		}
		break;
	case UPPER_LEFT:
		for (j = starty; j >= stopy; stopx--, j--)
		{
			if (action == SHOWDOT_SAVE)
			{
				get_line(j, startx, stopx, s_save_dots + ct);
				sym_fill_line(j, startx, stopx, s_fill_buffer);
			}
			else
			{
				sym_put_line(j, startx, stopx, s_save_dots + ct);
			}
			ct += stopx-startx + 1;
		}
		break;
	}
	if (action == SHOWDOT_SAVE)
	{
		(*g_plot_color)(g_col, g_row, s_show_dot_color);
	}
}

static int calculate_type_show_dot(void)
{
	int out, startx, starty, stopx, stopy, direction, width;
	direction = JUST_A_POINT;
	startx = stopx = g_col;
	starty = stopy = g_row;
	width = s_show_dot_width + 1;
	if (width > 0)
	{
		if (g_col + width <= g_x_stop && g_row + width <= g_y_stop)
		{
			/* preferred showdot shape */
			direction = UPPER_LEFT;
			startx = g_col;
			stopx  = g_col + width;
			starty = g_row + width;
			stopy  = g_row + 1;
		}
		else if (g_col-width >= s_ix_start && g_row + width <= g_y_stop)
		{
			/* second choice */
			direction = UPPER_RIGHT;
			startx = g_col-width;
			stopx  = g_col;
			starty = g_row + width;
			stopy  = g_row + 1;
		}
		else if (g_col-width >= s_ix_start && g_row-width >= s_iy_start)
		{
			direction = LOWER_RIGHT;
			startx = g_col-width;
			stopx  = g_col;
			starty = g_row-width;
			stopy  = g_row-1;
		}
		else if (g_col + width <= g_x_stop && g_row-width >= s_iy_start)
		{
			direction = LOWER_LEFT;
			startx = g_col;
			stopx  = g_col + width;
			starty = g_row-width;
			stopy  = g_row-1;
		}
	}
	show_dot_save_restore(startx, stopx, starty, stopy, direction, SHOWDOT_SAVE);
	if (orbit_delay > 0)
	{
		sleep_ms(orbit_delay);
	}
	out = (*g_calculate_type_temp)();
	show_dot_save_restore(startx, stopx, starty, stopy, direction, SHOWDOT_RESTORE);
	return out;
}

/******* calculate_fractal - the top level routine for generating an image *******/
int calculate_fractal(void)
{
	matherr_ct = 0;
	g_num_attractors = 0;          /* default to no known finite attractors  */
	display3d = 0;
	basin = 0;
	/* added yet another level of indirection to g_put_color!!! TW */
	g_put_color = putcolor_a;
	if (g_is_true_color && truemode)
	{
		/* Have to force passes = 1 */
		usr_stdcalcmode = stdcalcmode = '1';
	}
	if (truecolor)
	{
		check_writefile(g_light_name, ".tga");
		if (startdisk1(g_light_name, NULL, 0) == 0)
		{
			/* Have to force passes = 1 */
			usr_stdcalcmode = stdcalcmode = '1';
			g_put_color = put_truecolor_disk;
		}
		else
		{
			truecolor = 0;
		}
	}
	if (!g_use_grid)
	{
		if (usr_stdcalcmode != 'o')
		{
			usr_stdcalcmode = stdcalcmode = '1';
		}
	}

	init_misc();  /* set up some variables in parser.c */
	reset_clock();

	/* following delta values useful only for types with rotation disabled */
	/* currently used only by bifurcation */
	if (integerfractal)
	{
		distest = 0;
	}
	parm.x   = param[0];
	parm.y   = param[1];
	parm2.x  = param[2];
	parm2.y  = param[3];

	if (LogFlag && colors < 16)
	{
		stopmsg(0, "Need at least 16 colors to use logmap");
		LogFlag = 0;
	}

	if (g_use_old_periodicity)
	{
		g_next_saved_incr = 1;
		g_first_saved_and = 1;
	}
	else
	{
		g_next_saved_incr = (int)log10(maxit); /* works better than log() */
		if (g_next_saved_incr < 4)
		{
			g_next_saved_incr = 4; /* maintains image with low iterations */
		}
		g_first_saved_and = (long)((g_next_saved_incr*2) + 1);
	}

	LogTable = NULL;
	MaxLTSize = maxit;
	Log_Calc = 0;
	/* below, INT_MAX = 32767 only when an integer is two bytes.  Which is not true for Xfractint. */
	/* Since 32767 is what was meant, replaced the instances of INT_MAX with 32767. */
	if (LogFlag && (((maxit > 32767) && (save_release > 1920))
		|| Log_Fly_Calc == 1))
	{
		Log_Calc = 1; /* calculate on the fly */
		SetupLogTable();
	}
	else if (LogFlag && (((maxit > 32767) && (save_release <= 1920))
		|| Log_Fly_Calc == 2))
	{
		MaxLTSize = 32767;
		Log_Calc = 0; /* use logtable */
	}
	else if (rangeslen && (maxit >= 32767))
	{
		MaxLTSize = 32766;
	}

	if ((LogFlag || rangeslen) && !Log_Calc)
	{
		LogTable = (BYTE *)malloc((long)MaxLTSize + 1);

		if (LogTable == NULL)
		{
			if (rangeslen || Log_Fly_Calc == 2)
			{
				stopmsg(0, "Insufficient memory for logmap/ranges with this maxiter");
			}
			else
			{
				stopmsg(0, "Insufficient memory for logTable, using on-the-fly routine");
				Log_Fly_Calc = 1;
				Log_Calc = 1; /* calculate on the fly */
				SetupLogTable();
			}
		}
		else if (rangeslen)  /* Can't do ranges if MaxLTSize > 32767 */
		{
			int i, k, l, m, numval, flip, altern;
			i = k = l = 0;
			LogFlag = 0; /* ranges overrides logmap */
			while (i < rangeslen)
			{
				m = flip = 0;
				altern = 32767;
				numval = ranges[i++];
				if (numval < 0)
				{
					altern = ranges[i++];    /* sub-range iterations */
					numval = ranges[i++];
				}
				if (numval > (int)MaxLTSize || i >= rangeslen)
				{
					numval = (int)MaxLTSize;
				}
				while (l <= numval)
				{
					LogTable[l++] = (BYTE)(k + flip);
					if (++m >= altern)
					{
						flip ^= 1;            /* Alternate colors */
						m = 0;
					}
				}
				++k;
				if (altern != 32767)
				{
					++k;
				}
			}
		}
		else
		{
			SetupLogTable();
		}
	}
	g_magnitude_limit = 4L << bitshift;                 /* CALCMAND magnitude limit */

	g_atan_colors = (save_release > 2002) ? colors : 180;

	/* ORBIT stuff */
	g_show_orbit = start_showorbit;
	g_orbit_index = 0;
	g_orbit_color = 15;
	if (colors < 16)
	{
		g_orbit_color = 1;
	}

	if (inversion[0] != 0.0)
	{
		g_f_radius    = inversion[0];
		g_f_x_center   = inversion[1];
		g_f_y_center   = inversion[2];

		if (inversion[0] == AUTOINVERT)  /*  auto calc radius 1/6 screen */
		{
			inversion[0] = min(fabs(xxmax - xxmin), fabs(yymax - yymin)) / 6.0;
			fix_inversion(&inversion[0]);
			g_f_radius = inversion[0];
		}

		if (g_invert < 2 || inversion[1] == AUTOINVERT)  /* xcenter not already set */
		{
			inversion[1] = (xxmin + xxmax) / 2.0;
			fix_inversion(&inversion[1]);
			g_f_x_center = inversion[1];
			if (fabs(g_f_x_center) < fabs(xxmax-xxmin) / 100)
			{
				inversion[1] = g_f_x_center = 0.0;
			}
		}

		if (g_invert < 3 || inversion[2] == AUTOINVERT)  /* ycenter not already set */
		{
			inversion[2] = (yymin + yymax) / 2.0;
			fix_inversion(&inversion[2]);
			g_f_y_center = inversion[2];
			if (fabs(g_f_y_center) < fabs(yymax-yymin) / 100)
			{
				inversion[2] = g_f_y_center = 0.0;
			}
		}

		g_invert = 3; /* so values will not be changed if we come back */
	}

	g_close_enough = ddelmin*pow(2.0, -(double)(abs(g_periodicity_check)));
	s_rq_limit_save = g_rq_limit;
	g_rq_limit2 = sqrt(g_rq_limit);
	if (integerfractal)          /* for integer routines (lambda) */
	{
		g_parameter_l.x = (long)(parm.x*fudge);    /* real portion of Lambda */
		g_parameter_l.y = (long)(parm.y*fudge);    /* imaginary portion of Lambda */
		g_parameter2_l.x = (long)(parm2.x*fudge);  /* real portion of Lambda2 */
		g_parameter2_l.y = (long)(parm2.y*fudge);  /* imaginary portion of Lambda2 */
		g_limit_l = (long)(g_rq_limit*fudge);      /* stop if magnitude exceeds this */
		if (g_limit_l <= 0)
		{
			g_limit_l = 0x7fffffffL; /* klooge for integer math */
		}
		g_limit2_l = (long)(g_rq_limit2*fudge);    /* stop if magnitude exceeds this */
		g_close_enough_l = (long)(g_close_enough*fudge); /* "close enough" value */
		g_init_orbit_l.x = (long)(initorbit.x*fudge);
		g_init_orbit_l.y = (long)(initorbit.y*fudge);
	}
	g_resuming = (calc_status == CALCSTAT_RESUMABLE);
	if (!g_resuming) /* free resume_info memory if any is hanging around */
	{
		end_resume();
		if (resave_flag)
		{
			updatesavename(savename); /* do the pending increment */
			resave_flag = RESAVE_NO;
			started_resaves = FALSE;
		}
		calctime = 0;
	}

	if (curfractalspecific->calculate_type != standard_fractal
		&& curfractalspecific->calculate_type != calculate_mandelbrot
		&& curfractalspecific->calculate_type != calculate_mandelbrot_fp
		&& curfractalspecific->calculate_type != lyapunov
		&& curfractalspecific->calculate_type != froth_calc)
	{
		g_calculate_type = curfractalspecific->calculate_type; /* per_image can override */
		g_symmetry = curfractalspecific->symmetry; /*   calculate_type & symmetry  */
		g_plot_color = g_put_color; /* defaults when setsymmetry not called or does nothing */
		s_iy_start = s_ix_start = g_yy_start = g_xx_start = g_yy_begin = g_xx_begin = 0;
		g_y_stop = g_yy_stop = ydots -1;
		g_x_stop = g_xx_stop = xdots -1;
		calc_status = CALCSTAT_IN_PROGRESS; /* mark as in-progress */
		distest = 0; /* only standard escape time engine supports distest */
		/* per_image routine is run here */
		if (curfractalspecific->per_image())
		{ /* not a stand-alone */
			/* next two lines in case periodicity changed */
			g_close_enough = ddelmin*pow(2.0, -(double)(abs(g_periodicity_check)));
			g_close_enough_l = (long)(g_close_enough*fudge); /* "close enough" value */
			setsymmetry(g_symmetry, 0);
			timer(TIMER_ENGINE, g_calculate_type); /* non-standard fractal engine */
		}
		if (check_key())
		{
			if (calc_status == CALCSTAT_IN_PROGRESS) /* calctype didn't set this itself, */
			{
				calc_status = CALCSTAT_NON_RESUMABLE;   /* so mark it interrupted, non-resumable */
			}
		}
		else
		{
			calc_status = CALCSTAT_COMPLETED; /* no key, so assume it completed */
		}
	}
	else /* standard escape-time engine */
	{
		if (stdcalcmode == '3')  /* convoluted 'g' + '2' hybrid */
		{
			int oldcalcmode;
			oldcalcmode = stdcalcmode;
			if (!g_resuming || g_three_pass)
			{
				stdcalcmode = 'g';
				g_three_pass = 1;
				timer(TIMER_ENGINE, (int(*)())perform_work_list);
				if (calc_status == CALCSTAT_COMPLETED)
				{
					/* '2' is silly after 'g' for low rez */
					stdcalcmode = (xdots >= 640) ? '2' : '1';
					timer(TIMER_ENGINE, (int(*)())perform_work_list);
					g_three_pass = 0;
				}
			}
			else /* resuming '2' pass */
			{
				stdcalcmode = (xdots >= 640) ? '2' : '1';
				timer(TIMER_ENGINE, (int (*)()) perform_work_list);
			}
			stdcalcmode = (char)oldcalcmode;
		}
		else /* main case, much nicer! */
		{
			g_three_pass = 0;
			timer(TIMER_ENGINE, (int(*)())perform_work_list);
		}
	}
	calctime += timer_interval;

	if (LogTable && !Log_Calc)
	{
		free(LogTable);   /* free if not using extraseg */
		LogTable = NULL;
	}
	if (g_type_specific_work_area)
	{
		free_workarea();
	}

	if (curfractalspecific->calculate_type == froth_calc)
	{
		froth_cleanup();
	}
	if ((soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP) /* close sound write file */
	{
		sound_close();
	}
	if (truecolor)
	{
		enddisk();
	}
	return (calc_status == CALCSTAT_COMPLETED) ? 0 : -1;
}

/* locate alternate math record */
int find_alternate_math(int type, int math)
{
	int i;
	if (math == 0)
	{
		return -1;
	}
	for (i = 0; i < g_alternate_math_len; i++)
	{
		if ((type == g_alternate_math[i].type) && g_alternate_math[i].math)
		{
			return i;
		}
	}
	return -1;
}


/**************** general escape-time engine routines *********************/

static void perform_work_list()
{
	int (*sv_orbitcalc)(void) = NULL;  /* function that calculates one orbit */
	int (*sv_per_pixel)(void) = NULL;  /* once-per-pixel init */
	int (*sv_per_image)(void) = NULL;  /* once-per-image setup */
	int i, alt;

	alt = find_alternate_math(fractype, bf_math);
	if (alt > -1)
	{
		sv_orbitcalc = curfractalspecific->orbitcalc;
		sv_per_pixel = curfractalspecific->per_pixel;
		sv_per_image = curfractalspecific->per_image;
		curfractalspecific->orbitcalc = g_alternate_math[alt].orbitcalc;
		curfractalspecific->per_pixel = g_alternate_math[alt].per_pixel;
		curfractalspecific->per_image = g_alternate_math[alt].per_image;
	}
	else
	{
		bf_math = 0;
	}

	if (potflag && pot16bit)
	{
		int tmpcalcmode = stdcalcmode;

		stdcalcmode = '1'; /* force 1 pass */
		if (g_resuming == 0)
		{
			if (pot_startdisk() < 0)
			{
				pot16bit = 0;       /* startdisk failed or cancelled */
				stdcalcmode = (char)tmpcalcmode;    /* maybe we can carry on??? */
			}
		}
	}
	if (stdcalcmode == 'b' && (curfractalspecific->flags & NOTRACE))
	{
		stdcalcmode = '1';
	}
	if (stdcalcmode == 'g' && (curfractalspecific->flags & NOGUESS))
	{
		stdcalcmode = '1';
	}
	if (stdcalcmode == 'o' && (curfractalspecific->calculate_type != standard_fractal))
	{
		stdcalcmode = '1';
	}

	/* default setup a new g_work_list */
	g_num_work_list = 1;
	g_work_list[0].xx_start = g_work_list[0].xx_begin = 0;
	g_work_list[0].yy_start = g_work_list[0].yy_begin = 0;
	g_work_list[0].xx_stop = xdots - 1;
	g_work_list[0].yy_stop = ydots - 1;
	g_work_list[0].pass = g_work_list[0].sym = 0;
	if (g_resuming) /* restore g_work_list, if we can't the above will stay in place */
	{
		int vsn;
		vsn = start_resume();
		get_resume(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
		end_resume();
		if (vsn < 2)
		{
			g_xx_begin = 0;
		}
	}

	if (distest) /* setup stuff for distance estimator */
	{
		double ftemp, ftemp2, delxx, delyy2, delyy, delxx2, dxsize, dysize;
		double aspect;
		if (pseudox && pseudoy)
		{
			aspect = (double)pseudoy/(double)pseudox;
			dxsize = pseudox-1;
			dysize = pseudoy-1;
		}
		else
		{
			aspect = (double)ydots/(double)xdots;
			dxsize = xdots-1;
			dysize = ydots-1;
		}

		delxx  = (xxmax - xx3rd) / dxsize; /* calculate stepsizes */
		delyy  = (yymax - yy3rd) / dysize;
		delxx2 = (xx3rd - xxmin) / dysize;
		delyy2 = (yy3rd - yymin) / dxsize;

		/* in case it's changed with <G> */
		g_use_old_distance_test = (save_release < 1827) ? 1 : 0;

		g_rq_limit = s_rq_limit_save; /* just in case changed to DEM_BAILOUT earlier */
		if (distest != 1 || colors == 2) /* not doing regular outside colors */
		{
			if (g_rq_limit < DEM_BAILOUT)         /* so go straight for dem bailout */
			{
				g_rq_limit = DEM_BAILOUT;
			}
		}
		/* must be mandel type, formula, or old PAR/GIF */
		s_dem_mandelbrot =
			(curfractalspecific->tojulia != NOFRACTAL
			|| g_use_old_distance_test
			|| fractype == FORMULA
			|| fractype == FFORMULA) ?
				TRUE : FALSE;

		s_dem_delta = sqr(delxx) + sqr(delyy2);
		ftemp = sqr(delyy) + sqr(delxx2);
		if (ftemp > s_dem_delta)
		{
			s_dem_delta = ftemp;
		}
		if (distestwidth == 0)
		{
			distestwidth = 1;
		}
		ftemp = distestwidth;
		/* multiply by thickness desired */
		s_dem_delta *= (distestwidth > 0) ? sqr(ftemp)/10000 : 1/(sqr(ftemp)*10000); 
		s_dem_width = (sqrt(sqr(xxmax-xxmin) + sqr(xx3rd-xxmin) )*aspect
			+ sqrt(sqr(yymax-yymin) + sqr(yy3rd-yymin) ) ) / distest;
		ftemp = (g_rq_limit < DEM_BAILOUT) ? DEM_BAILOUT : g_rq_limit;
		ftemp += 3; /* bailout plus just a bit */
		ftemp2 = log(ftemp);
		s_dem_too_big = g_use_old_distance_test ?
			sqr(ftemp)*sqr(ftemp2)*4 / s_dem_delta
			:
			fabs(ftemp)*fabs(ftemp2)*2 / sqrt(s_dem_delta);
	}

	while (g_num_work_list > 0)
	{
		/* per_image can override */
		g_calculate_type = curfractalspecific->calculate_type;
		g_symmetry = curfractalspecific->symmetry; /*   calctype & symmetry  */
		g_plot_color = g_put_color; /* defaults when setsymmetry not called or does nothing */

		/* pull top entry off g_work_list */
		s_ix_start = g_xx_start = g_work_list[0].xx_start;
		g_x_stop  = g_xx_stop  = g_work_list[0].xx_stop;
		g_xx_begin  = g_work_list[0].xx_begin;
		s_iy_start = g_yy_start = g_work_list[0].yy_start;
		g_y_stop  = g_yy_stop  = g_work_list[0].yy_stop;
		g_yy_begin  = g_work_list[0].yy_begin;
		s_work_pass = g_work_list[0].pass;
		s_work_sym  = g_work_list[0].sym;
		--g_num_work_list;
		for (i = 0; i < g_num_work_list; ++i)
		{
			g_work_list[i] = g_work_list[i + 1];
		}

		calc_status = CALCSTAT_IN_PROGRESS; /* mark as in-progress */

		curfractalspecific->per_image();
		if (showdot >= 0)
		{
			find_special_colors();
			switch (autoshowdot)
			{
			case 'd':
				s_show_dot_color = g_color_dark % colors;
				break;
			case 'm':
				s_show_dot_color = g_color_medium % colors;
				break;
			case 'b':
			case 'a':
				s_show_dot_color = g_color_bright % colors;
				break;
			default:
				s_show_dot_color = showdot % colors;
				break;
			}
			if (sizedot <= 0)
			{
				s_show_dot_width = -1;
			}
			else
			{
				double dshowdot_width;
				dshowdot_width = (double)sizedot*xdots/1024.0;
				/*
					Arbitrary sanity limit, however s_show_dot_width will
					overflow if dshowdot width gets near 256.
				*/
				if (dshowdot_width > 150.0)
				{
					s_show_dot_width = 150;
				}
				else if (dshowdot_width > 0.0)
				{
					s_show_dot_width = (int)dshowdot_width;
				}
				else
				{
					s_show_dot_width = -1;
				}
			}
#ifdef SAVEDOTS_USES_MALLOC
			while (s_show_dot_width >= 0)
			{
				/*
					We're using near memory, so get the amount down
					to something reasonable. The polynomial used to
					calculate s_save_dots_len is exactly right for the
					triangular-shaped shotdot cursor. The that cursor
					is changed, this formula must match.
				*/
				while ((s_save_dots_len = sqr(s_show_dot_width) + 5*s_show_dot_width + 4) > 1000)
				{
					s_show_dot_width--;
				}
				s_save_dots = (BYTE *)malloc(s_save_dots_len);
				if (s_save_dots != NULL)
				{
					s_save_dots_len /= 2;
					s_fill_buffer = s_save_dots + s_save_dots_len;
					memset(s_fill_buffer, s_show_dot_color, s_save_dots_len);
					break;
				}
				/*
					There's even less free memory than we thought, so reduce
					s_show_dot_width still more
				*/
				s_show_dot_width--;
			}
			if (s_save_dots == NULL)
			{
				s_show_dot_width = -1;
			}
#else
			while ((s_save_dots_len = sqr(s_show_dot_width) + 5*s_show_dot_width + 4) > 2048)
			{
				s_show_dot_width--;
			}
			s_save_dots = (BYTE *)decoderline;
			s_save_dots_len /= 2;
			s_fill_buffer = s_save_dots + s_save_dots_len;
			memset(s_fill_buffer, s_show_dot_color, s_save_dots_len);
#endif
			g_calculate_type_temp = g_calculate_type;
			g_calculate_type    = calculate_type_show_dot;
		}

		/* some common initialization for escape-time pixel level routines */
		g_close_enough = ddelmin*pow(2.0, -(double)(abs(g_periodicity_check)));
		g_close_enough_l = (long)(g_close_enough*fudge); /* "close enough" value */
		g_input_counter = g_max_input_counter;

		setsymmetry(g_symmetry, 1);

		if (!(g_resuming) && (labs(LogFlag) == 2 || (LogFlag && Log_Auto_Calc)))
		{  /* calculate round screen edges to work out best start for logmap */
			LogFlag = (automatic_log_map()*(LogFlag / labs(LogFlag)));
			SetupLogTable();
		}

		/* call the appropriate escape-time engine */
		switch (stdcalcmode)
		{
		case 's':
			if (DEBUGFLAG_SOI_LONG_DOUBLE == debugflag)
			{
				soi_ldbl();
			}
			else
			{
				soi();
			}
			break;
		case 't':
			tesseral();
			break;
		case 'b':
			boundary_trace_main();
			break;
		case 'g':
			solid_guess();
			break;
		case 'd':
			diffusion_scan();
			break;
		case 'o':
			draw_orbits();
			break;
		default:
			one_or_two_pass();
		}
#ifdef SAVEDOTS_USES_MALLOC
		if (s_save_dots != NULL)
		{
			free(s_save_dots);
			s_save_dots = NULL;
			s_fill_buffer = NULL;
		}
#endif
		if (check_key()) /* interrupted? */
		{
			break;
		}
	}

	if (g_num_work_list > 0)
	{  /* interrupted, resumable */
		alloc_resume(sizeof(g_work_list) + 20, 2);
		put_resume(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
	}
	else
	{
		calc_status = CALCSTAT_COMPLETED; /* completed */
	}
	if (sv_orbitcalc != NULL)
	{
		curfractalspecific->orbitcalc = sv_orbitcalc;
		curfractalspecific->per_pixel = sv_per_pixel;
		curfractalspecific->per_image = sv_per_image;
	}
}

static int diffusion_scan(void)
{
	double log2;

	log2 = (double) log (2.0);

	g_got_status = GOT_STATUS_DIFFUSION;

	/* note: the max size of 2048x2048 gives us a 22 bit counter that will */
	/* fit any 32 bit architecture, the maxinum limit for this case would  */
	/* be 65536x65536 (HB) */

	g_bits = (unsigned) (min (log (g_y_stop-s_iy_start + 1), log(g_x_stop-s_ix_start + 1) )/log2 );
	g_bits <<= 1; /* double for two axes */
	g_diffusion_limit = 1l << g_bits;

	if (diffusion_engine() == -1)
	{
		work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop,
			(int)(g_diffusion_counter >> 16),            /* high, */
			(int)(g_diffusion_counter & 0xffff),         /* low order words */
			s_work_sym);
		return -1;
	}

	return 0;
}

/* little function that plots a filled square of color c, size s with
	top left cornet at (x, y) with optimization from sym_fill_line */
static void diffusion_plot_block(int x, int y, int s, int c)
{
	int ty;
	memset(g_stack, c, s);
	for (ty = y; ty < y + s; ty++)
	{
		sym_fill_line(ty, x, x + s - 1, g_stack);
	}
}

/* function that does the same as above, but checks the limits in x and y */
static void plot_block_lim(int x, int y, int s, int c)
{
	int ty;
	memset(g_stack, (c), (s));
	for (ty = y; ty < min(y + s, g_y_stop + 1); ty++)
	{
		sym_fill_line(ty, x, min(x + s - 1, g_x_stop), g_stack);
	}
}

/* function: count_to_int(dif_offset, g_diffusion_counter, colo, rowo) */
static void count_to_int(int dif_offset, unsigned long C, int *x, int *y)
{
	*x = s_diffusion_la[C & 0xFF];
	*y = s_diffusion_lb[C & 0xFF];
	C >>= 8;
	*x <<= 4;
	*x += s_diffusion_la[C & 0xFF];
	*y <<= 4;
	*y += s_diffusion_lb[C & 0xFF];
	C >>= 8;
	*x <<= 4;
	*x += s_diffusion_la[C & 0xFF];
	*y <<= 4;
	*y += s_diffusion_lb[C & 0xFF];
	C >>= 8;
	*x >>= dif_offset;
	*y >>= dif_offset;
}

/* REMOVED: counter byte 3 */                                                          \
/* (x) < <= 4; (x) += s_diffusion_la[tC&0(x)FF]; (y) < <= 4; (y) += s_diffusion_lb[tC&0(x)FF]; tC >>= 8;
	--> eliminated this and made (*) because fractint user coordinates up to
	2048(x)2048 what means a counter of 24 bits or 3 bytes */

/* Calculate the point */
static int diffusion_point(int row, int col)
{
	g_reset_periodicity = 1;
	if ((*g_calculate_type)() == -1)
	{
		return TRUE;
	}
	g_reset_periodicity = 0;
	(*g_plot_color)(col, row, g_color);
	return FALSE;
}

static int diffusion_block(int row, int col, int sqsz)
{
	g_reset_periodicity = 1;
	if ((*g_calculate_type)() == -1)
	{
		return TRUE;
	}
	g_reset_periodicity = 0;
	diffusion_plot_block(col, row, sqsz, g_color);
	return FALSE;
}

static int diffusion_block_lim(int row, int col, int sqsz)
{
	g_reset_periodicity = 1;
	if ((*g_calculate_type)() == -1)
	{
		return TRUE;
	}
	g_reset_periodicity = 0;
	plot_block_lim(col, row, sqsz, g_color);
	return FALSE;
}

static int diffusion_engine(void)
{
	double log2 = (double) log (2.0);
	int i, j;
	int nx, ny; /* number of tyles to build in x and y dirs */
	/* made this to complete the area that is not */
	/* a square with sides like 2 ** n */
	int rem_x, rem_y; /* what is left on the last tile to draw */
	int dif_offset; /* offset for adjusting looked-up values */
	int sqsz;  /* size of the block being filled */
	int colo, rowo; /* original col and row */
	int s = 1 << (g_bits/2); /* size of the square */

	nx = (int) floor((g_x_stop-s_ix_start + 1)/s );
	ny = (int) floor((g_y_stop-s_iy_start + 1)/s );

	rem_x = (g_x_stop-s_ix_start + 1) - nx*s;
	rem_y = (g_y_stop-s_iy_start + 1) - ny*s;

	if (g_yy_begin == s_iy_start && s_work_pass == 0)  /* if restarting on pan: */
	{
		g_diffusion_counter = 0L;
	}
	else
	{
		/* g_yy_begin and passes contain data for resuming the type: */
		g_diffusion_counter = (((long) ((unsigned)g_yy_begin)) << 16) | ((unsigned)s_work_pass);
	}

	dif_offset = 12-(g_bits/2); /* offset to adjust coordinates */
				/* (*) for 4 bytes use 16 for 3 use 12 etc. */

	/*************************************/
	/* only the points (dithering only) :*/
	if (fillcolor == 0 )
	{
		while (g_diffusion_counter < (g_diffusion_limit >> 1))
		{
			count_to_int(dif_offset, g_diffusion_counter, &colo, &rowo);
			i = 0;
			g_col = s_ix_start + colo; /* get the right tiles */
			do
			{
				j = 0;
				g_row = s_iy_start + rowo;
				do
				{
					if (diffusion_point(g_row, g_col))
					{
						return -1;
					}
					j++;
					g_row += s;                  /* next tile */
				}
				while (j < ny);
				/* in the last y tile we may not need to plot the point */
				if (rowo < rem_y)
				{
					if (diffusion_point(g_row, g_col))
					{
						return -1;
					}
				}
				i++;
				g_col += s;
			}
			while (i < nx);
			/* in the last x tiles we may not need to plot the point */
			if (colo < rem_x)
			{
				g_row = s_iy_start + rowo;
				j = 0;
				do
				{
					if (diffusion_point(g_row, g_col))
					{
						return -1;
					}
					j++;
					g_row += s; /* next tile */
				}
				while (j < ny);
				if (rowo < rem_y)
				{
					if (diffusion_point(g_row, g_col))
					{
						return -1;
					}
				}
			}
			g_diffusion_counter++;
		}
	}
	else
	{
		/*********************************/
		/* with progressive filling :    */
		while (g_diffusion_counter < (g_diffusion_limit >> 1))
		{
			sqsz = 1 << ((int)(g_bits-(int)(log(g_diffusion_counter + 0.5)/log2 )-1)/2 );
			count_to_int(dif_offset, g_diffusion_counter, &colo, &rowo);

			i = 0;
			do
			{
				j = 0;
				do
				{
					g_col = s_ix_start + colo + i*s; /* get the right tiles */
					g_row = s_iy_start + rowo + j*s;

					if (diffusion_block(g_row, g_col, sqsz))
					{
						return -1;
					}
					j++;
				}
				while (j < ny);
				/* in the last tile we may not need to plot the point */
				if (rowo < rem_y)
				{
					g_row = s_iy_start + rowo + ny*s;
					if (diffusion_block(g_row, g_col, sqsz))
					{
						return -1;
					}
				}
				i++;
			}
			while (i < nx);
			/* in the last tile we may not need to plot the point */
			if (colo < rem_x)
			{
				g_col = s_ix_start + colo + nx*s;
				j = 0;
				do
				{
					g_row = s_iy_start + rowo + j*s; /* get the right tiles */
					if (diffusion_block_lim(g_row, g_col, sqsz))
					{
						return -1;
					}
					j++;
				}
				while (j < ny);
				if (rowo < rem_y)
				{
					g_row = s_iy_start + rowo + ny*s;
					if (diffusion_block_lim(g_row, g_col, sqsz))
					{
						return -1;
					}
				}
			}

			g_diffusion_counter++;
		}
	}
	/* from half g_diffusion_limit on we only plot 1x1 points :-) */
	while (g_diffusion_counter < g_diffusion_limit)
	{
		count_to_int(dif_offset, g_diffusion_counter, &colo, &rowo);

		i = 0;
		do
		{
			j = 0;
			do
			{
				g_col = s_ix_start + colo + i*s; /* get the right tiles */
				g_row = s_iy_start + rowo + j*s;
				if (diffusion_point(g_row, g_col))
				{
					return -1;
				}
				j++;
			}
			while (j < ny);
			/* in the last tile we may not need to plot the point */
			if (rowo < rem_y)
			{
				g_row = s_iy_start + rowo + ny*s;
				if (diffusion_point(g_row, g_col))
				{
					return -1;
				}
			}
			i++;
		}
		while (i < nx);
		/* in the last tile we may nnt need to plot the point */
		if (colo < rem_x)
		{
			g_col = s_ix_start + colo + nx*s;
			j = 0;
			do
			{
				g_row = s_iy_start + rowo + j*s; /* get the right tiles */
				if (diffusion_point(g_row, g_col))
				{
					return -1;
				}
				j++;
			}
			while (j < ny);
			if (rowo < rem_y)
			{
				g_row = s_iy_start + rowo + ny*s;
				if (diffusion_point(g_row, g_col))
				{
					return -1;
				}
			}
		}
		g_diffusion_counter++;
	}

	return 0;
}

static int draw_rectangle_orbits()
{
	/* draw a rectangle */
	g_row = g_yy_begin;
	g_col = g_xx_begin;

	while (g_row <= g_y_stop)
	{
		g_current_row = g_row;
		while (g_col <= g_x_stop)
		{
			if (plotorbits2dfloat() == -1)
			{
				work_list_add(g_xx_start, g_xx_stop, g_col, g_yy_start, g_yy_stop, g_row, 0, s_work_sym);
				return -1; /* interrupted */
			}
			++g_col;
		}
		g_col = s_ix_start;
		++g_row;
	}

	return 0;
}

static int draw_line_orbits(void)
{
	int dX, dY;                     /* vector components */
	int final,                      /* final row or column number */
		G,                  /* used to test for new row or column */
		inc1,           /* G increment when row or column doesn't change */
		inc2;               /* G increment when row or column changes */
	char pos_slope;

	dX = g_x_stop - s_ix_start;                   /* find vector components */
	dY = g_y_stop - s_iy_start;
	pos_slope = (char)(dX > 0);                   /* is slope positive? */
	if (dY < 0)
	{
		pos_slope = (char)!pos_slope;
	}
	if (abs(dX) > abs(dY))                /* shallow line case */
	{
		if (dX > 0)         /* determine start point and last column */
		{
			g_col = g_xx_begin;
			g_row = g_yy_begin;
			final = g_x_stop;
		}
		else
		{
			g_col = g_x_stop;
			g_row = g_y_stop;
			final = g_xx_begin;
		}
		inc1 = 2*abs(dY);            /* determine increments and initial G */
		G = inc1 - abs(dX);
		inc2 = 2*(abs(dY) - abs(dX));
		if (pos_slope)
		{
			while (g_col <= final)    /* step through columns checking for new row */
			{
				if (plotorbits2dfloat() == -1)
				{
					work_list_add(g_xx_start, g_xx_stop, g_col, g_yy_start, g_yy_stop, g_row, 0, s_work_sym);
					return -1; /* interrupted */
				}
				g_col++;
				if (G >= 0)             /* it's time to change rows */
				{
					g_row++;      /* positive slope so increment through the rows */
					G += inc2;
				}
				else                        /* stay at the same row */
				{
					G += inc1;
				}
			}
		}
		else
		{
			while (g_col <= final)    /* step through columns checking for new row */
			{
				if (plotorbits2dfloat() == -1)
				{
					work_list_add(g_xx_start, g_xx_stop, g_col, g_yy_start, g_yy_stop, g_row, 0, s_work_sym);
					return -1; /* interrupted */
				}
				g_col++;
				if (G > 0)              /* it's time to change rows */
				{
					g_row--;      /* negative slope so decrement through the rows */
					G += inc2;
				}
				else                        /* stay at the same row */
				{
					G += inc1;
				}
			}
		}
	}   /* if |dX| > |dY| */
	else                            /* steep line case */
	{
		if (dY > 0)             /* determine start point and last row */
		{
			g_col = g_xx_begin;
			g_row = g_yy_begin;
			final = g_y_stop;
		}
		else
		{
			g_col = g_x_stop;
			g_row = g_y_stop;
			final = g_yy_begin;
		}
		inc1 = 2*abs(dX);            /* determine increments and initial G */
		G = inc1 - abs(dY);
		inc2 = 2*(abs(dX) - abs(dY));
		if (pos_slope)
		{
			while (g_row <= final)    /* step through rows checking for new column */
			{
				if (plotorbits2dfloat() == -1)
				{
					work_list_add(g_xx_start, g_xx_stop, g_col, g_yy_start, g_yy_stop, g_row, 0, s_work_sym);
					return -1; /* interrupted */
				}
				g_row++;
				if (G >= 0)                 /* it's time to change columns */
				{
					g_col++;  /* positive slope so increment through the columns */
					G += inc2;
				}
				else                    /* stay at the same column */
				{
					G += inc1;
				}
			}
		}
		else
		{
			while (g_row <= final)    /* step through rows checking for new column */
			{
				if (plotorbits2dfloat() == -1)
				{
					work_list_add(g_xx_start, g_xx_stop, g_col, g_yy_start, g_yy_stop, g_row, 0, s_work_sym);
					return -1; /* interrupted */
				}
				g_row++;
				if (G > 0)                  /* it's time to change columns */
				{
					g_col--;  /* negative slope so decrement through the columns */
					G += inc2;
				}
				else                    /* stay at the same column */
				{
					G += inc1;
				}
			}
		}
	}
	return 0;
}

/* TODO: this code does not yet work??? */
static int draw_function_orbits(void)
{
	double Xctr, Yctr;
	LDBL Magnification; /* LDBL not really needed here, but used to match function parameters */
	double Xmagfactor, Rotation, Skew;
	int angle;
	double factor = PI / 180.0;
	double theta;
	double xfactor = xdots / 2.0;
	double yfactor = ydots / 2.0;

	angle = g_xx_begin;  /* save angle in x parameter */

	cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
	if (Rotation <= 0)
	{
		Rotation += 360;
	}

	while (angle < Rotation)
	{
		theta = (double)angle*factor;
		g_col = (int)(xfactor + (Xctr + Xmagfactor*cos(theta)));
		g_row = (int)(yfactor + (Yctr + Xmagfactor*sin(theta)));
		if (plotorbits2dfloat() == -1)
		{
			work_list_add(angle, 0, 0, 0, 0, 0, 0, s_work_sym);
			return -1; /* interrupted */
		}
		angle++;
	}
	return 0;
}

static int draw_orbits(void)
{
	g_got_status = GOT_STATUS_ORBITS; /* for <tab> screen */
	g_total_passes = 1;

	if (plotorbits2dsetup() == -1)
	{
		stdcalcmode = 'g';
		return -1;
	}

	switch (g_orbit_draw_mode)
	{
	case ORBITDRAW_RECTANGLE:	return draw_rectangle_orbits();	break;
	case ORBITDRAW_LINE:		return draw_line_orbits();		break;
	case ORBITDRAW_FUNCTION:	return draw_function_orbits();	break;

	default:
		assert(FALSE);
	}

	return 0;
}

static int one_or_two_pass(void)
{
	int i;

	g_total_passes = 1;
	if (stdcalcmode == '2')
	{
		g_total_passes = 2;
	}
	if (stdcalcmode == '2' && s_work_pass == 0) /* do 1st pass of two */
	{
		if (standard_calculate(1) == -1)
		{
			work_list_add(g_xx_start, g_xx_stop, g_col, g_yy_start, g_yy_stop, g_row, 0, s_work_sym);
			return -1;
		}
		if (g_num_work_list > 0) /* g_work_list not empty, defer 2nd pass */
		{
			work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, g_yy_start, 1, s_work_sym);
			return 0;
		}
		s_work_pass = 1;
		g_xx_begin = g_xx_start;
		g_yy_begin = g_yy_start;
	}
	/* second or only pass */
	if (standard_calculate(2) == -1)
	{
		i = g_yy_stop;
		if (g_y_stop != g_yy_stop) /* must be due to symmetry */
		{
			i -= g_row - s_iy_start;
		}
		work_list_add(g_xx_start, g_xx_stop, g_col, g_row, i, g_row, s_work_pass, s_work_sym);
		return -1;
	}

	return 0;
}

static int _fastcall standard_calculate(int passnum)
{
	g_got_status = GOT_STATUS_12PASS;
	g_current_pass = passnum;
	g_row = g_yy_begin;
	g_col = g_xx_begin;

	while (g_row <= g_y_stop)
	{
		g_current_row = g_row;
		g_reset_periodicity = 1;
		while (g_col <= g_x_stop)
		{
			/* on 2nd pass of two, skip even pts */
			if (g_quick_calculate && !g_resuming)
			{
				g_color = getcolor(g_col, g_row);
				if (g_color != inside)
				{
					++g_col;
					continue;
				}
			}
			if (passnum == 1 || stdcalcmode == '1' || (g_row&1) != 0 || (g_col&1) != 0)
			{
				if ((*g_calculate_type)() == -1) /* standard_fractal(), calculate_mandelbrot() or calculate_mandelbrot_fp() */
				{
					return -1; /* interrupted */
				}
				g_resuming = 0; /* reset so g_quick_calculate works */
				g_reset_periodicity = 0;
				if (passnum == 1) /* first pass, copy pixel and bump col */
				{
					if ((g_row&1) == 0 && g_row < g_y_stop)
					{
						(*g_plot_color)(g_col, g_row + 1, g_color);
						if ((g_col&1) == 0 && g_col < g_x_stop)
						{
							(*g_plot_color)(g_col + 1, g_row + 1, g_color);
						}
					}
					if ((g_col&1) == 0 && g_col < g_x_stop)
					{
						(*g_plot_color)(++g_col, g_row, g_color);
					}
				}
			}
			++g_col;
		}
		g_col = s_ix_start;
		if (passnum == 1 && (g_row&1) == 0)
		{
			++g_row;
		}
		++g_row;
	}
	return 0;
}


int calculate_mandelbrot(void)              /* fast per pixel 1/2/b/g, called with row & col set */
{
	/* setup values from array to avoid using es reg in calcmand.asm */
	linitx = lxpixel();
	linity = lypixel();
	if (calcmandasm() >= 0)
	{
		if ((LogTable || Log_Calc) /* map color, but not if maxit & adjusted for inside, etc */
				&& (g_real_color_iter < maxit || (inside < 0 && g_color_iter == maxit)))
			g_color_iter = logtablecalc(g_color_iter);
		g_color = abs((int)g_color_iter);
		if (g_color_iter >= colors)  /* don't use color 0 unless from inside/outside */
		{
			if (save_release <= 1950)
			{
				if (colors < 16)
				{
					g_color &= g_and_color;
				}
				else
				{
					g_color = ((g_color - 1) % g_and_color) + 1;  /* skip color zero */
				}
			}
			else
			{
				g_color = (colors < 16) ?
					(int)(g_color_iter & g_and_color)
					: (int)(((g_color_iter - 1) % g_and_color) + 1);
			}
		}
		if (debugflag != DEBUGFLAG_BNDTRACE_NONZERO)
		{
			if (g_color <= 0 && stdcalcmode == 'b')   /* fix BTM bug */
			{
				g_color = 1;
			}
		}
		(*g_plot_color)(g_col, g_row, g_color);
	}
	else
	{
		g_color = (int)g_color_iter;
	}
	return g_color;
}

/************************************************************************/
/* added by Wes Loewer - sort of a floating point version of calculate_mandelbrot() */
/* can also handle invert, any g_rq_limit, potflag, zmag, epsilon cross,     */
/* and all the current outside options    -Wes Loewer 11/03/91          */
/************************************************************************/
int calculate_mandelbrot_fp(void)
{
	if (g_invert)
	{
		invertz2(&g_initial_z);
	}
	else
	{
		g_initial_z.x = dxpixel();
		g_initial_z.y = dypixel();
	}
	if (g_calculate_mandelbrot_asm_fp() >= 0)
	{
		if (potflag)
		{
			g_color_iter = potential(g_magnitude, g_real_color_iter);
		}
		if ((LogTable || Log_Calc) /* map color, but not if maxit & adjusted for inside, etc */
				&& (g_real_color_iter < maxit || (inside < 0 && g_color_iter == maxit)))
			g_color_iter = logtablecalc(g_color_iter);
		g_color = abs((int)g_color_iter);
		if (g_color_iter >= colors)  /* don't use color 0 unless from inside/outside */
		{
			if (save_release <= 1950)
			{
				if (colors < 16)
				{
					g_color &= g_and_color;
				}
				else
				{
					g_color = ((g_color - 1) % g_and_color) + 1;  /* skip color zero */
				}
			}
			else
			{
				g_color = (colors < 16) ?
					(int)(g_color_iter & g_and_color)
					: (int)(((g_color_iter - 1) % g_and_color) + 1);
			}
		}
		if (debugflag != DEBUGFLAG_BNDTRACE_NONZERO)
		{
			if (g_color == 0 && stdcalcmode == 'b' )   /* fix BTM bug */
			{
				g_color = 1;
			}
		}
		(*g_plot_color)(g_col, g_row, g_color);
	}
	else
	{
		g_color = (int)g_color_iter;
	}
	return g_color;
}
#define STARTRAILMAX FLT_MAX   /* just a convenient large number */
#define green 2
#define yellow 6
#if 0
#define NUMSAVED 40     /* define this to save periodicity analysis to file */
#endif
#if 0
#define MINSAVEDAND 3   /* if not defined, old method used */
#endif
int standard_fractal(void)       /* per pixel 1/2/b/g, called with row & col set */
{
#ifdef NUMSAVED
	_CMPLX savedz[NUMSAVED];
	long caught[NUMSAVED];
	long changed[NUMSAVED];
	int zctr = 0;
#endif
	long savemaxit;
	double tantable[16];
	int hooper = 0;
	long lcloseprox;
	double memvalue = 0.0;
	double min_orbit = 100000.0; /* orbit value closest to origin */
	long   min_index = 0;        /* iteration of min_orbit */
	long cyclelen = -1;
	long savedcoloriter = 0;
	int caught_a_cycle;
	long savedand;
	int savedincr;       /* for periodicity checking */
	_LCMPLX lsaved;
	int i, attracted;
	_LCMPLX lat;
	_CMPLX  at;
	_CMPLX deriv;
	long dem_color = -1;
	_CMPLX dem_new;
	int check_freq;
	double totaldist = 0.0;
	_CMPLX lastz;

	lcloseprox = (long)(g_proximity*fudge);
	savemaxit = maxit;
#ifdef NUMSAVED
	for (i = 0; i < NUMSAVED; i++)
	{
		caught[i] = 0L;
		changed[i] = 0L;
	}
#endif
	if (inside == STARTRAIL)
	{
		int i;
		for (i = 0; i < 16; i++)
		{
			tantable[i] = 0.0;
		}
		if (save_release > 1824)
		{
			maxit = 16;
		}
	}
	if (g_periodicity_check == 0 || inside == ZMAG || inside == STARTRAIL)
	{
		g_old_color_iter = 2147483647L;       /* don't check periodicity at all */
	}
	else if (inside == PERIOD)   /* for display-periodicity */
	{
		g_old_color_iter = (maxit/5)*4;       /* don't check until nearly done */
	}
	else if (g_reset_periodicity)
	{
		g_old_color_iter = 255;               /* don't check periodicity 1st 250 iterations */
	}

	/* Jonathan - how about this idea ? skips first saved value which never works */
#ifdef MINSAVEDAND
	if (g_old_color_iter < MINSAVEDAND)
	{
		g_old_color_iter = MINSAVEDAND;
	}
#else
	if (g_old_color_iter < g_first_saved_and) /* I like it! */
	{
		g_old_color_iter = g_first_saved_and;
	}
#endif
	/* really fractal specific, but we'll leave it here */
	if (!integerfractal)
	{
		if (useinitorbit == 1)
		{
			s_saved_z = initorbit;
		}
		else
		{
			s_saved_z.x = 0;
			s_saved_z.y = 0;
		}
#ifdef NUMSAVED
		savedz[zctr++] = saved;
#endif
		if (bf_math)
		{
			if (decimals > 200)
			{
				g_input_counter = -1;
			}
			if (bf_math == BIGNUM)
			{
				clear_bn(bnsaved.x);
				clear_bn(bnsaved.y);
			}
			else if (bf_math == BIGFLT)
			{
				clear_bf(bfsaved.x);
				clear_bf(bfsaved.y);
			}
		}
		g_initial_z.y = dypixel();
		if (distest)
		{
			if (g_use_old_distance_test)
			{
				g_rq_limit = s_rq_limit_save;
				if (distest != 1 || colors == 2) /* not doing regular outside colors */
					if (g_rq_limit < DEM_BAILOUT)   /* so go straight for dem bailout */
					{
						g_rq_limit = DEM_BAILOUT;
					}
				dem_color = -1;
			}
			deriv.x = 1;
			deriv.y = 0;
			g_magnitude = 0;
		}
	}
	else
	{
		if (useinitorbit == 1)
		{
			lsaved = g_init_orbit_l;
		}
		else
		{
			lsaved.x = 0;
			lsaved.y = 0;
		}
		g_initial_z_l.y = lypixel();
	}
	g_orbit_index = 0;
	g_color_iter = 0;
	if (fractype == JULIAFP || fractype == JULIA)
	{
		g_color_iter = -1;
	}
	caught_a_cycle = 0;
	if (inside == PERIOD)
	{
		savedand = 16;           /* begin checking every 16th cycle */
	}
	else
	{
		/* Jonathan - don't understand such a low savedand -- how about this? */
#ifdef MINSAVEDAND
		savedand = MINSAVEDAND;
#else
		savedand = g_first_saved_and;                /* begin checking every other cycle */
#endif
	}
	savedincr = 1;               /* start checking the very first time */

	if (inside <= BOF60 && inside >= BOF61)
	{
		g_magnitude = g_magnitude_l = 0;
		min_orbit = 100000.0;
	}
	overflow = 0;                /* reset integer math overflow flag */

	curfractalspecific->per_pixel(); /* initialize the calculations */

	attracted = FALSE;

	if (outside == TDIS)
	{
		if (integerfractal)
		{
			g_old_z.x = ((double)g_old_z_l.x) / fudge;
			g_old_z.y = ((double)g_old_z_l.y) / fudge;
		}
		else if (bf_math == BIGNUM)
		{
			g_old_z = complex_bn_to_float(&bnold);
		}
		else if (bf_math == BIGFLT)
		{
			g_old_z = complex_bf_to_float(&bfold);
		}
		lastz.x = g_old_z.x;
		lastz.y = g_old_z.y;
	}

	check_freq = (((soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_X || showdot >= 0) && orbit_delay > 0)
		? 16 : 2048;

	if (g_show_orbit)
	{
		sound_write_time();
	}
	while (++g_color_iter < maxit)
	{
		/* calculation of one orbit goes here */
		/* input in "g_old_z" -- output in "new" */
		if (g_color_iter % check_freq == 0)
		{
			if (check_key())
			{
				return -1;
			}
		}

		if (distest)
		{
			double ftemp;
			/* Distance estimator for points near Mandelbrot set */
			/* Original code by Phil Wilson, hacked around by PB */
			/* Algorithms from Peitgen & Saupe, Science of Fractal Images, p.198 */
			ftemp = s_dem_mandelbrot
				? 2*(g_old_z.x*deriv.x - g_old_z.y*deriv.y) + 1
				: 2*(g_old_z.x*deriv.x - g_old_z.y*deriv.y);
			deriv.y = 2*(g_old_z.y*deriv.x + g_old_z.x*deriv.y);
			deriv.x = ftemp;
			if (g_use_old_distance_test)
			{
				if (sqr(deriv.x) + sqr(deriv.y) > s_dem_too_big)
				{
					break;
				}
			}
			else if (save_release > 1950)
				if (max(fabs(deriv.x), fabs(deriv.y)) > s_dem_too_big)
				{
					break;
				}
			/* if above exit taken, the later test vs s_dem_delta will place this
				point on the boundary, because mag(g_old_z) < bailout just now */

			if (curfractalspecific->orbitcalc() || (overflow && save_release > 1826))
			{
				if (g_use_old_distance_test)
				{
					if (dem_color < 0)
					{
						dem_color = g_color_iter;
						dem_new = g_new_z;
					}
					if (g_rq_limit >= DEM_BAILOUT
						|| g_magnitude >= (g_rq_limit = DEM_BAILOUT)
						|| g_magnitude == 0)
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			g_old_z = g_new_z;
		}

		/* the usual case */
		else if ((curfractalspecific->orbitcalc() && inside != STARTRAIL)
				|| overflow)
			break;
		if (g_show_orbit)
		{
			if (!integerfractal)
			{
				if (bf_math == BIGNUM)
				{
					g_new_z = complex_bn_to_float(&bnnew);
				}
				else if (bf_math == BIGFLT)
				{
					g_new_z = complex_bf_to_float(&bfnew);
				}
				plot_orbit(g_new_z.x, g_new_z.y, -1);
			}
			else
			{
				plot_orbit_i(g_new_z_l.x, g_new_z_l.y, -1);
			}
		}
		if (inside < -1)
		{
			if (bf_math == BIGNUM)
			{
				g_new_z = complex_bn_to_float(&bnnew);
			}
			else if (bf_math == BIGFLT)
			{
				g_new_z = complex_bf_to_float(&bfnew);
			}
			if (inside == STARTRAIL)
			{
				if (0 < g_color_iter && g_color_iter < 16)
				{
					if (integerfractal)
					{
						g_new_z.x = g_new_z_l.x;
						g_new_z.x /= fudge;
						g_new_z.y = g_new_z_l.y;
						g_new_z.y /= fudge;
					}

					if (save_release > 1824)
					{
						if (g_new_z.x > STARTRAILMAX)
						{
							g_new_z.x = STARTRAILMAX;
						}
						if (g_new_z.x < -STARTRAILMAX)
						{
							g_new_z.x = -STARTRAILMAX;
						}
						if (g_new_z.y > STARTRAILMAX)
						{
							g_new_z.y = STARTRAILMAX;
						}
						if (g_new_z.y < -STARTRAILMAX)
						{
							g_new_z.y = -STARTRAILMAX;
						}
						tempsqrx = g_new_z.x*g_new_z.x;
						tempsqry = g_new_z.y*g_new_z.y;
						g_magnitude = tempsqrx + tempsqry;
						g_old_z = g_new_z;
					}
					{
						int tmpcolor;
						tmpcolor = (int)(((g_color_iter - 1) % g_and_color) + 1);
						tantable[tmpcolor-1] = g_new_z.y/(g_new_z.x + .000001);
					}
				}
			}
			else if (inside == EPSCROSS)
			{
				hooper = 0;
				if (integerfractal)
				{
					if (labs(g_new_z_l.x) < labs(lcloseprox))
					{
						hooper = (lcloseprox > 0? 1 : -1); /* close to y axis */
						goto plot_inside;
					}
					else if (labs(g_new_z_l.y) < labs(lcloseprox))
					{
						hooper = (lcloseprox > 0 ? 2: -2); /* close to x axis */
						goto plot_inside;
					}
				}
				else
				{
					if (fabs(g_new_z.x) < fabs(g_proximity))
					{
						hooper = (g_proximity > 0? 1 : -1); /* close to y axis */
						goto plot_inside;
					}
					else if (fabs(g_new_z.y) < fabs(g_proximity))
					{
						hooper = (g_proximity > 0? 2 : -2); /* close to x axis */
						goto plot_inside;
					}
				}
			}
			else if (inside == FMODI)
			{
				double mag;
				if (integerfractal)
				{
					g_new_z.x = ((double)g_new_z_l.x) / fudge;
					g_new_z.y = ((double)g_new_z_l.y) / fudge;
				}
				mag = fmod_test();
				if (mag < g_proximity)
				{
					memvalue = mag;
				}
			}
			else if (inside <= BOF60 && inside >= BOF61)
			{
				if (integerfractal)
				{
					if (g_magnitude_l == 0 || !g_no_magnitude_calculation)
					{
						g_magnitude_l = lsqr(g_new_z_l.x) + lsqr(g_new_z_l.y);
					}
					g_magnitude = g_magnitude_l;
					g_magnitude = g_magnitude / fudge;
				}
				else if (g_magnitude == 0.0 || !g_no_magnitude_calculation)
				{
					g_magnitude = sqr(g_new_z.x) + sqr(g_new_z.y);
				}
				if (g_magnitude < min_orbit)
				{
					min_orbit = g_magnitude;
					min_index = g_color_iter + 1;
				}
			}
		}

		if (outside == TDIS || outside == FMOD)
		{
			if (bf_math == BIGNUM)
			{
				g_new_z = complex_bn_to_float(&bnnew);
			}
			else if (bf_math == BIGFLT)
			{
				g_new_z = complex_bf_to_float(&bfnew);
			}
			if (outside == TDIS)
			{
				if (integerfractal)
				{
					g_new_z.x = ((double)g_new_z_l.x) / fudge;
					g_new_z.y = ((double)g_new_z_l.y) / fudge;
				}
				totaldist += sqrt(sqr(lastz.x-g_new_z.x) + sqr(lastz.y-g_new_z.y));
				lastz.x = g_new_z.x;
				lastz.y = g_new_z.y;
			}
			else if (outside == FMOD)
			{
				double mag;
				if (integerfractal)
				{
					g_new_z.x = ((double)g_new_z_l.x) / fudge;
					g_new_z.y = ((double)g_new_z_l.y) / fudge;
				}
				mag = fmod_test();
				if (mag < g_proximity)
				{
					memvalue = mag;
				}
			}
		}

		if (g_num_attractors > 0)       /* finite attractor in the list   */
		{                         /* NOTE: Integer code is UNTESTED */
			if (integerfractal)
			{
				for (i = 0; i < g_num_attractors; i++)
				{
					lat.x = g_new_z_l.x - g_attractors_l[i].x;
					lat.x = lsqr(lat.x);
					if (lat.x < l_at_rad)
					{
						lat.y = g_new_z_l.y - g_attractors_l[i].y;
						lat.y = lsqr(lat.y);
						if (lat.y < l_at_rad)
						{
							if ((lat.x + lat.y) < l_at_rad)
							{
								attracted = TRUE;
								if (finattract < 0)
								{
									g_color_iter = (g_color_iter % g_attractor_period[i]) + 1;
								}
								break;
							}
						}
					}
				}
			}
			else
			{
				for (i = 0; i < g_num_attractors; i++)
				{
					at.x = g_new_z.x - g_attractors[i].x;
					at.x = sqr(at.x);
					if (at.x < f_at_rad)
					{
						at.y = g_new_z.y - g_attractors[i].y;
						at.y = sqr(at.y);
						if (at.y < f_at_rad)
						{
							if ((at.x + at.y) < f_at_rad)
							{
								attracted = TRUE;
								if (finattract < 0)
								{
									g_color_iter = (g_color_iter % g_attractor_period[i]) + 1;
								}
								break;
							}
						}
					}
				}
			}
			if (attracted)
			{
				break;              /* AHA! Eaten by an attractor */
			}
		}

		if (g_color_iter > g_old_color_iter) /* check periodicity */
		{
			if ((g_color_iter & savedand) == 0)            /* time to save a new value */
			{
				savedcoloriter = g_color_iter;
				if (integerfractal)
				{
					lsaved = g_new_z_l; /* integer fractals */
				}
				else if (bf_math == BIGNUM)
				{
					copy_bn(bnsaved.x, bnnew.x);
					copy_bn(bnsaved.y, bnnew.y);
				}
				else if (bf_math == BIGFLT)
				{
					copy_bf(bfsaved.x, bfnew.x);
					copy_bf(bfsaved.y, bfnew.y);
				}
				else
				{
					s_saved_z = g_new_z;  /* floating pt fractals */
#ifdef NUMSAVED
					if (zctr < NUMSAVED)
					{
						changed[zctr]  = g_color_iter;
						savedz[zctr++] = s_saved_z;
					}
#endif
				}
				if (--savedincr == 0)    /* time to lengthen the periodicity? */
				{
					savedand = (savedand << 1) + 1;       /* longer periodicity */
					savedincr = g_next_saved_incr; /* restart counter */
				}
			}
			else                /* check against an old save */
			{
				if (integerfractal)     /* floating-pt periodicity chk */
				{
					if (labs(lsaved.x - g_new_z_l.x) < g_close_enough_l)
					{
						if (labs(lsaved.y - g_new_z_l.y) < g_close_enough_l)
						{
							caught_a_cycle = 1;
						}
					}
				}
				else if (bf_math == BIGNUM)
				{
					if (cmp_bn(abs_a_bn(sub_bn(bntmp, bnsaved.x, bnnew.x)), bnclosenuff) < 0)
					{
						if (cmp_bn(abs_a_bn(sub_bn(bntmp, bnsaved.y, bnnew.y)), bnclosenuff) < 0)
						{
							caught_a_cycle = 1;
						}
					}
				}
				else if (bf_math == BIGFLT)
				{
					if (cmp_bf(abs_a_bf(sub_bf(bftmp, bfsaved.x, bfnew.x)), bfclosenuff) < 0)
					{
						if (cmp_bf(abs_a_bf(sub_bf(bftmp, bfsaved.y, bfnew.y)), bfclosenuff) < 0)
						{
							caught_a_cycle = 1;
						}
					}
				}
				else
				{
					if (fabs(s_saved_z.x - g_new_z.x) < g_close_enough)
					{
						if (fabs(s_saved_z.y - g_new_z.y) < g_close_enough)
						{
							caught_a_cycle = 1;
						}
					}
#ifdef NUMSAVED
					{
						int i;
						for (i = 0; i <= zctr; i++)
						{
							if (caught[i] == 0)
							{
								if (fabs(savedz[i].x - g_new_z.x) < g_close_enough)
								{
									if (fabs(savedz[i].y - g_new_z.y) < g_close_enough)
									{
										caught[i] = g_color_iter;
									}
								}
							}
						}
					}
#endif
				}
				if (caught_a_cycle)
				{
#ifdef NUMSAVED
					char msg[MSGLEN];
					static FILE *fp = NULL;
					static char c;
					if (fp == NULL)
					{
						fp = dir_fopen(workdir, "cycles.txt", "w");
					}
#endif
					cyclelen = g_color_iter-savedcoloriter;
#ifdef NUMSAVED
					fprintf(fp, "row %3d col %3d len %6ld iter %6ld savedand %6ld\n",
						g_row, g_col, cyclelen, g_color_iter, savedand);
					if (zctr > 1 && zctr < NUMSAVED)
					{
						int i;
						for (i = 0; i < zctr; i++)
						{
							fprintf(fp, "   caught %2d saved %6ld iter %6ld\n", i, changed[i], caught[i]);
						}
					}
					fflush(fp);
#endif
					g_color_iter = maxit - 1;
				}
			}
		}
	}  /* end while (g_color_iter++ < maxit) */

	if (g_show_orbit)
	{
		orbit_scrub();
	}

	g_real_color_iter = g_color_iter;           /* save this before we start adjusting it */
	if (g_color_iter >= maxit)
	{
		g_old_color_iter = 0;         /* check periodicity immediately next time */
	}
	else
	{
		g_old_color_iter = g_color_iter + 10;    /* check when past this + 10 next time */
		if (g_color_iter == 0)
		{
			g_color_iter = 1;         /* needed to make same as calculate_mandelbrot */
		}
	}

	if (potflag)
	{
		if (integerfractal)       /* adjust integer fractals */
		{
			g_new_z.x = ((double)g_new_z_l.x) / fudge;
			g_new_z.y = ((double)g_new_z_l.y) / fudge;
		}
		else if (bf_math == BIGNUM)
		{
			g_new_z.x = (double)bntofloat(bnnew.x);
			g_new_z.y = (double)bntofloat(bnnew.y);
		}
		else if (bf_math == BIGFLT)
		{
			g_new_z.x = (double)bftofloat(bfnew.x);
			g_new_z.y = (double)bftofloat(bfnew.y);
		}
		g_magnitude = sqr(g_new_z.x) + sqr(g_new_z.y);
		g_color_iter = potential(g_magnitude, g_color_iter);
		if (LogTable || Log_Calc)
		{
			g_color_iter = logtablecalc(g_color_iter);
		}
		goto plot_pixel;          /* skip any other adjustments */
	}

	if (g_color_iter >= maxit)              /* an "inside" point */
	{
		goto plot_inside;         /* distest, decomp, biomorph don't apply */
	}


	if (outside < -1)  /* these options by Richard Hughes modified by TW */
	{
		if (integerfractal)
		{
			g_new_z.x = ((double)g_new_z_l.x) / fudge;
			g_new_z.y = ((double)g_new_z_l.y) / fudge;
		}
		else if (bf_math == 1)
		{
			g_new_z.x = (double)bntofloat(bnnew.x);
			g_new_z.y = (double)bntofloat(bnnew.y);
		}
		/* Add 7 to overcome negative values on the MANDEL    */
		if (outside == REAL)               /* "real" */
		{
			g_color_iter += (long)g_new_z.x + 7;
		}
		else if (outside == IMAG)          /* "imag" */
		{
			g_color_iter += (long)g_new_z.y + 7;
		}
		else if (outside == MULT  && g_new_z.y)  /* "mult" */
		{
			g_color_iter = (long)((double)g_color_iter*(g_new_z.x/g_new_z.y));
		}
		else if (outside == SUM)           /* "sum" */
		{
			g_color_iter += (long)(g_new_z.x + g_new_z.y);
		}
		else if (outside == ATAN)          /* "atan" */
		{
			g_color_iter = (long)fabs(atan2(g_new_z.y, g_new_z.x)*g_atan_colors/PI);
		}
		else if (outside == FMOD)
		{
			g_color_iter = (long)(memvalue*colors / g_proximity);
		}
		else if (outside == TDIS)
		{
			g_color_iter = (long)(totaldist);
		}

		/* eliminate negative colors & wrap arounds */
		if ((g_color_iter <= 0 || g_color_iter > maxit) && outside != FMOD)
		{
			g_color_iter = (save_release < 1961) ? 0 : 1;
		}
	}

	if (distest)
	{
		double dist, temp;
		dist = sqr(g_new_z.x) + sqr(g_new_z.y);
		if (dist == 0 || overflow)
		{
			dist = 0;
		}
		else
		{
			temp = log(dist);
			dist = dist*sqr(temp) / (sqr(deriv.x) + sqr(deriv.y) );
		}
		if (dist < s_dem_delta)     /* point is on the edge */
		{
			if (distest > 0)
			{
				goto plot_inside;   /* show it as an inside point */
			}
			g_color_iter = -distest;       /* show boundary as specified color */
			goto plot_pixel;       /* no further adjustments apply */
		}
		if (colors == 2)
		{
			g_color_iter = !inside;   /* the only useful distest 2 color use */
			goto plot_pixel;       /* no further adjustments apply */
		}
		if (distest > 1)          /* pick color based on distance */
		{
			if (g_old_demm_colors) /* this one is needed for old color scheme */
			{
				g_color_iter = (long)sqrt(sqrt(dist) / s_dem_width + 1);
			}
			else if (g_use_old_distance_test)
			{
				g_color_iter = (long)sqrt(dist / s_dem_width + 1);
			}
			else
			{
				g_color_iter = (long)(dist / s_dem_width + 1);
			}
			g_color_iter &= LONG_MAX;  /* oops - color can be negative */
			goto plot_pixel;       /* no further adjustments apply */
		}
		if (g_use_old_distance_test)
		{
			g_color_iter = dem_color;
			g_new_z = dem_new;
		}
		/* use pixel's "regular" color */
	}

	if (decomp[0] > 0)
	{
		decomposition();
	}
	else if (biomorph != -1)
	{
		if (integerfractal)
		{
			if (labs(g_new_z_l.x) < g_limit2_l || labs(g_new_z_l.y) < g_limit2_l)
			{
				g_color_iter = biomorph;
			}
		}
		else if (fabs(g_new_z.x) < g_rq_limit2 || fabs(g_new_z.y) < g_rq_limit2)
		{
			g_color_iter = biomorph;
		}
	}

	if (outside >= 0 && attracted == FALSE) /* merge escape-time stripes */
	{
		g_color_iter = outside;
	}
	else if (LogTable || Log_Calc)
	{
		g_color_iter = logtablecalc(g_color_iter);
	}
	goto plot_pixel;

plot_inside: /* we're "inside" */
	if (g_periodicity_check < 0 && caught_a_cycle)
	{
		g_color_iter = 7;           /* show periodicity */
	}
	else if (inside >= 0)
	{
		g_color_iter = inside;              /* set to specified color, ignore logpal */
	}
	else
	{
		if (inside == STARTRAIL)
		{
			int i;
			double diff;
			g_color_iter = 0;
			for (i = 1; i < 16; i++)
			{
				diff = tantable[0] - tantable[i];
				if (fabs(diff) < .05)
				{
					g_color_iter = i;
					break;
				}
			}
		}
		else if (inside == PERIOD)
		{
			g_color_iter = (cyclelen > 0) ? cyclelen : maxit;
		}
		else if (inside == EPSCROSS)
		{
			if (hooper == 1)
			{
				g_color_iter = green;
			}
			else if (hooper == 2)
			{
				g_color_iter = yellow;
			}
			else if (hooper == 0)
			{
				g_color_iter = maxit;
			}
			if (g_show_orbit)
			{
				orbit_scrub();
			}
		}
		else if (inside == FMODI)
		{
			g_color_iter = (long)(memvalue*colors / g_proximity);
		}
		else if (inside == ATANI)          /* "atan" */
		{
			if (integerfractal)
			{
				g_new_z.x = ((double)g_new_z_l.x) / fudge;
				g_new_z.y = ((double)g_new_z_l.y) / fudge;
				g_color_iter = (long)fabs(atan2(g_new_z.y, g_new_z.x)*g_atan_colors/PI);
			}
			else
			{
				g_color_iter = (long)fabs(atan2(g_new_z.y, g_new_z.x)*g_atan_colors/PI);
			}
		}
		else if (inside == BOF60)
		{
			g_color_iter = (long)(sqrt(min_orbit)*75);
		}
		else if (inside == BOF61)
		{
			g_color_iter = min_index;
		}
		else if (inside == ZMAG)
		{
			g_color_iter = integerfractal ?
				(long)(((double)g_magnitude_l/fudge)*(maxit >> 1) + 1)
				: (long)((sqr(g_new_z.x) + sqr(g_new_z.y))*(maxit >> 1) + 1);
		}
		else /* inside == -1 */
		{
			g_color_iter = maxit;
		}
		if (LogTable || Log_Calc)
		{
			g_color_iter = logtablecalc(g_color_iter);
		}
	}

plot_pixel:
	g_color = abs((int)g_color_iter);
	if (g_color_iter >= colors)  /* don't use color 0 unless from inside/outside */
	{
		if (save_release <= 1950)
		{
			if (colors < 16)
			{
				g_color &= g_and_color;
			}
			else
			{
				g_color = ((g_color - 1) % g_and_color) + 1;  /* skip color zero */
			}
		}
		else
		{
			g_color = (colors < 16) ?
				(int)(g_color_iter & g_and_color)
				: (int)(((g_color_iter - 1) % g_and_color) + 1);
		}
	}
	if (debugflag != DEBUGFLAG_BNDTRACE_NONZERO)
	{
		if (g_color <= 0 && stdcalcmode == 'b' )   /* fix BTM bug */
		{
			g_color = 1;
		}
	}
	(*g_plot_color)(g_col, g_row, g_color);

	maxit = savemaxit;
	g_input_counter -= abs((int)g_real_color_iter);
	if (g_input_counter <= 0)
	{
		if (check_key())
		{
			return -1;
		}
		g_input_counter = g_max_input_counter;
	}
	return g_color;
}
#undef green
#undef yellow

#define cos45  sin45
#define lcos45 lsin45

/**************** standardfractal doodad subroutines *********************/
static void decomposition(void)
{
	/* static double cos45     = 0.70710678118654750; */ /* cos 45  degrees */
	static double sin45     = 0.70710678118654750; /* sin 45     degrees */
	static double cos22_5   = 0.92387953251128670; /* cos 22.5   degrees */
	static double sin22_5   = 0.38268343236508980; /* sin 22.5   degrees */
	static double cos11_25  = 0.98078528040323040; /* cos 11.25  degrees */
	static double sin11_25  = 0.19509032201612820; /* sin 11.25  degrees */
	static double cos5_625  = 0.99518472667219690; /* cos 5.625  degrees */
	static double sin5_625  = 0.09801714032956060; /* sin 5.625  degrees */
	static double tan22_5   = 0.41421356237309500; /* tan 22.5   degrees */
	static double tan11_25  = 0.19891236737965800; /* tan 11.25  degrees */
	static double tan5_625  = 0.09849140335716425; /* tan 5.625  degrees */
	static double tan2_8125 = 0.04912684976946725; /* tan 2.8125 degrees */
	static double tan1_4063 = 0.02454862210892544; /* tan 1.4063 degrees */
	/* static long lcos45     ;*/ /* cos 45   degrees */
	static long lsin45     ; /* sin 45     degrees */
	static long lcos22_5   ; /* cos 22.5   degrees */
	static long lsin22_5   ; /* sin 22.5   degrees */
	static long lcos11_25  ; /* cos 11.25  degrees */
	static long lsin11_25  ; /* sin 11.25  degrees */
	static long lcos5_625  ; /* cos 5.625  degrees */
	static long lsin5_625  ; /* sin 5.625  degrees */
	static long ltan22_5   ; /* tan 22.5   degrees */
	static long ltan11_25  ; /* tan 11.25  degrees */
	static long ltan5_625  ; /* tan 5.625  degrees */
	static long ltan2_8125 ; /* tan 2.8125 degrees */
	static long ltan1_4063 ; /* tan 1.4063 degrees */
	static long reset_fudge = -1;
	int temp = 0;
	int save_temp = 0;
	int i;
	_LCMPLX lalt;
	_CMPLX alt;
	g_color_iter = 0;
	if (integerfractal) /* the only case */
	{
		if (reset_fudge != fudge)
		{
			reset_fudge = fudge;
			/* lcos45     = (long)(cos45*fudge); */
			lsin45     = (long)(sin45*fudge);
			lcos22_5   = (long)(cos22_5*fudge);
			lsin22_5   = (long)(sin22_5*fudge);
			lcos11_25  = (long)(cos11_25*fudge);
			lsin11_25  = (long)(sin11_25*fudge);
			lcos5_625  = (long)(cos5_625*fudge);
			lsin5_625  = (long)(sin5_625*fudge);
			ltan22_5   = (long)(tan22_5*fudge);
			ltan11_25  = (long)(tan11_25*fudge);
			ltan5_625  = (long)(tan5_625*fudge);
			ltan2_8125 = (long)(tan2_8125*fudge);
			ltan1_4063 = (long)(tan1_4063*fudge);
		}
		if (g_new_z_l.y < 0)
		{
			temp = 2;
			g_new_z_l.y = -g_new_z_l.y;
		}

		if (g_new_z_l.x < 0)
		{
			++temp;
			g_new_z_l.x = -g_new_z_l.x;
		}
		if (decomp[0] == 2 && save_release >= 1827)
		{
			save_temp = temp;
			if (temp == 2)
			{
				save_temp = 3;
			}
			else if (temp == 3)
			{
				save_temp = 2;
			}
		}

		if (decomp[0] >= 8)
		{
			temp <<= 1;
			if (g_new_z_l.x < g_new_z_l.y)
			{
				++temp;
				lalt.x = g_new_z_l.x; /* just */
				g_new_z_l.x = g_new_z_l.y; /* swap */
				g_new_z_l.y = lalt.x; /* them */
			}

			if (decomp[0] >= 16)
			{
				temp <<= 1;
				if (multiply(g_new_z_l.x, ltan22_5, bitshift) < g_new_z_l.y)
				{
					++temp;
					lalt = g_new_z_l;
					g_new_z_l.x = multiply(lalt.x, lcos45, bitshift) +
						multiply(lalt.y, lsin45, bitshift);
					g_new_z_l.y = multiply(lalt.x, lsin45, bitshift) -
						multiply(lalt.y, lcos45, bitshift);
				}

				if (decomp[0] >= 32)
				{
					temp <<= 1;
					if (multiply(g_new_z_l.x, ltan11_25, bitshift) < g_new_z_l.y)
					{
						++temp;
						lalt = g_new_z_l;
						g_new_z_l.x = multiply(lalt.x, lcos22_5, bitshift) +
							multiply(lalt.y, lsin22_5, bitshift);
						g_new_z_l.y = multiply(lalt.x, lsin22_5, bitshift) -
							multiply(lalt.y, lcos22_5, bitshift);
					}

					if (decomp[0] >= 64)
					{
						temp <<= 1;
						if (multiply(g_new_z_l.x, ltan5_625, bitshift) < g_new_z_l.y)
						{
							++temp;
							lalt = g_new_z_l;
							g_new_z_l.x = multiply(lalt.x, lcos11_25, bitshift) +
								multiply(lalt.y, lsin11_25, bitshift);
							g_new_z_l.y = multiply(lalt.x, lsin11_25, bitshift) -
								multiply(lalt.y, lcos11_25, bitshift);
						}

						if (decomp[0] >= 128)
						{
							temp <<= 1;
							if (multiply(g_new_z_l.x, ltan2_8125, bitshift) < g_new_z_l.y)
							{
								++temp;
								lalt = g_new_z_l;
								g_new_z_l.x = multiply(lalt.x, lcos5_625, bitshift) +
									multiply(lalt.y, lsin5_625, bitshift);
								g_new_z_l.y = multiply(lalt.x, lsin5_625, bitshift) -
									multiply(lalt.y, lcos5_625, bitshift);
							}

							if (decomp[0] == 256)
							{
								temp <<= 1;
								if (multiply(g_new_z_l.x, ltan1_4063, bitshift) < g_new_z_l.y)
								{
									if ((g_new_z_l.x*ltan1_4063 < g_new_z_l.y))
									{
										++temp;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	else /* double case */
	{
		if (g_new_z.y < 0)
		{
			temp = 2;
			g_new_z.y = -g_new_z.y;
		}
		if (g_new_z.x < 0)
		{
			++temp;
			g_new_z.x = -g_new_z.x;
		}
		if (decomp[0] == 2 && save_release >= 1827)
		{
			save_temp = temp;
			if (temp == 2)
			{
				save_temp = 3;
			}
			else if (temp == 3)
			{
				save_temp = 2;
			}
		}
		if (decomp[0] >= 8)
		{
			temp <<= 1;
			if (g_new_z.x < g_new_z.y)
			{
				++temp;
				alt.x = g_new_z.x; /* just */
				g_new_z.x = g_new_z.y; /* swap */
				g_new_z.y = alt.x; /* them */
			}
			if (decomp[0] >= 16)
			{
				temp <<= 1;
				if (g_new_z.x*tan22_5 < g_new_z.y)
				{
					++temp;
					alt = g_new_z;
					g_new_z.x = alt.x*cos45 + alt.y*sin45;
					g_new_z.y = alt.x*sin45 - alt.y*cos45;
				}

				if (decomp[0] >= 32)
				{
					temp <<= 1;
					if (g_new_z.x*tan11_25 < g_new_z.y)
					{
						++temp;
						alt = g_new_z;
						g_new_z.x = alt.x*cos22_5 + alt.y*sin22_5;
						g_new_z.y = alt.x*sin22_5 - alt.y*cos22_5;
					}

					if (decomp[0] >= 64)
					{
						temp <<= 1;
						if (g_new_z.x*tan5_625 < g_new_z.y)
						{
							++temp;
							alt = g_new_z;
							g_new_z.x = alt.x*cos11_25 + alt.y*sin11_25;
							g_new_z.y = alt.x*sin11_25 - alt.y*cos11_25;
						}

						if (decomp[0] >= 128)
						{
							temp <<= 1;
							if (g_new_z.x*tan2_8125 < g_new_z.y)
							{
								++temp;
								alt = g_new_z;
								g_new_z.x = alt.x*cos5_625 + alt.y*sin5_625;
								g_new_z.y = alt.x*sin5_625 - alt.y*cos5_625;
							}

							if (decomp[0] == 256)
							{
								temp <<= 1;
								if ((g_new_z.x*tan1_4063 < g_new_z.y))
								{
									++temp;
								}
							}
						}
					}
				}
			}
		}
	}
	for (i = 1; temp > 0; ++i)
	{
		if (temp & 1)
		{
			g_color_iter = (1 << i) - 1 - g_color_iter;
		}
		temp >>= 1;
	}
	if (decomp[0] == 2 && save_release >= 1827)
	{
		g_color_iter = (save_temp & 2) ? 1 : 0;
		if (colors == 2)
		{
			g_color_iter++;
		}
	}
	else if (decomp[0] == 2 && save_release < 1827)
	{
		g_color_iter &= 1;
	}
	if (colors > decomp[0])
	{
		g_color_iter++;
	}
}

/******************************************************************/
/* Continuous potential calculation for Mandelbrot and Julia      */
/* Reference: Science of Fractal Images p. 190.                   */
/* Special thanks to Mark Peterson for his "MtMand" program that  */
/* beautifully approximates plate 25 (same reference) and spurred */
/* on the inclusion of similar capabilities in FRACTINT.          */
/*                                                                */
/* The purpose of this function is to calculate a color value     */
/* for a fractal that varies continuously with the screen pixels  */
/* locations for better rendering in 3D.                          */
/*                                                                */
/* Here "magnitude" is the modulus of the orbit value at          */
/* "iterations". The potparms[] are user-entered paramters        */
/* controlling the level and slope of the continuous potential    */
/* surface. Returns color.  - Tim Wegner 6/25/89                  */
/*                                                                */
/*                     -- Change history --                       */
/*                                                                */
/* 09/12/89   - added floatflag support and fixed float underflow */
/*                                                                */
/******************************************************************/

static int _fastcall potential(double mag, long iterations)
{
	float f_mag, f_tmp, pot;
	double d_tmp;
	int i_pot;
	long l_pot;

	if (iterations < maxit)
	{
		l_pot = iterations + 2;
		pot = (float) l_pot;
		if (l_pot <= 0 || mag <= 1.0)
		{
			pot = 0.0f;
		}
		else
		{
			 /* pot = log(mag) / pow(2.0, (double)pot); */
			if (l_pot < 120 && !floatflag) /* empirically determined limit of fShift */
			{
				f_mag = (float)mag;
				fLog14(f_mag, f_tmp); /* this SHOULD be non-negative */
				fShift(f_tmp, (char)-l_pot, pot);
			}
			else
			{
				d_tmp = log(mag)/(double)pow(2.0, (double)pot);
				/* prevent float type underflow */
				pot = (d_tmp > FLT_MIN) ? (float) d_tmp : 0.0f;
			}
		}
		/* following transformation strictly for aesthetic reasons */
		/* meaning of parameters:
				potparam[0] -- zero potential level - highest color -
				potparam[1] -- slope multiplier -- higher is steeper
				potparam[2] -- g_rq_limit value if changeable (bailout for modulus) */

		if (pot > 0.0)
		{
			if (floatflag)
			{
				pot = (float)sqrt((double)pot);
			}
			else
			{
				fSqrt14(pot, f_tmp);
				pot = f_tmp;
			}
			pot = (float)(potparam[0] - pot*potparam[1] - 1.0);
		}
		else
		{
			pot = (float)(potparam[0] - 1.0);
		}
		if (pot < 1.0)
		{
			pot = 1.0f; /* avoid color 0 */
		}
	}
	else if (inside >= 0)
	{
		pot = (float) inside;
	}
	else /* inside < 0 implies inside = maxit, so use 1st pot param instead */
	{
		pot = (float)potparam[0];
	}

	l_pot = (long) pot*256;
	i_pot = (int) (l_pot >> 8);
	if (i_pot >= colors)
	{
		i_pot = colors - 1;
		l_pot = 255;
	}

	if (pot16bit)
	{
		if (!driver_diskp()) /* if g_put_color won't be doing it for us */
		{
			writedisk(g_col + sxoffs, g_row + syoffs, i_pot);
		}
		writedisk(g_col + sxoffs, g_row + sydots + syoffs, (int)l_pot);
	}

	return i_pot;
}


/******************* boundary trace method ***************************
Fractint's original btm was written by David Guenther.  There were a few
rare circumstances in which the original btm would not trace or fill
correctly, even on Mandelbrot Sets.  The code below was adapted from
"Mandelbrot Sets by Wesley Loewer" (see calmanfp.asm) which was written
before I was introduced to Fractint.  It should be noted that without
David Guenther's implimentation of a btm, I doubt that I would have been
able to impliment my own code into Fractint.  There are several things in
the following code that are not original with me but came from David
Guenther's code.  I've noted these places with the initials DG.

                                        Wesley Loewer 3/8/92
*********************************************************************/
#define bkcolor 0  /* I have some ideas for the future with this. -Wes */
#define advance_match()     coming_from = ((s_going_to = (s_going_to - 1) & 0x03) - 1) & 0x03
#define advance_no_match()  s_going_to = (s_going_to + 1) & 0x03

static int boundary_trace_main(void)
{
	enum direction coming_from;
	unsigned int match_found, continue_loop;
	int trail_color, fillcolor_used, last_fillcolor_used = -1;
	int max_putline_length;
	int right, left, length;
	if (inside == 0 || outside == 0)
	{
		stopmsg(0, "Boundary tracing cannot be used with inside=0 or outside=0");
		return -1;
	}
	if (colors < 16)
	{
		stopmsg(0, "Boundary tracing cannot be used with < 16 colors");
		return -1;
	}

	g_got_status = GOT_STATUS_BOUNDARY_TRACE;
	max_putline_length = 0; /* reset max_putline_length */
	for (g_current_row = s_iy_start; g_current_row <= g_y_stop; g_current_row++)
	{
		g_reset_periodicity = 1; /* reset for a new row */
		g_color = bkcolor;
		for (g_current_col = s_ix_start; g_current_col <= g_x_stop; g_current_col++)
		{
			if (getcolor(g_current_col, g_current_row) != bkcolor)
			{
				continue;
			}
			trail_color = g_color;
			g_row = g_current_row;
			g_col = g_current_col;
			if ((*g_calculate_type)() == -1) /* color, row, col are global */
			{
				if (showdot != bkcolor) /* remove showdot pixel */
				{
					(*g_plot_color)(g_col, g_row, bkcolor);
				}
				if (g_y_stop != g_yy_stop)  /* DG */
				{
					g_y_stop = g_yy_stop - (g_current_row - g_yy_start); /* allow for sym */
				}
				work_list_add(g_xx_start, g_xx_stop, g_current_col, g_current_row, g_y_stop, g_current_row, 0, s_work_sym);
				return -1;
			}
			g_reset_periodicity = 0; /* normal periodicity checking */

			/*
			This next line may cause a few more pixels to be calculated,
			but at the savings of quite a bit of overhead
			*/
			if (g_color != trail_color)  /* DG */
			{
				continue;
			}

			/* sweep clockwise to trace outline */
			s_trail_row = g_current_row;
			s_trail_col = g_current_col;
			trail_color = g_color;
			fillcolor_used = fillcolor > 0 ? fillcolor : trail_color;
			coming_from = West;
			s_going_to = East;
			match_found = 0;
			continue_loop = TRUE;
			do
			{
				step_col_row();
				if (g_row >= g_current_row
					&& g_col >= s_ix_start
					&& g_col <= g_x_stop
					&& g_row <= g_y_stop)
				{
					/* the order of operations in this next line is critical */
					g_color = getcolor(g_col, g_row);
					if (g_color == bkcolor && (*g_calculate_type)() == -1)
								/* color, row, col are global for (*g_calculate_type)() */
					{
						if (showdot != bkcolor) /* remove showdot pixel */
						{
							(*g_plot_color)(g_col, g_row, bkcolor);
						}
						if (g_y_stop != g_yy_stop)  /* DG */
						{
							g_y_stop = g_yy_stop - (g_current_row - g_yy_start); /* allow for sym */
						}
						work_list_add(g_xx_start, g_xx_stop, g_current_col, g_current_row, g_y_stop, g_current_row, 0, s_work_sym);
						return -1;
					}
					else if (g_color == trail_color)
					{
						if (match_found < 4) /* to keep it from overflowing */
						{
							match_found++;
						}
						s_trail_row = g_row;
						s_trail_col = g_col;
						advance_match();
					}
					else
					{
						advance_no_match();
						continue_loop = s_going_to != coming_from || match_found;
					}
				}
				else
				{
					advance_no_match();
					continue_loop = s_going_to != coming_from || match_found;
				}
			}
			while (continue_loop && (g_col != g_current_col || g_row != g_current_row));

			if (match_found <= 3)  /* DG */
			{
				/* no hole */
				g_color = bkcolor;
				g_reset_periodicity = 1;
				continue;
			}

			/*
			Fill in region by looping around again, filling lines to the left
			whenever s_going_to is South or West
			*/
			s_trail_row = g_current_row;
			s_trail_col = g_current_col;
			coming_from = West;
			s_going_to = East;
			do
			{
				match_found = FALSE;
				do
				{
					step_col_row();
					if (g_row >= g_current_row
						&& g_col >= s_ix_start
						&& g_col <= g_x_stop
						&& g_row <= g_y_stop
						&& getcolor(g_col, g_row) == trail_color)
						/* getcolor() must be last */
					{
						if (s_going_to == South
							|| (s_going_to == West && coming_from != East))
						{ /* fill a row, but only once */
							right = g_col;
							while (--right >= s_ix_start)
							{
								g_color = getcolor(right, g_row);
								if (g_color != trail_color)
								{
									break;
								}
							}
							if (g_color == bkcolor) /* check last color */
							{
								left = right;
								while (getcolor(--left, g_row) == bkcolor)
									/* Should NOT be possible for left < s_ix_start */
								{
									/* do nothing */
								}
								left++; /* one pixel too far */
								if (right == left) /* only one hole */
								{
									(*g_plot_color)(left, g_row, fillcolor_used);
								}
								else
								{ /* fill the line to the left */
									length = right-left + 1;
									if (fillcolor_used != last_fillcolor_used || length > max_putline_length)
									{ /* only reset g_stack if necessary */
										memset(g_stack, fillcolor_used, length);
										last_fillcolor_used = fillcolor_used;
										max_putline_length = length;
									}
									sym_fill_line(g_row, left, right, g_stack);
								}
							} /* end of fill line */

#if 0 /* don't interupt with a check_key() during fill */
							if (--g_input_counter <= 0)
							{
								if (check_key())
								{
									if (g_y_stop != g_yy_stop)
									{
										g_y_stop = g_yy_stop - (g_current_row - g_yy_start); /* allow for sym */
									}
									work_list_add(g_xx_start, g_xx_stop, g_current_col, g_current_row, g_y_stop, g_current_row, 0, s_work_sym);
									return -1;
								}
								g_input_counter = g_max_input_counter;
							}
#endif
						}
						s_trail_row = g_row;
						s_trail_col = g_col;
						advance_match();
						match_found = TRUE;
					}
					else
					{
						advance_no_match();
					}
				}
				while (!match_found && s_going_to != coming_from);

				if (!match_found)
				{ /* next one has to be a match */
					step_col_row();
					s_trail_row = g_row;
					s_trail_col = g_col;
					advance_match();
				}
			}
			while (s_trail_col != g_current_col || s_trail_row != g_current_row);
			g_reset_periodicity = 1; /* reset after a trace/fill */
			g_color = bkcolor;
		}
	}

	return 0;
}

/*******************************************************************/
/* take one step in the direction of s_going_to */
static void step_col_row()
{
	switch (s_going_to)
	{
	case North:
		g_col = s_trail_col;
		g_row = s_trail_row - 1;
		break;
	case East:
		g_col = s_trail_col + 1;
		g_row = s_trail_row;
		break;
	case South:
		g_col = s_trail_col;
		g_row = s_trail_row + 1;
		break;
		case West:
			g_col = s_trail_col - 1;
			g_row = s_trail_row;
			break;
	}
}

/******************* end of boundary trace method *******************/


/************************ super solid guessing *****************************/

/*
	I, Timothy Wegner, invented this solid_guessing idea and implemented it in
	more or less the overall framework you see here.  I am adding this note
	now in a possibly vain attempt to secure my place in history, because
	Pieter Branderhorst has totally rewritten this routine, incorporating
	a *MUCH* more sophisticated algorithm.  His revised code is not only
	faster, but is also more accurate. Harrumph!
*/

static int solid_guess(void)
{
	int i, x, y, xlim, ylim, blocksize;
	unsigned int *pfxp0, *pfxp1;
	unsigned int u;

	s_guess_plot = (g_plot_color != g_put_color && g_plot_color != symplot2 && g_plot_color != symplot2J);
	/* check if guessing at bottom & right edges is ok */
	s_bottom_guess = (g_plot_color == symplot2 || (g_plot_color == g_put_color && g_y_stop + 1 == ydots));
	s_right_guess  = (g_plot_color == symplot2J
		|| ((g_plot_color == g_put_color || g_plot_color == symplot2) && g_x_stop + 1 == xdots));

	/* there seems to be a bug in solid guessing at bottom and side */
	if (debugflag != DEBUGFLAG_SOLID_GUESS_BR)
	{
		s_bottom_guess = s_right_guess = FALSE;  /* TIW march 1995 */
	}

	i = s_max_block = blocksize = solid_guess_block_size();
	g_total_passes = 1;
	while ((i >>= 1) > 1)
	{
		++g_total_passes;
	}

	/* ensure window top and left are on required boundary, treat window
			as larger than it really is if necessary (this is the reason symplot
			routines must check for > xdots/ydots before plotting sym points) */
	s_ix_start &= -1 - (s_max_block-1);
	s_iy_start = g_yy_begin;
	s_iy_start &= -1 - (s_max_block-1);

	g_got_status = GOT_STATUS_GUESSING;

	if (s_work_pass == 0) /* otherwise first pass already done */
	{
		/* first pass, calc every blocksize**2 pixel, quarter result & paint it */
		g_current_pass = 1;
		if (s_iy_start <= g_yy_start) /* first time for this window, init it */
		{
			g_current_row = 0;
			memset(&s_t_prefix[1][0][0], 0, MAX_X_BLOCK*MAX_Y_BLOCK*2); /* noskip flags off */
			g_reset_periodicity = 1;
			g_row = s_iy_start;
			for (g_col = s_ix_start; g_col <= g_x_stop; g_col += s_max_block)
			{ /* calc top row */
				if ((*g_calculate_type)() == -1)
				{
					work_list_add(g_xx_start, g_xx_stop, g_xx_begin, g_yy_start, g_yy_stop, g_yy_begin, 0, s_work_sym);
					goto exit_solid_guess;
				}
				g_reset_periodicity = 0;
			}
		}
		else
		{
			memset(&s_t_prefix[1][0][0], -1, MAX_X_BLOCK*MAX_Y_BLOCK*2); /* noskip flags on */
		}
		for (y = s_iy_start; y <= g_y_stop; y += blocksize)
		{
			g_current_row = y;
			i = 0;
			if (y + blocksize <= g_y_stop)
			{ /* calc the row below */
				g_row = y + blocksize;
				g_reset_periodicity = 1;
				for (g_col = s_ix_start; g_col <= g_x_stop; g_col += s_max_block)
				{
					i = (*g_calculate_type)();
					if (i == -1)
					{
						break;
					}
					g_reset_periodicity = 0;
				}
			}
			g_reset_periodicity = 0;
			if (i == -1 || guess_row(1, y, blocksize) != 0) /* interrupted? */
			{
				if (y < g_yy_start)
				{
					y = g_yy_start;
				}
				work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, y, 0, s_work_sym);
				goto exit_solid_guess;
			}
		}

		if (g_num_work_list) /* work list not empty, just do 1st pass */
		{
			work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, g_yy_start, 1, s_work_sym);
			goto exit_solid_guess;
		}
		++s_work_pass;
		s_iy_start = g_yy_start & (-1 - (s_max_block-1));

		/* calculate skip flags for skippable blocks */
		xlim = (g_x_stop + s_max_block)/s_max_block + 1;
		ylim = ((g_y_stop + s_max_block)/s_max_block + 15)/16 + 1;
		if (!s_right_guess) /* no right edge guessing, zap border */
		{
			for (y = 0; y <= ylim; ++y)
			{
				s_t_prefix[1][y][xlim] = 0xffff;
			}
		}
		if (!s_bottom_guess) /* no bottom edge guessing, zap border */
		{
			i = (g_y_stop + s_max_block)/s_max_block + 1;
			y = i/16 + 1;
			i = 1 << (i&15);
			for (x = 0; x <= xlim; ++x)
			{
				s_t_prefix[1][y][x] |= i;
			}
		}
		/* set each bit in s_t_prefix[0] to OR of it & surrounding 8 in s_t_prefix[1] */
		for (y = 0; ++y < ylim; )
		{
			pfxp0 = (unsigned int *)&s_t_prefix[0][y][0];
			pfxp1 = (unsigned int *)&s_t_prefix[1][y][0];
			for (x = 0; ++x < xlim; )
			{
				++pfxp1;
				u = *(pfxp1-1)|*pfxp1|*(pfxp1 + 1);
				*(++pfxp0) = u | (u >> 1) | (u << 1)
					| ((*(pfxp1-(MAX_X_BLOCK + 1))
					| *(pfxp1-MAX_X_BLOCK)
					| *(pfxp1-(MAX_X_BLOCK-1))) >> 15)
					| ((*(pfxp1 + (MAX_X_BLOCK-1))
					| *(pfxp1 + MAX_X_BLOCK)
					| *(pfxp1 + (MAX_X_BLOCK + 1))) << 15);
			}
		}
	}
	else /* first pass already done */
	{
		memset(&s_t_prefix[0][0][0], -1, MAX_X_BLOCK*MAX_Y_BLOCK*2); /* noskip flags on */
	}
	if (g_three_pass)
	{
		goto exit_solid_guess;
	}

	/* remaining pass(es), halve blocksize & quarter each blocksize**2 */
	i = s_work_pass;
	while (--i > 0) /* allow for already done passes */
	{
		blocksize = blocksize >> 1;
	}
	g_reset_periodicity = 0;
	while ((blocksize = blocksize >> 1) >= 2)
	{
		if (stoppass > 0)
		{
			if (s_work_pass >= stoppass)
			{
				goto exit_solid_guess;
			}
		}
		g_current_pass = s_work_pass + 1;
		for (y = s_iy_start; y <= g_y_stop; y += blocksize)
		{
			g_current_row = y;
			if (guess_row(0, y, blocksize) != 0)
			{
				if (y < g_yy_start)
				{
					y = g_yy_start;
				}
				work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, y, s_work_pass, s_work_sym);
				goto exit_solid_guess;
			}
		}
		++s_work_pass;
		if (g_num_work_list /* work list not empty, do one pass at a time */
			&& blocksize > 2) /* if 2, we just did last pass */
		{
			work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, g_yy_start, s_work_pass, s_work_sym);
			goto exit_solid_guess;
		}
		s_iy_start = g_yy_start & (-1 - (s_max_block-1));
	}

	exit_solid_guess:
	return 0;
}

#define calcadot(c, x, y) \
	{ \
		g_col = x; \
		g_row = y; \
		c = (*g_calculate_type)(); \
		if (c == -1) \
		{ \
			return -1; \
		} \
	}

static int _fastcall guess_row(int firstpass, int y, int blocksize)
{
	int x, i, j, color;
	int xplushalf, xplusblock;
	int ylessblock, ylesshalf, yplushalf, yplusblock;
	int     c21, c31, c41;         /* cxy is the color of pixel at (x, y) */
	int c12, c22, c32, c42;         /* where c22 is the topleft corner of */
	int c13, c23, c33;             /* the block being handled in current */
	int     c24,    c44;         /* iteration                          */
	int guessed23, guessed32, guessed33, guessed12, guessed13;
	int prev11, fix21, fix31;
	unsigned int *pfxptr, pfxmask;

	c44 = c41 = c42 = 0;  /* just for warning */

	s_half_block = blocksize >> 1;
	i = y/s_max_block;
	pfxptr = (unsigned int *)&s_t_prefix[firstpass][(i >> 4) + 1][s_ix_start/s_max_block];
	pfxmask = 1 << (i&15);
	ylesshalf = y-s_half_block;
	ylessblock = y-blocksize; /* constants, for speed */
	yplushalf = y + s_half_block;
	yplusblock = y + blocksize;
	prev11 = -1;
	c24 = c12 = c13 = c22 = getcolor(s_ix_start, y);
	c31 = c21 = getcolor(s_ix_start, (y > 0) ? ylesshalf : 0);
	if (yplusblock <= g_y_stop)
	{
		c24 = getcolor(s_ix_start, yplusblock);
	}
	else if (!s_bottom_guess)
	{
		c24 = -1;
	}
	guessed12 = guessed13 = 0;

	for (x = s_ix_start; x <= g_x_stop; )  /* increment at end, or when doing continue */
	{
		if ((x&(s_max_block-1)) == 0)  /* time for skip flag stuff */
		{
			++pfxptr;
			if (firstpass == 0 && (*pfxptr&pfxmask) == 0)  /* check for fast skip */
			{
				/* next useful in testing to make skips visible */
				/*
				if (s_half_block == 1)
				{
					(*g_plot_color)(x + 1, y, 0); (*g_plot_color)(x, y + 1, 0); (*g_plot_color)(x + 1, y + 1, 0);
				}
				*/
				x += s_max_block;
				prev11 = c31 = c21 = c24 = c12 = c13 = c22;
				guessed12 = guessed13 = 0;
				continue;
			}
		}

		if (firstpass)  /* 1st pass, paint topleft corner */
		{
			plot_block(0, x, y, c22);
		}
		/* setup variables */
		xplushalf = x + s_half_block;
		xplusblock = xplushalf + s_half_block;
		if (xplushalf > g_x_stop)
		{
			if (!s_right_guess)
			{
				c31 = -1;
			}
		}
		else if (y > 0)
		{
			c31 = getcolor(xplushalf, ylesshalf);
		}
		if (xplusblock <= g_x_stop)
		{
			if (yplusblock <= g_y_stop)
			{
				c44 = getcolor(xplusblock, yplusblock);
			}
			c41 = getcolor(xplusblock, (y > 0) ? ylesshalf : 0);
			c42 = getcolor(xplusblock, y);
		}
		else if (!s_right_guess)
		{
			c41 = c42 = c44 = -1;
		}
		if (yplusblock > g_y_stop)
		{
			c44 = s_bottom_guess ? c42 : -1;
		}

		/* guess or calc the remaining 3 quarters of current block */
		guessed23 = guessed32 = guessed33 = 1;
		c23 = c32 = c33 = c22;
		if (yplushalf > g_y_stop)
		{
			if (!s_bottom_guess)
			{
				c23 = c33 = -1;
			}
			guessed23 = guessed33 = -1;
			guessed13 = 0; /* fix for ydots not divisible by four bug TW 2/16/97 */
		}
		if (xplushalf > g_x_stop)
		{
			if (!s_right_guess)
			{
				c32 = c33 = -1;
			}
			guessed32 = guessed33 = -1;
		}
		while (1) /* go around till none of 23, 32, 33 change anymore */
		{
			if (guessed33 > 0
				&& (c33 != c44 || c33 != c42 || c33 != c24 || c33 != c32 || c33 != c23))
			{
				calcadot(c33, xplushalf, yplushalf);
				guessed33 = 0;
			}
			if (guessed32 > 0
				&& (c32 != c33 || c32 != c42 || c32 != c31 || c32 != c21
					|| c32 != c41 || c32 != c23))
			{
				calcadot(c32, xplushalf, y);
				guessed32 = 0;
				continue;
			}
			if (guessed23 > 0
				&& (c23 != c33 || c23 != c24 || c23 != c13 || c23 != c12 || c23 != c32))
			{
				calcadot(c23, x, yplushalf);
				guessed23 = 0;
				continue;
			}
			break;
		}

		if (firstpass) /* note whether any of block's contents were calculated */
			if (guessed23 == 0 || guessed32 == 0 || guessed33 == 0)
			{
				*pfxptr |= pfxmask;
			}

		if (s_half_block > 1)  /* not last pass, check if something to display */
		{
			if (firstpass)  /* display guessed corners, fill in block */
			{
				if (s_guess_plot)
				{
					if (guessed23 > 0)
					{
						(*g_plot_color)(x, yplushalf, c23);
					}
					if (guessed32 > 0)
					{
						(*g_plot_color)(xplushalf, y, c32);
					}
					if (guessed33 > 0)
					{
						(*g_plot_color)(xplushalf, yplushalf, c33);
					}
				}
				plot_block(1, x, yplushalf, c23);
				plot_block(0, xplushalf, y, c32);
				plot_block(1, xplushalf, yplushalf, c33);
			}
			else  /* repaint changed blocks */
			{
				if (c23 != c22)
				{
					plot_block(-1, x, yplushalf, c23);
				}
				if (c32 != c22)
				{
					plot_block(-1, xplushalf, y, c32);
				}
				if (c33 != c22)
				{
					plot_block(-1, xplushalf, yplushalf, c33);
				}
			}
		}

		/* check if some calcs in this block mean earlier guesses need fixing */
		fix21 = ((c22 != c12 || c22 != c32)
			&& c21 == c22 && c21 == c31 && c21 == prev11
			&& y > 0
			&& (x == s_ix_start || c21 == getcolor(x-s_half_block, ylessblock))
			&& (xplushalf > g_x_stop || c21 == getcolor(xplushalf, ylessblock))
			&& c21 == getcolor(x, ylessblock));
		fix31 = (c22 != c32
			&& c31 == c22 && c31 == c42 && c31 == c21 && c31 == c41
			&& y > 0 && xplushalf <= g_x_stop
			&& c31 == getcolor(xplushalf, ylessblock)
			&& (xplusblock > g_x_stop || c31 == getcolor(xplusblock, ylessblock))
			&& c31 == getcolor(x, ylessblock));
		prev11 = c31; /* for next time around */
		if (fix21)
		{
			calcadot(c21, x, ylesshalf);
			if (s_half_block > 1 && c21 != c22)
			{
				plot_block(-1, x, ylesshalf, c21);
			}
		}
		if (fix31)
		{
			calcadot(c31, xplushalf, ylesshalf);
			if (s_half_block > 1 && c31 != c22)
			{
				plot_block(-1, xplushalf, ylesshalf, c31);
			}
		}
		if (c23 != c22)
		{
			if (guessed12)
			{
				calcadot(c12, x-s_half_block, y);
				if (s_half_block > 1 && c12 != c22)
				{
					plot_block(-1, x-s_half_block, y, c12);
				}
			}
			if (guessed13)
			{
				calcadot(c13, x-s_half_block, yplushalf);
				if (s_half_block > 1 && c13 != c22)
				{
					plot_block(-1, x-s_half_block, yplushalf, c13);
				}
			}
		}
		c22 = c42;
		c24 = c44;
		c13 = c33;
		c31 = c21 = c41;
		c12 = c32;
		guessed12 = guessed32;
		guessed13 = guessed33;
		x += blocksize;
	} /* end x loop */

	if (firstpass == 0 || s_guess_plot)
	{
		return 0;
	}

	/* paint rows the fast way */
	for (i = 0; i < s_half_block; ++i)
	{
		j = y + i;
		if (j <= g_y_stop)
		{
			put_line(j, g_xx_start, g_x_stop, &g_stack[g_xx_start]);
		}
		j = y + i + s_half_block;
		if (j <= g_y_stop)
		{
			put_line(j, g_xx_start, g_x_stop, &g_stack[g_xx_start + OLDMAXPIXELS]);
		}
		if (driver_key_pressed())
		{
			return -1;
		}
	}
	if (g_plot_color != g_put_color)  /* symmetry, just vertical & origin the fast way */
	{
		if (g_plot_color == symplot2J) /* origin sym, reverse lines */
		{
			for (i = (g_x_stop + g_xx_start + 1)/2; --i >= g_xx_start; )
			{
				color = g_stack[i];
				j = g_x_stop-(i-g_xx_start);
				g_stack[i] = g_stack[j];
				g_stack[j] = (BYTE)color;
				j += OLDMAXPIXELS;
				color = g_stack[i + OLDMAXPIXELS];
				g_stack[i + OLDMAXPIXELS] = g_stack[j];
				g_stack[j] = (BYTE)color;
			}
		}
		for (i = 0; i < s_half_block; ++i)
		{
			j = g_yy_stop-(y + i-g_yy_start);
			if (j > g_y_stop && j < ydots)
			{
				put_line(j, g_xx_start, g_x_stop, &g_stack[g_xx_start]);
			}
			j = g_yy_stop-(y + i + s_half_block-g_yy_start);
			if (j > g_y_stop && j < ydots)
			{
				put_line(j, g_xx_start, g_x_stop, &g_stack[g_xx_start + OLDMAXPIXELS]);
			}
			if (driver_key_pressed())
			{
				return -1;
			}
		}
	}
	return 0;
}

static void _fastcall plot_block(int buildrow, int x, int y, int color)
{
	int i, xlim, ylim;
	xlim = x + s_half_block;
	if (xlim > g_x_stop)
	{
		xlim = g_x_stop + 1;
	}
	if (buildrow >= 0 && !s_guess_plot) /* save it for later put_line */
	{
		if (buildrow == 0)
		{
			for (i = x; i < xlim; ++i)
			{
				g_stack[i] = (BYTE)color;
			}
		}
		else
		{
			for (i = x; i < xlim; ++i)
			{
				g_stack[i + OLDMAXPIXELS] = (BYTE)color;
			}
		}
		if (x >= g_xx_start) /* when x reduced for alignment, paint those dots too */
		{
			return; /* the usual case */
		}
	}
	/* paint it */
	ylim = y + s_half_block;
	if (ylim > g_y_stop)
	{
		if (y > g_y_stop)
		{
			return;
		}
		ylim = g_y_stop + 1;
	}
	for (i = x; ++i < xlim; )
	{
		(*g_plot_color)(i, y, color); /* skip 1st dot on 1st row */
	}
	while (++y < ylim)
	{
		for (i = x; i < xlim; ++i)
		{
			(*g_plot_color)(i, y, color);
		}
	}
}


/************************* symmetry plot setup ************************/

static int _fastcall x_symmetry_split(int xaxis_row, int xaxis_between)
{
	int i;
	if ((s_work_sym&0x11) == 0x10) /* already decided not sym */
	{
		return 1;
	}
	if ((s_work_sym&1) != 0) /* already decided on sym */
	{
		g_y_stop = (g_yy_start + g_yy_stop)/2;
	}
	else /* new window, decide */
	{
		s_work_sym |= 0x10;
		if (xaxis_row <= g_yy_start || xaxis_row >= g_yy_stop)
		{
			return 1; /* axis not in window */
		}
		i = xaxis_row + (xaxis_row - g_yy_start);
		if (xaxis_between)
		{
			++i;
		}
		if (i > g_yy_stop) /* split into 2 pieces, bottom has the symmetry */
		{
			if (g_num_work_list >= MAXCALCWORK-1) /* no room to split */
			{
				return 1;
			}
			g_y_stop = xaxis_row - (g_yy_stop - xaxis_row);
			if (!xaxis_between)
			{
				--g_y_stop;
			}
			work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_y_stop + 1, g_yy_stop, g_y_stop + 1, s_work_pass, 0);
			g_yy_stop = g_y_stop;
			return 1; /* tell set_symmetry no sym for current window */
		}
		if (i < g_yy_stop) /* split into 2 pieces, top has the symmetry */
		{
			if (g_num_work_list >= MAXCALCWORK-1) /* no room to split */
			{
				return 1;
			}
			work_list_add(g_xx_start, g_xx_stop, g_xx_start, i + 1, g_yy_stop, i + 1, s_work_pass, 0);
			g_yy_stop = i;
		}
		g_y_stop = xaxis_row;
		s_work_sym |= 1;
	}
	g_symmetry = 0;
	return 0; /* tell set_symmetry its a go */
}

static int _fastcall y_symmetry_split(int yaxis_col, int yaxis_between)
{
	int i;
	if ((s_work_sym&0x22) == 0x20) /* already decided not sym */
	{
		return 1;
	}
	if ((s_work_sym&2) != 0) /* already decided on sym */
	{
		g_x_stop = (g_xx_start + g_xx_stop)/2;
	}
	else /* new window, decide */
	{
		s_work_sym |= 0x20;
		if (yaxis_col <= g_xx_start || yaxis_col >= g_xx_stop)
		{
			return 1; /* axis not in window */
		}
		i = yaxis_col + (yaxis_col - g_xx_start);
		if (yaxis_between)
		{
			++i;
		}
		if (i > g_xx_stop) /* split into 2 pieces, right has the symmetry */
		{
			if (g_num_work_list >= MAXCALCWORK-1) /* no room to split */
			{
				return 1;
			}
			g_x_stop = yaxis_col - (g_xx_stop - yaxis_col);
			if (!yaxis_between)
			{
				--g_x_stop;
			}
			work_list_add(g_x_stop + 1, g_xx_stop, g_x_stop + 1, g_yy_start, g_yy_stop, g_yy_start, s_work_pass, 0);
			g_xx_stop = g_x_stop;
			return 1; /* tell set_symmetry no sym for current window */
		}
		if (i < g_xx_stop) /* split into 2 pieces, left has the symmetry */
		{
			if (g_num_work_list >= MAXCALCWORK-1) /* no room to split */
			{
				return 1;
			}
			work_list_add(i + 1, g_xx_stop, i + 1, g_yy_start, g_yy_stop, g_yy_start, s_work_pass, 0);
			g_xx_stop = i;
		}
		g_x_stop = yaxis_col;
		s_work_sym |= 2;
	}
	g_symmetry = 0;
	return 0; /* tell set_symmetry its a go */
}

#ifdef _MSC_VER
#pragma optimize ("ea", off)
#endif

static void _fastcall setsymmetry(int sym, int uselist) /* set up proper symmetrical plot functions */
{
	int i;
	int parmszero, parmsnoreal, parmsnoimag;
	int xaxis_row, yaxis_col;         /* pixel number for origin */
	int xaxis_between = 0, yaxis_between = 0; /* if axis between 2 pixels, not on one */
	int xaxis_on_screen = 0, yaxis_on_screen = 0;
	double ftemp;
	bf_t bft1;
	int saved = 0;
	g_symmetry = 1;
	if (stdcalcmode == 's' || stdcalcmode == 'o')
	{
		return;
	}
	if (sym == NOPLOT && forcesymmetry == 999)
	{
		g_plot_color = noplot;
		return;
	}
	/* NOTE: 16-bit potential disables symmetry */
	/* also any decomp= option and any inversion not about the origin */
	/* also any rotation other than 180deg and any off-axis stretch */
	if (bf_math)
	{
		if (cmp_bf(bfxmin, bfx3rd) || cmp_bf(bfymin, bfy3rd))
		{
			return;
		}
	}
	if ((potflag && pot16bit) || (g_invert && inversion[2] != 0.0)
			|| decomp[0] != 0
			|| xxmin != xx3rd || yymin != yy3rd)
	{
		return;
	}
	if (sym != XAXIS && sym != XAXIS_NOPARM && inversion[1] != 0.0 && forcesymmetry == 999)
	{
		return;
	}
	if (forcesymmetry < 999)
	{
		sym = forcesymmetry;
	}
	else if (forcesymmetry == 1000)
	{
		forcesymmetry = sym;  /* for backwards compatibility */
	}
	else if (outside == REAL || outside == IMAG || outside == MULT || outside == SUM
			|| outside == ATAN || g_bail_out_test == Manr || outside == FMOD)
		return;
	else if (inside == FMODI || outside == TDIS)
	{
		return;
	}
	parmszero = (parm.x == 0.0 && parm.y == 0.0 && useinitorbit != 1);
	parmsnoreal = (parm.x == 0.0 && useinitorbit != 1);
	parmsnoimag = (parm.y == 0.0 && useinitorbit != 1);
	switch (fractype)
	{
	case LMANLAMFNFN:      /* These need only P1 checked. */
	case FPMANLAMFNFN:     /* P2 is used for a switch value */
	case LMANFNFN:         /* These have NOPARM set in fractalp.c, */
	case FPMANFNFN:        /* but it only applies to P1. */
	case FPMANDELZPOWER:   /* or P2 is an exponent */
	case LMANDELZPOWER:
	case FPMANZTOZPLUSZPWR:
	case MARKSMANDEL:
	case MARKSMANDELFP:
	case MARKSJULIA:
	case MARKSJULIAFP:
		break;
	case FORMULA:  /* Check P2, P3, P4 and P5 */
	case FFORMULA:
		parmszero = (parmszero && param[2] == 0.0 && param[3] == 0.0
						&& param[4] == 0.0 && param[5] == 0.0
						&& param[6] == 0.0 && param[7] == 0.0
						&& param[8] == 0.0 && param[9] == 0.0);
		parmsnoreal = (parmsnoreal && param[2] == 0.0 && param[4] == 0.0
						&& param[6] == 0.0 && param[8] == 0.0);
		parmsnoimag = (parmsnoimag && param[3] == 0.0 && param[5] == 0.0
						&& param[7] == 0.0 && param[9] == 0.0);
		break;
	default:   /* Check P2 for the rest */
		parmszero = (parmszero && parm2.x == 0.0 && parm2.y == 0.0);
	}
	xaxis_row = yaxis_col = -1;
	if (bf_math)
	{
		saved = save_stack();
		bft1    = alloc_stack(rbflength + 2);
		xaxis_on_screen = (sign_bf(bfymin) != sign_bf(bfymax));
		yaxis_on_screen = (sign_bf(bfxmin) != sign_bf(bfxmax));
	}
	else
	{
		xaxis_on_screen = (sign(yymin) != sign(yymax));
		yaxis_on_screen = (sign(xxmin) != sign(xxmax));
	}
	if (xaxis_on_screen) /* axis is on screen */
	{
		if (bf_math)
		{
			/* ftemp = -yymax / (yymin-yymax); */
			sub_bf(bft1, bfymin, bfymax);
			div_bf(bft1, bfymax, bft1);
			neg_a_bf(bft1);
			ftemp = (double)bftofloat(bft1);
		}
		else
		{
			ftemp = -yymax / (yymin-yymax);
		}
		ftemp *= (ydots-1);
		ftemp += 0.25;
		xaxis_row = (int)ftemp;
		xaxis_between = (ftemp - xaxis_row >= 0.5);
		if (uselist == 0 && (!xaxis_between || (xaxis_row + 1)*2 != ydots))
		{
			xaxis_row = -1; /* can't split screen, so dead center or not at all */
		}
	}
	if (yaxis_on_screen) /* axis is on screen */
	{
		if (bf_math)
		{
			/* ftemp = -xxmin / (xxmax-xxmin); */
			sub_bf(bft1, bfxmax, bfxmin);
			div_bf(bft1, bfxmin, bft1);
			neg_a_bf(bft1);
			ftemp = (double)bftofloat(bft1);
		}
		else
		{
			ftemp = -xxmin / (xxmax-xxmin);
		}
		ftemp *= (xdots-1);
		ftemp += 0.25;
		yaxis_col = (int)ftemp;
		yaxis_between = (ftemp - yaxis_col >= 0.5);
		if (uselist == 0 && (!yaxis_between || (yaxis_col + 1)*2 != xdots))
		{
			yaxis_col = -1; /* can't split screen, so dead center or not at all */
		}
	}
	switch (sym)       /* symmetry switch */
	{
	case XAXIS_NOREAL:    /* X-axis Symmetry (no real param) */
		if (!parmsnoreal)
		{
			break;
		}
		goto xsym;
	case XAXIS_NOIMAG:    /* X-axis Symmetry (no imag param) */
		if (!parmsnoimag)
		{
			break;
		}
		goto xsym;
	case XAXIS_NOPARM:                        /* X-axis Symmetry  (no params)*/
		if (!parmszero)
		{
			break;
		}
		xsym:
	case XAXIS:                       /* X-axis Symmetry */
		if (x_symmetry_split(xaxis_row, xaxis_between) == 0)
		{
			g_plot_color = basin ? symplot2basin : symplot2;
		}
		break;
	case YAXIS_NOPARM:                        /* Y-axis Symmetry (No Parms)*/
		if (!parmszero)
		{
			break;
		}
	case YAXIS:                       /* Y-axis Symmetry */
		if (y_symmetry_split(yaxis_col, yaxis_between) == 0)
		{
			g_plot_color = symplot2Y;
		}
		break;
	case XYAXIS_NOPARM:                       /* X-axis AND Y-axis Symmetry (no parms)*/
		if (!parmszero)
		{
			break;
		}
	case XYAXIS:                      /* X-axis AND Y-axis Symmetry */
		x_symmetry_split(xaxis_row, xaxis_between);
		y_symmetry_split(yaxis_col, yaxis_between);
		switch (s_work_sym & 3)
		{
		case XAXIS: /* just xaxis symmetry */
			g_plot_color = basin ? symplot2basin : symplot2;
			break;
		case YAXIS: /* just yaxis symmetry */
			if (basin) /* got no routine for this case */
			{
				g_x_stop = g_xx_stop; /* fix what split should not have done */
				g_symmetry = 1;
			}
			else
			{
				g_plot_color = symplot2Y;
			}
			break;
		case XYAXIS: /* both axes */
			g_plot_color = basin ? symplot4basin : symplot4;
		}
		break;
	case ORIGIN_NOPARM:                       /* Origin Symmetry (no parms)*/
		if (!parmszero)
		{
			break;
		}
	case ORIGIN:                      /* Origin Symmetry */
		originsym:
		if (x_symmetry_split(xaxis_row, xaxis_between) == 0
			&& y_symmetry_split(yaxis_col, yaxis_between) == 0)
		{
			g_plot_color = symplot2J;
			g_x_stop = g_xx_stop; /* didn't want this changed */
		}
		else
		{
			g_y_stop = g_yy_stop; /* in case first split worked */
			g_symmetry = 1;
			s_work_sym = 0x30; /* let it recombine with others like it */
		}
		break;
	case PI_SYM_NOPARM:
		if (!parmszero)
		{
			break;
		}
	case PI_SYM:                      /* PI symmetry */
		if (bf_math)
		{
			if ((double)bftofloat(abs_a_bf(sub_bf(bft1, bfxmax, bfxmin))) < PI/4)
			{
				break; /* no point in pi symmetry if values too close */
			}
		}
		else
		{
			if (fabs(xxmax - xxmin) < PI/4)
			{
				break; /* no point in pi symmetry if values too close */
			}
		}
		if (g_invert && forcesymmetry == 999)
		{
			goto originsym;
		}
		g_plot_color = symPIplot;
		g_symmetry = 0;
		if (x_symmetry_split(xaxis_row, xaxis_between) == 0
				&& y_symmetry_split(yaxis_col, yaxis_between) == 0)
		{
			/* both axes or origin*/
			g_plot_color = (parm.y == 0.0) ? symPIplot4J : symPIplot2J; 
		}
		else
		{
			g_y_stop = g_yy_stop; /* in case first split worked */
			s_work_sym = 0x30;  /* don't mark pisym as ysym, just do it unmarked */
		}
		if (bf_math)
		{
			sub_bf(bft1, bfxmax, bfxmin);
			abs_a_bf(bft1);
			s_pixel_pi = (int)((PI/(double)bftofloat(bft1)*xdots)); /* PI in pixels */
		}
		else
		{
			s_pixel_pi = (int)((PI/fabs(xxmax-xxmin))*xdots); /* PI in pixels */
		}

		g_x_stop = g_xx_start + s_pixel_pi-1;
		if (g_x_stop > g_xx_stop)
		{
			g_x_stop = g_xx_stop;
		}
		if (g_plot_color == symPIplot4J)
		{
			i = (g_xx_start + g_xx_stop)/2;
			if (g_x_stop > i)
			{
				g_x_stop = i;
			}
		}
		break;
	default:                  /* no symmetry */
		break;
	}
	if (bf_math)
	{
		restore_stack(saved);
	}
}

#ifdef _MSC_VER
#pragma optimize ("ea", on)
#endif

/**************** tesseral method by CJLT begins here*********************/
/*  reworked by PB for speed and resumeability */

struct tess  /* one of these per box to be done gets stacked */
{
	int x1, x2, y1, y2;      /* left/right top/bottom x/y coords  */
	int top, bot, lft, rgt;  /* edge colors, -1 mixed, -2 unknown */
};

static int tesseral(void)
{
	struct tess *tp;

	s_guess_plot = (g_plot_color != g_put_color && g_plot_color != symplot2);
	tp = (struct tess *)&g_stack[0];
	tp->x1 = s_ix_start;                              /* set up initial box */
	tp->x2 = g_x_stop;
	tp->y1 = s_iy_start;
	tp->y2 = g_y_stop;

	if (s_work_pass == 0)  /* not resuming */
	{
		tp->top = tesseral_row(s_ix_start, g_x_stop, s_iy_start);     /* Do top row */
		tp->bot = tesseral_row(s_ix_start, g_x_stop, g_y_stop);      /* Do bottom row */
		tp->lft = tesseral_column(s_ix_start, s_iy_start + 1, g_y_stop-1); /* Do left column */
		tp->rgt = tesseral_column(g_x_stop, s_iy_start + 1, g_y_stop-1);  /* Do right column */
		if (check_key())  /* interrupt before we got properly rolling */
		{
			work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, g_yy_start, 0, s_work_sym);
			return -1;
		}
	}
	else  /* resuming, rebuild work stack */
	{
		int i, mid, curx, cury, xsize, ysize;
		struct tess *tp2;
		tp->top = tp->bot = tp->lft = tp->rgt = -2;
		cury = g_yy_begin & 0xfff;
		ysize = 1;
		i = (unsigned)g_yy_begin >> 12;
		while (--i >= 0)
		{
			ysize <<= 1;
		}
		curx = s_work_pass & 0xfff;
		xsize = 1;
		i = (unsigned)s_work_pass >> 12;
		while (--i >= 0)
		{
			xsize <<= 1;
		}
		while (1)
		{
			tp2 = tp;
			if (tp->x2 - tp->x1 > tp->y2 - tp->y1)  /* next divide down middle */
			{
				if (tp->x1 == curx && (tp->x2 - tp->x1 - 2) < xsize)
				{
					break;
				}
				mid = (tp->x1 + tp->x2) >> 1;                /* Find mid point */
				if (mid > curx)  /* stack right part */
				{
					memcpy(++tp, tp2, sizeof(*tp));
					tp->x2 = mid;
				}
				tp2->x1 = mid;
			}
			else  /* next divide across */
			{
				if (tp->y1 == cury && (tp->y2 - tp->y1 - 2) < ysize)
				{
					break;
				}
				mid = (tp->y1 + tp->y2) >> 1;                /* Find mid point */
				if (mid > cury)  /* stack bottom part */
				{
					memcpy(++tp, tp2, sizeof(*tp));
					tp->y2 = mid;
				}
				tp2->y1 = mid;
			}
		}
	}

	g_got_status = GOT_STATUS_TESSERAL; /* for tab_display */

	while (tp >= (struct tess *)&g_stack[0])  /* do next box */
	{
		g_current_col = tp->x1; /* for tab_display */
		g_current_row = tp->y1;

		if (tp->top == -1 || tp->bot == -1 || tp->lft == -1 || tp->rgt == -1)
		{
			goto tess_split;
		}
		/* for any edge whose color is unknown, set it */
		if (tp->top == -2)
		{
			tp->top = tesseral_check_row(tp->x1, tp->x2, tp->y1);
		}
		if (tp->top == -1)
		{
			goto tess_split;
		}
		if (tp->bot == -2)
		{
			tp->bot = tesseral_check_row(tp->x1, tp->x2, tp->y2);
		}
		if (tp->bot != tp->top)
		{
			goto tess_split;
		}
		if (tp->lft == -2)
		{
			tp->lft = tesseral_check_column(tp->x1, tp->y1, tp->y2);
		}
		if (tp->lft != tp->top)
		{
			goto tess_split;
		}
		if (tp->rgt == -2)
		{
			tp->rgt = tesseral_check_column(tp->x2, tp->y1, tp->y2);
		}
		if (tp->rgt != tp->top)
		{
			goto tess_split;
		}

		{
			int mid, midcolor;
			if (tp->x2 - tp->x1 > tp->y2 - tp->y1)  /* divide down the middle */
			{
				mid = (tp->x1 + tp->x2) >> 1;           /* Find mid point */
				midcolor = tesseral_column(mid, tp->y1 + 1, tp->y2-1); /* Do mid column */
				if (midcolor != tp->top)
				{
					goto tess_split;
				}
			}
			else  /* divide across the middle */
			{
				mid = (tp->y1 + tp->y2) >> 1;           /* Find mid point */
				midcolor = tesseral_row(tp->x1 + 1, tp->x2-1, mid); /* Do mid row */
				if (midcolor != tp->top)
				{
					goto tess_split;
				}
			}
		}

		{  /* all 4 edges are the same color, fill in */
			int i, j;
			i = 0;
			if (fillcolor != 0)
			{
				if (fillcolor > 0)
				{
					tp->top = fillcolor % colors;
				}
				if (s_guess_plot || (j = tp->x2 - tp->x1 - 1) < 2)  /* paint dots */
				{
					for (g_col = tp->x1 + 1; g_col < tp->x2; g_col++)
					{
						for (g_row = tp->y1 + 1; g_row < tp->y2; g_row++)
						{
							(*g_plot_color)(g_col, g_row, tp->top);
							if (++i > 500)
							{
								if (check_key())
								{
									goto tess_end;
								}
								i = 0;
							}
						}
					}
				}
				else  /* use put_line for speed */
				{
					memset(&g_stack[OLDMAXPIXELS], tp->top, j);
					for (g_row = tp->y1 + 1; g_row < tp->y2; g_row++)
					{
						put_line(g_row, tp->x1 + 1, tp->x2-1, &g_stack[OLDMAXPIXELS]);
						if (g_plot_color != g_put_color) /* symmetry */
						{
							j = g_yy_stop-(g_row-g_yy_start);
							if (j > g_y_stop && j < ydots)
							{
								put_line(j, tp->x1 + 1, tp->x2-1, &g_stack[OLDMAXPIXELS]);
							}
						}
						if (++i > 25)
						{
							if (check_key())
							{
								goto tess_end;
							}
							i = 0;
						}
					}
				}
			}
			--tp;
		}
		continue;

tess_split:
		{  /* box not surrounded by same color, sub-divide */
			int mid, midcolor;
			struct tess *tp2;
			if (tp->x2 - tp->x1 > tp->y2 - tp->y1)  /* divide down the middle */
			{
				mid = (tp->x1 + tp->x2) >> 1;                /* Find mid point */
				midcolor = tesseral_column(mid, tp->y1 + 1, tp->y2-1); /* Do mid column */
				if (midcolor == -3)
				{
					goto tess_end;
				}
				if (tp->x2 - mid > 1)  /* right part >= 1 column */
				{
					if (tp->top == -1)
					{
						tp->top = -2;
					}
					if (tp->bot == -1)
					{
						tp->bot = -2;
					}
					tp2 = tp;
					if (mid - tp->x1 > 1)  /* left part >= 1 col, stack right */
					{
						memcpy(++tp, tp2, sizeof(*tp));
						tp->x2 = mid;
						tp->rgt = midcolor;
					}
					tp2->x1 = mid;
					tp2->lft = midcolor;
				}
				else
				{
					--tp;
				}
			}
			else  /* divide across the middle */
			{
				mid = (tp->y1 + tp->y2) >> 1;                /* Find mid point */
				midcolor = tesseral_row(tp->x1 + 1, tp->x2-1, mid); /* Do mid row */
				if (midcolor == -3)
				{
					goto tess_end;
				}
				if (tp->y2 - mid > 1)  /* bottom part >= 1 column */
				{
					if (tp->lft == -1)
					{
						tp->lft = -2;
					}
					if (tp->rgt == -1)
					{
						tp->rgt = -2;
					}
					tp2 = tp;
					if (mid - tp->y1 > 1)  /* top also >= 1 col, stack bottom */
					{
						memcpy(++tp, tp2, sizeof(*tp));
						tp->y2 = mid;
						tp->bot = midcolor;
					}
					tp2->y1 = mid;
					tp2->top = midcolor;
				}
				else
				{
					--tp;
				}
			}
		}
	}

tess_end:
	if (tp >= (struct tess *)&g_stack[0])  /* didn't complete */
	{
		int i, xsize, ysize;
		xsize = ysize = 1;
		i = 2;
		while (tp->x2 - tp->x1 - 2 >= i)
		{
			i <<= 1;
			++xsize;
		}
		i = 2;
		while (tp->y2 - tp->y1 - 2 >= i)
		{
			i <<= 1;
			++ysize;
		}
		work_list_add(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop,
			(ysize << 12) + tp->y1, (xsize << 12) + tp->x1, s_work_sym);
		return -1;
	}
	return 0;
} /* tesseral */

static int _fastcall tesseral_check_column(int x, int y1, int y2)
{
	int i;
	i = getcolor(x, ++y1);
	while (--y2 > y1)
		if (getcolor(x, y2) != i)
		{
			return -1;
		}
	return i;
}

static int _fastcall tesseral_check_row(int x1, int x2, int y)
{
	int i;
	i = getcolor(x1, y);
	while (x2 > x1)
	{
		if (getcolor(x2, y) != i)
		{
			return -1;
		}
		--x2;
	}
	return i;
}

static int _fastcall tesseral_column(int x, int y1, int y2)
{
	int colcolor, i;
	g_col = x;
	g_row = y1;
	g_reset_periodicity = 1;
	colcolor = (*g_calculate_type)();
	g_reset_periodicity = 0;
	while (++g_row <= y2)  /* generate the column */
	{
		i = (*g_calculate_type)();
		if (i < 0)
		{
			return -3;
		}
		if (i != colcolor)
		{
			colcolor = -1;
		}
	}
	return colcolor;
}

static int _fastcall tesseral_row(int x1, int x2, int y)
{
	int rowcolor, i;
	g_row = y;
	g_col = x1;
	g_reset_periodicity = 1;
	rowcolor = (*g_calculate_type)();
	g_reset_periodicity = 0;
	while (++g_col <= x2)  /* generate the row */
	{
		i = (*g_calculate_type)();
		if (i < 0)
		{
			return -3;
		}
		if (i != rowcolor)
		{
			rowcolor = -1;
		}
	}
	return rowcolor;
}

/* added for testing automatic_log_map() */ /* CAE 9211 fixed missing comment */
/* insert at end of CALCFRAC.C */

static long automatic_log_map(void)   /*RB*/
{  /* calculate round screen edges to avoid wasted colours in logmap */
	long mincolour;
	int lag;
	int xstop = xdots - 1; /* don't use symetry */
	int ystop = ydots - 1; /* don't use symetry */
	long old_maxit;
	mincolour = LONG_MAX;
	g_row = 0;
	g_reset_periodicity = 0;
	old_maxit = maxit;
	for (g_col = 0; g_col < xstop; g_col++) /* top row */
	{
		g_color = (*g_calculate_type)();
		if (g_color == -1)
		{
			goto ack; /* key pressed, bailout */
		}
		if (g_real_color_iter < mincolour)
		{
			mincolour = g_real_color_iter;
			maxit = max(2, mincolour); /*speedup for when edges overlap lakes */
		}
		if (g_col >= 32)
		{
			(*g_plot_color)(g_col-32, g_row, 0);
		}
	}                                    /* these lines tidy up for BTM etc */
	for (lag = 32; lag > 0; lag--)
	{
		(*g_plot_color)(g_col-lag, g_row, 0);
	}

	g_col = xstop;
	for (g_row = 0; g_row < ystop; g_row++) /* right  side */
	{
		g_color = (*g_calculate_type)();
		if (g_color == -1)
		{
			goto ack; /* key pressed, bailout */
		}
		if (g_real_color_iter < mincolour)
		{
			mincolour = g_real_color_iter;
			maxit = max(2, mincolour); /*speedup for when edges overlap lakes */
		}
		if (g_row >= 32)
		{
			(*g_plot_color)(g_col, g_row-32, 0);
		}
	}
	for (lag = 32; lag > 0; lag--)
	{
		(*g_plot_color)(g_col, g_row-lag, 0);
	}

	g_col = 0;
	for (g_row = 0; g_row < ystop; g_row++) /* left  side */
	{
		g_color = (*g_calculate_type)();
		if (g_color == -1)
		{
			goto ack; /* key pressed, bailout */
		}
		if (g_real_color_iter < mincolour)
		{
			mincolour = g_real_color_iter;
			maxit = max(2, mincolour); /*speedup for when edges overlap lakes */
		}
		if (g_row >= 32)
		{
			(*g_plot_color)(g_col, g_row-32, 0);
		}
	}
	for (lag = 32; lag > 0; lag--)
	{
		(*g_plot_color)(g_col, g_row-lag, 0);
	}

	g_row = ystop;
	for (g_col = 0; g_col < xstop; g_col++) /* bottom row */
	{
		g_color = (*g_calculate_type)();
		if (g_color == -1)
		{
			goto ack; /* key pressed, bailout */
		}
		if (g_real_color_iter < mincolour)
		{
			mincolour = g_real_color_iter;
			maxit = max(2, mincolour); /*speedup for when edges overlap lakes */
		}
		if (g_col >= 32)
		{
			(*g_plot_color)(g_col-32, g_row, 0);
		}
	}
	for (lag = 32; lag > 0; lag--)
	{
		(*g_plot_color)(g_col-lag, g_row, 0);
	}

ack: /* bailout here if key is pressed */
	if (mincolour == 2)
	{
		g_resuming = 1; /* insure automatic_log_map not called again */
	}
	maxit = old_maxit;

	return mincolour;
}

/* Symmetry plot for period PI */
void _fastcall symPIplot(int x, int y, int color)
{
	while (x <= g_xx_stop)
	{
		g_put_color(x, y, color);
		x += s_pixel_pi;
	}
}
/* Symmetry plot for period PI plus Origin Symmetry */
void _fastcall symPIplot2J(int x, int y, int color)
{
	int i, j;
	while (x <= g_xx_stop)
	{
		g_put_color(x, y, color);
		if ((i = g_yy_stop-(y-g_yy_start)) > g_y_stop && i < ydots
				&& (j = g_xx_stop-(x-g_xx_start)) < xdots)
			g_put_color(j, i, color);
		x += s_pixel_pi;
	}
}
/* Symmetry plot for period PI plus Both Axis Symmetry */
void _fastcall symPIplot4J(int x, int y, int color)
{
	int i, j;
	while (x <= (g_xx_start + g_xx_stop)/2)
	{
		j = g_xx_stop-(x-g_xx_start);
		g_put_color(x , y , color);
		if (j < xdots)
		{
			g_put_color(j , y , color);
		}
		i = g_yy_stop-(y-g_yy_start);
		if (i > g_y_stop && i < ydots)
		{
			g_put_color(x , i , color);
			if (j < xdots)
			{
				g_put_color(j , i , color);
			}
		}
		x += s_pixel_pi;
	}
}

/* Symmetry plot for X Axis Symmetry */
void _fastcall symplot2(int x, int y, int color)
{
	int i;
	g_put_color(x, y, color);
	i = g_yy_stop-(y-g_yy_start);
	if (i > g_y_stop && i < ydots)
	{
		g_put_color(x, i, color);
	}
}

/* Symmetry plot for Y Axis Symmetry */
void _fastcall symplot2Y(int x, int y, int color)
{
	int i;
	g_put_color(x, y, color);
	i = g_xx_stop-(x-g_xx_start);
	if (i < xdots)
	{
		g_put_color(i, y, color);
	}
}

/* Symmetry plot for Origin Symmetry */
void _fastcall symplot2J(int x, int y, int color)
{
	int i, j;
	g_put_color(x, y, color);
	if ((i = g_yy_stop-(y-g_yy_start)) > g_y_stop && i < ydots
		&& (j = g_xx_stop-(x-g_xx_start)) < xdots)
	{
		g_put_color(j, i, color);
	}
}

/* Symmetry plot for Both Axis Symmetry */
void _fastcall symplot4(int x, int y, int color)
{
	int i, j;
	j = g_xx_stop-(x-g_xx_start);
	g_put_color(x , y, color);
	if (j < xdots)
	{
		g_put_color(j , y, color);
	}
	i = g_yy_stop-(y-g_yy_start);
	if (i > g_y_stop && i < ydots)
	{
		g_put_color(x , i, color);
		if (j < xdots)
		{
			g_put_color(j , i, color);
		}
	}
}

/* Symmetry plot for X Axis Symmetry - Striped Newtbasin version */
void _fastcall symplot2basin(int x, int y, int color)
{
	int i, stripe;
	g_put_color(x, y, color);
	stripe = (basin == 2 && color > 8) ? 8 : 0;
	i = g_yy_stop-(y-g_yy_start);
	if (i > g_y_stop && i < ydots)
	{
		color -= stripe;                    /* reconstruct unstriped color */
		color = (g_degree + 1-color) % g_degree + 1;  /* symmetrical color */
		color += stripe;                    /* add stripe */
		g_put_color(x, i, color) ;
	}
}

/* Symmetry plot for Both Axis Symmetry  - Newtbasin version */
void _fastcall symplot4basin(int x, int y, int color)
{
	int i, j, color1, stripe;
	if (color == 0) /* assumed to be "inside" color */
	{
		symplot4(x, y, color);
		return;
	}
	stripe = (basin == 2 && color > 8) ? 8 : 0;
	color -= stripe;               /* reconstruct unstriped color */
	color1 = (color < g_degree/2 + 2) ?
		(g_degree/2 + 2 - color) : (g_degree/2 + g_degree + 2 - color);
	j = g_xx_stop-(x-g_xx_start);
	g_put_color(x, y, color + stripe);
	if (j < xdots)
	{
		g_put_color(j, y, color1 + stripe);
	}
	i = g_yy_stop-(y-g_yy_start);
	if (i > g_y_stop && i < ydots)
	{
		g_put_color(x, i, stripe + (g_degree + 1 - color) % g_degree + 1);
		if (j < xdots)
		{
			g_put_color(j, i, stripe + (g_degree + 1 - color1) % g_degree + 1);
		}
	}
}

static void _fastcall put_truecolor_disk(int x, int y, int color)
{
	putcolor_a(x, y, color);
	targa_color(x, y, color);
}

/* Do nothing plot!!! */
#ifdef __CLINT__
#pragma argsused
#endif

void _fastcall noplot(int x, int y, int color)
{
	x = y = color = 0;  /* just for warning */
}
