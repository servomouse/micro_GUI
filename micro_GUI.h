#pragma once

#include <stdlib.h>
#include <stdint.h>

// User-specific defines:
#define X_SOCKET_PATH 		"/tmp/.X11-unix/X1"
#define XAUTHORITY_PATH 	"/run/user/1000/gdm/Xauthority"
// ~User-specific defines

int create_window(int height, int width);
int update_window(void);
int window_get_dimensions(int * width, int * height);