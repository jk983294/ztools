#include <zerg/tool/bash_cmd.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

using namespace std;

namespace zerg {

BashCommand::BashCommand(std::string command, std::string type) {
    m_command = command;
    m_type = type;
}

FILE* BashCommand::Run() {
    pid_t child_pid;
    int fd[2];
    if (pipe(fd) == -1) {
        perror("Failed to create pipe");
        exit(EXIT_FAILURE);
    }

    if ((child_pid = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {  // child
        if (m_type == "r") {
            close(fd[READ]);     // Close the READ end of the pipe since the child's fd is write-only
            dup2(fd[WRITE], 1);  // Redirect stdout to pipe
        } else {
            close(fd[WRITE]);   // Close the WRITE end of the pipe since the child's fd is read-only
            dup2(fd[READ], 0);  // Redirect stdin to pipe
        }

        setpgid(child_pid, child_pid);  // Needed so negative PIDs can kill children of /bin/sh
        execl("/bin/sh", "/bin/sh", "-c", m_command.c_str(), nullptr);
        exit(0);
    } else {
        if (m_type == "r") {
            close(fd[WRITE]);  // Close the WRITE end of the pipe since parent's fd is read-only
        } else {
            close(fd[READ]);  // Close the READ end of the pipe since parent's fd is write-only
        }
    }

    pid = child_pid;
    if (m_type == "r") {
        pFile = fdopen(fd[READ], "r");
        return pFile;
    }
    pFile = fdopen(fd[WRITE], "w");
    return pFile;
}

int BashCommand::Shutdown() {
    int stat;
    fclose(pFile);
    kill(pid, SIGKILL);
    while (waitpid(pid, &stat, 0) == -1) {
        if (errno != EINTR) {
            stat = -1;
            break;
        }
    }
    return stat;
}
}
