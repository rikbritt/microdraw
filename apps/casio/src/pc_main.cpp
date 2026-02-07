#include "microdraw.h"

extern void app_setup();
extern void app_loop();

int main(int argc, char* argv[])
{
	app_setup();

	bool run = true;

	while (run)
	{
		app_loop();

		run = md_exit_raised() == false;
	}

	md_deinit();
	return 0;
}