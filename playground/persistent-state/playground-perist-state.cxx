#include <string>

extern C {
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
}

static constexpr std:string SHMEM_NAME = "/OpenCSDSHMEM";

int main(int argc, char *argv[]) {
    int mem_fd = shm_open(SHMEM_NAME.c_string(), O_CREAT|O_RDWR);
    return 0;
}
