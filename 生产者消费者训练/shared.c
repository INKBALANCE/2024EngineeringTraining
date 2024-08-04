#include "shared.h"
#include <errno.h>

int create_shared_memory() {
    int size = sizeof(SharedBuffer);
    printf("Shared memory size: %d\n", size);

    int shmid = shmget(SHM_KEY, size, 0666 | IPC_CREAT | IPC_EXCL);
    if (shmid < 0) {
        if (errno == EEXIST) {
            shmid = shmget(SHM_KEY, size, 0666);
            if (shmid < 0) {
                perror("shmget retry failed");
                exit(1);
            }
        } else {
            perror("shmget failed");
            exit(1);
        }
    }
    return shmid;
}

void remove_shared_memory(int shmid) {
    shmctl(shmid, IPC_RMID, NULL);
}

