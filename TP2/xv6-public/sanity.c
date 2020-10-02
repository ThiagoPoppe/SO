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
    deb(n);

    exit();
}