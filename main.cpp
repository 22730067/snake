// {library}=================================================================================
#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
    #include <unistd.h>
    #include <cstdio>
#endif
// ------------------------------------------------------------------------------------------
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
// {global variable}=========================================================================
#ifdef _WIN32
    UINT original_input_cp;
    UINT original_output_cp;
#else
    termios oldt;
#endif
// ------------------------------------------------------------------------------------------
std::mutex cout_mutex;
std::mutex direction_mutex;

enum key_press {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    ESCAPE_KEY,
    IDLE
};

key_press direction = RIGHT;
key_press snake_direction = RIGHT;

int state = 1;
int debug = 0;

const int width = 70;
const int height = 40;

std::vector<std::vector<int>> board(height,std::vector<int>(width,0));
std::vector<std::vector<int>> change_board(height,std::vector<int>(width,1));
// {function}================================================================================
#ifdef _WIN32
    void set_terminal_mode() {
        // No need action
    }

    void restore_terminal_mode() {
        // No need action
    }

    key_press get_key() {
        int key = _getch();
        if (key == 0 || key == 0xE0) { // Special key pressed
            key = _getch();
            switch (key) {
                case 72: return UP;
                case 80: return DOWN;
                case 77: return RIGHT;
                case 75: return LEFT;
            }
        }
        if (key == 27) return ESCAPE_KEY;
        return IDLE; // Not an arrow key
    }

    void set_utf8_mode() {
        original_input_cp = GetConsoleCP();
        original_output_cp = GetConsoleOutputCP();
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    }

    void restore_utf8_mode() {
        SetConsoleCP(original_input_cp);
        SetConsoleOutputCP(original_output_cp);
    }

    void enable_ansi_escape_codes() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) {
            return; // Error getting handle
        }

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) {
            return; // Error getting mode
        }

        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(hOut, dwMode)) {
            return; // Error setting mode
        }
    }
#else
    void set_terminal_mode() {
        termios newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }

    void restore_terminal_mode() {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }

    key_press get_key() {
        int key = getchar();
        if (key == 27)
        {
            if (getchar() == '[')
            {
                key = getchar();
                switch (key)
                {
                    case 'A': return UP;
                    case 'B': return DOWN;
                    case 'C': return LEFT;
                    case 'D': return RIGHT;
                }
            }
        }
        else
        {
            return ESCAPE_KEY;
        }
        return IDLE; // Not an arrow key
    }
    void set_utf8_mode() {
        // No need action
    }

    void restore_utf8_mode() {
        // No need action
    }

    void enable_ansi_escape_codes() {
        // No need action
    }
#endif
// ▄--█--------------------------------------------------------------------------------------

// A function that will be executed by a thread. It captures key input.
void input_thread_function() {
    key_press key = get_key();
    if (key == IDLE) return;
    if (key == ESCAPE_KEY)
    {
        std::lock_guard<std::mutex> lock(direction_mutex);
        state = 0;
        return;
    }
    key_press reverse_direction;
    switch (key)
    {
    case LEFT:
        reverse_direction = RIGHT;
        break;
    case RIGHT:
        reverse_direction = LEFT;
        break;
    case UP:
        reverse_direction = DOWN;
        break;
    case DOWN:
        reverse_direction = UP;
        break;
    }
    {
        std::lock_guard<std::mutex> lock(direction_mutex);
        if (direction != reverse_direction)
        {
            snake_direction = direction;
            direction = key;
        }
    }
}

void debug_input() {
    key_press key;
    {
        std::lock_guard<std::mutex> lock(direction_mutex);
        key = snake_direction;
    }
    std::string output;
    switch (key)
    {
    case LEFT:
        output = "left ▄\n";
        break;
    case RIGHT:
        output = "right ▄\n";
        break;
    case UP:
        output = "up ▄\n";
        break;
    case DOWN:
        output = "down ▄\n";
        break;
    }
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "current direction: " << output;
    }
}

void goto_xy(int x, int y) {
    std::cout << "\x1b[" + std::to_string(x+1) + ";" + std::to_string(y+1) + "H";
}

void print_thread_function() {
    if (debug)
    {
        debug_input();
    }
    for (int i = 0; i < height; i+=2)
    {
        for (int j = 0; j < width; j++)
        {
            if (change_board[i][j] == 1)
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                change_board[i][j] = 0;
                change_board[i+1][j] = 0;
                goto_xy(i/2,j);
                if (board[i][j] == 1 && board[i+1][j] == 1)
                {
                    std::cout << "█";
                }
                else if (board[i][j] == 1 && board[i+1][j] == 0)
                {
                    std::cout << "▀";
                }
                else if (board[i][j] == 0 && board[i+1][j] == 1)
                {
                    std::cout << "▄";
                }
                else
                {
                    std::cout << " ";
                }
            }
        }
        std::cout << '\n';
    }
    std::cout.flush();
}

void init_board() {
    std::cout << "\x1b[?25l";
    for (int i = 0; i < width; i++)
    {
        board[0][i] = 1;
        board[height-1][i] = 1;
    }
    for (int j = 0; j < height; j++)
    {
        board[j][0] = 1;
        board[j][width-1] = 1;
    }
}

int main() {
    set_utf8_mode();
    set_terminal_mode();
    enable_ansi_escape_codes();
    init_board();
    while (state == 1)
    {
        std::thread input_thread(input_thread_function);
        std::thread print_thread(print_thread_function);
        if (input_thread.joinable())
            input_thread.join();
        if (print_thread.joinable())
            print_thread.join();
    }

    restore_terminal_mode();

    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "Main thread exiting." << std::endl;

    restore_utf8_mode();
    return 0;
}
