/*
 * This file implements the Game class.
 */
#include <cctype>
#include <cstring>
#include <algorithm>
#include "game.h"
#include <string>
#include <time.h> 
#include <stdlib.h>
using namespace std;

/*
 * Constructor
 * 
 * @param hero:  the initial state of the player
 */
Game::Game() : hero(1000, 1000, false, 0, 0)
{
	// init vars
	cursorRow = 0;
	cursorCol = 0;
	mapWindowRows = 0;
	mapWindowCols = 0;
	mapRow = 0;
	mapCol = 0;
}

/*
 * Destructor 
 */
Game::~Game()
{
	cursorRow = cursorCol = 0;	
}

/*
 * Play
 * 
 * Main function that drives game logic, called in driver code.
 * 
 * @param debug:  sets map to debug mode on true
 */
void Game::play(bool debug)
{
	map.set_debug(debug);

	// These are initialized to zero - map.load initializes with starting location
  int playerY = 0; 
  int playerX = 0;
  std::string mapName;

  // Print text
  std::cout << "Collect all 4 Royal Diamonds to win!" << endl;
  std::cout << "Run out of energy to lose!" << endl;
  std::cout << "Use the cursor to scroll, and WASD to move." << endl;
  std::cout << "Quit at anytime by typing a lowercase 'q'." << endl;
  std::cout << "Enter the name of the map file" << endl;
  std::cout << "Note: 'Map1.txt' is a valid map name" << endl;
  std::cin >> mapName;

	initscr(); //Initializes ncurses
	noecho(); // Prevents characters from being typed
	cbreak();
	curs_set(2);

	//Initializes two windows for map and menu
	// Map menu has LINES rows and COLS - 20 (menu is 20 cols)
	WINDOW * mapWindow = newwin(min(LINES, 128), min(COLS - 20, 128), 0, 0);
	getmaxyx(mapWindow, mapWindowRows, mapWindowCols);

  WINDOW * menuWindow = newwin(LINES, 20, 0, COLS - 20);

	keypad(mapWindow, true); // Allows all keypad keys to be recognized

	if (has_colors() == false)
	{
		endwin();
		// printf("Color isn't supported on this terminal.\n");
		exit(-1);
	}

	start_color();
	init_pair(MEADOW_PAIR, COLOR_BLACK, COLOR_GREEN);
	init_pair(WATER_PAIR, COLOR_BLACK, COLOR_BLUE);
	init_pair(SWAMP_PAIR, COLOR_BLACK, COLOR_MAGENTA);
	init_pair(WALL_PAIR, COLOR_BLACK, COLOR_WHITE);
	init_pair(EMPTY_PAIR, COLOR_WHITE, COLOR_BLACK);
	init_pair(PLAYER_PAIR, COLOR_YELLOW, COLOR_RED);
	init_pair(DIAMOND_PAIR, COLOR_WHITE, COLOR_CYAN);

	int ch = 0; //stores user input

	// load map file into array
	if(!map.load(playerY, playerX, mapName))
  {
    delwin(mapWindow);
    delwin(menuWindow);
    endwin();
    std::cout << endl << "ERROR: " << mapName << " is not a valid file!" << endl << endl;
    exit(-1);
  }

	// calculate starting window location based on player (try to center player if possible)
	// calculate y
	if (playerY < mapWindowRows)
	{
		mapRow = 0;
	}
	else if (playerY >= 127 - mapWindowRows)
	{
		mapRow = 127 - mapWindowRows;
	}
	else
	{
		// else, center vertically
		mapRow = playerY - (mapWindowRows / 2);
	}

	// calculate x
	if (playerX < mapWindowCols)
	{
		mapCol = 0;
	}
	else if (playerX >= 127 - mapWindowCols)
	{
		mapRow = 127 - mapWindowCols;
	}
	else
	{
		mapCol = playerX - (mapWindowCols / 2);
	}

	// Render map, player, and menu when the game starts up
	map.mark_visible(playerY, playerX, hero);
	map.render(mapWindow, mapRow, mapCol); // this will render plain terrain!
	render_player(mapWindow, playerY, playerX);
	wmove(mapWindow, cursorRow, cursorCol);

  curGrov = map.mapGetter(cursorRow + mapRow, cursorCol + mapCol); 
  menu.update(&curGrov, &hero);
	menu.render(menuWindow);

	while (ch != 'q' && !hero.dead() && hero.ret_diamonds() < 4) // Runs until 'q' is entered or hero is dead
	{
		ch = wgetch(mapWindow); // Gets character
		switch(ch)
		{
			case KEY_LEFT:
				if (cursorCol == 0) // check if window position is zero. If true, you are at the left edge of map and can't move
				{
					if (mapCol != 0)
						--mapCol; // shift the window left across the array
				}
				else
					--cursorCol; // otherwise, just decrement the cursor
				break;
			case KEY_RIGHT:
				if(cursorCol == mapWindowCols - 1)
				{
					if (mapCol != 128 - mapWindowCols) // can move right
						++mapCol;
				}
				else
					++cursorCol;
				break;
			case KEY_UP:
				if (cursorRow == 0)
				{
					if (mapRow != 0) // can move up
						--mapRow;
				}
				else
					--cursorRow; // decrement cursor
				break;
			case KEY_DOWN:
				if (cursorRow == mapWindowRows - 1)
				{
					if (mapRow != 128 - mapWindowRows) // can move down
						++mapRow;
				}
				else
					++cursorRow;
				break;
			default:
				map.player_move(ch, playerY, playerX, hero);
				handle_item(mapWindow, menuWindow, playerY, playerX, hero);

				break;
        
		}

		map.mark_visible(playerY, playerX, hero);
		map.render(mapWindow, mapRow, mapCol); // this will render plain terrain!
		// render player, if needed
		render_player(mapWindow, playerY, playerX);

		curGrov = map.mapGetter(cursorRow + mapRow, cursorCol + mapCol); 
		menu.update(&curGrov, &hero);
		menu.render(menuWindow);

		wmove(mapWindow, cursorRow, cursorCol);
		wnoutrefresh(menuWindow);

	} // While End

	delwin(mapWindow);	
	delwin(menuWindow);

  if(hero.dead() == true)
  {
    // Create Death Screen
    WINDOW * deathWindow = newwin(min(LINES, 148), min(COLS, 148), 0, 0);
    getmaxyx(deathWindow, mapWindowRows, mapWindowCols); // Now that both windows are gone, we can reuse mapWindowRows/Cols 
   
    // Print text in center of screen
    mvprintw((mapWindowRows / 2) - 2, (mapWindowCols -strlen("You have run out of energy.")) / 2, "You have run out of energy.");
    mvprintw((mapWindowRows / 2) - 1, (mapWindowCols - strlen("You fall to the ground, unable to move on.")) / 2, "You fall to the ground, unable to move on.");
    mvprintw((mapWindowRows / 2), (mapWindowCols - strlen("Your corpse's whiffles will be stolen by future travellers.")) / 2, "Your corpse's whiffles will be stolen by future travellers.");
    mvprintw((mapWindowRows / 2) + 1, (mapWindowCols - strlen("Press q to quit, so you may start a new hero's adventure.")) / 2, "Press q to quit, so you may start a new hero's adventure.");

    while (ch != 'q') // Runs until 'q' is entered or hero is dead
    {
      ch = getch(); // Gets character
    }
    delwin(deathWindow);
  }

  else if(hero.ret_diamonds() == 4)
  {
    // Create Win Screen
    WINDOW * winWindow = newwin(min(LINES, 148), min(COLS, 148), 0, 0);
    getmaxyx(winWindow, mapWindowRows, mapWindowCols); // Now that both windows are gone, we can reuse mapWindowRows/Cols
    // Print text in center of screen
    mvprintw((mapWindowRows / 2) - 2, (mapWindowCols -strlen("You have found all 4 diamonds!")) / 2, "You have found all 4 diamonds!");
    mvprintw((mapWindowRows / 2) - 1, (mapWindowCols - strlen("Energy: Infinite.")) / 2, "Energy: Infinite.");
    mvprintw((mapWindowRows / 2), (mapWindowCols - strlen("Whiffles: One Zillion Zillion")) / 2, "Whiffles: One Zillion Zillion");
    mvprintw((mapWindowRows / 2) + 1, (mapWindowCols - strlen("Press q to end the game.")) / 2, "Press q to end the game.");
    while (ch != 'q') // Runs until 'q' is entered or hero is dead
    {
      ch = getch(); // Gets character
    }
    delwin(winWindow);
  }
	endwin(); // Ends ncurses
	return;
}

/*
 * Render player
 * 
 * Determines if player is in the current window, renders the player if so
 * 
 * @param map_window:  window to render on
 * @param player_row:  player's row coordinate 0 - 127
 * @param player_col:  player's col coordinate 0 - 127
 */
void Game::render_player(WINDOW * & map_window, int player_row, int player_col)
{
	// check if player is in the window
	if (player_row >= mapRow  && player_row < mapRow + mapWindowRows && player_col >= mapCol && player_col < mapCol + mapWindowCols)
	{
		// player is in the window, render the player
		wattron(map_window, COLOR_PAIR(PLAYER_PAIR));
		mvwaddch(map_window, player_row - mapRow, player_col - mapCol, '@');
		wattroff(map_window, COLOR_PAIR(PLAYER_PAIR));
	}
}

/*
 * Handle Item
 * 
 * Handles item location based on the item at the current grovnik
 */
void Game::handle_item(WINDOW* mapWindow, WINDOW* menuWindow, int player_row, int player_col, player& hero)
{	
  bool destroy = false;
  bool use = false;
  bool ax = false; // used for detemine axe use
  bool pick = false; // used to determine pickaxe use

  Item* i;
  Axe* a;
  PickAxe* p;
  Food* f;
  Tree* t;
  Rock* r;

  //int energy;
  Grovnik & current = map.mapGetter(player_row, player_col);
  switch(current.object)
  {
    case 'F':
      i = current.item;
      f = dynamic_cast<Food*>(i);
      menu.prompt_buy(menuWindow, current.item);
      destroy = prompt(mapWindow);
      if(destroy && (f->get_cost() < hero.ret_whiffle()))
      {
        hero.remove_whiffle(current.item -> get_cost());
        hero.add_energy(f->ret_energy());
      }
      else
        destroy = false;

      f = NULL;
      i = NULL;
      break;

    // Axe Item
    case 'A':
      i = current.item;
      a = dynamic_cast<Axe*>(i);
      menu.prompt_buy(menuWindow, current.item);
      destroy = prompt(mapWindow);
      if(destroy && (a->get_cost() < hero.ret_whiffle()))
      {
        hero.remove_whiffle(a -> get_cost());
	      hero.pickup_axe();
      }
      else
        destroy = false;

      a = NULL;   
      i = NULL;
      break;

    // PickAxe item
    case 'P':
      i = current.item;
      p = dynamic_cast<PickAxe*>(i);
      menu.prompt_buy(menuWindow, current.item);
      destroy = prompt(mapWindow);
      if(destroy && (p->get_cost() < hero.ret_whiffle()))
      {
        hero.remove_whiffle(p -> get_cost());
	      hero.pickup_pick_axe();
      }
      else
        destroy = false;

      p = NULL;
      i = NULL;
      break;

    //Tree Obstacle
    case 'T':
      i = current.item;
      t = dynamic_cast<Tree*>(i);
      ax = hero.has_axe();

      if(ax)
      {
        menu.prompt_use(menuWindow, current.item);
        use = prompt(mapWindow);
      }

      if(use)
        hero.use_axe(t->get_energy_cost(), 2);
      else
        hero.remove_energy(t->get_energy_cost());

      destroy = true;
      i = nullptr;
      t = nullptr;
      break;

    case 'R':
      i = current.item;
      r = dynamic_cast<Rock*>(i);
      pick = hero.has_pick_axe();

      if(pick)
      {
        menu.prompt_use(menuWindow, current.item);
        use = prompt(mapWindow);     
      }

      if(use)
        hero.use_pick_axe(r->get_energy_cost(), 2);
      else
        hero.remove_energy(r->get_energy_cost());

      destroy = true;
      i = nullptr; 
      r = nullptr;
      break;

    case 'D':
      hero.add_diamonds(); // Edit player value
      destroy = true; // Set destroy
      break;
    case 'S':
      //destroy = true;
      menu.prompt_buy(menuWindow, current.item);
      destroy = prompt(mapWindow);
      if(destroy && (hero.ret_whiffle() > current.item-> get_cost()))
      {
        hero.add_ship();
        hero.remove_whiffle(current.item -> get_cost());
      }
      else
        destroy = false;
      break;
    case 'B':
      menu.prompt_buy(menuWindow, current.item);
      destroy = prompt(mapWindow);
      if(destroy && (hero.ret_whiffle() > current.item-> get_cost()))
      {
        hero.add_binoculars();
        hero.remove_whiffle(current.item -> get_cost());
      }
      else
        destroy = false;
      break;
    case '$': // Treasure
      destroy = true;
      hero.add_whiffle(rand() % 500);
      break;
  }

  if (destroy)
  {
    map.clear(player_row, player_col); // clear the object and delete the item at player location
  }
}

/*
 * Prompt
 * 
 * Helper function to get user input when required (ie buying a food/item)
 * 
 * @param win:  window to get input from
 */
bool Game::prompt(WINDOW* win)
{
  int input;

  while(1)
  {
    input = wgetch(win);

    if(input == 'Y' || input == 'y')
    {
      return true;
    }
    else if(input == 'N' || input == 'n')
    {
      return false;
    }
  }
}