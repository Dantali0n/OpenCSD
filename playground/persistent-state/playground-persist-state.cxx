#include "persist-state.hpp"

#include <iostream>

/**
 * Simple demo showing the use of shmem to have global host state across
 * processes. This example is not threadsafe, the applications can not be
 * launched multiple times concurrently due to a race condition.
 */
int main(int argc, char *argv[]) {
    size_t count = 0;

    PersistState state;

    if(state.read(count) < 0) return -1;
    if(state.write(count+1) < 0) return -1;
    if(state.read(count) < 0) return -1;

    std::cout << "Global count: " << count << std::endl;

    exit(EXIT_SUCCESS);
}
