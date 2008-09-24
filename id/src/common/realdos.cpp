#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "strcpy.h"

#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "Externals.h"
#include "filesystem.h"
#include "FullScreenChooser.h"
#include "idhelp.h"
#include "miscres.h"
#include "prompts2.h"
#include "realdos.h"
#include "TextColors.h"

// uncomment following for production version
/*
#define PRODUCTION
*/
enum
{
	MENU_HDG = 3,
	MENU_ITEM = 1,
	SWAPBLKLEN = 4096 // must be a power of 2
};

BYTE g_suffix[10000];
int g_release = 2099;	// this has 2 implied decimals; increment it every synch
int g_patch_level = 9;	// patchlevel for this version
//int g_video_table_len;                 // number of entries in above
int g_cfg_line_nums[MAXVIDEOMODES] = { 0 };

static BYTE *s_temp_text_save = 0;
static int s_text_x_dots = 0;
static int s_text_y_dots = 0;
static bool s_full_menu = false;
static int menu_check_key(int curkey, int choice);

/*	text_temp_message(msg)

	displays a text message of up to 40 characters, waits
	for a key press, restores the prior display, and returns (without
	eating the key).
*/
int text_temp_message(const std::string &message)
{
	return text_temp_message(message.c_str());
}
int text_temp_message(char const *msgparm)
{
	if (show_temp_message(msgparm))
	{
		return -1;
	}

	driver_wait_key_pressed(0); // wait for a keystroke but don't eat it
	clear_temp_message();
	return 0;
}

void free_temp_message()
{
	delete[] s_temp_text_save;
	s_temp_text_save = 0;
}

int show_temp_message(const std::string &message)
{
	return show_temp_message(message.c_str());
}

int show_temp_message(char const *msgparm)
{
	static long size = 0;
	char msg[41];
	BYTE *fontptr = 0;
	int i;
	int xrepeat = 0;
	int yrepeat = 0;
	int save_sxoffs;
	int save_syoffs;

	strncpy(msg, msgparm, 40);
	msg[40] = 0; // ensure max message len of 40 chars
	if (driver_diskp())  // disk video, screen in text mode, easy
	{
		disk_video_status(0, msg);
		return 0;
	}
	if (g_command_initialize)      // & command_files hasn't finished 1st try
	{
		// TODO : don't use printf!
#if 0
		printf("%s\n", msg);
#endif
		return 0;
	}

	xrepeat = (g_screen_width >= 640) ? 2 : 1;
	yrepeat = (g_screen_height >= 300) ? 2 : 1;
	s_text_x_dots = int(strlen(msg))*xrepeat*8;
	s_text_y_dots = yrepeat*8;

	// worst case needs 10k
	if (s_temp_text_save != 0)
	{
		if (size != long(s_text_x_dots)*s_text_y_dots)
		{
			free_temp_message();
		}
	}
	size = long(s_text_x_dots)*s_text_y_dots;
	save_sxoffs = g_screen_x_offset;
	save_syoffs = g_screen_y_offset;
	g_screen_x_offset = 0;
	g_screen_y_offset = 0;
	if (s_temp_text_save == 0) // only save screen first time called
	{
		s_temp_text_save = new BYTE[s_text_x_dots*s_text_y_dots];
		if (s_temp_text_save == 0)
		{
			return -1; // sorry, message not displayed
		}
		for (i = 0; i < s_text_y_dots; ++i)
		{
			get_line(i, 0, s_text_x_dots-1, &s_temp_text_save[i*s_text_x_dots]);
		}
	}

	g_.DAC().FindSpecialColors(); // get g_.DAC().Dark() & g_color_medium set
	driver_display_string(0, 0, g_.DAC().Medium(), g_.DAC().Dark(), msg);
	g_screen_x_offset = save_sxoffs;
	g_screen_y_offset = save_syoffs;

	return 0;
}

void clear_temp_message()
{
	int i;
	int save_sxoffs;
	int save_syoffs;
	if (driver_diskp()) // disk video, easy
	{
		disk_video_status(0, "");
	}
	else if (s_temp_text_save != 0)
	{
		save_sxoffs = g_screen_x_offset;
		save_syoffs = g_screen_y_offset;
		g_screen_x_offset = 0;
		g_screen_y_offset = 0;
		for (i = 0; i < s_text_y_dots; ++i)
		{
			put_line(i, 0, s_text_x_dots-1, &s_temp_text_save[i*s_text_x_dots]);
		}
		if (!g_using_jiim)  // jiim frees memory with free_temp_message()
		{
			delete[] s_temp_text_save;
			s_temp_text_save = 0;
		}
		g_screen_x_offset = save_sxoffs;
		g_screen_y_offset = save_syoffs;
	}
}

void help_title()
{
	driver_set_clear(); // clear the screen
	std::ostringstream message;
	message << boost::format("Iterated Dynamics Version %d.%01d") % (g_release/100) % ((g_release % 100)/10);
	if (g_release % 10)
	{
		message << boost::format("%01d") % (g_release % 10);
	}
	if (g_patch_level)
	{
		message << "." << g_patch_level;
	}
	put_string_center(0, 0, 80, C_TITLE, message.str().c_str());

// uncomment next for production executable:
#if defined(PRODUCTION) || defined(XFRACT)
	return;
	/*NOTREACHED*/
#else
	if (DEBUGMODE_NO_DEV_HEADING == g_debug_mode)
	{
		return;
	}
#define DEVELOPMENT
#ifdef DEVELOPMENT
	driver_put_string(0, 2, C_TITLE_DEV, "Development Version");
#else
	driver_put_string(0, 3, C_TITLE_DEV, "Customized Version");
#endif
	driver_put_string(0, 55, C_TITLE_DEV, "Not for Public Release");
#endif
}


int put_string_center(int row, int col, int width, int attr, const std::string &msg)
{
	return put_string_center(row, col, width, attr, msg.c_str());
}

int put_string_center(int row, int col, int width, int attr, char const *msg)
{
	char buf[81];
	int i;
	int j;
	int k;
	i = 0;
#ifdef XFRACT
	if (width >= 80) // Some systems choke in column 80
	{
		width = 79;
	}
#endif
	while (msg[i])
	{
		++i; // strlen for a
	}
	if (i == 0)
	{
		return -1;
	}
	if (i >= width) // sanity check
	{
		i = width - 1;
	}
	j = (width - i)/2;
	j -= (width + 10 - i)/20; // when wide a bit left of center looks better
	std::fill(buf, buf + width, ' ');
	buf[width] = 0;
	i = 0;
	k = j;
	while (msg[i])
	{
		buf[k++] = msg[i++]; // strcpy for a
	}
	driver_put_string(row, col, attr, buf);
	return j;
}

// ------------------------------------------------------------------------

int main_menu(bool full_menu)
{
	char const *choices[44]; // 2 columns*22 rows
	int attributes[44];
	int choicekey[44];
	int i;
	int nextleft;
	int nextright;
	int showjuliatoggle;
	bool save_tab_display_enabled = g_tab_display_enabled;

top:
	s_full_menu = full_menu;
	g_tab_display_enabled = false;
	showjuliatoggle = 0;
	for (i = 0; i < 44; ++i)
	{
		attributes[i] = 256;
		choices[i] = "";
		choicekey[i] = -1;
	}
	nextleft = -2;
	nextright = -1;

	if (full_menu)
	{
		nextleft += 2;
		choices[nextleft] = "      CURRENT IMAGE         ";
		attributes[nextleft] = 256 + MENU_HDG;

		nextleft += 2;
		choicekey[nextleft] = 13; // enter
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = (g_externs.CalculationStatus() == CALCSTAT_RESUMABLE) ?
			"continue calculation        " :
			"return to image             ";

		nextleft += 2;
		choicekey[nextleft] = 9; // tab
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "info about image      <tab> ";

		nextleft += 2;
		choicekey[nextleft] = 'o';
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "orbits window          <o>  ";
		if (!fractal_type_julia_or_inverse(g_fractal_type))
		{
			nextleft += 2;
		}
	}

	nextleft += 2;
	choices[nextleft] = "      NEW IMAGE             ";
	attributes[nextleft] = 256 + MENU_HDG;

	nextleft += 2;
	choicekey[nextleft] = IDK_DELETE;
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "select video mode...  <del> ";

	nextleft += 2;
	choicekey[nextleft] = 't';
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "select fractal type    <t>  ";

	if (full_menu)
	{
		if ((!fractal_type_none(g_current_fractal_specific->tojulia)
				&& g_parameters[P1_REAL] == 0.0 && g_parameters[P1_IMAG] == 0.0)
			|| !fractal_type_none(g_current_fractal_specific->tomandel))
		{
			nextleft += 2;
			choicekey[nextleft] = IDK_SPACE;
			attributes[nextleft] = MENU_ITEM;
			choices[nextleft] = "toggle to/from julia <space>";
			showjuliatoggle = 1;
		}
		if (fractal_type_julia_or_inverse(g_fractal_type))
		{
			nextleft += 2;
			choicekey[nextleft] = 'j';
			attributes[nextleft] = MENU_ITEM;
			choices[nextleft] = "toggle to/from inverse <j>  ";
			showjuliatoggle = 1;
		}

		nextleft += 2;
		choicekey[nextleft] = 'h';
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "return to prior image  <h>   ";

		nextleft += 2;
		choicekey[nextleft] = IDK_BACKSPACE;
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "reverse thru history <ctl-h> ";
	}
	else
	{
		nextleft += 2;
	}

	nextleft += 2;
	choices[nextleft] = "      OPTIONS                ";
	attributes[nextleft] = 256 + MENU_HDG;

	nextleft += 2;
	choicekey[nextleft] = 'x';
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "basic options...       <x>  ";

	nextleft += 2;
	choicekey[nextleft] = 'y';
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "extended options...    <y>  ";

	nextleft += 2;
	choicekey[nextleft] = 'z';
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "type-specific parms... <z>  ";

	nextleft += 2;
	choicekey[nextleft] = 'p';
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "passes options...      <p>  ";

	nextleft += 2;
	choicekey[nextleft] = 'v';
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "view window options... <v>  ";

	if (showjuliatoggle == 0)
	{
		nextleft += 2;
		choicekey[nextleft] = 'i';
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "fractal 3D parms...    <i>  ";
	}

	nextleft += 2;
	choicekey[nextleft] = IDK_CTL_B;
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "browse parms...      <ctl-b>";

	if (full_menu)
	{
		nextleft += 2;
		choicekey[nextleft] = IDK_CTL_E;
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "evolver parms...     <ctl-e>";

		// TODO: sound support for unix/X11
		nextleft += 2;
		choicekey[nextleft] = IDK_CTL_F;
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "sound parms...       <ctl-f>";
	}

	nextright += 2;
	attributes[nextright] = 256 + MENU_HDG;
	choices[nextright] = "        FILE                  ";

	nextright += 2;
	choicekey[nextright] = '@';
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "run saved command set... <@>  ";

	if (full_menu)
	{
		nextright += 2;
		choicekey[nextright] = 's';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "save image to file       <s>  ";
	}

	nextright += 2;
	choicekey[nextright] = 'r';
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "load image from file...  <r>  ";

	nextright += 2;
	choicekey[nextright] = '3';
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "3d transform from file...<3>  ";

	if (full_menu)
	{
		nextright += 2;
		choicekey[nextright] = '#';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "3d overlay from file.....<#>  ";

		nextright += 2;
		choicekey[nextright] = 'b';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "save current parameters..<b>  ";

		nextright += 2;
		choicekey[nextright] = 16;
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "print image          <ctl-p>  ";
	}

	nextright += 2;
	choicekey[nextright] = 'd';
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "start command shell      <d>  ";

	nextright += 2;
	choicekey[nextright] = 'g';
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "give command string      <g>  ";

	nextright += 2;
	choicekey[nextright] = IDK_ESC;
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "quit Iterated Dynamics  <esc> ";

	nextright += 2;
	choicekey[nextright] = IDK_INSERT;
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "restart Iterated Dynamics<ins>";

	if (full_menu && g_.RealDAC())
	{
		nextright += 2;
		choices[nextright] = "       COLORS                 ";
		attributes[nextright] = 256 + MENU_HDG;

		nextright += 2;
		choicekey[nextright] = 'c';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "color cycling mode       <c>  ";

		nextright += 2;
		choicekey[nextright] = '+';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "rotate palette      <+>, <->  ";

		nextright += 2;
		choicekey[nextright] = 'e';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "palette editing mode     <e>  ";

		nextright += 2;
		choicekey[nextright] = 'a';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "make starfield           <a>  ";
	}

	nextright += 2;
	choicekey[nextright] = IDK_CTL_A;
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "ant automaton          <ctl-a>";

	nextright += 2;
	choicekey[nextright] = IDK_CTL_S;
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "stereogram             <ctl-s>";

	i = driver_key_pressed() ? driver_get_key() : 0;
	if (menu_check_key(i, 0) == 0)
	{
		nextleft += 2;
		if (nextleft < nextright)
		{
			nextleft = nextright + 1;
		}
		set_help_mode(IDHELP_MAIN);
		i = full_screen_choice(CHOICE_MENU | CHOICE_CRUNCH,
			"MAIN MENU",
			0, 0, nextleft, (char **) choices, attributes,
			2, nextleft/2, 29, 0, 0, 0, 0, menu_check_key);
		if (i == -1)     // escape
		{
			i = IDK_ESC;
		}
		else if (i < 0)
		{
			i = -i;
		}
		else                      // user selected a choice
		{
			i = choicekey[i];
			if (-10 == i)
			{
				set_help_mode(IDHELP_ZOOM);
				help(ACTION_CALL);
				i = 0;
			}
		}
	}
	if (i == IDK_ESC)             // escape from menu exits
	{
		help_title();
		driver_set_attr(1, 0, C_GENERAL_MED, 24*80);
		for (i = 9; i <= 11; ++i)
		{
			driver_set_attr(i, 18, C_GENERAL_INPUT, 40);
		}
		put_string_center(10, 18, 40, C_GENERAL_INPUT,
			"Exit from Iterated Dynamics (y/n)? y");
		driver_hide_text_cursor();
		while ((i = driver_get_key()) != 'y' && i != 'Y' && i != 13)
		{
			if (i == 'n' || i == 'N')
			{
				goto top;
			}
		}
		goodbye();
	}
	if (i == IDK_TAB)
	{
		tab_display();
		i = 0;
	}
	if (i == IDK_ENTER || i == IDK_ENTER_2)
	{
		i = 0;                 // don't trigger new calc
	}
	g_tab_display_enabled = save_tab_display_enabled;
	return i;
}

static int menu_check_key(int curkey, int choice)
{ // choice is dummy used by other routines called by full_screen_choice()
	int testkey;
	testkey = choice; // for warning only
	testkey = (curkey >= 'A' && curkey <= 'Z') ? curkey + ('a'-'A') : curkey;
	if (testkey == '2')
	{
		testkey = '@';
	}
	if (strchr("#@2txyzgvir3dj", testkey) || testkey == IDK_INSERT || testkey == IDK_CTL_B
		|| testkey == IDK_ESC || testkey == IDK_DELETE || testkey == IDK_CTL_F)
	{
		return -testkey;
	}
	if (s_full_menu)
	{
		if (strchr("\\sobpkrh", testkey) || testkey == IDK_TAB
			|| testkey == IDK_CTL_A || testkey == IDK_CTL_E || testkey == IDK_BACKSPACE
			|| testkey == IDK_CTL_S || testkey == IDK_CTL_U) // ctrl-A, E, H, P, S, U
		{
			return -testkey;
		}
		if (testkey == ' ')
		{
			if ((!fractal_type_none(g_current_fractal_specific->tojulia)
					&& g_parameters[P1_REAL] == 0.0
					&& g_parameters[P1_IMAG] == 0.0)
				|| !fractal_type_none(g_current_fractal_specific->tomandel))
			{
				return -testkey;
			}
		}
		if (g_.RealDAC())
		{
			if (strchr("c+-", testkey))
			{
				return -testkey;
			}
			if (testkey == 'a' || (testkey == 'e'))
			{
				return -testkey;
			}
		}
		// Alt-A and Alt-S
		if (testkey == IDK_ALT_A || testkey == IDK_ALT_S )
		{
			return -testkey;
		}
	}
	if (check_video_mode_key(testkey) >= 0)
	{
		return -testkey;
	}
	return 0;
}

int input_field(
		int options,          // &1 numeric, &2 integer, &4 double
		int attr,             // display attribute
		char *fld,            // the field itself
		int len,              // field length (declare as 1 larger for \0)
		int row,              // display row
		int col,              // display column
		int (*check_keystroke)(int key)  // routine to check non data keys, or 0
		)
{
	char savefld[81];
	char buf[81];
	int offset;
	int curkey;
	int display;
	int i;
	int j;
	int ret;

	MouseModeSaver saved_mouse(LOOK_MOUSE_NONE);
	ret = -1;
	strcpy(savefld, fld);
	int insert = 0;
	bool started = false;
	offset = 0;
	display = 1;
	while (true)
	{
		strcpy(buf, fld);
		i = int(strlen(buf));
		while (i < len)
		{
			buf[i++] = ' ';
		}
		buf[len] = 0;
		if (display)  // display current value
		{
			driver_put_string(row, col, attr, buf);
			display = 0;
		}
		curkey = driver_key_cursor(row + insert, col + offset);  // get a keystroke
		if (curkey == 1047) // numeric slash
		{
			curkey = 47;
		}
		switch (curkey)
		{
		case IDK_ENTER:
		case IDK_ENTER_2:
			ret = 0;
			goto inpfld_end;
		case IDK_ESC:
			goto inpfld_end;
		case IDK_RIGHT_ARROW:
			if (offset < len-1)
			{
				++offset;
			}
			started = true;
			break;
		case IDK_LEFT_ARROW:
			if (offset > 0)
			{
				--offset;
			}
			started = true;
			break;
		case IDK_HOME:
			offset = 0;
			started = true;
			break;
		case IDK_END:
			offset = int(strlen(fld));
			started = true;
			break;
		case IDK_BACKSPACE:
		case 127:                              // backspace
			assert(127 != curkey);
			if (offset > 0)
			{
				j = int(strlen(fld));
				for (i = offset-1; i < j; ++i)
				{
					fld[i] = fld[i + 1];
				}
				--offset;
			}
			started = true;
			display = 1;
			break;
		case IDK_DELETE:
			j = int(strlen(fld));
			for (i = offset; i < j; ++i)
			{
				fld[i] = fld[i + 1];
			}
			started = true;
			display = 1;
			break;
		case IDK_INSERT:
			insert ^= 0x8000;
			started = true;
			break;
		case IDK_F5:
			strcpy(fld, savefld);
			insert = 0;
			started = false;
			offset = 0;
			display = 1;
			break;
		default:
			if (nonalpha(curkey))
			{
				if (check_keystroke)
				{
					ret = check_keystroke(curkey);
					if (ret != 0)
					{
						goto inpfld_end;
					}
				}
				break;                                // non alphanum char
			}
			if (offset >= len)                // at end of field
			{
				break;
			}
			if (insert && started && strlen(fld) >= (size_t)len)
			{
				break;                                // insert & full
			}
			if ((options & INPUTFIELD_NUMERIC)
					&& (curkey < '0' || curkey > '9')
					&& curkey != '+' && curkey != '-')
			{
				if (options & INPUTFIELD_INTEGER)
				{
					break;
				}
				// allow scientific notation, and specials "e" and "p"
				if (((curkey != 'e' && curkey != 'E') || offset >= 18)
						&& ((curkey != 'p' && curkey != 'P') || offset != 0 )
						&& curkey != '.')
				{
					break;
				}
			}
			if (!started) // first char is data, zap field
			{
				fld[0] = 0;
			}
			if (insert)
			{
				j = int(strlen(fld));
				while (j >= offset)
				{
					fld[j + 1] = fld[j];
					--j;
				}
			}
			if ((size_t)offset >= strlen(fld))
			{
				fld[offset + 1] = 0;
			}
			fld[offset++] = (char)curkey;
			// if "e" or "p" in first col make number e or pi
			if ((options & (INPUTFIELD_NUMERIC | INPUTFIELD_INTEGER)) == INPUTFIELD_NUMERIC)  // floating point
			{
				double tmpd;
				bool specialv = false;
				if (fld[0] == 'e' || fld[0] == 'E')
				{
					tmpd = std::exp(1.0);
					specialv = true;
				}
				if (fld[0] == 'p' || fld[0] == 'P')
				{
					tmpd = std::atan(1.0)*4.0;
					specialv = true;
				}
				if (specialv)
				{
					if ((options & INPUTFIELD_DOUBLE) == 0)
					{
						round_float_d(&tmpd);
					}
					strcpy(fld, boost::format("%.15g") % tmpd);
					offset = 0;
				}
			}
			started = true;
			display = 1;
		}
	}

inpfld_end:
	return ret;
}

static int line_count(std::string const &text)
{
	using namespace std;
	return 1 + int(count_if(text.begin(), text.end(), bind1st(equal_to<char>(), '\n')));
}

static void field_prompt_lines_boxwidth(std::string const &hdg, int len, int &titlelines, int &boxwidth)
{
	char const *charptr = hdg.c_str();                         // count title lines, find widest
	int lineLength = 0;
	boxwidth = 0;
	titlelines = 1;
	while (*charptr)
	{
		if (*(charptr++) == '\n')
		{
			++titlelines;
			lineLength = -1;
		}
		if (++lineLength > boxwidth)
		{
			boxwidth = lineLength;
		}
	}
	if (len > boxwidth)
	{
		boxwidth = len;
	}
}

int field_prompt(std::string const &hdg,
				 char *fld, int len, int (*check_keystroke)(int key))
{
	return field_prompt(hdg, "", fld, len, check_keystroke);
}

int field_prompt(std::string const &hdg, // heading, \n delimited lines
		std::string const &instr,		// additional instructions or 0
		char *fld,						// the field itself
		int len,						// field length (declare as 1 larger for \0)
		int (*check_keystroke)(int key) // routine to check non data keys, or 0
		)
{
	int boxwidth;
	int titlelines;
	int titlecol;
	int titlerow;
	int promptcol;
	int i;
	int j;
	char buf[81];
	help_title();                           // clear screen, display title
	driver_set_attr(1, 0, C_PROMPT_BKGRD, 24*80);     // init rest to background
	field_prompt_lines_boxwidth(hdg, len, titlelines, boxwidth);
	i = titlelines + 4;                    // total rows in box
	titlerow = (25 - i)/2;               // top row of it all when centered
	titlerow -= titlerow/4;              // higher is better if lots extra
	titlecol = (80 - boxwidth)/2;        // center the box
	titlecol -= (90 - boxwidth)/20;
	promptcol = titlecol - (boxwidth-len)/2;
	j = titlecol;                          // add margin at each side of box
	i = (82-boxwidth)/4;
	if (i > 3)
	{
		i = 3;
	}
	j -= i;
	boxwidth += i*2;
	for (i = -1; i < titlelines + 3; ++i)    // draw empty box
	{
		driver_set_attr(titlerow + i, j, C_PROMPT_LO, boxwidth);
	}
	g_text_cbase = titlecol;                  // set left margin for driver_put_string
	driver_put_string(titlerow, 0, C_PROMPT_HI, hdg); // display heading
	g_text_cbase = 0;
	i = titlerow + titlelines + 4;
	if (instr.length())  // display caller's instructions
	{
		char const *charptr = instr.c_str();
		j = -1;
		while ((buf[++j] = *(charptr++)) != 0)
		{
			if (buf[j] == '\n')
			{
				buf[j] = 0;
				put_string_center(i++, 0, 80, C_PROMPT_BKGRD, buf);
				j = -1;
			}
		}
		put_string_center(i, 0, 80, C_PROMPT_BKGRD, buf);
	}
	else                                   // default instructions
	{
		put_string_center(i, 0, 80, C_PROMPT_BKGRD, "Press ENTER when finished (or ESCAPE to back out)");
	}
	return input_field(0, C_PROMPT_INPUT, fld, len,
				titlerow + titlelines + 1, promptcol, check_keystroke);
}


/* thinking(1, message):
		if thinking message not yet on display, it is displayed;
		otherwise the wheel is updated
		returns 0 to keep going, -1 if keystroke pending
	thinking(0, 0):
		call this when thinking phase is done
	*/

int thinking(int options, char const *msg)
{
	static int thinkstate = -1;
	char *wheel[] = {"-", "\\", "|", "/"};
	static int thinkcol;
	static int count = 0;
	char buf[81];
	if (options == 0)
	{
		if (thinkstate >= 0)
		{
			thinkstate = -1;
			driver_unstack_screen();
		}
		return 0;
	}
	if (thinkstate < 0)
	{
		driver_stack_screen();
		thinkstate = 0;
		help_title();
		strcpy(buf, "  ");
		strcat(buf, msg);
		strcat(buf, "    ");
		driver_put_string(4, 10, C_GENERAL_HI, buf);
		thinkcol = g_text_col - 3;
		count = 0;
	}
	if ((count++) < 100)
	{
		return 0;
	}
	count = 0;
	driver_put_string(4, thinkcol, C_GENERAL_HI, wheel[thinkstate]);
	driver_hide_text_cursor(); // turn off cursor
	thinkstate = (thinkstate + 1) & 3;
	return driver_key_pressed();
}

int check_video_mode_key(int k)
{
	int i;
	// returns g_video_table entry number if the passed keystroke is a
	// function key currently assigned to a video mode, -1 otherwise
	if (k == 1400)              // special value from select_vid_mode
	{
		return MAXVIDEOMODES-1; // for last entry with no key assigned
	}
	if (k != 0)
	{
		for (i = 0; i < g_.VideoTableLength(); ++i)
		{
			if (g_.VideoTable(i).keynum == k)
			{
				return i;
			}
		}
	}
	return -1;
}

int check_vidmode_keyname(char const *kname)
{
	// returns key number for the passed keyname, 0 if not a keyname
	int i;
	int keyset;
	keyset = 1058;
	if (*kname == 'S' || *kname == 's')
	{
		keyset = IDK_SF1 - 1;
		++kname;
	}
	else if (*kname == 'C' || *kname == 'c')
	{
		keyset = IDK_CTL_F1 - 1;
		++kname;
	}
	else if (*kname == 'A' || *kname == 'a')
	{
		keyset = IDK_ALT_F1 - 1;
		++kname;
	}
	if (*kname != 'F' && *kname != 'f')
	{
		return 0;
	}
	if (*++kname < '1' || *kname > '9')
	{
		return 0;
	}
	i = *kname - '0';
	if (*++kname != 0 && *kname != ' ')
	{
		if (*kname != '0' || i != 1)
		{
			return 0;
		}
		i = 10;
		++kname;
	}
	while (*kname)
	{
		if (*(kname++) != ' ')
		{
			return 0;
		}
	}
	i += keyset;
	if (i < 2)
	{
		i = 0;
	}
	return i;
}