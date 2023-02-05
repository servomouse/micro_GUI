#pragma once

#include <stdlib.h>
#include <stdint.h>

// User-specific defines:
#define X_SOCKET_PATH 		"/tmp/.X11-unix/X1"
#define XAUTHORITY_PATH 	"/run/user/1000/gdm/Xauthority"
// ~User-specific defines

int create_window(int height, int width);
int update_window(uint32_t width, uint32_t height, uint32_t *buf);
int update_image(uint32_t width, uint32_t height, uint32_t *buf, uint32_t x_off, uint32_t y_off);
int window_get_dimensions(int * width, int * height);
int window_get_position(int * x, int * y);
int window_move(int x, int y);