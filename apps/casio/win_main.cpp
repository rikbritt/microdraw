#include "microdraw.h"

extern void setup();
extern void loop();

int main(int argc, char* argv[])
{
    setup();
    while (run)
    {
        run = md_exit_raised() == false;
        loop();
    }

    md_deinit();
    return 0;
}