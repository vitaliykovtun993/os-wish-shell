// wish - Wisconsin Shell (OSTEP processes-shell project), C++ implementation.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <unistd.h>

namespace {

// The one and only error message required by the specification.
void print_error() {
    const char error_message[] = "An error has occurred\n";
    ssize_t written = write(STDERR_FILENO, error_message, strlen(error_message));
    (void)written;
}

// Handles one line of input. Parsing and execution come in the next stages.
void process_line(const std::string &line) {
    (void)line;
}

}  // namespace

int main(int argc, char *argv[]) {
    // wish [batch_file] -- anything more is an error.
    if (argc > 2) {
        print_error();
        exit(1);
    }

    FILE *input = stdin;
    bool interactive = true;
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (input == nullptr) {
            print_error();
            exit(1);
        }
        interactive = false;
    }

    char *buffer = nullptr;
    size_t capacity = 0;
    while (true) {
        if (interactive) {
            std::cout << "wish> " << std::flush;
        }

        ssize_t length = getline(&buffer, &capacity, input);
        if (length == -1) {
            break;  // EOF
        }

        std::string line(buffer, length);
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        process_line(line);
    }

    free(buffer);
    if (input != stdin) {
        fclose(input);
    }
    exit(0);
}
