#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include "error.h"
#include "graph.h"
#include "circular_buffer.h"

#define SHM_NAME "/bufshm"
#define SEM_USED "/used"
#define SEM_FREE "/free"
#define SEM_RES "/res"

struct bufshm *bufshm;
int shmfd, created_buf = 0;
sem_t *s_used, *s_free, *s_res;

void init_buf(void)
{
    created_buf = 1;
    s_used = sem_open(SEM_USED, O_CREAT | O_EXCL, 0600, 0);
    s_free = sem_open(SEM_FREE, O_CREAT | O_EXCL, 0600, MAX_DATA);
    s_res = sem_open(SEM_RES, O_CREAT | O_EXCL, 0600, 1);

    if (s_used == SEM_FAILED || s_free == SEM_FAILED || s_res == SEM_FAILED)
    {
        close_shm_sem();
        ERROR_MSG("failed to create semaphores", "");
    }

    bufshm = get_shm();
}

void get_buffer(void)
{
    s_used = sem_open(SEM_USED, 0);
    s_free = sem_open(SEM_FREE, MAX_DATA);
    s_res = sem_open(SEM_RES, 1);

    if (s_used == SEM_FAILED || s_free == SEM_FAILED || s_res == SEM_FAILED)
    {
        close_shm_sem();
        ERROR_MSG("failed to open sem", "");
    }

    bufshm = get_shm();
}

void close_all(void)
{
    if (created_buf)
        close_shm_sem_unlink();
    else
        close_shm_sem();
}

void write_buf(graph_t *g)
{
    sem_wait(s_res);
    sem_wait(s_free);
    struct bufshm *bufshm = get_buf();
    bufshm->buf[bufshm->wr_pos] = *g;
    sem_post(s_used);
    bufshm->wr_pos++;
    bufshm->wr_pos %= MAX_DATA;
    sem_post(s_res);
}

void read_buf(graph_t *graph)
{
    sem_wait(s_used);
    struct bufshm *bufshm = get_buf();
    graph = &bufshm->buf[bufshm->rd_pos];
    sem_post(s_free);
    bufshm->rd_pos++;
    bufshm->rd_pos %= MAX_DATA;
}

/**
 * @brief close shared memory and close and  unlink semaphores 
 */
static void close_shm_sem_unlink(void)
{
    close_shm_sem();
    sem_unlink(SEM_FREE);
    sem_unlink(SEM_RES);
    sem_unlink(SEM_USED);
}

/**
 * @brief close shared memory and semaphores
 */
static void close_shm_sem(void)
{
    munmap(bufshm, sizeof(*bufshm));
    close(shmfd);
    shm_unlink(SHM_NAME);
    sem_close(SEM_FREE);
    sem_close(SEM_USED);
    sem_close(SEM_RES);
}

static struct bufshm *get_shm(void)
{
    // create and/or open the shared memory object:
    shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);
    if (shmfd == -1)
        ERROR_MSG("shared memory opening failed", "");

    // set the size of the shared memory:
    if (ftruncate(shmfd, sizeof(struct bufshm)) < 0)
        ERROR_MSG("shared memory size failed", "");

    // map shared memory object:
    struct bufshm *bufshm;
    bufshm = mmap(NULL, sizeof(*bufshm), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (bufshm == MAP_FAILED)
        ERROR_MSG("shared memory mapping failed", "");

    // if (close(shmfd) == -1)
    //     ERROR_MSG("shared memory closing failed", "");

    return bufshm;
}
