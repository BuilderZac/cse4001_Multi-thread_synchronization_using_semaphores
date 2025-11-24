//
// Example from: http://www.amparo.net/ce155/sem-ex.c
//
// Adapted using some code from Downey's book on semaphores
//
// Compilation:
//
//       g++ main.cpp -lpthread -o main -lm
// or 
//      make
//

// I mostly just copyed from the book that was linked.

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <semaphore.h>  /* Semaphore */
#include <iostream>
using namespace std;

/*
 This wrapper class for semaphore.h functions is from:
 http://stackoverflow.com/questions/2899604/using-sem-t-in-a-qt-project
 */
class Semaphore {
public:
    // Constructor
    Semaphore(int initialValue)
    {
        sem_init(&mSemaphore, 0, initialValue);
    }
    // Destructor
    ~Semaphore()
    {
        sem_destroy(&mSemaphore); /* destroy semaphore */
    }
    
    // wait
    void wait()
    {
        sem_wait(&mSemaphore);
    }
    // signal
    void signal()
    {
        sem_post(&mSemaphore);
    }
    
    
private:
    sem_t mSemaphore;
};

class Lightswitch {
    int counter;
    Semaphore mutex;
public:
    Lightswitch() : counter(0), mutex(1) {
        counter = 0;
        mutex = Semaphore(1);
    }

    void lock(Semaphore& semaphore) {
        mutex.wait();
        counter += 1;
        if (counter == 1) {
            semaphore.wait();
        }
        mutex.signal();
    }

    void unlock(Semaphore& semaphore) {
        mutex.wait();
        counter -= 1;
        if (counter == 0) {
            semaphore.signal();
        }
        mutex.signal();
    }
};

Semaphore turnstile(1);
Semaphore roomEmpty(1);
Lightswitch readSwitch;

void* Reader(void* threadID) {
    int x = (long)threadID;
    while (1) {
        turnstile.wait();
        turnstile.signal();

        readSwitch.lock(roomEmpty);
        printf("Reader %d reading\n", x);
        fflush(stdout);
        sleep(1); // simulate reading
        readSwitch.unlock(roomEmpty);
        sleep(2);
    }
}

void* Writer(void* threadID) {
    int x = (long)threadID;
    while (1) {
        turnstile.wait();
        roomEmpty.wait();
        printf("Writer %d writing\n", x);
        fflush(stdout);
        sleep(2); // simulate writing
        roomEmpty.signal();
        turnstile.signal();
        sleep(3);
    }
}

void run_no_starve_readers_writers() {
    const int numReaders = 5;
    const int numWriters = 5;
    pthread_t readerThread[numReaders];
    pthread_t writerThread[numWriters];

    for (long r = 0; r < numReaders; ++r) {
        pthread_create(&readerThread[r], NULL, Reader, (void*)(r+1));
    }
    for (long w = 0; w < numWriters; ++w) {
        pthread_create(&writerThread[w], NULL, Writer, (void*)(w+1));
    }
    pthread_exit(NULL);
}

Semaphore noReaders(1);
Semaphore noWriters(1);
Lightswitch readSwitchWP;     // WP = writer-priority
Lightswitch writeSwitch;

void* ReaderWP(void* threadID) {
    int x = (long)threadID;
    while (1) {
        noReaders.wait();
        readSwitchWP.lock(noWriters);
        noReaders.signal();

        printf("Reader %d reading (writer-priority)\n", x);
        fflush(stdout);
        sleep(1); // simulate reading

        readSwitchWP.unlock(noWriters);
        sleep(2);
    }
}

void* WriterWP(void* threadID) {
    int x = (long)threadID;
    while (1) {
        writeSwitch.lock(noReaders);
        noWriters.wait();

        printf("Writer %d writing (writer-priority)\n", x);
        fflush(stdout);
        sleep(2); // simulate writing

        noWriters.signal();
        writeSwitch.unlock(noReaders);
        sleep(3);
    }
}

void run_writer_priority_readers_writers() {
    const int numReaders = 5;
    const int numWriters = 5;
    pthread_t readerThread[numReaders];
    pthread_t writerThread[numWriters];

    for (long r = 0; r < numReaders; ++r) {
        pthread_create(&readerThread[r], NULL, ReaderWP, (void*)(r+1));
    }
    for (long w = 0; w < numWriters; ++w) {
        pthread_create(&writerThread[w], NULL, WriterWP, (void*)(w+1));
    }
    pthread_exit(NULL);
}

Semaphore forks[5] = {Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1)};
int left(int i) { return i; }
int right(int i) { return (i + 1) % 5; }

void get_forks(int i) {
    forks[right(i)].wait();
    forks[left(i)].wait();
}

void put_forks(int i) {
    forks[right(i)].signal();
    forks[left(i)].signal();
}

void* Philosopher1(void* threadID) {
    int i = (long)threadID;
    while (1) {
        printf("Philosopher %d thinking.\n", i);
        fflush(stdout);
        sleep(1);

        get_forks(i);
        printf("Philosopher %d eating.\n", i);
        fflush(stdout);
        sleep(1);

        put_forks(i);
    }
}

void run_dining_philosophers_1() {
    pthread_t threads[5];
    for (long i = 0; i < 5; ++i)
        pthread_create(&threads[i], NULL, Philosopher1, (void*)i);

    pthread_exit(NULL);
}

Semaphore footman(4);

void get_forks_footman(int i) {
    footman.wait();
    forks[right(i)].wait();
    forks[left(i)].wait();
}

void put_forks_footman(int i) {
    forks[right(i)].signal();
    forks[left(i)].signal();
    footman.signal();
}

void* Philosopher1F(void* threadID) {
    int i = (long)threadID;
    while (1) {
        printf("Philosopher %d thinking (footman).\n", i);
        fflush(stdout);
        sleep(1);

        get_forks_footman(i);
        printf("Philosopher %d eating (footman).\n", i);
        fflush(stdout);
        sleep(1);

        put_forks_footman(i);
    }
}

void run_dining_philosophers_1_footman() {
    pthread_t threads[5];
    for (long i = 0; i < 5; ++i)
        pthread_create(&threads[i], NULL, Philosopher1F, (void*)i);

    pthread_exit(NULL);
}

void* Philosopher2(void* threadID) {
    int i = (long)threadID;
    while (1) {
        printf("Philosopher %d thinking (asymmetric).\n", i);
        fflush(stdout);
        sleep(1);

        if (i % 2 == 0) {
            forks[left(i)].wait();
            forks[right(i)].wait();
        } else {
            forks[right(i)].wait();
            forks[left(i)].wait();
        }
        printf("Philosopher %d eating (asymmetric).\n", i);
        fflush(stdout);
        sleep(1);

        forks[left(i)].signal();
        forks[right(i)].signal();
    }
}

void run_dining_philosophers_2() {
    pthread_t threads[5];
    for (long i = 0; i < 5; ++i)
        pthread_create(&threads[i], NULL, Philosopher2, (void*)i);

    pthread_exit(NULL);
}


/* global vars */
const int bufferSize = 5;
const int numConsumers = 3; 
const int numProducers = 3; 

/* semaphores are declared global so they can be accessed
 in main() and in thread routine. */
Semaphore Mutex(1);
Semaphore Spaces(bufferSize);
Semaphore Items(0);             



/*
    Producer function 
*/
void *Producer ( void *threadID )
{
    // Thread number 
    int x = (long)threadID;

    while( 1 )
    {
        sleep(3); // Slow the thread down a bit so we can see what is going on
        Spaces.wait();
        Mutex.wait();
            printf("Producer %d adding item to buffer \n", x);
            fflush(stdout);
        Mutex.signal();
        Items.signal();
    }

}

/*
    Consumer function 
*/
void *Consumer ( void *threadID )
{
    // Thread number 
    int x = (long)threadID;
    
    while( 1 )
    {
        Items.wait();
        Mutex.wait();
            printf("Consumer %d removing item from buffer \n", x);
            fflush(stdout);
        Mutex.signal();
        Spaces.signal();
        sleep(5);   // Slow the thread down a bit so we can see what is going on
    }

}


int main(int argc, char **argv )
{
    int problem = atoi(argv[1]);

    switch (problem) {
        case 1:
            run_no_starve_readers_writers();
            break;
        case 2:
            run_writer_priority_readers_writers();
            break;
        case 3:
            run_dining_philosophers_1();
            break;
        case 4:
            run_dining_philosophers_2();
            break;

        default:
            printf("Invalid problem number: %d\n", problem);
            exit(1);
    }
}
