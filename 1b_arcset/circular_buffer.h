#define MAX_DATA (50)

static struct bufshm
{
    unsigned int wr_pos;
    unsigned int rd_pos;
    graph_t buf[MAX_DATA];
};

void init_buf(void);
void get_buffer(void);
void close_all(void);
void write_buf(graph_t *g);
void read_buf(graph_t *graph);
static void close_shm_sem_unlink(void);
static void close_shm_sem(void);
static struct bufshm *get_shm(void);
