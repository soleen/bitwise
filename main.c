/* Copyright 2019
 * Ramon Fried <ramon.fried@gmail.com
 */

#include <stdlib.h>
#include <form.h>
#include <ctype.h>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "bitwise.h"

#define BIT(nr) (1UL << (nr))
/*

 01010101 | 01010101|
	 32  
*/
#define MAX_DEC_DIGITS 19
#define MAX_HEX_DIGITS 16
#define MAX_OCTA_DIGITS 32

#ifdef TRACE
FILE *fd;
#endif

#if 1
static FIELD *field[5];
static FORM  *form;
static uint64_t val;

WINDOW *fields_win;
WINDOW *binary_win;
char binary_field[128];
int base[4] = {
	10,
	16,
	8,
	2,
};


static void update_binary()
{
	int i;
	int pos = 0;

	for (i = 63; i >= 0; i--) {
		if (val & BIT(i))
			binary_field[pos] = '1';
		else
			binary_field[pos] = '0';
		binary_field[pos+1] = ' ';
		pos+=2;
	}
	binary_field[pos] = '\0';

	mvwprintw(binary_win, 1, 2, "%s", binary_field);
	wrefresh(binary_win);
}

static void update_fields(int index)
{
	FIELD *tmp_field = current_field(form);
	int *cur_base = field_userptr(tmp_field);
	char *buffer;
	char number[64];
	int *base;

	buffer = field_buffer(tmp_field, 0);
	assert(buffer);
	val = strtoll(buffer, NULL, *cur_base);
	LOG("Val = %lu\n", val);
	for (int i=0; i < 3; i++) {
		if (i == index)
			continue;
		base = field_userptr(field[i]);
		lltostr(val, number, *base);
		LOG("updating field %d\n", i);
		set_field_buffer(field[i], 0, number);
	}
	form_driver(form, REQ_VALIDATION);
	wrefresh(fields_win);
	refresh();
}

void set_active_field()
{
	set_field_fore(current_field(form), COLOR_PAIR(1));/* Put the field with blue background */
	set_field_back(current_field(form), COLOR_PAIR(2));/* and white foreground (characters */

	for (int i=0; i < 3; i++) {
		if (field[i] == current_field(form))
			continue;
		set_field_fore(field[i], COLOR_PAIR(0));
		set_field_back(field[i], COLOR_PAIR(0));
	}
}
						/* are printed in white 	*/


int main(int argc, char *argv[])
{

	int ch, rows, cols;
	int rc;

#ifdef TRACE
	fd = fopen("log.txt", "w");
#endif
	init_terminal();
	refresh();


/* Initialize the fields */
	field[0] = new_field(1, MAX_DEC_DIGITS, 1,
				10, 0, 0);

	field[1] = new_field(1, MAX_HEX_DIGITS, 1,
				40, 0, 0);
	field[2] = new_field(1, MAX_OCTA_DIGITS, 1,
				70, 0, 0);
	field[3] = NULL;
/*
	field[3] = new_field(1, 15, 10, 20, 0, 0);
	field[4] = NULL;
*/
	//for (int i=0; i < 4; i++) {
	for (int i=0; i < 3; i++) {
		set_field_back(field[i], A_UNDERLINE);
		field_opts_off(field[i], O_AUTOSKIP );
		set_field_buffer(field[i], 0, "0");
		set_field_userptr(field[i], &base[i]);
	}

	init_pair(1, COLOR_BLUE, COLOR_BLACK);
	init_pair(2, COLOR_BLUE, COLOR_BLACK);



	//refresh();

	form = new_form(field);
	if (!form)
		die("new form failed\n");

	scale_form(form, &rows, &cols);

	fields_win = newwin(rows + 3, cols + 3, 0, 4);
	keypad(fields_win, TRUE);

	binary_win = newwin(4, COLS - 1, 8,1);
	box(binary_win, 0, 0);

	rc = set_form_win(form, fields_win);
	if (rc != E_OK)
		die("set_form_win failed\n");

	rc = set_form_sub(form, derwin(fields_win, rows, cols, 2, 2));
	if (rc != E_OK)
		die("set_form_sub failed\n");

	set_current_field(form, field[0]);
	set_active_field();

	box(fields_win, 0, 0);
	mvwprintw(fields_win, 1, 10, "Decimal:");
	mvwprintw(fields_win, 1, 40, "Hexdecimal:");
	mvwprintw(fields_win, 1, 70, "Octal:");

	wrefresh(fields_win);
	update_binary();
	refresh();

	rc = post_form(form);
	if (rc != E_OK)
		die("post_form failed: %d\n", rc);

	int times = 0;

	while((ch = wgetch(fields_win)) != KEY_F(1)) {
		LOG("times: %u ch= %d\n", times++, ch);

		switch(ch) {
		case KEY_RIGHT:
			LOG("Key right\n");
			/* Go to next field */
			form_driver(form, REQ_NEXT_FIELD);
			/* Go to the end of the present buffer */
			/* Leaves nicely at the last character */
			form_driver(form, REQ_END_LINE);
			set_active_field();
			wrefresh(fields_win);
//			refresh();
			break;
		case KEY_LEFT:
			LOG("Key left\n");
			/* Go to previous field */
			form_driver(form, REQ_PREV_FIELD);
			form_driver(form, REQ_END_LINE);
			set_active_field();
			wrefresh(fields_win);
//			refresh();
			break;
		case KEY_BACKSPACE:
		case 127:
			LOG("Backspace\n");
			form_driver(form, REQ_DEL_PREV);
			form_driver(form, REQ_VALIDATION);
			update_fields(field_index(current_field(form)));
			update_binary();
			break;
		default:
			{
				FIELD *tmp_field = current_field(form);
				int *cur_base = field_userptr(tmp_field);

				LOG("default char\n");

				if (validate_input(ch, *cur_base))
					break;

				form_driver(form, ch);
				form_driver(form, REQ_VALIDATION);
				update_fields(field_index(tmp_field));
				update_binary();
				break;
			}
		}
//		wrefresh(fields_win);
		refresh();
	}
	getch();
	unpost_form(form);
	free_form(form);
	for (int i=0; i < 4; i++)
		free_field(field[i]);

	fclose(fd);
	deinit_terminal();

	return 0;
}

#else

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);

int main()
{
	FIELD *field[3];
	FORM  *my_form;
	WINDOW *my_form_win;
	int ch, rows, cols;
	
	/* Initialize curses */
	initscr();
	start_color();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	/* Initialize few color pairs */
   	init_pair(1, COLOR_RED, COLOR_BLACK);

	/* Initialize the fields */
	field[0] = new_field(1, 10, 6, 1, 0, 0);
	field[1] = new_field(1, 10, 8, 1, 0, 0);
	field[2] = NULL;

	/* Set field options */
	set_field_back(field[0], A_UNDERLINE);
	field_opts_off(field[0], O_AUTOSKIP); /* Don't go to next field when this */
					      /* Field is filled up 		*/
	set_field_back(field[1], A_UNDERLINE); 
	field_opts_off(field[1], O_AUTOSKIP);
	
	/* Create the form and post it */
	my_form = new_form(field);
	
	/* Calculate the area required for the form */
	scale_form(my_form, &rows, &cols);

	/* Create the window to be associated with the form */
        my_form_win = newwin(rows + 4, cols + 4, 4, 4);
        keypad(my_form_win, TRUE);

	/* Set main window and sub window */
        set_form_win(my_form, my_form_win);
        set_form_sub(my_form, derwin(my_form_win, rows, cols, 2, 2));

	/* Print a border around the main window and print a title */
        box(my_form_win, 0, 0);
	print_in_middle(my_form_win, 1, 0, cols + 4, "My Form", COLOR_PAIR(1));
	
	post_form(my_form);
	wrefresh(my_form_win);

	mvprintw(LINES - 2, 0, "Use UP, DOWN arrow keys to switch between fields");
	refresh();

	/* Loop through to get user requests */
	while((ch = wgetch(my_form_win)) != KEY_F(1))
	{	switch(ch)
		{	case KEY_DOWN:
				/* Go to next field */
				form_driver(my_form, REQ_NEXT_FIELD);
				/* Go to the end of the present buffer */
				/* Leaves nicely at the last character */
				form_driver(my_form, REQ_END_LINE);
				break;
			case KEY_UP:
				/* Go to previous field */
				form_driver(my_form, REQ_PREV_FIELD);
				form_driver(my_form, REQ_END_LINE);
				break;
			default:
				/* If this is a normal character, it gets */
				/* Printed				  */	
				form_driver(my_form, ch);
				break;
		}
	}

	/* Un post form and free the memory */
	unpost_form(my_form);
	free_form(my_form);
	free_field(field[0]);
	free_field(field[1]); 

	endwin();
	return 0;
}

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color)
{	int length, x, y;
	float temp;

	if(win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if(startx != 0)
		x = startx;
	if(starty != 0)
		y = starty;
	if(width == 0)
		width = 80;

	length = strlen(string);
	temp = (width - length)/ 2;
	x = startx + (int)temp;
	wattron(win, color);
	mvwprintw(win, y, x, "%s", string);
	wattroff(win, color);
	refresh();
}
#endif
