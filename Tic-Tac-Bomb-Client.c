#include <stdio.h>                      // Include standard input/output functions
#include "ui.h"                         // Include the UI header for interface_title() function
#include <stdlib.h>                     // Include standard library functions
#include <string.h>                     // Include string manipulation functions
#include <unistd.h>                     // Include POSIX API functions for close()
#include <arpa/inet.h>                  // Include definitions for internet operations
#include <sys/socket.h>                 // Include definitions for socket operations
#include <netinet/in.h>                 // Include definitions for internet address family       

#define BOARD_SIZE 9                    // Define the size of the tic-tac-toe board
#define ZONE_COUNT 4                    // Define the number of shield zones in the game
#define BUFFER_SIZE 1024                // Define the buffer size for sending and receiving messages
#define COLOR_RED "\x1b[31m"            // Define ANSI escape code for red color
#define COLOR_GREEN "\x1b[32m"          // Define ANSI escape code for green color
#define COLOR_RESET "\x1b[0m"           // Define ANSI escape code to reset color

typedef struct                          // Define a structure for shield zones in the game
{
    int active;
    char owner;
    int turnsLeft;
} ShieldZone;

static void printCell(const char board[BOARD_SIZE], int index)
{       // Print a single cell of the board with appropriate formatting and colors based on its content (X, O, or empty), and include cell identifiers for empty cells to guide player input
    if (board[index] == 'X') 
    {
        printf("   " COLOR_RED "X" COLOR_RESET "   ");
        return;
    }
    if (board[index] == 'O') 
    {
        printf("   " COLOR_GREEN "O" COLOR_RESET "   ");
        return;
    }
    switch (index) 
    {
        case 0: printf("  1,a  "); return;
        case 2: printf("  3,b  "); return;
        case 6: printf("  7,c  "); return;
        case 8: printf("  9,d  "); return;
        default: printf("   %c   ", board[index]); return;
    }
}

static void displayBoard(const char board[BOARD_SIZE]) 
{       // Display the tic-tac-toe board in a formatted manner, showing the current state of the game and providing visual cues for cell positions and corners to assist players in making their moves
    printf("\n");
    printf("         (Corners: 1,a  3,b  7,c  9,d)\n");
    printf("         |-------|-------|-------|\n");
    printf("         |");
    printCell(board, 0); printf("|");
    printCell(board, 1); printf("|");
    printCell(board, 2); printf("|\n");
    printf("         |-------|-------|-------|\n");
    printf("         |");
    printCell(board, 3); printf("|");
    printCell(board, 4); printf("|");
    printCell(board, 5); printf("|\n");
    printf("         |-------|-------|-------|\n");
    printf("         |");
    printCell(board, 6); printf("|");
    printCell(board, 7); printf("|");
    printCell(board, 8); printf("|\n");
    printf("         |-------|-------|-------|\n");
}

static void printStatus(const int movePoints[2])                            // Print the current status of the game, including move points for both players
{
    printf("Type 'ability' to open abilities menu.\n");
    printf("Lifecycle: marks dissolve after 3 moves of the mark owner.\n");
    printf("Move Points => " COLOR_RED "X" COLOR_RESET ":%d  " COLOR_GREEN "O" COLOR_RESET ":%d\n", movePoints[0], movePoints[1]);
}

static void trimNewline(char *text)                                         // Remove the trailing newline character from a string, if present
{
    size_t len = strlen(text);
    if (len > 0 && text[len - 1] == '\n') {
        text[len - 1] = '\0';
    }
}

static int send_all(int sock, const char *buf, int len)                     // Send all bytes in the buffer to the specified socket, handling partial sends
{   
    int sent = 0;
    while (sent < len)
    {
        int n = (int) send(sock, buf + sent, (size_t)(len - sent), 0);              // Send data to the socket
        if (n <= 0) return 0;
        sent += n;
    }
    return 1;
}

static int send_line(int sock, const char *line) 
{
    char out[BUFFER_SIZE];                                      // Buffer to hold the formatted line to be sent
    int n = snprintf(out, sizeof(out), "%s\n", line);           // Format the line to be sent, appending a newline character
    if (n <= 0 || n >= (int)sizeof(out)) return 0;
    return send_all(sock, out, n);
}

static int recv_line(int sock, char *out, size_t outSize)       // Receive a line of text from the specified socket, handling partial receives and ensuring null-termination
{
    size_t i = 0;
    while (i + 1 < outSize) 
    {
        char c;
        int n = (int)recv(sock, &c, 1, 0);
        if (n <= 0) return 0;
        if (c == '\r') continue;
        if (c == '\n') 
        {
            out[i] = '\0';
            return 1;
        }
        out[i++] = c;
    }
    out[outSize - 1] = '\0';            // Ensure null-termination if the buffer limit is reached
    return 1;
}

static int parse_state_line(const char *line, char board[BOARD_SIZE], int movePoints[2], int totalMovePoints[2], char *currentPlayer, ShieldZone shields[ZONE_COUNT]) 
{ // Parse the state line received from the server and populate the game state variables accordingly
    char boardStr[BOARD_SIZE + 1];      // Buffer to hold the board state as a string
    int s0a, s1a, s2a, s3a;             // Variables to hold the active status of each shield zone
    char s0o, s1o, s2o, s3o;            // Variables to hold the owner of each shield zone
    int s0t, s1t, s2t, s3t;             // Variables to hold the turns left for each shield zone
    int n = sscanf(
        line,
        "STATE %9s %d %d %d %d %c %d %c %d %d %c %d %d %c %d %d %c %d",
        boardStr,
        &movePoints[0],
        &movePoints[1],
        &totalMovePoints[0],
        &totalMovePoints[1],
        currentPlayer,
        &s0a, &s0o, &s0t,
        &s1a, &s1o, &s1t,
        &s2a, &s2o, &s2t,
        &s3a, &s3o, &s3t
    );
    if (n != 18) return 0;
    memcpy(board, boardStr, BOARD_SIZE);
    shields[0].active = s0a; shields[0].owner = s0o; shields[0].turnsLeft = s0t;
    shields[1].active = s1a; shields[1].owner = s1o; shields[1].turnsLeft = s1t;
    shields[2].active = s2a; shields[2].owner = s2o; shields[2].turnsLeft = s2t;
    shields[3].active = s3a; shields[3].owner = s3o; shields[3].turnsLeft = s3t;
    return 1;
}

int main(int argc, char *argv[]) 
{
    int clientSocket = -1;
    int portNo;
    char trailing;
    const char *serverIp;
    struct sockaddr_in serverAddr;

    char board[BOARD_SIZE] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    int movePoints[2] = {0, 0};
    int totalMovePoints[2] = {0, 0};
    ShieldZone shields[ZONE_COUNT] = {{0, '-', 0}, {0, '-', 0}, {0, '-', 0}, {0, '-', 0}};
    char currentPlayer = 'X';

    char line[BUFFER_SIZE];
    char input[BUFFER_SIZE];

    if (argc != 3) 
    {
        printf("Invalid: %s [port no] [host ip address]\n", argv[0]);
        return 1;
    }
    if (sscanf(argv[1], "%d %c", &portNo, &trailing) != 1 || portNo < 1 || portNo > 65535) 
    {
        printf("Invalid port. Use 1-65535.\n");
        return 1;
    }
    serverIp = argv[2];

    interface_title();
    printf("Tic Tac Bomb Client\n");

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) 
    {
        printf("Failed to create socket.\n");
        return 1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((unsigned short)portNo);
    if (inet_pton(AF_INET, serverIp, &serverAddr.sin_addr) <= 0) 
    {
        printf("Invalid server IP address: %s\n", serverIp);
        close(clientSocket);
        return 1;
    }

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        printf("Connection failed to %s:%d\n", serverIp, portNo);
        close(clientSocket);
        return 1;
    }

    printf("Connected to server %s:%d\n", serverIp, portNo);

    while (1) 
    {
        if (!recv_line(clientSocket, line, sizeof(line))) 
        {
            printf("Match terminated: other player left the game.\n");
            break;
        }

        if (strncmp(line, "MSG ", 4) == 0) 
        {
            printf("%s\n", line + 4);
            continue;
        }

        if (strncmp(line, "STATE ", 6) == 0) 
        {
            if (parse_state_line(line, board, movePoints, totalMovePoints, &currentPlayer, shields)) 
            {
                (void)shields;
                displayBoard(board);
                printStatus(movePoints);
                if (currentPlayer == 'X') printf("Current turn: " COLOR_RED "X" COLOR_RESET "\n");
                else printf("Current turn: " COLOR_GREEN "O" COLOR_RESET "\n");
            }
            continue;
        }

        if (strncmp(line, "MOVEPOINTS ", 11) == 0) 
        {
            if (sscanf(line, "MOVEPOINTS %d %d", &totalMovePoints[0], &totalMovePoints[1]) == 2) 
            {
                printf("Updated move points => X:%d O:%d\n", totalMovePoints[0], totalMovePoints[1]);
            }
            continue;
        }

        if (strcmp(line, "YOURTURN") == 0) 
        {
            printf("Your command > ");
            if (fgets(input, sizeof(input), stdin) == NULL) 
            {
                printf("Input error.\n");
                break;
            }
            trimNewline(input);
            if (!send_line(clientSocket, input)) 
            {
                printf("Failed to send command.\n");
                break;
            }
            continue;
        }

        if (strncmp(line, "GAMEOVER ", 9) == 0) 
        {
            char winner = line[9];
            if (winner == 'D') printf("Game over: It's a draw.\n");
            else if (winner == 'X') printf("Game over: Player " COLOR_RED "X" COLOR_RESET " wins.\n");
            else if (winner == 'O') printf("Game over: Player " COLOR_GREEN "O" COLOR_RESET " wins.\n");
            else printf("Game over.\n");
            continue;
        }

        if (strcmp(line, "ASKREPLAY") == 0) 
        {
            char answer[16];
            while (1) 
            {
                printf("Play Again? (y/n): ");
                if (fgets(answer, sizeof(answer), stdin) == NULL) continue;
                trimNewline(answer);
                if (strlen(answer) == 1 && (answer[0] == 'y' || answer[0] == 'Y' || answer[0] == 'n' || answer[0] == 'N')) break;
                printf("Please answer y or n.\n");
            }
            if (answer[0] == 'y' || answer[0] == 'Y') send_line(clientSocket, "REPLAY y");
            else send_line(clientSocket, "REPLAY n");
            continue;
        }

        if (strcmp(line, "SESSIONEND") == 0) 
        {
            printf("Match terminated.\n");
            break;
        }
    }

    close(clientSocket);
    return 0;
}
