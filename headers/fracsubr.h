#pragma once
#if !defined(FRACSUBR_H)
#define FRACSUBR_H

extern int                   g_resume_len;
extern bool                  g_tab_or_help;
extern bool                  g_use_grid;

extern void wait_until(int index, uclock_t wait_time);
extern void free_grid_pointers();
extern void calcfracinit();
extern void adjust_corner();
extern int put_resume(int, ...);
extern int get_resume(int, ...);
extern int alloc_resume(int, int);
extern int start_resume();
extern void end_resume();
extern void sleepms(long);
extern void reset_clock();
extern void iplot_orbit(long, long, int);
extern void plot_orbit(double, double, int);
extern void scrub_orbit();
extern int add_worklist(int, int, int, int, int, int, int, int);
extern void tidy_worklist();
extern void get_julia_attractor(double, double);
extern int ssg_blocksize();
extern void symPIplot(int, int, int);
extern void symPIplot2J(int, int, int);
extern void symPIplot4J(int, int, int);
extern void symplot2(int, int, int);
extern void symplot2Y(int, int, int);
extern void symplot2J(int, int, int);
extern void symplot4(int, int, int);
extern void symplot2basin(int, int, int);
extern void symplot4basin(int, int, int);
extern void noplot(int, int, int);
extern void fractal_floattobf();
extern void adjust_cornerbf();
extern void set_grid_pointers();
extern void fill_dx_array();
extern void fill_lx_array();
extern bool snd_open();
extern void w_snd(int);
extern void snd_time_write();
extern void close_snd();

#endif
