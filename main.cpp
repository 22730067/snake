#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// Define a global mutex to protect std::cout and other shared resources
std::mutex cout_mutex;

// --- Platform-specific includes and function definitions ---

#ifdef _WIN32
    #include <conio.h>

    void set_terminal_mode() {
    }

    void restore_terminal_mode() {
    }

    int get_arrow_key() {
        int key = _getch();
        if (key == 0 || key == 0xE0) { // Special key pressed
            key = _getch();
            switch (key) {
                case 72: return 1; // Up
                case 80: return 2; // Down
                case 77: return 3; // Right
                case 75: return 4; // Left
                case 71: return 5; // q
                case 51: return 5; // Q
            }
        }
        return 0; // Not an arrow key
    }

#else
    #include <termios.h>
    #include <unistd.h>
    #include <cstdio>
    #include <sys/ioctl.h>

    termios oldt;

    // Sets the terminal to "raw" mode to read single characters without waiting for enter.
    void set_terminal_mode() {
        termios newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        winsize ws;
        ws.ws_row = 120;
        ws.ws_col = 48;
        ioctl(STDOUT_FILENO, TIOCSWINSZ, &ws);
    }

    // Restores the terminal to its original state.
    void restore_terminal_mode() {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }

    // Function to get a single character from the console on Unix-like systems.
    // Handles the multi-character escape sequence for arrow keys.
    int get_arrow_key() {
        int key = getchar();
        if (key == 27) { // Escape sequence
            getchar(); // Consume '['
            key = getchar();
            switch (key) {
                case 'A': return 1; // Up
                case 'B': return 2; // Down
                case 'C': return 3; // Right
                case 'D': return 4; // Left
            }
        }
        if (key == 'q' || key == 'Q') return 5;
        return 0; // Not an arrow key
    }

#endif
////////////////////////////////////////////////////////////////////////////////////////////////

enum INPUT_KEY {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    QUIT,
    IDLE
};

INPUT_KEY global_key = IDLE;
int status = 0;

// A function that will be executed by a thread. It captures arrow key input.
void input_thread_function() {
    int key;
    key = get_arrow_key();
    if (key != 0) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        switch (key) {
            case 1: global_key = UP; break;
            case 2: global_key = DOWN; break;
            case 3: global_key = RIGHT; break;
            case 4: global_key = LEFT; break;
            case 5: status = 1; break;
        }
    }
}

void print_thread_function() {
    std::lock_guard<std::mutex> lock(cout_mutex);
    if (global_key == UP) std::cout << "up\n";
    else if (global_key == DOWN) std::cout << "down\n";
    else if (global_key == LEFT) std::cout << "left\n";
    else if (global_key == RIGHT) std::cout << "right\n";
}

int main() {
    set_terminal_mode(); // Set terminal to raw mode for real-time input

    while(status == 0) {
        std::thread input_thread(input_thread_function);
        std::thread print_thread(print_thread_function);
        if (input_thread.joinable()) {
            input_thread.join();
        }
        if (print_thread.joinable()) {
            print_thread.join();
        }

    }

    restore_terminal_mode(); // Restore terminal to normal mode
    std::cout << "Main thread exiting." << std::endl;

    return 0;
}
