#include "menu.h"
#include <nds.h>
#include "ui.h"
#define FONT_WIDTH  6
#define FONT_HEIGHT 10
#include "nds_platform.h" 
#include "device.h"
#include <ctime>

#define bootmsg "\n WARNING: READ THIS BEFORE CONTINUING\n" 			\
				" ------------------------------------\n\n" 			\
				" If you don't know what you're doing: STOP\n" 			\
				" Open your browser to https://3ds.guide/\n" 			\
				" and follow the steps provided there.\n\n" 			\
				" This software writes directly to your\n" 				\
				" flashcart. It's possible you may BRICK \n"			\
				" your flashcart.\n\n ALWAYS KEEP A BACKUP\n\n" 		\
				" <A> CONTINUE  <B> POWEROFF"

using namespace flashcart_core;
using namespace ncgc;

int global_loglevel = 1; //https://github.com/ntrteam/flashcart_core/blob/master/platform.h#L6 

void print_boot_msg(void)
{
	ClearScreen(TOP_SCREEN, COLOR_RED);
	DrawString(TOP_SCREEN, 0, 0, COLOR_WHITE, bootmsg);

	while (true)
	{
		scanKeys();
		if (keysDown() & KEY_B)
		{
			exit(0);
		}
		if (keysDown() & KEY_A)
		{
			break;
		}
	}
}

void WaitPress(u32 KEY) {
	while (true) { scanKeys(); if (keysDown() & KEY) { break; } }
}

bool ntrCardReset()
{
	if (isDSiMode())
	{
		// Reset card slot
		disableSlot1();
		for(int i = 0; i < 25; i++) { swiWaitForVBlank(); }
		enableSlot1();
		for(int i = 0; i < 15; i++) { swiWaitForVBlank(); }
	}
	else
	{
		REG_ROMCTRL = 0;
		REG_AUXSPICNT = 0;
		for (int i = 0; i < 25; i++) swiWaitForVBlank();
		REG_AUXSPICNT = CARD_CR1_ENABLE | CARD_CR1_IRQ;
		REG_ROMCTRL = CARD_nRESET | CARD_SEC_SEED;
		while (REG_ROMCTRL & CARD_BUSY) ;
		cardReset();
		while (REG_ROMCTRL & CARD_BUSY) ;
	}
	return true;
}

void menu_lvl1(Flashcart* cart, bool isDevMode)
{
	u32 menu_sel = 0;
	
	NTRCard card(ntrCardReset);
	DrawHeader(TOP_SCREEN, "Select flashcart", ((SCREENWIDTH - (16 * FONT_WIDTH)) / 2));
	DrawInfo(global_loglevel);
	DrawHeader(BOTTOM_SCREEN, "Flashcart info", ((SCREENWIDTH - (14 * FONT_WIDTH)) / 2));
	DrawStringF(BOTTOM_SCREEN, FONT_WIDTH, FONT_HEIGHT * 2, COLOR_WHITE, "%s\n%s", flashcart_list->at(0)->getAuthor(), flashcart_list->at(0)->getDescription());
	u32 flashcart_list_size = flashcart_list->size();

	while (true) //This will be our MAIN loop
	{
		bool reprintFlag = false;

		for (u32 i = 0; i < flashcart_list_size; i++)
		{
			cart = flashcart_list->at(i);
			if (!isDSiMode() && strcmp(cart->getShortName(), "R4iSDHC.hk") == 0) // R4iSDHC.hk only support init from RAW, so hide it on DSMode
			{
				flashcart_list_size--;
				break; // because R4iSDHC.hk is the last one of flashcart_list
			}
			DrawString(TOP_SCREEN, FONT_WIDTH, ((i + 2) * FONT_HEIGHT), (i == menu_sel) ? COLOR_RED : COLOR_WHITE, cart->getName());
		}
		scanKeys();
		if (keysDown() & KEY_DOWN && menu_sel < (flashcart_list_size - 1))
		{
			menu_sel++;
			reprintFlag = true;
		}
		if (keysDown() & KEY_UP   && menu_sel > 0)
		{
			menu_sel--;
			reprintFlag = true;
		}
		if (keysDown() & KEY_Y) {
			if (global_loglevel == 4) {
				global_loglevel = 0; //if you scroll past the end it puts you back at the top
			}
			else {
				global_loglevel++;
			}
			DrawInfo(global_loglevel);
		}
		if (keysDown() & KEY_A)
		{
			cart = flashcart_list->at(menu_sel); //Set the cart equal to whatever we had selected from before
			if (isDSiMode() || strcmp(cart->getShortName(), "DSTT") == 0) card.init();
			else card.state(NTRState::Key2);
			if (!cart->initialize(&card)) //If cart initialization fails, do all this and then break to main menu
			{
				DrawString(TOP_SCREEN, FONT_WIDTH, 8 * FONT_HEIGHT, COLOR_RED, "Flashcart setup failed!\nPress <B>");
				while (true) { scanKeys(); if (keysDown() & KEY_B) { DrawHeader(TOP_SCREEN, "Select flashcart", ((SCREENWIDTH - (16 * FONT_WIDTH)) / 2)); DrawInfo(global_loglevel); break; } }
			}
			else
			{
				menu_lvl2(cart, isDevMode); //There is a while loop over at menu_lvl2(), the statements underneath won't get executed immediately
				DrawHeader(TOP_SCREEN, "Select flashcart", ((SCREENWIDTH - (16 * FONT_WIDTH)) / 2));
				DrawInfo(global_loglevel);
				reprintFlag = true;
			}
		}

		if (reprintFlag)
		{
			cart = flashcart_list->at(menu_sel);
			DrawHeader(BOTTOM_SCREEN, "Flashcart info", ((SCREENWIDTH - (14 * FONT_WIDTH)) / 2));
			DrawStringF(BOTTOM_SCREEN, FONT_WIDTH, FONT_HEIGHT * 2, COLOR_WHITE, "%s\n%s", cart->getAuthor(), cart->getDescription());
		}
	}
}

void menu_lvl2(Flashcart* cart, bool isDevMode)
{
	DrawHeader(TOP_SCREEN, cart->getName(), ((SCREENWIDTH - (strlen(cart->getName()) * FONT_WIDTH)) / 2));
	int menu_sel = 0;

	while (true)
	{
		scanKeys();
		DrawString(TOP_SCREEN, FONT_WIDTH, (2 * FONT_HEIGHT), (menu_sel == 0) ? COLOR_RED : COLOR_WHITE, "Inject FIRM");	//0
		DrawString(TOP_SCREEN, FONT_WIDTH, (3 * FONT_HEIGHT), (menu_sel == 1) ? COLOR_RED : COLOR_WHITE, "Inject GCD");		//1
		DrawString(TOP_SCREEN, FONT_WIDTH, (4 * FONT_HEIGHT), (menu_sel == 2) ? COLOR_RED : COLOR_WHITE, "Dump flash");		//2
		if(isDSiMode())
		{
			DrawString(TOP_SCREEN, FONT_WIDTH, (5 * FONT_HEIGHT), (menu_sel == 3) ? COLOR_RED : COLOR_WHITE, "Restore flash");	//3
		}

		if (keysDown() & KEY_DOWN && menu_sel < (isDSiMode() ? 3 : 2))
		{
			menu_sel++;
		}
		if (keysDown() & KEY_UP   && menu_sel > 0)
		{
			menu_sel--;
		}
		if (keysDown() & KEY_B)
		{
			break;
		}
		int ntrboot_return = 0;
		
		if (keysDown() & KEY_A)
		{
			const char *menu_strings[] = {
				"About to inject FIRM!\nEnter button combination to proceed:",
				"About to inject GCD!\nEnter button combination to proceed:",
				"About to dump flash!\nEnter button combination to proceed:",
				"About to restore flash!\nEnter button combination to proceed:"
			};

			DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (8 * FONT_HEIGHT), COLOR_WHITE, menu_strings[menu_sel]);
			if (d0k3_buttoncombo(10 * FONT_WIDTH, 12 * FONT_HEIGHT))
			{
				ClearScreen(BOTTOM_SCREEN, COLOR_BLACK);
				if (menu_sel == 0) {
					ntrboot_return = InjectFIRM(cart, false, isDevMode);
				} else if (menu_sel == 1) {
					ntrboot_return = InjectFIRM(cart, true, isDevMode);
				} else if (menu_sel == 2) {
					ntrboot_return = DumpFlash(cart);
				} else if (menu_sel == 3) {
					ntrboot_return = RestoreFlash(cart);
				}

				switch (ntrboot_return) {
					case FAT_MOUNT_FAILED:
						DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (15 * FONT_HEIGHT), COLOR_RED, "Failed to mount fat!\nPress <B> to return to menu...");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case FILE_OPEN_FAILED:
						DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (15 * FONT_HEIGHT), COLOR_RED, "Failed to open file!\nPress <B> to return to menu...");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case FILE_IO_FAILED:
						DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (15 * FONT_HEIGHT), COLOR_RED, (menu_sel == 2) ? "Failed to write file!\nPress <B> to return to menu..." : "Failed to read file!\nPress <B> to return to menu...");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case INJECT_OR_DUMP_FAILED:
						{
							const char *err_strings[] = {
								"Failed to inject FIRM!\nPress <B> to return to menu...",
								"Failed to inject GCD!\nPress <B> to return to menu...",
								"Failed to dump flash!\nPress <B> to return to menu...",
								"Failed to restore flash!\nPress <B> to return to menu..."
							};

							DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (15 * FONT_HEIGHT), COLOR_RED, err_strings[menu_sel]);
							WaitPress(KEY_B);
							ClearScreen(TOP_SCREEN, COLOR_BLACK);
						}
						break;

					case NO_BACKUP_FOUND:
						DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (15 * FONT_HEIGHT), COLOR_RED, "No backup file found!\nMake sure you dumped the flash first!\nPress <B> to return to menu...");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case ALL_OK:
						DrawString(TOP_SCREEN, (1 * FONT_WIDTH), (15 * FONT_HEIGHT), COLOR_GREEN, "Success! Press <A> to return to main menu");
						WaitPress(KEY_A);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						ClearScreen(BOTTOM_SCREEN, COLOR_BLACK);
						break;
					}
			}
			else
			{
				DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (14 * FONT_HEIGHT), COLOR_WHITE, "Stopped by user...\nPress <B> to return to main menu");
				WaitPress(KEY_B);
			}
			break;
		}
	}
}

//Will print out a gm9/sb9si like "input button combo to continue" prompt, also does the button checking for it
//Requires #include <ctime> for the rand() seed

const char rancombo_symbols[5] = { '\x1B', '\x18', '\x1A', '\x19', 'A' }; // Left, Up, Right, Down
const u32 rancombo_inputs[5] = { KEY_LEFT, KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_A };

bool d0k3_buttoncombo(int cur_c, int cur_r)
{
	srand(time(NULL));
	int num_rancombo[5] = { (rand() % 4), (rand() % 4), (rand() % 4), (rand() % 4), 4 }; // zero based, '4' is the 5th item (A)
	char print_rancombo[5] = { ' ', ' ', ' ', ' ', ' ' };
	u32 check_rancombo[5] = { 0, 0, 0, 0, 0 };
	for (int i = 0; i < 5; i++) {
		print_rancombo[i] = rancombo_symbols[num_rancombo[i]];
	}
	for (int i = 0; i < 5; i++) {
		check_rancombo[i] = rancombo_inputs[num_rancombo[i]];
	}
	int depth = 0; //How far in we are, how many buttons have been pressed so far, how *deep* we are

	while (true) {
		int temp_c = cur_c;
		u16 cur_color = COLOR_GREEN;
		for (int i = 0; i < 5; i++) {
			if (i >= depth) { cur_color = COLOR_WHITE; }
			d0k3_buttoncombo_print_chars(temp_c, cur_r, cur_color, print_rancombo[i]);
			temp_c += 4 * FONT_WIDTH; //3 for our printout ('<', 'arrow', '>'), and one for the space that follows it
		}

		scanKeys();
		if (keysDown()) {
			if (keysDown() & check_rancombo[depth]) {
				depth++;
			}
			else if (keysDown() & KEY_B) {
				return false;
			}
			else {
				depth = 0;
			}
		}

		// this is sorta hacky but otherwise the A button doesnt go green
		if (depth == 5) {
			depth++;
		}
		else if (depth == 6) {
			return true;
		}
	}
}

void d0k3_buttoncombo_print_chars(int collumn, int row, u16 color, char character)
{
	DrawCharacter(TOP_SCREEN, '<', collumn, row, color);
	collumn += FONT_WIDTH;
	DrawCharacter(TOP_SCREEN, character, collumn, row, color);
	collumn += FONT_WIDTH;
	DrawCharacter(TOP_SCREEN, '>', collumn, row, color);
}
