// DCC641 - Fundamentos de Sistemas Paralelos e Distribuídos
// Primeiro Exercício de Programação
// Nome: Bernardo Reis de Almeida
// Matrícula: 2021032234

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Info that defines each thread encapsulated as a thread object.
typedef struct thread
{
    int id, group, num_of_moves;
    int **moves;
} thread_t;

// Models a single cell of the board for synchronization efforts.
// Each cell has a lock, a condition variable and accomodates potentially two threads from distinct groups.
// Each thread must acquire the lock of the cell it is trying to move to in order to move.
// If the cell is occupied, the thread should wait in its condition variable.
typedef struct board_cell
{
    pthread_mutex_t lock;
    pthread_cond_t cond;
    thread_t *threads[2];
} board_cell_t;

// Models the whole board as a matrix of cells with side of size.
// In total, there are size*size locks and condition variables, one for each cell.
typedef struct board
{
    int size;
    int num_of_threads;
    board_cell_t **cells;
} board_t;

// Encapsulates the arguments passed to each thread (the argument to be passed to the function given to the pthread object).
// These arguments are a thread object (as defined previously) and a board.
typedef struct thread_args
{
    thread_t thread;
    board_t *board;
} thread_args_t;

// Function that simulates computational time for each thread.
void passa_tempo(int tid, int x, int y, int decimos)
{
    struct timespec zzz, agora;
    static struct timespec inicio = {0, 0};
    int tstamp;

    if ((inicio.tv_sec == 0) && (inicio.tv_nsec == 0))
    {
        clock_gettime(CLOCK_REALTIME, &inicio);
    }

    zzz.tv_sec = decimos / 10;
    zzz.tv_nsec = (decimos % 10) * 100L * 1000000L;

    clock_gettime(CLOCK_REALTIME, &agora);
    tstamp = (10 * agora.tv_sec + agora.tv_nsec / 100000000L) - (10 * inicio.tv_sec + inicio.tv_nsec / 100000000L);

    printf("%3d [ %2d @(%2d,%2d) z%4d\n", tstamp, tid, x, y, decimos);

    nanosleep(&zzz, NULL);

    clock_gettime(CLOCK_REALTIME, &agora);
    tstamp = (10 * agora.tv_sec + agora.tv_nsec / 100000000L) - (10 * inicio.tv_sec + inicio.tv_nsec / 100000000L);

    printf("%3d ) %2d @(%2d,%2d) z%4d\n", tstamp, tid, x, y, decimos);
}

// Function that initializes the board and allocates its memory.
void initialize_board(board_t *board)
{
    // Instantiates internal attributes.
    scanf("%d %d", &board->size, &board->num_of_threads);

    // Allocates memory for the cells that make up the board.
    board->cells = (board_cell_t **)calloc(board->size, sizeof(board_cell_t *));
    for (int i = 0; i < board->size; i++)
        board->cells[i] = (board_cell_t *)calloc(board->size, sizeof(board_cell_t));

    // Initializes synchronization mechanisms of each cell.
    for (int i = 0; i < board->size; i++)
    {
        for (int j = 0; j < board->size; j++)
        {
            pthread_mutex_init(&board->cells[i][j].lock, NULL);
            pthread_cond_init(&board->cells[i][j].cond, NULL);
            board->cells[i][j].threads[0] = NULL;
            board->cells[i][j].threads[1] = NULL;
        }
    }
}

// Function that initializes a thread argument and allocates its memory.
// It envolves the initialization of the respective thread object and the assignment of the board.
void initialize_thread_args(board_t *board, thread_args_t *args)
{
    // Reads the ID, group and number of moves.
    scanf("%d %d %d", &args->thread.id, &args->thread.group, &args->thread.num_of_moves);

    // Allocates memory for the sequence of moves.
    args->thread.moves = (int **)calloc(args->thread.num_of_moves, sizeof(int *));
    for (int j = 0; j < args->thread.num_of_moves; j++)
        args->thread.moves[j] = (int *)calloc(3, sizeof(int));

    // Reads the sequence of moves.
    for (int j = 0; j < args->thread.num_of_moves; j++)
        scanf("%d %d %d", &args->thread.moves[j][0], &args->thread.moves[j][1], &args->thread.moves[j][2]);

    // Each thread gets a reference to the shared board.
    args->board = board;
}

// Function that deallocates the memory allocated for a board.
void destroy_board(board_t *board)
{
    // Deallocates allocated memory.
    for (int i = 0; i < board->size; i++)
        free(board->cells[i]);
    free(board->cells);
}

// Function that deallocates the memory allocated for a thread object.
void destroy_thread_args(thread_args_t *args)
{
    // Deallocates allocated memory.
    for (int j = 0; j < args->thread.num_of_moves; j++)
        free(args->thread.moves[j]);
    free(args->thread.moves);
}

// Walks the thread through its respective path, given in the arguments.
void *thread_walk(void *args)
{
    // Extracts the current thread's information from the thread argument (thread object and shared board).
    thread_t *thread = &((thread_args_t *)args)->thread;
    board_t *board = ((thread_args_t *)args)->board;

    // Guard for errors. If there are no moves, just finalizes execution.
    if (thread->num_of_moves == 0)
        return NULL;

    // Creates auxiliary variables to track the thread's position (first or second) in the current and the previous cells.
    int last_position;
    int curr_position;

    // Going to the first position in the board.
    // Extracts the coordinates and the amount of time to spend there.
    int x = thread->moves[0][0];
    int y = thread->moves[0][1];
    int time = thread->moves[0][2];

    // Tries to get the lock to manipulate the cell it is trying to move to.
    pthread_mutex_lock(&board->cells[x][y].lock);
    // After acquiring the lock and with the right to manipulate the cell exclusively...
    // Wait in the condition variable if there are one from the same group or two in total inside the cell, releasing the lock.
    while ((board->cells[x][y].threads[0] != NULL && board->cells[x][y].threads[1] != NULL)                     // Both slots are full.
           || (board->cells[x][y].threads[0] != NULL && board->cells[x][y].threads[0]->group == thread->group)  // There is a thread from the same group in slot 0.
           || (board->cells[x][y].threads[1] != NULL && board->cells[x][y].threads[1]->group == thread->group)) // There is a thread from the same group in slot 1.
        pthread_cond_wait(&board->cells[x][y].cond, &board->cells[x][y].lock);
    // After it is possible to move to the cell, the thread reacquires the lock and...
    // Decides which slot to go to based on availability (which one is currently empty).
    (board->cells[x][y].threads[0] == NULL) ? (curr_position = 0) : (curr_position = 1);
    board->cells[x][y].threads[curr_position] = thread;
    // The thread then releases the lock for the current cell, as it will not be manipulated anymore.
    pthread_mutex_unlock(&board->cells[x][y].lock);
    // Executes the function to simulate computational time.
    passa_tempo(thread->id, x, y, time);

    // After being put in the first position of the board, traverses each move, executing it.
    for (int i = 1; i < thread->num_of_moves; i++)
    {
        // Updates auxiliary variables.
        last_position = curr_position;

        // Gathers old coordinates.
        int old_x = thread->moves[i - 1][0];
        int old_y = thread->moves[i - 1][1];

        // Gathers next coordinates and amount of time to spend in the next cell.
        x = thread->moves[i][0];
        y = thread->moves[i][1];
        time = thread->moves[i][2];

        // Guard for errors. If the next cell is the same, just spend more time there.
        if((old_x == x) && (old_y == y))
        {
            // Executes the function to simulate computational time.
            passa_tempo(thread->id, x, y, time);
            // Goes to the next position.
            continue;
        }

        // Tries to get the lock to manipulate the cell it is trying to move to.
        pthread_mutex_lock(&board->cells[x][y].lock);
        // After acquiring the lock and with the right to manipulate the cell exclusively...
        // Wait in the condition variable if there are one from the same group or two in total inside the cell, releasing the lock.
        while (((board->cells[x][y].threads[0] != NULL) && (board->cells[x][y].threads[1] != NULL))                     // Both slots are full.
               || ((board->cells[x][y].threads[0] != NULL) && (board->cells[x][y].threads[0]->group == thread->group))  // There is a thread from the same group in slot 0.
               || ((board->cells[x][y].threads[1] != NULL) && (board->cells[x][y].threads[1]->group == thread->group))) // There is a thread from the same group in slot 1.
            pthread_cond_wait(&board->cells[x][y].cond, &board->cells[x][y].lock);
        // After it is possible to move to the cell, the thread reacquires the lock and...
        // Decides which slot to go to based on availability (which one is currently empty).
        (board->cells[x][y].threads[0] == NULL) ? (curr_position = 0) : (curr_position = 1);
        board->cells[x][y].threads[curr_position] = thread;
        // The thread removes its position from the previous cell and signals it to the other threads potentially waiting.
        board->cells[old_x][old_y].threads[last_position] = NULL;
        pthread_cond_broadcast(&board->cells[old_x][old_y].cond);
        // The thread then releases the lock for the current cell, as it will not be manipulated anymore.
        pthread_mutex_unlock(&board->cells[x][y].lock);
        // Executes the function to simulate computational time.
        passa_tempo(thread->id, x, y, time);
    }

    // After finishing the last move and passing time, the thread removes its position from the current cell
    // and signals it to the other threads potentially waiting, finalizing its own execution.
    board->cells[x][y].threads[curr_position] = NULL;
    pthread_cond_broadcast(&board->cells[x][y].cond);

    // Finishes the execution.
    return NULL;
}

int main()
{
    // Instantiates and initializes the board.
    board_t board;
    initialize_board(&board);

    // Instantiates and initializes the pthreads and the thread arguments (thread objects associated with a shared board).
    pthread_t threads[board.num_of_threads];
    thread_args_t args[board.num_of_threads];
    for (int i = 0; i < board.num_of_threads; i++)
        initialize_thread_args(&board, &args[i]);

    // Executes each thread.
    for (int i = 0; i < board.num_of_threads; i++)
        pthread_create(&threads[i], NULL, thread_walk, &args[i]);

    // Waits for the completion of all threads before exiting.
    for (int i = 0; i < board.num_of_threads; i++)
        pthread_join(threads[i], NULL);

    // Destroys the variables defining the problem.
    // Necessary for dynamic memory deallocation.
    for (int i = 0; i < board.num_of_threads; i++)
        destroy_thread_args(&args[i]);
    destroy_board(&board);

    return 0;
}