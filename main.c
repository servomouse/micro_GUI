#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 

#include "micro_GUI.h"

int main(void)
{
	create_window(320, 320);
    int w = 0, h = 0;
    window_get_dimensions(&w, &h);
    sleep(1);
    printf("w = %d, h = %d\n", w, h);

    while (1)
	{
        update_window();
        usleep(10000);
    }
}