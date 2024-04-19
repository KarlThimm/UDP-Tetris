/*
Author for UDP Connection: Karl Thimm
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ncurses.h>
#include <sys/time.h>
#include <time.h>

typedef struct udp {
    int sequence;
    int ack;
    char buffer;
} UDP;

#define PORT 5555
#define SERVER_IP "127.0.0.1"
#define PACKET_LOSS_RATIO 0.3

int packets_transmission = 0;    // Count of initial transmissions
int packets_retransmissions = 0; // Count of retransmissions only
int packets_dropped = 0;         // Packets lost due to loss simulation
int packets_success = 0;         // Total packets successfully sent (initial + retransmissions)
int acks_received = 0;
int timeout_expirations = 0;

char isGameActive = TRUE;
int sequence = 0;

int simulate_loss(void) {
    return ((double)rand() / (double)RAND_MAX) < PACKET_LOSS_RATIO;
}

int main() {
    struct sockaddr_in server_addr;
    int client_fd, len, n;
    UDP send_frame, recv_frame;

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    initscr();
    cbreak();
    noecho();

    srand(time(NULL));
    len = sizeof(server_addr);

    while (isGameActive) {
        int acked = 0;
        char c = getch();
        if (c != ERR) {
            switch (c) {
            case 's':
            case 'd':
            case 'a':
            case 'w':
                clear();
                send_frame.sequence = sequence;
                send_frame.ack = 0;
                send_frame.buffer = c;
                printw("Packet %d generated for transmission\n", sequence);
                refresh();
                packets_transmission++;

                if (!simulate_loss()) {
                    sendto(client_fd, &send_frame, sizeof(UDP), 0, (const struct sockaddr *)&server_addr, len);
                    printw("Packet %d successfully transmitted\n", sequence);
                    refresh();
                    packets_success++;
                } else {
                    printw("Packet %d lost\n", sequence);
                    refresh();
                    packets_dropped++;
                }
                break;
            case 'q':
                send_frame.sequence = sequence;
                send_frame.ack = 0;
                send_frame.buffer = 'Q';  // Special signal to quit
                sendto(client_fd, &send_frame, sizeof(UDP), 0, (const struct sockaddr *)&server_addr, len);
                clear();
                printw("Q Pressed, Quitting Game");
                refresh();
                sleep(2); // Short delay to display message
                isGameActive = FALSE; // Set the game loop to end
                continue;
            }
        }

        while (acked == 0 && c != ERR) {
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(client_fd, &fds);
            int result = select(client_fd + 1, &fds, NULL, NULL, &timeout);
            if (result == -1) {
                perror("select error");
                isGameActive = FALSE;
            } else if (result == 0) {
                printw("Timeout expired for packet numbered %d\n", sequence);
                printw("Packet %d generated for re-transmission\n", sequence);
                refresh();
                packets_retransmissions++;
                timeout_expirations++;
                sendto(client_fd, &send_frame, sizeof(UDP), 0, (const struct sockaddr *)&server_addr, len);
                packets_success++;
            } else {
                n = recvfrom(client_fd, &recv_frame, sizeof(UDP), 0, NULL, NULL);
                if (recv_frame.sequence == sequence + 1 && recv_frame.ack == 1) {
                    printw("ACK %d received\n", recv_frame.sequence);
                    refresh();
                    acks_received++;
                    acked = 1;
                }
            }
        }
        sequence++;
    }
    close(client_fd);
    endwin();

    printf("Initial data packets transmitted: %d\n", packets_transmission);
    printf("Total data packets generated for retransmission: %d\n", packets_transmission + packets_retransmissions);
    printf("Data packets lost due to simulation: %d\n", packets_dropped);
    printf("Data packets transmitted successfully: %d\n", packets_success);
    printf("Acknowledgements received: %d\n", acks_received);
    printf("Timeout occurrences: %d\n", timeout_expirations);

    return 0;
}
