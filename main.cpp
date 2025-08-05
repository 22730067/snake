// A cross-platform example to capture real-time arrow key input.
// This code uses preprocessor directives to handle platform-specific libraries
// for Windows and Unix-like systems (Linux, macOS).

#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// Define a global mutex to protect std::cout and other shared resources
std::mutex cout_mutex;

// --- Platform-specific includes and function definitions ---

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>

    // Store original code pages to restore later
    UINT original_input_cp;
    UINT original_output_cp;

    // Sets the console to UTF-8 mode on Windows.
    void set_utf8_mode() {
        // Save original code pages
        original_input_cp = GetConsoleCP();
        original_output_cp = GetConsoleOutputCP();

        // Set to UTF-8
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    }

    // Restores the original console code pages on Windows.
    void restore_utf8_mode() {
        SetConsoleCP(original_input_cp);
        SetConsoleOutputCP(original_output_cp);
    }

    void set_terminal_mode() {
        // No special setup is required for conio.h's _getch()
    }

    void restore_terminal_mode() {
        // No special cleanup is required
    }

    // Function to get a single character from the console on Windows.
    // Handles the two-character sequence for special keys like arrows.
    int get_arrow_key() {
        int key = _getch();
        if (key == 0 || key == 0xE0) { // Special key pressed
            key = _getch();
            switch (key) {
                case 72: return 1; // Up
                case 80: return 2; // Down
                case 77: return 3; // Right
                case 75: return 4; // Left
            }
        }
        return 0; // Not an arrow key
    }

#else
    #include <termios.h>
    #include <unistd.h>
    #include <cstdio>

    termios oldt;

    // Sets the terminal to "raw" mode to read single characters without waiting for enter.
    void set_terminal_mode() {
        termios newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
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
        return 0; // Not an arrow key
    }

#endif

// A function that will be executed by a thread. It captures arrow key input.
void input_thread_function() {
    // Lock guard to protect the output
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Input thread started. Press arrow keys. Press 'q' to quit." << std::endl;
    }

    int key;
    do {
        key = get_arrow_key();
        if (key != 0) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            switch (key) {
                case 1: std::cout << "Up Arrow press▄ed." << std::endl; break;
                case 2: std::cout << "Down Arrow pr▄essed." << std::endl; break;
                case 3: std::cout << "Right Arrow▄ pressed." << std::endl; break;
                case 4: std::cout << "Left Arr▄ow pressed." << std::endl; break;
            }
        }
    } while (key != 'q'); // Loop until 'q' is pressed

    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "Input thread finished." << std::endl;
}

int main() {
    #ifdef _WIN32
        set_utf8_mode();
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "UTF-8 mode enabled on Windows. Test characters: " << "你好, 世界!" << std::endl;
        }
    #else
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "This program does not change the console size on Unix-like systems." << std::endl;
        }
    #endif

    set_terminal_mode(); // Set terminal to raw mode for real-time input

    // Create and start the input thread
    std::thread input_thread(input_thread_function);

    // Wait for the input thread to finish
    input_thread.join();

    restore_terminal_mode(); // Restore terminal to normal mode

    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "Main thread exiting." << std::endl;

    #ifdef _WIN32
        restore_utf8_mode();
    #endif

    return 0;
}
