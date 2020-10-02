#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define deb(x) printf(1, "%s=%d\n", #x, x)

int main(int argc, const char** argv) {
    if (argc < 2) {
        printf(2, "Usage: sanity <n>\n");
        exit();
    }

    int n = atoi(argv[1]);
    for (int i = 0; i < n; i++) {
        int f = fork();

        // Failed fork
        if (f < 0) {
            printf(2, "%d failed fork\n", getpid());
            exit();
        }

        // Child process
        else if (f == 0) {
            exit();
        }
    }

    // Waiting for processes (avoid zombie)
    for (int i = 0; i < n; i++)
        wait();

    exit();
}