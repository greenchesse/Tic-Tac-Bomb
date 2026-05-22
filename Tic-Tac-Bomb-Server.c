#include <stdio.h>                              // Include standard input/output functions
#include "ui.h"                             // Include the UI header for interface_title() function
#include <stdlib.h>                         // Include standard library functions
#include <string.h>             // Include string manipulation functions
#include <ctype.h>                                  // Include character type functions for tolower()
#include <time.h>                           // Include time functions for seeding random number generator
#include <unistd.h>                     // Include POSIX API functions for close()
#include <arpa/inet.h>              // Include definitions for internet operations
#include <sys/socket.h>                                 // Include definitions for socket operations
#include <netinet/in.h>                 // Include definitions for internet address family

#define BOARD_SIZE 9                        // Define the size of the tic-tac-toe board
#define ZONE_COUNT 4                                // Define the number of shield zones in the game
#define SHIELD_DURATION_TURNS 2                     // Define how many turns a shield lasts
#define MOVE_LIFETIME_PLAYER_MOVES 3                // A mark dissolves after 3 moves of its owner
#define BUFFER_SIZE 1024                    // Define the buffer size for sending and receiving messages
#define COLOR_RED "\x1b[31m"                                // Define ANSI escape code for red color
#define COLOR_GREEN "\x1b[32m"                  // Define ANSI escape code for green color
#define COLOR_RESET "\x1b[0m"                   // Define ANSI escape code to reset color

typedef struct 
{
    int active;
    char owner;
    int turnsLeft;
} ShieldZone;                   // Define a structure for shield zones in the game

static const int zoneCells[ZONE_COUNT][4] = 
{
    {0, 1, 3, 4},
    {1, 2, 4, 5},
    {3, 4, 6, 7},
    {4, 5, 7, 8}
};

static void printCell(const char board[BOARD_SIZE], int index) 
{                               // Print a single cell of the board with appropriate formatting and colors
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

static void displayBoard(const char board[BOARD_SIZE])          // Display the tic-tac-toe board in a formatted manner
{
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

static void initializeBoard(char board[BOARD_SIZE])                 // Initialize the tic-tac-toe board with numbers 1-9 representing empty cells
{
    int i;
    for (i = 0; i < BOARD_SIZE; i++) 
    {
        board[i] = (char)('1' + i);
    }
}

static int isTaken(char cell)               // Check if a cell is taken by either player (X or O)
{
    return cell == 'X' || cell == 'O';
}

static int playerIndex(char player)             // Get the index of the player (0 for X, 1 for O) based on the character representation
{
    return (player == 'X') ? 0 : 1;
}

static int checkWinnerFor(const char board[BOARD_SIZE], char player)    // Check if the specified player has won the game by having three in a row
{
    const int winLines[8][3] = 
    {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
        {0, 4, 8}, {2, 4, 6}
    };
    int i;
    for (i = 0; i < 8; i++) 
    {
        int a = winLines[i][0];
        int b = winLines[i][1];
        int c = winLines[i][2];
        if (board[a] == player && board[b] == player && board[c] == player) return 1;
    }
    return 0;
}

static int isDraw(const char board[BOARD_SIZE])             // Check if the game is a draw by verifying that all cells are taken and there is no winner
{
    int i;
    for (i = 0; i < BOARD_SIZE; i++) 
    {
        if (!isTaken(board[i])) return 0;
    }
    return 1;
}

static void trimNewline(char *text)             // Remove the trailing newline character from a string, if present, to clean up user input
{
    size_t len = strlen(text);
    if (len > 0 && text[len - 1] == '\n') text[len - 1] = '\0';

}

static int parseZoneLetter(char letter)             // Convert a zone letter (a, b, c, d) to its corresponding index (0-3) for shield and bomb abilities
{
    switch ((char)tolower((unsigned char)letter)) 
    {
        case 'a': return 0;
        case 'b': return 1;
        case 'c': return 2;
        case 'd': return 3;
        default: return -1;
    }
}

static int isCellShielded(const ShieldZone shields[ZONE_COUNT], int cell, char defender)        //
{       // Check if a cell is currently protected by an active shield owned by the defender, which would prevent it from being affected by bombs
    int zone;
    int i;
    for (zone = 0; zone < ZONE_COUNT; zone++) 
    {
        if (!shields[zone].active || shields[zone].owner != defender) continue;
        for (i = 0; i < 4; i++) 
        {
            if (zoneCells[zone][i] == cell) return 1;
        }
    }
    return 0;
}

static void tickShields(ShieldZone shields[ZONE_COUNT])         // Decrease the turns left for active shields and deactivate them if their duration has expired, ensuring that shield effects are properly timed in the game
{
    int zone;
    for (zone = 0; zone < ZONE_COUNT; zone++) 
    {
        if (!shields[zone].active) continue;
        shields[zone].turnsLeft--;
        if (shields[zone].turnsLeft <= 0) 
        {
            shields[zone].active = 0;
            shields[zone].owner = '\0';
            shields[zone].turnsLeft = 0;
        }
    }
}

static int applyMoveDecay(char board[BOARD_SIZE], int cellBirth[BOARD_SIZE], const int playerTurns[2]) 
{           // Check each cell on the board to see if any marks should dissolve due to reaching their lifetime limit, and update the board state accordingly
    int i;
    int removed = 0;
    for (i = 0; i < BOARD_SIZE; i++) 
    {
        int ownerIndex;
        if (!isTaken(board[i])) continue;
        if (cellBirth[i] < 0) continue;
        ownerIndex = playerIndex(board[i]);
        if (playerTurns[ownerIndex] - cellBirth[i] >= MOVE_LIFETIME_PLAYER_MOVES) 
        {
            board[i] = (char)('1' + i);
            cellBirth[i] = -1;
            removed = 1;
        }
    }
    return removed;
}

static int applyBomb(char board[BOARD_SIZE], ShieldZone shields[ZONE_COUNT], int lost[2][BOARD_SIZE], int cellBirth[BOARD_SIZE], int zone) 
{               // Apply the bomb effect to the specified zone, destroying any unshielded marks in that zone and updating the board state and lost cells accordingly
    int i;
    int changed = 0;
    for (i = 0; i < 4; i++) 
    {
        int cell = zoneCells[zone][i];
        char owner = board[cell];
        if (!isTaken(owner)) continue;
        if (isCellShielded(shields, cell, owner)) continue;
        lost[playerIndex(owner)][cell] = 1;
        board[cell] = (char)('1' + cell);
        cellBirth[cell] = -1;
        changed = 1;
    }
    if (!changed) printf("Bomb exploded but no tile was destroyed.\n");
    else printf("Bomb from %c-zone destroyed available tiles.\n", (char)('a' + zone));
    return 1;
}

static int applyShield(ShieldZone shields[ZONE_COUNT], char caster, int zone) 
{           // Apply a shield to the specified zone for the caster, activating the shield and setting its duration, which will protect cells in that zone from bomb effects
    shields[zone].active = 1;
    shields[zone].owner = caster;
    shields[zone].turnsLeft = SHIELD_DURATION_TURNS;
    printf("Shield placed on zone %c for 2 turns.\n", (char)('a' + zone));
    return 1;
}

static int applyHeal(char board[BOARD_SIZE], int lost[2][BOARD_SIZE], int cellBirth[BOARD_SIZE], int placedOnTurn, char caster, int cell) 
{               // Apply a heal to the specified cell for the caster, restoring a previously lost mark if the cell is valid and was lost by the caster, which allows players to recover from bomb damage under certain conditions
    int pIndex = playerIndex(caster);
    if (cell < 0 || cell >= BOARD_SIZE) 
    {
        printf("Heal target must be from 1 to 9.\n");
        return 0;
    }
    if (isTaken(board[cell])) 
    {
        printf("Cell %d is occupied. Heal requires an empty cell.\n", cell + 1);
        return 0;
    }
    if (!lost[pIndex][cell]) 
    {
        printf("You did not lose cell %d before, heal is invalid there.\n", cell + 1);
        return 0;
    }
    board[cell] = caster;
    lost[pIndex][cell] = 0;
    cellBirth[cell] = placedOnTurn;
    printf("Heal restored your mark at cell %d.\n", cell + 1);
    return 1;
}

static int applySteal(int movePoints[2], char caster) 
{               // Apply the steal ability for the caster, which has a 50/50 chance to succeed and allows the caster to take all of the enemy's move points if successful, or lose their turn if it fails, adding an element of risk and reward to the game
    int me = playerIndex(caster);
    int enemy = 1 - me;
    int success = rand() % 2;
    if (success) 
    {
        movePoints[me] += movePoints[enemy];
        movePoints[enemy] = 0;
        printf("Steal success! You took all enemy move points. You can still move.\n");
    } 
    else printf("Steal failed. Turn lost.\n");
    return success;
}

static void printStatus(const int movePoints[2]) 
{           // Print the current status of the game, including move points for both players and instructions for using abilities, to keep players informed about their resources and options during their turn
    printf("Type 'ability' to open abilities menu.\n");
    printf("Lifecycle: marks dissolve after %d moves of the mark owner.\n", MOVE_LIFETIME_PLAYER_MOVES);
    printf("Move Points => " COLOR_RED "X" COLOR_RESET ":%d  " COLOR_GREEN "O" COLOR_RESET ":%d\n", movePoints[0], movePoints[1]);
}

static void printAbilityMenu(const ShieldZone shields[ZONE_COUNT]) 
{           // Print the ability menu with available actions and their costs, as well as the status of active shields, to inform players about their strategic options and the current state of shield zones in the game
    int zone;
    printf("=== Ability Menu ===\n");
    printf("bomb a|b|c|d   (cost 4)\n");
    printf("shield a|b|c|d (cost 3)\n");
    printf("heal 1-9       (cost 2)\n");
    printf("steal          (cost 0, 50/50)\n");
    printf("Active shields: ");
    for (zone = 0; zone < ZONE_COUNT; zone++) 
    {
        if (shields[zone].active) printf("%c(%c,%d) ", (char)('a' + zone), shields[zone].owner, shields[zone].turnsLeft);
        else printf("%c(-) ", (char)('a' + zone));
    }
    printf("\n");
}

// 0 invalid, 1 valid end-turn, 2 steal success keep-turn, 3 ability menu
static int processTurn(const char *line, char board[BOARD_SIZE], int movePoints[2], ShieldZone shields[ZONE_COUNT], int lost[2][BOARD_SIZE], int cellBirth[BOARD_SIZE], int placedOnTurn, char player) 
{
    int value;
    char zoneLetter;
    int me = playerIndex(player);

    if (sscanf(line, "%d", &value) == 1) 
    {
        if (value < 1 || value > 9) 
        {
            printf("Normal move must be 1-9.\n");
            return 0;
        }
        if (isTaken(board[value - 1])) 
        {
            printf("Cell %d is already taken.\n", value);
            return 0;
        }
        board[value - 1] = player;
        cellBirth[value - 1] = placedOnTurn;
        return 1;
    }
    if (sscanf(line, "bomb %c", &zoneLetter) == 1) 
    {
        int zone = parseZoneLetter(zoneLetter);
        if (zone < 0) 
        {
            printf("Bomb zone must be a, b, c, or d.\n");
            return 0;
        }
        if (movePoints[me] < 4) 
        {
            printf("Bomb needs 4 move points.\n");
            return 0;
        }
        movePoints[me] -= 4;
        return applyBomb(board, shields, lost, cellBirth, zone);
    }
    if (sscanf(line, "shield %c", &zoneLetter) == 1) 
    {
        int zone = parseZoneLetter(zoneLetter);
        if (zone < 0) 
        {
            printf("Shield zone must be a, b, c, or d.\n");
            return 0;
        }
        if (movePoints[me] < 3) 
        {
            printf("Shield needs 3 move points.\n");
            return 0;
        }
        movePoints[me] -= 3;
        return applyShield(shields, player, zone);
    }
    if (sscanf(line, "heal %d", &value) == 1) 
    {
        if (movePoints[me] < 2) 
        {
            printf("Heal needs 2 move points.\n");
            return 0;
        }
        if (!applyHeal(board, lost, cellBirth, placedOnTurn, player, value - 1)) 
        {
            return 0;
        }
        movePoints[me] -= 2;
        return 1;
    }
    if (strcmp(line, "steal") == 0) return applySteal(movePoints, player) ? 2 : 1;
    if (strcmp(line, "ability") == 0) 
    {
        printAbilityMenu(shields);
        return 3;
    }
    printf("Invalid command.\n");
    printf("Use: 1-9 | ability | bomb a|b|c|d | shield a|b|c|d | heal 1-9 | steal\n");
    return 0;
}

static char decideStartingPlayer(void) 
{           // Decide which player goes first by performing a coin flip, where Player X chooses heads or tails, and the result determines who starts the game, adding an element of chance to the initial turn order
    char input[32];
    char guess;
    char toss;
    while (1) 
    {
        printf("Coin flip - Player X choose heads or tails (h/t): ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Input error. Try again.\n");
            continue;
        }
        trimNewline(input);
        if (strlen(input) == 1) 
        {
            guess = (char)tolower((unsigned char)input[0]);
            if (guess == 'h' || guess == 't') 
            {
                break;
            }
        }
        printf("Invalid choice. Type h or t only.\n");
    }
    toss = (rand() % 2 == 0) ? 'h' : 't';
    printf("Coin result: %s\n", toss == 'h' ? "heads" : "tails");
    if (guess == toss) {
        printf("Player " COLOR_RED "X" COLOR_RESET " guessed correctly and goes first.\n");
        return 'X';
    }
    printf("Player " COLOR_RED "X" COLOR_RESET " guessed wrong. Player " COLOR_GREEN "O" COLOR_RESET " goes first.\n");
    return 'O';
}

static int send_all(int sock, const char *buf, int len) 
{           // Send all bytes in the buffer to the specified socket, handling partial sends, to ensure that complete messages are transmitted over the network without truncation
    int sent = 0;
    while (sent < len) 
    {
        int n = (int)send(sock, buf + sent, (size_t)(len - sent), 0);
        if (n <= 0) {
            return 0;
        }
        sent += n;
    }
    return 1;
}

static int send_line(int sock, const char *line) 
{           // Send a line of text followed by a newline character to the specified socket, ensuring that the message is properly formatted for the client to read
    char out[BUFFER_SIZE];
    int n = snprintf(out, sizeof(out), "%s\n", line);
    if (n <= 0 || n >= (int)sizeof(out)) 
    {
        return 0;
    }
    return send_all(sock, out, n);
}

static int recv_line(int sock, char *out, size_t outSize) 
{           // Receive a line of text from the specified socket, handling the newline character and null-terminating the string, to properly read messages from the client
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
    out[outSize - 1] = '\0';
    return 1;
}

static void send_message(int sock, const char *msg) 
{           // Send a message to the client by prefixing it with "MSG " and using the send_line function, which allows the server to communicate various information and updates to the client in a consistent format
    char line[BUFFER_SIZE];
    snprintf(line, sizeof(line), "MSG %s", msg);
    send_line(sock, line);
}

static void send_ability_menu(int sock, const ShieldZone shields[ZONE_COUNT]) 
{           // Send the ability menu with available actions and their costs, as well as the status of active shields, to the client, allowing players to view their strategic options and the current state of shield zones in the game when they request the ability menu
    int zone;
    char line[BUFFER_SIZE];
    char shieldsLine[BUFFER_SIZE];
    size_t used = 0;

    send_message(sock, "=== Ability Menu ===");
    send_message(sock, "bomb a|b|c|d   (cost 4)");
    send_message(sock, "shield a|b|c|d (cost 3)");
    send_message(sock, "heal 1-9       (cost 2)");
    send_message(sock, "steal          (cost 0, 50/50)");

    used += (size_t)snprintf(shieldsLine + used, sizeof(shieldsLine) - used, "Active shields: ");
    for (zone = 0; zone < ZONE_COUNT && used < sizeof(shieldsLine); zone++) 
    {
        if (shields[zone].active) 
        {
            used += (size_t)snprintf(
                shieldsLine + used,
                sizeof(shieldsLine) - used,
                "%c(%c,%d) ",
                (char)('a' + zone),
                shields[zone].owner,
                shields[zone].turnsLeft
            );
        } 
        else
        {
            used += (size_t)snprintf(shieldsLine + used, sizeof(shieldsLine) - used, "%c(-) ", (char)('a' + zone));
        }
    }
    snprintf(line, sizeof(line), "%s", shieldsLine);
    send_message(sock, line);
}

static void send_state(int sock, const char board[BOARD_SIZE], const int movePoints[2], const int totalMovePoints[2], char currentPlayer, const ShieldZone shields[ZONE_COUNT]) 
{           // Send the current game state to the client, including the board configuration, move points, current player, and shield statuses, formatted in a way that the client can parse and update its display accordingly
    char line[BUFFER_SIZE];
    char boardStr[BOARD_SIZE + 1];
    int i;
    for (i = 0; i < BOARD_SIZE; i++) 
    {
        boardStr[i] = board[i];
    }
    boardStr[BOARD_SIZE] = '\0';

    snprintf(               // Format the state line with all relevant game information, including the board state, move points, current player, and shield statuses, to be sent to the client for synchronization of the game state
        line,
        sizeof(line),
        "STATE %s %d %d %d %d %c %d %c %d %d %c %d %d %c %d %d %c %d",
        boardStr,
        movePoints[0],
        movePoints[1],
        totalMovePoints[0],
        totalMovePoints[1],
        currentPlayer,
        shields[0].active, shields[0].active ? shields[0].owner : '-', shields[0].turnsLeft,
        shields[1].active, shields[1].active ? shields[1].owner : '-', shields[1].turnsLeft,
        shields[2].active, shields[2].active ? shields[2].owner : '-', shields[2].turnsLeft,
        shields[3].active, shields[3].active ? shields[3].owner : '-', shields[3].turnsLeft
    );
    send_line(sock, line);      
}

static void send_gameover(int sock, char winner) 
{       // Send a game over message to the client with the winner's character, allowing the client to display the end of the game and the result to the player
    char line[32];
    snprintf(line, sizeof(line), "GAMEOVER %c", winner);
    send_line(sock, line);
}

static void send_move_points(int sock, const int totalMovePoints[2]) 
{
    char line[64];
    snprintf(line, sizeof(line), "MOVEPOINTS %d %d", totalMovePoints[0], totalMovePoints[1]);
    send_line(sock, line);
}

static int ask_yes_no_local(const char *prompt) 
{       // Prompt the local player with a yes/no question and return 1 for yes and 0 for no, ensuring that the input is valid and providing feedback for invalid responses to guide the player in making a correct choice
    char line[32];
    while (1) 
    {
        printf("%s", prompt);
        if (fgets(line, sizeof(line), stdin) == NULL) 
        {
            continue;
        }
        trimNewline(line);
        if (strlen(line) == 1) 
        {
            char c = (char)tolower((unsigned char)line[0]);
            if (c == 'y') return 1;

            if (c == 'n') return 0;
        }
        printf("Please answer y or n.\n");
    }
}

int main(int argc, char *argv[]) 
{
    int listenSocket = -1;
    int clientSocket = -1;
    int portNo;
    char trailing;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    char board[BOARD_SIZE];
    int movePoints[2];
    int totalMovePoints[2] = {0, 0};
    int playerTurns[2];
    int lost[2][BOARD_SIZE] = {{0}};
    int cellBirth[BOARD_SIZE];
    ShieldZone shields[ZONE_COUNT] = {{0, '\0', 0}, {0, '\0', 0}, {0, '\0', 0}, {0, '\0', 0}};
    char currentPlayer;
    int disconnected = 0;
    int i;

    char input[BUFFER_SIZE];

    if (argc != 2) 
    {
        printf("Invalid: %s [port no.]\n", argv[0]);
        return 1;
    }
    if (sscanf(argv[1], "%d %c", &portNo, &trailing) != 1 || portNo < 1 || portNo > 65535) 
    {
        printf("Invalid port. Use 1-65535.\n");
        return 1;
    }

    interface_title();
    printf("Tic Tac Bomb Server\n");

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) 
    {
        printf("Failed to create socket.\n");
        return 1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons((unsigned short)portNo);

    if (bind(listenSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        printf("Bind failed on port %d.\n", portNo);
        close(listenSocket);
        return 1;
    }
    if (listen(listenSocket, 1) < 0) 
    {
        printf("Listen failed.\n");
        close(listenSocket);
        return 1;
    }

    printf("Waiting for client on port %d...\n", portNo);
    clientSocket = accept(listenSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (clientSocket < 0) 
    {
        printf("Accept failed.\n");
        close(listenSocket);
        return 1;
    }

    srand((unsigned int)time(NULL));
    currentPlayer = decideStartingPlayer();

    send_message(clientSocket, "Connected. You are Player O.");
    send_message(clientSocket, currentPlayer == 'X' ? "Player X starts." : "Player O starts.");

    while (!disconnected) 
    {
        int roundFinished = 0;
        char roundWinner = '\0';

        initializeBoard(board);
        movePoints[0] = 0;
        movePoints[1] = 0;
        playerTurns[0] = 0;
        playerTurns[1] = 0;
        memset(lost, 0, sizeof(lost));
        for (i = 0; i < ZONE_COUNT; i++) 
        {
            shields[i].active = 0;
            shields[i].owner = '\0';
            shields[i].turnsLeft = 0;
        }
        for (i = 0; i < BOARD_SIZE; i++) 
        {
            cellBirth[i] = -1;
        }

        while (!roundFinished) 
        {
            int result;
            int me = playerIndex(currentPlayer);

            displayBoard(board);
            printStatus(movePoints);
            send_state(clientSocket, board, movePoints, totalMovePoints, currentPlayer, shields);

            if (currentPlayer == 'X') 
            {
                printf("Player " COLOR_RED "X" COLOR_RESET " turn > ");
                if (fgets(input, sizeof(input), stdin) == NULL) 
                {
                    printf("Input error.\n");
                    continue;
                }
                trimNewline(input);
                result = processTurn(input, board, movePoints, shields, lost, cellBirth, playerTurns[me] + 1, currentPlayer);
                if (result == 0) continue;
                if (result == 3) 
                {
                    send_ability_menu(clientSocket, shields);
                    continue;
                }
                if (result == 2) 
                {
                    send_message(clientSocket, "Player X steal success and can move again.");
                    continue;
                }
            } 
            else 
            {
                send_message(clientSocket, "Your turn. Enter command.");
                if (!send_line(clientSocket, "YOURTURN")) 
                {
                    disconnected = 1;
                    break;
                }
                if (!recv_line(clientSocket, input, sizeof(input))) 
                {
                    disconnected = 1;
                    break;
                }
                result = processTurn(input, board, movePoints, shields, lost, cellBirth, playerTurns[me] + 1, currentPlayer);
                if (result == 0) 
                {
                    send_message(clientSocket, "Invalid/failed command. Try again.");
                    continue;
                }
                if (result == 3) 
                {
                    send_ability_menu(clientSocket, shields);
                    continue;
                }
                if (result == 2) 
                {
                    send_message(clientSocket, "Steal success! You can move again.");
                    continue;
                }
            }

            movePoints[me] += 1;
            playerTurns[me] += 1;

            if (checkWinnerFor(board, currentPlayer)) 
            {
                displayBoard(board);
                roundWinner = currentPlayer;
                if (currentPlayer == 'X') printf("Player " COLOR_RED "X" COLOR_RESET " wins!\n");
                else 
                {
                    printf("Player " COLOR_GREEN "O" COLOR_RESET " wins!\n");
                }
                send_state(clientSocket, board, movePoints, totalMovePoints, currentPlayer, shields);
                send_gameover(clientSocket, currentPlayer);
                roundFinished = 1;
                continue;
            }
            if (isDraw(board)) 
            {
                displayBoard(board);
                roundWinner = 'D';
                printf("It's a draw!\n");
                send_state(clientSocket, board, movePoints, totalMovePoints, currentPlayer, shields);
                send_gameover(clientSocket, 'D');
                roundFinished = 1;
                continue;
            }

            tickShields(shields);
            if (applyMoveDecay(board, cellBirth, playerTurns)) 
            {
                printf("Some old marks dissolved due to lifecycle.\n");
                send_message(clientSocket, "Some old marks dissolved due to lifecycle.");
            }
            currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
        }
        if (disconnected) break;

        if (roundWinner == 'X') 
        {
            totalMovePoints[0] += 5;
            totalMovePoints[1] += 3;
            currentPlayer = 'O';
        } 
        else if (roundWinner == 'O') 
        {
            totalMovePoints[1] += 5;
            totalMovePoints[0] += 3;
            currentPlayer = 'X';
        } 
        else currentPlayer = 'X';

        printf("Move points => X:%d O:%d\n", totalMovePoints[0], totalMovePoints[1]);
        send_move_points(clientSocket, totalMovePoints);
        {
            int serverReplay = ask_yes_no_local("Play Again? (y/n): ");
            int clientReplay;
            char replayLine[32];

            if (!serverReplay) 
            {
                send_line(clientSocket, "SESSIONEND");
                break;
            }

            if (!send_line(clientSocket, "ASKREPLAY")) 
            {
                disconnected = 1;
                break;
            }
            if (!recv_line(clientSocket, replayLine, sizeof(replayLine))) 
            {
                disconnected = 1;
                break;
            }

            clientReplay = (strcmp(replayLine, "REPLAY y") == 0);
            if (!clientReplay) 
            {
                send_line(clientSocket, "SESSIONEND");
                break;
            }

            send_message(clientSocket, roundWinner == 'D'
                ? "Next round starts. Previous round draw, Player X starts."
                : (currentPlayer == 'X' ? "Next round starts. Loser was X, so X starts." : "Next round starts. Loser was O, so O starts."));
        }
    }

    if (disconnected) 
    {
        printf("Match terminated: other player left the game.\n");
    }

    close(clientSocket);
    close(listenSocket);
    return 0;
}
