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

// Directories searched for executables, in order. Changed by `path`.
std::vector<std::string> search_path = {"/bin"};

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

// Finds an executable in the search path. Returns an empty string if the
// program is not found in any directory.
std::string find_program(const std::string &name) {
    for (const std::string &dir : search_path) {
        std::string candidate = dir + "/" + name;
        if (access(candidate.c_str(), X_OK) == 0) {
            return candidate;
        }
    }
    return "";
}

// exit: takes no arguments.
void builtin_exit(const std::vector<std::string> &args) {
    if (args.size() != 1) {
        print_error();
        return;
    }
    exit(0);
}

// cd: takes exactly one argument.
void builtin_cd(const std::vector<std::string> &args) {
    if (args.size() != 2 || chdir(args[1].c_str()) != 0) {
        print_error();
    }
}

// path: replaces the search path with the given directories (possibly none).
void builtin_path(const std::vector<std::string> &args) {
    search_path.assign(args.begin() + 1, args.end());
}

// Runs a built-in command. Returns false if the command is not a built-in.
bool run_builtin(const std::vector<std::string> &args) {
    if (args[0] == "exit") {
        builtin_exit(args);
    } else if (args[0] == "cd") {
        builtin_cd(args);
    } else if (args[0] == "path") {
        builtin_path(args);
    } else {
        return false;
    }
    return true;
}

// Runs an external program in a child process and waits for it to finish.
void run_program(const std::vector<std::string> &args) {
    std::string program = find_program(args[0]);
    if (program.empty()) {
        print_error();
        return;
    }

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

    if (!run_builtin(args)) {
        run_program(args);
    }
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
