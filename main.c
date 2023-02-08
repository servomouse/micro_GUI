#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "micro_GUI.h"


#define WIDTH 480
#define HEIGHT 480
uint32_t frame[HEIGHT][WIDTH] = {[0 ... HEIGHT-1][0 ... WIDTH-1] = 0x00505050};
// int temp[128] = {[0 ... 127] = 0x10};

void draw_rect_rand(int pos_x, int pos_y, int x, int y, uint32_t color)
{
    int rx, ry, rc;
    for(int t=0; t<100000; t++)
    {
        rx = rand() & 0xFFF;
        ry = rand() & 0xFFF;
        rc = rand() & 0xFFFFFF;
        if(rx > pos_x && rx < pos_x+x && ry > pos_y && ry<pos_y+y)
            frame[ry][rx] = rc;

    }
}

void draw_rectangle(int pos_x, int pos_y, int x, int y, uint32_t color)
{
    for(int i=pos_x; i<pos_x+x; i++)
    {
        frame[pos_y][i] = color;
        frame[pos_y + y][i] = color;
    }
    for(int i=pos_y; i<pos_y+y; i++)
    {
        frame[i][pos_x] = color;
        frame[i][pos_x+x] = color;
    }
}

void update_fb(void)
{
    for(int y=0; y<HEIGHT; y++)
        update_image(WIDTH, 1, &frame[y][0], 0, y);
}

uint32_t *create_image(uint32_t width, uint32_t height)
{
    // printf("%s, %s, %d", __func__, __FILE__, __LINE__);
    uint32_t *image = malloc(width * height * 4);
    if(NULL == image)
    {
        printf("malloc error in %s in %s on %d", __func__, __FILE__, __LINE__);
    }
    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            // TT RR GG BB
            image[(y*width) + x] = 0x000000FF;
        }
    }

    return image;
}

int i = 2;

void* foo(void* p)
{
  // Print value received as argument:
  printf("Value recevied as argument in starting routine: ");
  printf("%i\n", * (int*)p);

  // Return reference to global variable:
  pthread_exit(&i);
}

int main(void)
{
    srand(time(NULL));
	create_window(480, 480);
    int w = 0, h = 0;
    window_get_dimensions(&w, &h);
    sleep(1);
    printf("w = %d, h = %d\n", w, h);
    w = 255;
    h = 255;
    int counter = 0;
    pthread_t id;
    int j = 1;
    pthread_create(&id, NULL, foo, &j);
    int* ptr;
    pthread_join(id, (void**)&ptr); // Wait for foo() and retrieve value in ptr;
    printf("Value recevied by parent from child: ");
    printf("%i\n", *ptr);
    
    while (1)
	{
        update_fb();
        draw_rect_rand(0, 0, 480, 480, 0x000000FF);
        sleep(0.1);
    }
}
/*

typedef struct{
    uint32_t len;
    uint8_t data[0];
}data_t;

int server(void)
{
    char server_message[] = "You have reached the server!";

    // create the server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // define the server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9002);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to our specified IP and port
    bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));

    // second agrument is a backlog - how many connections can be waiting for this socket simultaneously
    listen(server_socket, 5);

    int client_socket = accept(server_socket, NULL, NULL);
    uint8_t buf[4096] = {0};
    data_t *rec_buf = (data_t*)buf;
    uint8_t txb[1024] = {0};
    data_t *tx_buf = (data_t *)txb;
    tx_buf->len = sizeof(server_message);
    memcpy(tx_buf->data, server_message, sizeof(server_message));
    

    int recl = recv(client_socket, buf, 4, 0);
    if(recl == 4)
    {
        if(rec_buf->len > sizeof(buf)-4)
            rec_buf->len = sizeof(buf)-4;
        recv(client_socket, rec_buf->data, rec_buf->len, 0);
    }
    printf("received %d bytes of data: %s", rec_buf->len, rec_buf->data);

    // send the message
    send(client_socket, tx_buf, 4 + tx_buf->len, 0);


    // close the socket
    close(server_socket);
    return 0;
}

int client(void)
{
    char cli_msg[] = "Hello world!\n";
	// create a socket
	int network_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	// specify an address for the socket
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9002);
	server_address.sin_addr.s_addr = INADDR_ANY;

	int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	// check for error with the connection
	if (connection_status == -1){
		printf("There was an error making a connection to the remote socket \n\n");
	}
	
	// receive data from the server
	// char server_response[256];
	// int l = recv(network_socket, &server_response, sizeof(server_response), 0);

	// print out the server's response
	// printf("The server sent %d bytes of data: %s\n", l, server_response);

    uint8_t buf[4096];
    data_t *rec_buf = (data_t*)buf;
    uint8_t txb[1024] = {0};
    data_t *tx_buf = (data_t *)txb;
    tx_buf->len = sizeof(cli_msg);
    memcpy(tx_buf->data, cli_msg, sizeof(cli_msg));
    

    // send the message
    send(network_socket, txb, 4 + tx_buf->len, 0);

    int recl = recv(network_socket, buf, 4, 0);
    if(recl == 4)
    {
        if(rec_buf->len > sizeof(buf)-4)
            rec_buf->len = sizeof(buf)-4;
        recl = recv(network_socket, &buf[4], rec_buf->len, 0);
    }
    printf("received %d bytes of data: %s\n", rec_buf->len, rec_buf->data);
    // for(int i=0; i<rec_buf->len; i++)
    //     printf(" 0x%02x", rec_buf->data[i]);
    // printf("\n");


	// and then close the socket
	close(network_socket);
}

int main(void)
{
    // server();
    client();
    return 0;
}
*/
