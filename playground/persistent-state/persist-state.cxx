#include "persist-state.hpp"

PersistState::PersistState() {
    // Keep track of datastructure initialization responsibility.
    bool perform_init = true;

    // Creation is attempted first as this solves race conditions resulting in
    // slightly slower 'happy path'.
    // Try to create a new shm but only if it does not exist
    int mem_fd = shm_open(SHMEM_NAME.c_str(), O_CREAT | O_EXCL | O_RDWR,
                          S_IRUSR | S_IWUSR);

    // If creation failed it might already exist, if so don't initialize the
    // datastructure
    if(mem_fd < 0) {
        perform_init = false;
        mem_fd = shm_open(SHMEM_NAME.c_str(), O_RDWR, S_IRWXO | S_IRWXG | S_IRWXU);
    }

    // If still failed, abort
    if(mem_fd < 0) return;

    if (ftruncate(mem_fd, sizeof(struct shmbuf)) == -1) return;

    buffer = (struct shmbuf*) mmap(NULL, sizeof(*buffer), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0);

    // If mapping fails abort
    if (buffer == MAP_FAILED) return;

    if(perform_init) {
        if(sem_init(&buffer->sem1, 1, 1) < 0) return;
        buffer->cnt = 0;
    }

    initialized = true;
}

int PersistState::read(size_t &count) {
    if(!initialized) return -1;

    sem_wait(&buffer->sem1);
    count = buffer->cnt;
    sem_post(&buffer->sem1);

    return 0;
}

int PersistState::write(size_t count) {
    if(!initialized) return -1;

    sem_wait(&buffer->sem1);
    buffer->cnt = count;
    sem_post(&buffer->sem1);

    return 0;
}
