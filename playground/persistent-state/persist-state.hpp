#ifndef PERSIST_STATE_HPP
#define PERSIST_STATE_HPP

extern "C" {
    #include <sys/mman.h>
    #include <fcntl.h>
    #include <semaphore.h>
    #include <sys/stat.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <ctype.h>
}

#include <string>

/**
 * Datastructure for shared buffer.
 */
struct shmbuf {
    sem_t  sem1;            /* POSIX unnamed semaphore */
    size_t cnt;             /* Global counter */
};

static const std::string SHMEM_NAME = "/opencsdshmem";

class PersistState {
protected:
    bool initialized = false;
    struct shmbuf* buffer;
public:
    PersistState();

    int write(size_t count);
    int read(size_t &count);
};

#endif // PERSIST_STATE_HPP