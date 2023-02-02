#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 

#include "micro_GUI.h"

int main(void)
{
	init_gui();
	create_window(320, 320);

    while (1)
	{
        update_window();
        usleep(10000);
    }
}