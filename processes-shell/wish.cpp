// wish - Wisconsin Shell (OSTEP processes-shell project), C++ implementation.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

namespace {

// The one and only error message required by the specification.
void print_error() {
    const char error_message[] = "An error has occurred\n";
    ssize_t written = write(STDERR_FILENO, error_message, strlen(error_message));
    (void)written;
}

// Splits a line into words separated by any amount of spaces and tabs.
std::vector<std::string> tokenize(const std::string &line) {
    std::vector<std::string> tokens;
    const char *whitespace = " \t";
    size_t start = line.find_first_not_of(whitespace);
    while (start != std::string::npos) {
        size_t end = line.find_first_of(whitespace, start);
        tokens.push_back(line.substr(start, end - start));
        start = line.find_first_not_of(whitespace, end);
    }
    return tokens;
}

// Runs an external program in a child process and waits for it to finish.
// For now programs are looked up only in /bin.
void run_program(const std::vector<std::string> &args) {
    std::string program = "/bin/" + args[0];

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
        return;
    }

    if (pid == 0) {
        // execv() needs a NULL-terminated array of C strings.
        std::vector<char *> argv;
        for (const std::string &arg : args) {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execv(program.c_str(), argv.data());
        // execv() returns only on failure.
        print_error();
        _exit(1);
    }

    if (waitpid(pid, nullptr, 0) < 0) {
        print_error();
    }
}

// Handles one line of input.
void process_line(const std::string &line) {
    std::vector<std::string> args = tokenize(line);
    if (args.empty()) {
        return;
    }

    if (args[0] == "exit") {
        exit(0);
    }

    run_program(args);
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
