#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 

#include "micro_GUI.h"

int main(void)
{
	create_window(320, 320);

    while (1)
	{
        update_window();
        usleep(10000);
    }
}