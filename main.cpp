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
#endif
// ▄-----------------------------------------------------------------------------------------

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

void print_thread_function() {
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

int main() {
    set_utf8_mode();
    set_terminal_mode();

    while (state == 1)
    {
        std::thread input_thread(input_thread_function);
        std:: thread print_thread(print_thread_function);
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
