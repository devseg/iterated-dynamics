#pragma once
#if !defined(CALCFRAC_H)
#define CALCFRAC_H

#include "big.h"

#include <vector>

#define MAX_CALC_WORK 12

struct WORKLIST     // work list entry for std escape time engines
{
    int xxstart;    // screen window for this entry
    int xxstop;
    int yystart;
    int yystop;
    int yybegin;    // start row within window, for 2pass/ssg resume
    int sym;        // if symmetry in window, prevents bad combines
    int pass;       // for 2pass and solid guessing
    int xxbegin;    // start col within window, =0 except on resume
};

extern int                   g_and_color;           // AND mask for iteration to get color index
extern int                   g_atan_colors;
extern DComplex              g_attractor[];
extern int                   g_attractor_period[];
extern int                   g_attractors;
extern int                 (*g_calc_type)();
extern bool                  g_cellular_next_screen;
extern double                g_close_enough;
extern double                g_close_proximity;
extern int                   g_col;
extern int                   g_color;
extern long                  g_color_iter;
extern int                   g_current_column;
extern int                   g_current_pass;
extern int                   g_current_row;
extern unsigned int          g_diffusion_bits;
extern unsigned long         g_diffusion_counter;
extern unsigned long         g_diffusion_limit;
extern char                  g_draw_mode;
extern double                g_f_radius;
extern double                g_f_x_center;
extern double                g_f_y_center;
extern long                  g_first_saved_and;
extern int                   g_got_status;
extern int                   g_i_x_start;
extern int                   g_i_x_stop;
extern int                   g_i_y_start;
extern int                   g_i_y_stop;
extern DComplex              g_init;
extern int                   g_invert;
extern int                   g_keyboard_check_interval;
extern LComplex              g_l_attractor[];
extern long                  g_l_close_enough;
extern LComplex              g_l_init_orbit;
extern long                  g_l_magnitude;
extern long                  g_l_magnitude_limit;
extern long                  g_l_magnitude_limit2;
extern double                g_magnitude;
extern bool                  g_magnitude_calc;
extern double                g_magnitude_limit;
extern double                g_magnitude_limit2;
extern int                   g_max_keyboard_check_interval;
extern DComplex              g_new_z;
extern int                   g_num_work_list;
extern long                  g_old_color_iter;
extern bool                  g_old_demm_colors;
extern DComplex              g_old_z;
extern int                   g_orbit_color;
extern int                   g_orbit_save_index;
extern int                   g_periodicity_check;
extern int                   g_periodicity_next_saved_incr;
extern int                   g_pi_in_pixels;
extern void                (*g_plot)(int, int, int);
extern void                (*g_put_color)(int, int, int);
extern bool                  g_quick_calc;
extern long                  g_real_color_iter;
extern bool                  g_reset_periodicity;
extern std::vector<BYTE>     g_resume_data;
extern bool                  g_resuming;
extern int                   g_row;
extern bool                  g_show_orbit;
extern symmetry_type         g_symmetry;
extern bool                  g_three_pass;
extern int                   g_total_passes;
extern DComplex              g_tmp_z;
extern bool                  g_use_old_periodicity;
extern bool                  g_use_old_distance_estimator;
extern WORKLIST              g_work_list[MAX_CALC_WORK];
extern int                   g_work_pass;
extern int                   g_work_symmetry;
extern int                   g_xx_start;
extern int                   g_xx_stop;
extern int                   g_yy_start;
extern int                   g_yy_stop;

extern long multiply(long x, long y, int n);
extern long divide(long x, long y, int n);
extern int calcfract();
extern int calcmand();
extern int calcmandfp();
extern int standard_fractal();
extern int test();
extern int plasma();
extern int diffusion();
extern int Bifurcation();
extern int BifurcLambda();
extern int BifurcSetTrigPi();
extern int LongBifurcSetTrigPi();
extern int BifurcAddTrigPi();
extern int LongBifurcAddTrigPi();
extern int BifurcMay();
extern bool BifurcMaySetup();
extern int LongBifurcMay();
extern int BifurcLambdaTrig();
extern int LongBifurcLambdaTrig();
extern int BifurcVerhulstTrig();
extern int LongBifurcVerhulstTrig();
extern int BifurcStewartTrig();
extern int LongBifurcStewartTrig();
extern int popcorn();
extern int lyapunov();
extern bool lya_setup();
extern int cellular();
extern bool CellularSetup();
extern int calcfroth();
extern int froth_per_pixel();
extern int froth_per_orbit();
extern bool froth_setup();
extern int logtable_in_extra_ok();
extern int find_alternate_math(fractal_type type, bf_math_type math);

#endif
