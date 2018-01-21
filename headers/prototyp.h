#ifndef PROTOTYP_H
#define PROTOTYP_H
// includes needed to define the prototypes
#include "mpmath.h"
#include "big.h"
#include "fractint.h"
#include "externs.h"
// maintain the common prototypes in this file

#ifdef XFRACT
#include "unixprot.h"
#else
#ifdef _WIN32
#include "winprot.h"
#endif
#endif
extern long multiply(long x, long y, int n);
extern long divide(long x, long y, int n);
extern void spindac(int dir, int inc);
extern void put_line(int row, int startcol, int stopcol, BYTE const *pixels);
extern void get_line(int row, int startcol, int stopcol, BYTE *pixels);
extern void find_special_colors();
extern int getakeynohelp();
extern long readticker();
extern int get_sound_params();
extern void setnullvideo();
// framain2 -- C file prototypes
extern main_state big_while_loop(bool *kbdmore, bool *stacked, bool resume_flag);
extern bool check_key();
extern int cmp_line(BYTE *, int);
extern int key_count(int);
extern main_state main_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked);
extern int pot_line(BYTE *, int);
extern int sound_line(BYTE *, int);
extern int timer(int, int (*subrtn)(), ...);
extern void clear_zoombox();
extern void flip_image(int kbdchar);
extern void reset_zoom_corners();
extern void history_init();
// frasetup -- C file prototypes
extern bool VLSetup();
extern bool MandelSetup();
extern bool MandelfpSetup();
extern bool JuliaSetup();
extern bool NewtonSetup();
extern bool StandaloneSetup();
extern bool UnitySetup();
extern bool JuliafpSetup();
extern bool MandellongSetup();
extern bool JulialongSetup();
extern bool TrigPlusSqrlongSetup();
extern bool TrigPlusSqrfpSetup();
extern bool TrigPlusTriglongSetup();
extern bool TrigPlusTrigfpSetup();
extern bool FnPlusFnSym();
extern bool ZXTrigPlusZSetup();
extern bool LambdaTrigSetup();
extern bool JuliafnPlusZsqrdSetup();
extern bool SqrTrigSetup();
extern bool FnXFnSetup();
extern bool MandelTrigSetup();
extern bool MarksJuliaSetup();
extern bool MarksJuliafpSetup();
extern bool SierpinskiSetup();
extern bool SierpinskiFPSetup();
extern bool StandardSetup();
extern bool LambdaTrigOrTrigSetup();
extern bool JuliaTrigOrTrigSetup();
extern bool ManlamTrigOrTrigSetup();
extern bool MandelTrigOrTrigSetup();
extern bool HalleySetup();
extern bool dynam2dfloatsetup();
extern bool PhoenixSetup();
extern bool MandPhoenixSetup();
extern bool PhoenixCplxSetup();
extern bool MandPhoenixCplxSetup();
// gifview -- C file prototypes
extern int get_byte();
extern int get_bytes(BYTE *, int);
extern int gifview();
// hcmplx -- C file prototypes
extern void HComplexTrig0(DHyperComplex *, DHyperComplex *);
// intro -- C file prototypes
extern void intro();
// jiim -- C file prototypes
extern void Jiim(jiim_types which);
extern LComplex PopLong();
extern DComplex PopFloat();
extern LComplex DeQueueLong();
extern DComplex DeQueueFloat();
extern LComplex ComplexSqrtLong(long, long);
extern DComplex ComplexSqrtFloat(double, double);
extern bool Init_Queue(unsigned long);
extern void   Free_Queue();
extern void   ClearQueue();
extern int    QueueEmpty();
extern int    QueueFull();
extern int    QueueFullAlmost();
extern int    PushLong(long, long);
extern int    PushFloat(float, float);
extern int    EnQueueLong(long, long);
extern int    EnQueueFloat(float, float);
// line3d -- C file prototypes
extern int line3d(BYTE *, unsigned int);
extern int targa_color(int, int, int);
extern bool targa_validate(char const *File_Name);
bool startdisk1(char const *File_Name2, FILE *Source, bool overlay);
// loadfdos -- C file prototypes
extern int get_video_mode(FRACTAL_INFO *info, ext_blk_3 *blk_3_info);
// loadfile -- C file prototypes
extern int read_overlay();
extern void set_if_old_bif();
extern void set_function_parm_defaults();
extern int fgetwindow();
extern void backwards_v18();
extern void backwards_v19();
extern void backwards_v20();
extern bool check_back();
// loadmap -- C file prototypes
//extern void SetTgaColors();
extern bool ValidateLuts(char const *mapname);
extern void SetColorPaletteName(char const *fn);
// lorenz -- C file prototypes
extern bool orbit3dlongsetup();
extern bool orbit3dfloatsetup();
extern int lorenz3dlongorbit(long *, long *, long *);
extern int lorenz3d1floatorbit(double *, double *, double *);
extern int lorenz3dfloatorbit(double *, double *, double *);
extern int lorenz3d3floatorbit(double *, double *, double *);
extern int lorenz3d4floatorbit(double *, double *, double *);
extern int henonfloatorbit(double *, double *, double *);
extern int henonlongorbit(long *, long *, long *);
extern int inverse_julia_orbit(double *, double *, double *);
extern int Minverse_julia_orbit();
extern int Linverse_julia_orbit();
extern int inverse_julia_per_image();
extern int rosslerfloatorbit(double *, double *, double *);
extern int pickoverfloatorbit(double *, double *, double *);
extern int gingerbreadfloatorbit(double *, double *, double *);
extern int rosslerlongorbit(long *, long *, long *);
extern int kamtorusfloatorbit(double *, double *, double *);
extern int kamtoruslongorbit(long *, long *, long *);
extern int hopalong2dfloatorbit(double *, double *, double *);
extern int chip2dfloatorbit(double *, double *, double *);
extern int quadruptwo2dfloatorbit(double *, double *, double *);
extern int threeply2dfloatorbit(double *, double *, double *);
extern int martin2dfloatorbit(double *, double *, double *);
extern int orbit2dfloat();
extern int orbit2dlong();
extern int funny_glasses_call(int (*)());
extern int ifs();
extern int orbit3dfloat();
extern int orbit3dlong();
extern int iconfloatorbit(double *, double *, double *);
extern int latoofloatorbit(double *, double *, double *);
extern bool setup_convert_to_screen(affine *);
extern int plotorbits2dsetup();
extern int plotorbits2dfloat();
// lsys -- C file prototypes
extern LDBL  getnumber(char const **str);
extern bool ispow2(int);
extern int Lsystem();
extern bool LLoad();
// miscfrac -- C file prototypes
extern void froth_cleanup();
// miscovl -- C file prototypes
extern void make_batch_file();
extern void edit_text_colors();
extern int select_video_mode(int);
extern void format_vid_table(int choice, char *buf);
extern void make_mig(unsigned int, unsigned int);
extern int getprecdbl(int);
extern int getprecbf(int);
extern int getprecbf_mag();
extern void parse_comments(char *value);
extern void init_comments();
extern void write_batch_parms(char const *colorinf, bool colorsonly, int maxcolor, int i, int j);
extern std::string expand_comments(char const *source);
// miscres -- C file prototypes
extern void restore_active_ovly();
extern void findpath(char const *filename, char *fullpathname);
extern void notdiskmsg();
extern void cvtcentermag(double *, double *, LDBL *, double *, double *, double *);
extern void cvtcorners(double, double, LDBL, double, double, double);
extern void cvtcentermagbf(bf_t, bf_t, LDBL *, double *, double *, double *);
extern void cvtcornersbf(bf_t, bf_t, LDBL, double, double, double);
extern void updatesavename(char *filename);
extern void updatesavename(std::string &filename);
extern int check_writefile(char *name, char const *ext);
extern int check_writefile(std::string &name, char const *ext);
extern void showtrig(char *buf);
extern int set_trig_array(int k, char const *name);
extern void set_trig_pointers(int);
extern int tab_display();
extern int endswithslash(char const *fl);
extern int ifsload();
extern bool find_file_item(char *filename, char const *itemname, FILE **fileptr, int itemtype);
extern bool find_file_item(std::string &filename, char const *itemname, FILE **fileptr, int itemtype);
extern int file_gets(char *buf, int maxlen, FILE *infile);
extern void roundfloatd(double *);
extern void fix_inversion(double *);
extern int ungetakey(int);
extern void get_calculation_time(char *msg, long ctime);
// mpmath_c -- C file prototypes
extern MP *MPsub(MP, MP);
extern MP *MPsub086(MP, MP);
extern MP *MPsub386(MP, MP);
extern MP *MPabs(MP);
extern MPC MPCsqr(MPC);
extern MP MPCmod(MPC);
extern MPC MPCmul(MPC, MPC);
extern MPC MPCdiv(MPC, MPC);
extern MPC MPCadd(MPC, MPC);
extern MPC MPCsub(MPC, MPC);
extern MPC MPCpow(MPC, int);
extern int MPCcmp(MPC, MPC);
extern DComplex MPC2cmplx(MPC);
extern MPC cmplx2MPC(DComplex);
extern void setMPfunctions();
extern DComplex ComplexPower(DComplex, DComplex);
extern void SetupLogTable();
extern long logtablecalc(long);
extern long ExpFloat14(long);
extern bool ComplexNewtonSetup();
extern int ComplexNewton();
extern int ComplexBasin();
extern int GausianNumber(int, int);
extern void Arcsinz(DComplex z, DComplex *rz);
extern void Arccosz(DComplex z, DComplex *rz);
extern void Arcsinhz(DComplex z, DComplex *rz);
extern void Arccoshz(DComplex z, DComplex *rz);
extern void Arctanhz(DComplex z, DComplex *rz);
extern void Arctanz(DComplex z, DComplex *rz);
// msccos -- C file prototypes
extern double _cos(double);
// parser -- C file prototypes
struct fn_operand
{ // function, load, store pointers
    void (*function)();
    Arg *operand;
};
extern unsigned long NewRandNum();
extern void lRandom();
extern void dRandom();
extern void mRandom();
extern void SetRandFnct();
extern void RandomSeed();
extern void lStkSRand();
extern void mStkSRand();
extern void dStkSRand();
extern void dStkAbs();
extern void mStkAbs();
extern void lStkAbs();
extern void dStkSqr();
extern void mStkSqr();
extern void lStkSqr();
extern void dStkAdd();
extern void mStkAdd();
extern void lStkAdd();
extern void dStkSub();
extern void mStkSub();
extern void lStkSub();
extern void dStkConj();
extern void mStkConj();
extern void lStkConj();
extern void dStkZero();
extern void mStkZero();
extern void lStkZero();
extern void dStkOne();
extern void mStkOne();
extern void lStkOne();
extern void dStkReal();
extern void mStkReal();
extern void lStkReal();
extern void dStkImag();
extern void mStkImag();
extern void lStkImag();
extern void dStkNeg();
extern void mStkNeg();
extern void lStkNeg();
extern void dStkMul();
extern void mStkMul();
extern void lStkMul();
extern void dStkDiv();
extern void mStkDiv();
extern void lStkDiv();
extern void StkSto();
extern void StkLod();
extern void dStkMod();
extern void mStkMod();
extern void lStkMod();
extern void StkClr();
extern void dStkFlip();
extern void mStkFlip();
extern void lStkFlip();
extern void dStkSin();
extern void mStkSin();
extern void lStkSin();
extern void dStkTan();
extern void mStkTan();
extern void lStkTan();
extern void dStkTanh();
extern void mStkTanh();
extern void lStkTanh();
extern void dStkCoTan();
extern void mStkCoTan();
extern void lStkCoTan();
extern void dStkCoTanh();
extern void mStkCoTanh();
extern void lStkCoTanh();
extern void dStkRecip();
extern void mStkRecip();
extern void lStkRecip();
extern void StkIdent();
extern void dStkSinh();
extern void mStkSinh();
extern void lStkSinh();
extern void dStkCos();
extern void mStkCos();
extern void lStkCos();
extern void dStkCosXX();
extern void mStkCosXX();
extern void lStkCosXX();
extern void dStkCosh();
extern void mStkCosh();
extern void lStkCosh();
extern void dStkLT();
extern void mStkLT();
extern void lStkLT();
extern void dStkGT();
extern void mStkGT();
extern void lStkGT();
extern void dStkLTE();
extern void mStkLTE();
extern void lStkLTE();
extern void dStkGTE();
extern void mStkGTE();
extern void lStkGTE();
extern void dStkEQ();
extern void mStkEQ();
extern void lStkEQ();
extern void dStkNE();
extern void mStkNE();
extern void lStkNE();
extern void dStkOR();
extern void mStkOR();
extern void lStkOR();
extern void dStkAND();
extern void mStkAND();
extern void lStkAND();
extern void dStkLog();
extern void mStkLog();
extern void lStkLog();
extern void FPUcplxexp(DComplex *, DComplex *);
extern void dStkExp();
extern void mStkExp();
extern void lStkExp();
extern void dStkPwr();
extern void mStkPwr();
extern void lStkPwr();
extern void dStkASin();
extern void mStkASin();
extern void lStkASin();
extern void dStkASinh();
extern void mStkASinh();
extern void lStkASinh();
extern void dStkACos();
extern void mStkACos();
extern void lStkACos();
extern void dStkACosh();
extern void mStkACosh();
extern void lStkACosh();
extern void dStkATan();
extern void mStkATan();
extern void lStkATan();
extern void dStkATanh();
extern void mStkATanh();
extern void lStkATanh();
extern void dStkCAbs();
extern void mStkCAbs();
extern void lStkCAbs();
extern void dStkSqrt();
extern void mStkSqrt();
extern void lStkSqrt();
extern void dStkFloor();
extern void mStkFloor();
extern void lStkFloor();
extern void dStkCeil();
extern void mStkCeil();
extern void lStkCeil();
extern void dStkTrunc();
extern void mStkTrunc();
extern void lStkTrunc();
extern void dStkRound();
extern void mStkRound();
extern void lStkRound();
extern void (*mtrig0)();
extern void (*mtrig1)();
extern void (*mtrig2)();
extern void (*mtrig3)();
extern void EndInit();
extern void NotAFnct();
extern void FnctNotFound();
extern int CvtStk();
extern int fFormula();
extern void RecSortPrec();
extern int Formula();
extern int BadFormula();
extern int form_per_pixel();
extern int frm_get_param_stuff(char const *Name);
extern bool RunForm(char const *Name, bool from_prompts1c);
extern bool fpFormulaSetup();
extern bool intFormulaSetup();
extern void init_misc();
extern void free_workarea();
extern int fill_if_group(int endif_index, JUMP_PTRS_ST *jump_data);
// plot3d -- C file prototypes
extern void draw_line(int, int, int, int, int);
extern void plot3dsuperimpose16(int, int, int);
extern void plot3dsuperimpose256(int, int, int);
extern void plotIFS3dsuperimpose256(int, int, int);
extern void plot3dalternate(int, int, int);
extern void plot_setup();
// printer -- C file prototypes
extern void Print_Screen();
// prompts1 -- C file prototypes
struct fullscreenvalues;
extern int fullscreen_prompt(
    char const *hdg,
    int numprompts,
    char const **prompts,
    fullscreenvalues *values,
    int fkeymask,
    char *extrainfo);
extern long get_file_entry(int type, char const *title, char const *fmask,
                    char *filename, char *entryname);
extern long get_file_entry(int type, char const *title, char const *fmask,
                    std::string &filename, char *entryname);
extern long get_file_entry(int type, char const *title, char const *fmask,
                    std::string &filename, std::string &entryname);
extern int get_fracttype();
extern int get_fract_params(int);
extern int get_fract3d_params();
extern int get_3d_params();
extern int prompt_valuestring(char *buf, fullscreenvalues const *val);
extern void setbailoutformula(bailouts);
extern int find_extra_param(fractal_type type);
extern void load_params(fractal_type fractype);
extern bool check_orbit_name(char const *orbitname);
struct entryinfo;
extern int scan_entries(FILE *infile, struct entryinfo *ch, char const *itemname);
// prompts2 -- C file prototypes
extern int get_toggles();
extern int get_toggles2();
extern int passes_options();
extern int get_view_params();
extern int get_starfield_params();
extern int get_commands();
extern void goodbye();
extern bool isadirectory(char const *s);
extern bool getafilename(char const *hdg, char const *file_template, char *flname);
extern bool getafilename(char const *hdg, char const *file_template, std::string &flname);
extern int splitpath(char const *file_template, char *drive, char *dir, char *fname, char *ext);
extern int makepath(char *template_str, char const *drive, char const *dir, char const *fname, char const *ext);
extern int fr_findfirst(char const *path);
extern int fr_findnext();
extern void shell_sort(void *, int n, unsigned, int (*fct)(VOIDPTR, VOIDPTR));
extern void fix_dirname(char *dirname);
extern void fix_dirname(std::string &dirname);
extern int merge_pathnames(char *oldfullpath, char const *newfilename, cmd_file mode);
extern int merge_pathnames(std::string &oldfullpath, char const *newfilename, cmd_file mode);
extern int get_browse_params();
extern int get_cmd_string();
extern int get_rds_params();
extern int starfield();
extern int get_a_number(double *, double *);
extern int lccompare(VOIDPTR, VOIDPTR);
extern int dir_remove(char const *dir, char const *filename);
extern FILE *dir_fopen(char const *dir, char const *filename, char const *mode);
extern void extract_filename(char *target, char const *source);
extern std::string extract_filename(char const *source);
extern const char *has_ext(char const *source);
// realdos -- C file prototypes
extern int showvidlength();
extern bool stopmsg(int flags, char const* msg);
extern void blankrows(int, int, int);
extern int texttempmsg(char const *);
extern int fullscreen_choice(
    int options,
    char const *hdg,
    char const *hdg2,
    char const *instr,
    int numchoices,
    char const **choices,
    int *attributes,
    int boxwidth,
    int boxdepth,
    int colwidth,
    int current,
    void (*formatitem)(int, char*),
    char *speedstring,
    int (*speedprompt)(int row, int col, int vid, char const *speedstring, int speed_match),
    int (*checkkey)(int, int)
);
extern bool showtempmsg(char const *);
extern void cleartempmsg();
extern void helptitle();
extern int putstringcenter(int row, int col, int width, int attr, char const *msg);
extern int main_menu(int);
extern int input_field(int options, int attr, char *fld, int len, int row, int col,
    int (*checkkey)(int curkey));
extern int field_prompt(char const *hdg, char const *instr, char *fld, int len,
    int (*checkkey)(int curkey));
extern bool thinking(int options, char const *msg);
extern void discardgraphics();
extern void load_fractint_config();
extern int check_vidmode_key(int, int);
extern int check_vidmode_keyname(char const *kname);
extern void vidmode_keyname(int k, char *buf);
extern void freetempmsg();
extern void load_videotable(int);
extern void bad_fractint_cfg_msg();
// rotate -- C file prototypes
extern void rotate(int);
extern void save_palette();
extern bool load_palette();
// slideshw -- C file prototypes
extern int slideshw();
extern slides_mode startslideshow();
extern void stopslideshow();
extern void recordshw(int);
extern int handle_special_keys(int ch);
// stereo -- C file prototypes
extern bool do_AutoStereo();
extern int outline_stereo(BYTE *, int);
// testpt -- C file prototypes
extern int teststart();
extern void testend();
extern int testpt(double, double, double, double, long, int);
// zoom -- C file prototypes
extern void drawbox(bool draw_it);
extern void moveboxf(double, double);
extern void resizebox(int);
extern void chgboxi(int, int);
extern void zoomout();
extern void aspectratio_crop(float, float);
extern int init_pan_or_recalc(bool);
extern void drawlines(struct coords, struct coords, int, int);
extern void addbox(struct coords);
extern void clearbox();
extern void dispbox();
// fractalb.c -- C file prototypes
extern DComplex cmplxbntofloat(BNComplex *);
extern DComplex cmplxbftofloat(BFComplex *);
extern void comparevalues(char const *s, LDBL x, bn_t bnx);
extern void comparevaluesbf(char const *s, LDBL x, bf_t bfx);
extern void show_var_bf(char const *s, bf_t n);
extern void show_two_bf(char const *s1, bf_t t1, char const *s2, bf_t t2, int digits);
extern void bfcornerstofloat();
extern void showcornersdbl(char const *s);
extern bool MandelbnSetup();
extern int mandelbn_per_pixel();
extern int juliabn_per_pixel();
extern int JuliabnFractal();
extern int JuliaZpowerbnFractal();
extern BNComplex *cmplxlog_bn(BNComplex *t, BNComplex *s);
extern BNComplex *cplxmul_bn(BNComplex *t, BNComplex *x, BNComplex *y);
extern BNComplex *ComplexPower_bn(BNComplex *t, BNComplex *xx, BNComplex *yy);
extern bool MandelbfSetup();
extern int mandelbf_per_pixel();
extern int juliabf_per_pixel();
extern int JuliabfFractal();
extern int JuliaZpowerbfFractal();
extern BFComplex *cmplxlog_bf(BFComplex *t, BFComplex *s);
extern BFComplex *cplxmul_bf(BFComplex *t, BFComplex *x, BFComplex *y);
extern BFComplex *ComplexPower_bf(BFComplex *t, BFComplex *xx, BFComplex *yy);
// memory -- C file prototypes
// TODO: Get rid of this and use regular memory routines;
// see about creating standard disk memory routines for disk video
extern void DisplayHandle(U16 handle);
extern int MemoryType(U16 handle);
extern void InitMemory();
extern void ExitCheck();
extern U16 MemoryAlloc(U16 size, long count, int stored_at);
extern void MemoryRelease(U16 handle);
extern bool CopyFromMemoryToHandle(BYTE const *buffer, U16 size, long count, long offset, U16 handle);
extern bool CopyFromHandleToMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
extern bool SetMemory(int value, U16 size, long count, long offset, U16 handle);
// soi -- C file prototypes
extern void soi();
/*
 *  uclock -- C file prototypes
 *  The  uclock_t typedef placed here because uclock.h
 *  prototype is for DOS version only.
 */
typedef unsigned long uclock_t;
extern uclock_t usec_clock();
extern void restart_uclock();
extern void wait_until(int index, uclock_t wait_time);
extern void init_failure(char const *message);
extern int expand_dirname(char *dirname, char *drive);
extern bool abortmsg(char const *file, unsigned int line, int flags, char const *msg);
#define ABORT(flags_, msg_) abortmsg(__FILE__, __LINE__, flags_, msg_)
extern long stackavail();
extern int getcolor(int, int);
extern int out_line(BYTE *, int);
extern void setvideomode(int, int, int, int);
extern int pot_startdisk();
extern void putcolor_a(int, int, int);
extern int startdisk();
#endif
