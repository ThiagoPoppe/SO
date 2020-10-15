#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define ITER 5
#define CPU_BOUND 0
#define SCPU      1
#define IO_BOUND  2

void show_means(int acc_matrix[3][4], int nTypes[3], int type) {
    if (nTypes[type] == 0)
        printf(1, "  - No processes of this type were runned\n\n");
    else {
        printf(1, "  - Average ready      time   = %d\n", acc_matrix[type][0] / nTypes[type]);
        printf(1, "  - Average run        time   = %d\n", acc_matrix[type][1] / nTypes[type]);
        printf(1, "  - Average sleeping   time   = %d\n", acc_matrix[type][2] / nTypes[type]);
        printf(1, "  - Average turnaround time   = %d\n", acc_matrix[type][3] / nTypes[type]);
        printf(1, "\n");
    }
}

int main(int argc, char* argv[]) {
    int n;
    int dummy1, dummy2;
    int retime, rutime, stime;
    
    int nTypes[3];
    int acc_matrix[3][4]; // i = type; j = 0 (ready), 1 (run), 2 (sleep), 3 (turnaround)

    memset(nTypes, 0, sizeof(nTypes));
    memset(acc_matrix, 0, sizeof(acc_matrix));

    if (argc != 2) {
        printf(2, "Usage: sanity <n>\n");
        exit();
    }

    n = atoi(argv[1]);
    for (int i = 0; i < ITER; i++) {
        for (int j = 0; j < n; j++) {
            int k = fork();

            // Failed fork
            if (k < 0) {
                printf(1, "Process %d failed fork\n", getpid());
                exit();
            }

            // Child process
            else if (k == 0) {
                int type = getpid() % 3;
                
                // CPU-bound process gets more priority
                // S-bound   process gets medium priority
                // I/O-bound process gets lower priority 
                if      (type == CPU_BOUND) setprio(2);
                else if (type == SCPU)      setprio(1);
                else if (type == IO_BOUND)  setprio(0);

                if (type == CPU_BOUND) {
                    for (dummy1 = 0; dummy1 < 100; dummy1++)
                        for (dummy2 = 0; dummy2 < 1000000; dummy2++)
                            asm("nop");
                }

                else if (type == SCPU) {
                    for (dummy1 = 0; dummy1 < 100; dummy1++) {
                        for (dummy2 = 0; dummy2 < 1000000; dummy2++) {
                            if (dummy2 % 10000 == 0)
                                yield();
                        }
                    }
                }

                // I/O-bound process
                else {
                    for (dummy1 = 0; dummy1 < 100; dummy1++)
                        sleep(1);
                }

                exit();
            }
        }

        // Waits for children to exit (avoid zombie!)
        for (int j = 0; j < n; j++) {
            int pid = wait2(&retime, &rutime, &stime);
            int type = pid % 3;
            nTypes[type]++;

            acc_matrix[type][0] += retime;
            acc_matrix[type][1] += rutime;
            acc_matrix[type][2] += stime;
            acc_matrix[type][3] += retime + rutime + stime;
            
            if (type == CPU_BOUND)
                printf(1, "PID = %d  Type = CPU-Bound  Ready time = %d  Run time = %d  Sleeping time = %d\n", 
                pid, retime, rutime, stime);
            else if (type == SCPU)
                printf(1, "PID = %d  Type = S-Bound  Ready time = %d  Run time = %d  Sleeping time = %d\n", 
                pid, retime, rutime, stime);
            else
                printf(1, "PID = %d  Type = I/O-Bound  Ready time = %d  Run time = %d  Sleeping time = %d\n", 
                pid, retime, rutime, stime);
        }
    }

    printf(1, "\n");
    printf(1, "CPU-Bound statistics:\n");
    show_means(acc_matrix, nTypes, CPU_BOUND);

    printf(1, "S-Bound statistics:\n");
    show_means(acc_matrix, nTypes, SCPU);

    printf(1, "I/O-Bound statistics:\n");
    show_means(acc_matrix, nTypes, IO_BOUND);

    exit();
}