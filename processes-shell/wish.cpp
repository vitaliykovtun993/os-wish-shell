// wish - Wisconsin Shell (OSTEP processes-shell project), C++ implementation.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

// Directories searched for executables, in order. Changed by `path`.
std::vector<std::string> search_path = {"/bin"};

// A parsed command: program name with arguments and an optional file
// that receives both stdout and stderr (empty if there is no redirection).
struct Command {
    std::vector<std::string> args;
    std::string out_file;
};

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

// Parses "args [> file]". The '>' does not need spaces around it.
// Returns false on a syntax error: more than one '>', no command before it,
// or anything other than exactly one file name after it.
bool parse_command(const std::string &text, Command &cmd) {
    size_t redirect = text.find('>');
    if (redirect == std::string::npos) {
        cmd.args = tokenize(text);
        cmd.out_file.clear();
        return true;
    }

    if (text.find('>', redirect + 1) != std::string::npos) {
        return false;
    }

    cmd.args = tokenize(text.substr(0, redirect));
    std::vector<std::string> files = tokenize(text.substr(redirect + 1));
    if (cmd.args.empty() || files.size() != 1) {
        return false;
    }
    cmd.out_file = files[0];
    return true;
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

// Starts an external program in a child process without waiting for it.
// Returns the child's pid, or -1 if the program could not be started.
pid_t start_program(const Command &cmd) {
    std::string program = find_program(cmd.args[0]);
    if (program.empty()) {
        print_error();
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
        return -1;
    }

    if (pid == 0) {
        // Redirection sends both stdout and stderr to the file.
        if (!cmd.out_file.empty()) {
            int fd = open(cmd.out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0 || dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0) {
                print_error();
                _exit(1);
            }
            close(fd);
        }

        // execv() needs a NULL-terminated array of C strings.
        std::vector<char *> argv;
        for (const std::string &arg : cmd.args) {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execv(program.c_str(), argv.data());
        // execv() returns only on failure.
        print_error();
        _exit(1);
    }

    return pid;
}

// Splits a line into the parts separated by '&'. Empty parts are kept
// (and later ignored), so "&" or "cmd &" are not errors.
std::vector<std::string> split_parallel(const std::string &line) {
    std::vector<std::string> parts;
    size_t start = 0;
    size_t amp;
    while ((amp = line.find('&', start)) != std::string::npos) {
        parts.push_back(line.substr(start, amp - start));
        start = amp + 1;
    }
    parts.push_back(line.substr(start));
    return parts;
}

// Handles one line of input: starts every command of "cmd1 & cmd2 & ..."
// first, then waits for all of them to finish.
void process_line(const std::string &line) {
    std::vector<pid_t> children;

    for (const std::string &part : split_parallel(line)) {
        Command cmd;
        if (!parse_command(part, cmd)) {
            print_error();
            continue;
        }
        if (cmd.args.empty()) {
            continue;
        }

        if (!run_builtin(cmd.args)) {
            pid_t pid = start_program(cmd);
            if (pid > 0) {
                children.push_back(pid);
            }
        }
    }

    for (pid_t pid : children) {
        if (waitpid(pid, nullptr, 0) < 0) {
            print_error();
        }
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
