/*
 * helpcom.h
 *
 * Common #defines, structures and code for HC.C and HELP.C
 *
 */
#ifndef HELPCOM_H
#define HELPCOM_H
/*
 * help file signature
 */
#define HELP_SIG           (0xAFBC1823UL)
/*
 * commands embedded in the help text
 */
#define CMD_LITERAL       1   // next char taken literally
#define CMD_PARA          2   // paragraph start code
#define CMD_LINK          3   // hot-link start/end code
#define CMD_FF            4   // force a form-feed
#define CMD_XONLINE       5   // exclude from online help on/off
#define CMD_XDOC          6   // exclude from printed document on/off
#define CMD_CENTER        7   // center this line
#define CMD_SPACE         8   // next byte is count of spaces
#define MAX_CMD           8
/*
 * on-line help dimensions
 */
#define SCREEN_WIDTH      (78)
#define SCREEN_DEPTH      (22)
#define SCREEN_INDENT     (1)
/*
 * printed document dimensions
 */
#define PAGE_WIDTH         (72)  // width of printed text
#define PAGE_INDENT        (2)   // indent all text by this much
#define TITLE_INDENT       (1)   // indent titles by this much
#define PAGE_RDEPTH        (59)  // the total depth (inc. heading)
#define PAGE_HEADING_DEPTH (3)   // depth of the heading
#define PAGE_DEPTH         (PAGE_RDEPTH-PAGE_HEADING_DEPTH) // depth of text
/*
 * Document page-break macros.  Goto to next page if this close (or closer)
 * to end of page when starting a CONTENT, TOPIC, or at a BLANK line.
 */
#define CONTENT_BREAK (7)  // start of a "DocContent" entry
#define TOPIC_BREAK   (4)  // start of each topic under a DocContent entry
#define BLANK_BREAK   (2)  // a blank line
/*
 * tokens returned by find_token_length
 */
enum class token_types
{
    DONE = 0,       // len == 0
    SPACE = 1,      // a run of spaces
    LINK = 2,       // an entire link
    PARA = 3,       // a CMD_PARA
    NL = 4,         // a new-line ('\n')
    FF = 5,         // a form-feed (CMD_FF)
    WORD = 6,       // a word
    XONLINE = 7,    // a CMD_XONLINE
    XDOC = 8,       // a CMD_XDOC
    CENTER = 9      // a CMD_CENTER
};
/*
 * modes for find_token_length() and find_line_width()
 */
enum class token_modes
{
    NONE = 0,
    ONLINE = 1,
    DOC = 2
};
/*
 * struct PD_INFO used by process_document()
 */
struct PD_INFO
{
    // used by process_document -- look but don't touch!
    int page_num;
    int line_num;
    // PD_GET_TOPIC is allowed to change these
    char *curr;
    unsigned  len;
    // PD_GET_CONTENT is allowed to change these
    char *id;
    char *title;
    bool new_page;
    // general parameters
    char *s;
    int i;
};
/*
 * Commands passed to (*get_info)() and (*output)() by process_document()
 */
enum  PD_COMMANDS
{
    // commands sent to pd_output
    PD_HEADING,         // call at the top of each page
    PD_FOOTING,          // called at the end of each page
    PD_PRINT,            // called to send text to the printer
    PD_PRINTN,           // called to print a char n times
    PD_PRINT_SEC,        // called to print the section title line
    PD_START_SECTION,    // called at the start of each section
    PD_START_TOPIC,      // called at the start of each topic
    PD_SET_SECTION_PAGE, // set the current sections page number
    PD_SET_TOPIC_PAGE,   // set the current topics page number
    PD_PERIODIC,         // called just before curr is incremented to next token
    // commands sent to pd_get_info
    PD_GET_CONTENT,
    PD_GET_TOPIC,
    PD_RELEASE_TOPIC,
    PD_GET_LINK_PAGE
};
typedef bool (*PD_FUNC)(int cmd, PD_INFO *pd, void *info);
extern token_types _find_token_length(char const *curr, unsigned len, int *size, int *width);
extern token_types find_token_length(token_modes mode, char const *curr, unsigned len, int *size, int *width);
extern int find_line_width(token_modes mode, char const *curr, unsigned len);
extern bool process_document(PD_FUNC get_info, PD_FUNC output, void *info);
extern int help(int);
extern int read_help_topic(int , int , int , void *);
extern bool makedoc_msg_func(int pnum, int num_pages);
extern void print_document(char const *outfname, bool (*msg_func)(int, int), int save_extraseg);
extern int init_help();
extern void end_help();
extern bool is_hyphen(char const *ptr);
#ifndef XFRACT
inline int getint(char const *ptr)
{
    return *reinterpret_cast<const int *>(ptr);
}
inline void setint(char *ptr, int n)
{
    *reinterpret_cast<int *>(ptr) = n;
}
#else
extern int getint(char const *ptr);
extern void setint(char *ptr, int n);
#endif
#endif
