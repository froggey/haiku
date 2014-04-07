/*
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include <boot/menu.h>
#include <boot/platform/generic/text_menu.h>
#include <safemode.h>


static void
warm_reboot(char key)
{
	
}


void
platform_add_menus(Menu* menu)
{
	// we don't have any platform specific menus at present
}


void
platform_update_menu_item(Menu* menu, MenuItem* item)
{
	platform_generic_update_text_menu_item(menu, item);
}


void
platform_run_menu(Menu* menu)
{
	platform_generic_run_text_menu(menu);
}


size_t
platform_get_user_input_text(Menu* menu, MenuItem* item, char* buffer,
	size_t bufferSize)
{
	return platform_generic_get_user_input_text(menu, item, buffer,
		bufferSize);
}
