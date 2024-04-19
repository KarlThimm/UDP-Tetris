/*
Author for UDP Connection: Karl Thimm
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h> 
#include <errno.h> 

#define ROWS 20 
#define COLS 15
#define TRUE 1
#define FALSE 0
#define Server_Port 5555

char Table[ROWS][COLS] = {0};
int score = 0;
char isGameActive = TRUE;
suseconds_t timer = 400000; 
int decrease = 1000;

typedef struct udp
{
	int sequence;
	int ack;
	char buffer;
} UDP;

typedef struct
{
	char **array;
	int width, row, col;
} Shape;
Shape current;

const Shape ShapesArray[7] = {
	{(char *[]){(char[]){0, 1, 1}, (char[]){1, 1, 0}, (char[]){0, 0, 0}}, 3},								// S shape
	{(char *[]){(char[]){1, 1, 0}, (char[]){0, 1, 1}, (char[]){0, 0, 0}}, 3},								// Z shape
	{(char *[]){(char[]){0, 1, 0}, (char[]){1, 1, 1}, (char[]){0, 0, 0}}, 3},								// T shape
	{(char *[]){(char[]){0, 0, 1}, (char[]){1, 1, 1}, (char[]){0, 0, 0}}, 3},								// L shape
	{(char *[]){(char[]){1, 0, 0}, (char[]){1, 1, 1}, (char[]){0, 0, 0}}, 3},								// flipped L shape
	{(char *[]){(char[]){1, 1}, (char[]){1, 1}}, 2},														// square shape
	{(char *[]){(char[]){0, 0, 0, 0}, (char[]){1, 1, 1, 1}, (char[]){0, 0, 0, 0}, (char[]){0, 0, 0, 0}}, 4} // long bar shape
																											// you can add any shape like it's done above. Don't be naughty.
};

Shape CopyShape(Shape shape) {
    Shape new_shape = shape;
    char **copyshape = shape.array;
    new_shape.array = (char **)malloc(new_shape.width * sizeof(char *));
    for (int i = 0; i < new_shape.width; i++) {
        new_shape.array[i] = (char *)malloc(new_shape.width * sizeof(char));
        for (int j = 0; j < new_shape.width; j++) {
            new_shape.array[i][j] = copyshape[i][j];
        }
    }
    return new_shape;
}

void DeleteShape(Shape shape) {
    for (int i = 0; i < shape.width; i++) {
        free(shape.array[i]);
    }
    free(shape.array);
}

int CheckPosition(Shape shape) {
    char **array = shape.array;
    for (int i = 0; i < shape.width; i++) {
        for (int j = 0; j < shape.width; j++) {
            if ((shape.col + j < 0 || shape.col + j >= COLS || shape.row + i >= ROWS)) {
                if (array[i][j]) return FALSE;
            } else if (Table[shape.row + i][shape.col + j] && array[i][j]) return FALSE;
        }
    }
    return TRUE;
}

void SetNewRandomShape() {
    Shape new_shape = CopyShape(ShapesArray[rand() % 7]);
    new_shape.col = rand() % (COLS - new_shape.width + 1);
    new_shape.row = 0;
    DeleteShape(current);
    current = new_shape;
    if (!CheckPosition(current)) {
        isGameActive = FALSE;
    }
}

void RotateShape(Shape shape) {
    Shape temp = CopyShape(shape);
    int width = shape.width;
    for (int i = 0; i < width; i++) {
        for (int j = 0, k = width - 1; j < width; j++, k--) {
            shape.array[i][j] = temp.array[k][i];
        }
    }
    DeleteShape(temp);
}

void WriteToTable() {
    for (int i = 0; i < current.width; i++) {
        for (int j = 0; j < current.width; j++) {
            if (current.array[i][j])
                Table[current.row + i][current.col + j] = current.array[i][j];
        }
    }
}

void RemoveFullRowsAndUpdateScore()
{
	int i, j, sum, count = 0;
	for (i = 0; i < ROWS; i++)
	{
		sum = 0;
		for (j = 0; j < COLS; j++)
		{
			sum += Table[i][j];
		}
		if (sum == COLS)
		{
			count++;
			int l, k;
			for (k = i; k >= 1; k--)
				for (l = 0; l < COLS; l++)
					Table[k][l] = Table[k - 1][l];
			for (l = 0; l < COLS; l++)
				Table[k][l] = 0;
			timer -= decrease--;
		}
	}
	score += 100 * count;
}

void PrintTable()
{
	char Buffer[ROWS][COLS] = {0};
	int i, j;
	for (i = 0; i < current.width; i++)
	{
		for (j = 0; j < current.width; j++)
		{
			if (current.array[i][j])
				Buffer[current.row + i][current.col + j] = current.array[i][j];
		}
	}
	clear();
	for (i = 0; i < COLS - 9; i++)
		printw(" ");
		printw("Covid Tetris\n");
	for (i = 0; i < ROWS; i++)
	{
		for (j = 0; j < COLS; j++)
		{
			printw("%c ", (Table[i][j] + Buffer[i][j]) ? '#' : '.');
		}
		printw("\n");
	}
	printw("\nScore: %d\n", score);
}

void ManipulateCurrent(char action)
{
	Shape temp = CopyShape(current);
	switch (action)
	{
	case 's':
		temp.row++; // move down
		if (CheckPosition(temp))
			current.row++;
		else
		{
			WriteToTable();
			RemoveFullRowsAndUpdateScore();
			SetNewRandomShape();
		}
		break;
	case 'd':
		temp.col++; // move right
		if (CheckPosition(temp))
			current.col++;
		break;
	case 'a':
		temp.col--; // move left
		if (CheckPosition(temp))
			current.col--;
		break;
	case 'w':
		RotateShape(temp); // rotate clockwise
		if (CheckPosition(temp))
			RotateShape(current);
		break;
	case 'q':
        clear(); // Refresh the screen
        printw("Q Pressed, Quitting Game");
        refresh();
        sleep(2); // Short delay to display message
        isGameActive = FALSE; // Set the game loop to end
        break;
	}
	DeleteShape(temp);
	PrintTable();
}

struct timeval before_now, now;
int hasToUpdate()
{
	return ((suseconds_t)(now.tv_sec * 1000000 + now.tv_usec) - ((suseconds_t)before_now.tv_sec * 1000000 + before_now.tv_usec)) > timer;
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    int server_fd, len, n;
    UDP recv_frame, send_frame;

    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(Server_Port);

    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    if (bind(server_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    len = sizeof(client_addr);
    srand(time(0));
    score = 0;
	int c;
    initscr();
    gettimeofday(&before_now, NULL);
    timeout(1);
    SetNewRandomShape();
    PrintTable();

       while (isGameActive) {
        n = recvfrom(server_fd, &recv_frame, sizeof(UDP), 0, (struct sockaddr *)&client_addr, &len);
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                refresh();
                gettimeofday(&now, NULL);
                if (hasToUpdate()) {
                    ManipulateCurrent('s');
                    gettimeofday(&before_now, NULL);
                }
            }
        } else {
            if (recv_frame.buffer == 'Q') {  // Check if quit signal is received
                clear();
                printw("Quit signal received from client.");
                refresh();
                sleep(2);
                isGameActive = FALSE;
                continue;
            }
            send_frame.sequence = recv_frame.sequence + 1;
            send_frame.ack = 1;
            send_frame.buffer = recv_frame.buffer;
            ManipulateCurrent(recv_frame.buffer);
            refresh();
            sendto(server_fd, &send_frame, sizeof(UDP), 0, (const struct sockaddr *)&client_addr, len);
        }
    }

    close(server_fd);
    DeleteShape(current);
    endwin();
    int i, j;
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            printf("%c ", Table[i][j] ? '#' : '.');
        }
        printf("\n");
    }

    printf("\nGame Over!\n");
    printf("\nScore: %d\n", score);
    return 0;
}