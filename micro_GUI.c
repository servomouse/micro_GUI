#include "micro_GUI.h"
#include "micro_GUI_defines.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>   // for getting file info

// currently it is just a copy of the source code from https://github.com/abainbridge/x11_socket 
// with some clean up to make the code more readable for me


#define CRITICAL_ERROR	{printf("%s, %s, %d\n", __func__, __FILE__, __LINE__); exit(0);}

#define RED "\e[0;31m"
#define NC "\e[0m"

#define COOKIE_LEN 16

#define X11_CW_EVENT_MASK 				1<<11
#define X11_EVENT_MASK_KEY_PRESS 		1
#define X11_EVENT_MASK_POINTER_MOTION 	1<<6

typedef struct __attribute__((packed))
{
    uint8_t order;
    uint8_t pad1;
    uint16_t major_version, minor_version;
    uint16_t auth_proto_name_len;
    uint16_t auth_proto_data_len;
    uint16_t pad2;
} connection_request_t;

typedef struct __attribute__((packed))
{
    uint32_t root_id;
    uint32_t colormap;
    uint32_t white, black;
    uint32_t input_mask;
    uint16_t width, height;
    uint16_t width_mm, height_mm;
    uint16_t maps_min, maps_max;
    uint32_t root_visual_id;
    uint8_t backing_store;
    uint8_t save_unders;
    uint8_t depth;
    uint8_t allowed_depths_len;
} screen_t;

typedef struct __attribute__((packed))
{
    uint8_t depth;
    uint8_t bpp;
    uint8_t scanline_pad;
    uint8_t pad[5];
} pixmap_format_t;

typedef struct __attribute__((packed))
{
    uint32_t release;
    uint32_t id_base, id_mask;
    uint32_t motion_buffer_size;
    uint16_t vendor_len;
    uint16_t request_max;
    uint8_t num_screens;
    uint8_t num_pixmap_formats;
    uint8_t image_byte_order;
    uint8_t bitmap_bit_order;
    uint8_t scanline_unit, scanline_pad;
    uint8_t keycode_min, keycode_max;
    uint32_t pad;
    char vendor_string[1];
} connection_reply_success_body_t;

typedef struct __attribute__((packed))
{
    uint8_t success;
    uint8_t pad;
    uint16_t major_version, minor_version;
    uint16_t len;
} connection_reply_header_t;

typedef struct __attribute__((packed))
{
    uint8_t group;
    uint8_t bits;
    uint16_t colormap_entries;
    uint32_t mask_red, mask_green, mask_blue;
    uint32_t pad;
} visual_t;

typedef struct
{
    int socket_fd;

    connection_reply_header_t connection_reply_header;
    connection_reply_success_body_t *connection_reply_success_body;
    pixmap_format_t *pixmap_formats;
    screen_t *screens;
    uint32_t next_resource_id;
    uint32_t graphics_context_id;
    uint32_t window_id;
} state_t;

static state_t * window;

static void soc_write(int fd, const void *buf, size_t count)
{
    if(write(fd, buf, count) != count)
        CRITICAL_ERROR;
}

static void soc_read(int fd, void *buf, size_t count)
{
    if(recvfrom(fd, buf, count, 0, NULL, NULL) != count)
        CRITICAL_ERROR;
}

// Read Xauthority cookie
uint8_t * read_magic_cookie(void)
{
    static uint8_t cookie[COOKIE_LEN] = {0};
    struct stat st;
	FILE *xauth_file = fopen(XAUTHORITY_PATH, "rb");
    if(!xauth_file)
    {
        fprintf(stderr, RED "ERROR: can't open " XAUTHORITY_PATH " !" NC);
        exit(EXIT_FAILURE); // this is, in fact, critical error
    }
    fseek(xauth_file, -1L * COOKIE_LEN, SEEK_END); 
    int len = fread(cookie, 1, COOKIE_LEN, xauth_file); // Read last 16 bytes
    if(16 != len)
    {
        fprintf(stderr, RED "ERROR: can't read " XAUTHORITY_PATH " !" NC);
        exit(EXIT_FAILURE); // this is, in fact, critical error
    }
    return cookie;
}

void x11_init(state_t *state)
{
    // Open socket and connect.
    state->socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (state->socket_fd < 0)
        CRITICAL_ERROR;
	
    struct sockaddr_un serv_addr = {0};
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, X_SOCKET_PATH);
    if (connect(state->socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        CRITICAL_ERROR;

    char * magic_cookie = read_magic_cookie();

    // Send connection request.
    connection_request_t request = { 0 };
    request.order = 'l';  // Little endian.
    request.major_version = 11;
    request.minor_version =  0;
    request.auth_proto_name_len = 18;
    request.auth_proto_data_len = 16;
    soc_write(state->socket_fd, &request, sizeof(connection_request_t));
    soc_write(state->socket_fd, "MIT-MAGIC-COOKIE-1\0\0", 20);
    soc_write(state->socket_fd, magic_cookie, COOKIE_LEN);

    // Read connection reply header.
    soc_read(state->socket_fd, &state->connection_reply_header, sizeof(connection_reply_header_t));
    if (state->connection_reply_header.success == 0)
        CRITICAL_ERROR;

    // Read rest of connection reply.
    state->connection_reply_success_body = malloc(state->connection_reply_header.len * 4);
    soc_read(state->socket_fd, state->connection_reply_success_body, state->connection_reply_header.len * 4);

    // Set some pointers into the connection reply because they'll be convenient later.
    int vendor_len_plus_padding = (state->connection_reply_success_body->vendor_len + 3) & ~3;
    state->pixmap_formats = (pixmap_format_t *)(state->connection_reply_success_body->vendor_string + vendor_len_plus_padding);
    state->screens = (screen_t *)(state->pixmap_formats + state->connection_reply_success_body->num_pixmap_formats);
    state->next_resource_id = state->connection_reply_success_body->id_base;
}

static uint32_t generate_id(state_t *state)
{
    return state->next_resource_id++;
}

void create_gc(state_t *state)
{
    state->graphics_context_id = generate_id(state);
    int const len = 4;
    uint32_t packet[len];
    packet[0] = X_CreateGC | len<<16;
    packet[1] = state->graphics_context_id;
    packet[2] = state->screens[0].root_id;
    packet[3] = 0; // Value mask.

    soc_write(state->socket_fd, packet, len * 4);
}

static void put_window(state_t *state, uint16_t w, uint16_t h, uint32_t window_parent)
{
    state->window_id = generate_id(state);

    struct {    // xCreateWindowReq
        uint8_t reqType;
        uint8_t depth;
        uint16_t length;
        uint32_t w_id;
        uint32_t parent;
        int16_t x;
        int16_t y;
        uint16_t width;  
        uint16_t height;  
        uint16_t borderWidth;  
        uint16_t class;
        uint32_t visual;
        uint32_t mask;
    } req;

    req.reqType = X_CreateWindow;
    req.depth = 0;
    req.length = sizeof(req)/4;
    req.w_id = state->window_id;
    req.parent = window_parent;
    req.x = 0;  // System will position window.
    req.y = 0;  // System will position window.
    req.width = w;
    req.height = h;
    req.borderWidth = 0;
    req.class = 0;
    req.visual = 0;
    req.mask = 0;

    // int const len = 8;
    // uint32_t packet[len];
    // packet[0] = X_CreateWindow | len<<16;
    // packet[1] = state->window_id;
    // packet[2] = window_parent;
    // packet[3] = 0; // x,y pos. System will position window.
    // packet[4] = w | (h<<16);
    // packet[5] = 0; // DEFAULT_BORDER and DEFAULT_GROUP.
    // packet[6] = 0; // Visual: Copy from parent.
    // packet[7] = 0; // value_mask;

    soc_write(state->socket_fd, &req, sizeof(req));
}


void map_window(state_t *state)
{
    int const len = 2;
    uint32_t packet[len];
    packet[0] = X_MapWindow | len<<16;
    packet[1] = state->window_id;
    soc_write(state->socket_fd, packet, 8);
}


void put_image(uint32_t width, uint32_t height, uint32_t *buf)
{
    uint32_t bmp_format = 2 << 8;
    uint32_t request_len = (uint32_t)(width * height + 6) << 16;
    uint32_t buf_size = width * height * 4; // buffer size on bytes
    
    uint32_t *packet = malloc(24 + buf_size);
    memcpy(&packet[6], buf, buf_size);
    packet[0] = X_PutImage | bmp_format | request_len;
    packet[1] = window->window_id;
    packet[2] = window->graphics_context_id;
    packet[3] = width | (height << 16); // Width and height.
    packet[4] = 0; // Dst X and Y.
    packet[5] = 24 << 8; // Bit depth.

    soc_write(window->socket_fd, packet, 24 + buf_size);
    free(packet);
}

static int init_gui(void)
{
    static state_t app;
    if(NULL == window)
    {
        window = &app;
        x11_init(window);
        create_gc(window);
        return EXIT_SUCCESS;
    }
    fprintf(stderr, RED "ERROR: call init_gui() only once!" NC);
    return EXIT_FAILURE;  // window already created, but the error is not critical
}

int create_window(int height, int width)
{
	if(EXIT_FAILURE == init_gui())
        return EXIT_FAILURE;
    put_window(window, width, height, window->screens[0].root_id);
    map_window(window);
    return EXIT_SUCCESS;
}

int update_window(uint32_t width, uint32_t height, uint32_t *buf)
{
    put_image(width, height, buf);
    return EXIT_SUCCESS;
}

int update_image(uint32_t width, uint32_t height, uint32_t *buf, uint32_t x_off, uint32_t y_off)
{
    uint32_t bmp_format = 2 << 8;
    uint32_t request_len = (uint32_t)(width * height + 6) << 16;
    uint32_t buf_size = width * height * 4; // buffer size on bytes
    
    uint32_t *packet = malloc(24 + buf_size);
    memcpy(&packet[6], buf, buf_size);
    packet[0] = X_PutImage | bmp_format | request_len;
    packet[1] = window->window_id;
    packet[2] = window->graphics_context_id;
    packet[3] = width | (height << 16); // Width and height.
    packet[4] = x_off | (y_off << 16); // Dst X and Y.
    packet[5] = 24 << 8; // Bit depth.

    soc_write(window->socket_fd, packet, 24 + buf_size);
    free(packet);
    return EXIT_SUCCESS;
}

int window_get_dimensions(int * width, int * height)
{
    struct __attribute__((packed)){
        uint8_t type;   /* X_Reply */
        uint8_t depth;
        uint16_t sequenceNumber;
        uint32_t length;  /* 0 */
        uint32_t root;
        uint16_t x, y;
        uint16_t width, height;
        uint16_t borderWidth;
        uint16_t pad1;
        uint32_t pad2;
        uint32_t pad3;
    } rep;

    struct  __attribute__((packed)){
        uint8_t reqType;
        uint8_t pad;
        uint16_t length;
        uint32_t id;  /* a Window, Drawable, Font, GContext, Pixmap, etc. */
    } req;

    req.reqType = X_GetGeometry;
    req.length = sizeof(req) / 4;
    req.id = window->window_id;
    soc_write(window->socket_fd, &req, sizeof(req));

    // Read connection reply header.

    uint8_t * p = (uint8_t*)&rep;
    uint8_t * r = (uint8_t*)&req;
    soc_read(window->socket_fd, &rep, sizeof(rep));
    // *root = rep.root;
    // *x = rep.x;
    // *y = rep.y;
    *width = rep.width;
    *height = rep.height;
    // *borderWidth = rep.borderWidth;
    // *depth = rep.depth;
    return EXIT_SUCCESS;
}

int window_get_position(int * x, int * y)
{
    struct __attribute__((packed)){
        uint8_t type;   /* X_Reply */
        uint8_t depth;
        uint16_t sequenceNumber;
        uint32_t length;  /* 0 */
        uint32_t root;
        uint16_t x, y;
        uint16_t width, height;
        uint16_t borderWidth;
        uint16_t pad1;
        uint32_t pad2;
        uint32_t pad3;
    } rep;

    struct  __attribute__((packed)){
        uint8_t reqType;
        uint8_t pad;
        uint16_t length;
        uint32_t id;  /* a Window, Drawable, Font, GContext, Pixmap, etc. */
    } req;

    req.reqType = X_GetGeometry;
    req.length = sizeof(req) / 4;
    req.id = window->window_id;
    soc_write(window->socket_fd, &req, sizeof(req));

    // Read connection reply header.

    uint8_t * p = (uint8_t*)&rep;
    uint8_t * r = (uint8_t*)&req;
    soc_read(window->socket_fd, &rep, sizeof(rep));
    // *root = rep.root;
    *x = rep.x;
    *y = rep.y;
    // *width = rep.width;
    // *height = rep.height;
    // *borderWidth = rep.borderWidth;
    // *depth = rep.depth;
    return EXIT_SUCCESS;
}

int window_move(int x, int y)
{
    struct {
        uint8_t reqType;
        uint8_t pad;
        uint16_t length;
        uint32_t id;
        uint16_t mask;
        uint16_t pad2;
        uint32_t x;
        uint32_t y;
    } req = {0};
    req.reqType = X_ConfigureWindow;
    req.length = sizeof(req)/4;
    req.id = window->window_id;
    req.mask = CWX | CWY;
    int curr_x, curr_y;
    window_get_position(&curr_x, &curr_y);
    printf("current position: %d, %d ", curr_x, curr_y);
    req.x = curr_x + x;
    req.x = curr_y + y;
    soc_write(window->socket_fd, &req, sizeof(req));
    // window_get_position(&curr_x, &curr_y);
    // printf("new position: %d, %d\n", curr_x, curr_y);
    return EXIT_SUCCESS;
}

