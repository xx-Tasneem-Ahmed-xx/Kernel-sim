#define _POSIX_C_SOURCE 200809L
#include "headers.h"
#include <signal.h>

PriorityQueue pq;
PCBQueue *waiting_queue = NULL;

int msgq_id;
key_t msgq_key;
pid_t scheduler_pid;
int semid;

typedef struct
{
    SchedulingAlgorithm algorithm;
    int quantum;
    const char *filename;
} SchedulingParams;

void clear_resources(int signum);

void read_processes(const char *filename, PriorityQueue *pq);

void initializeIPC();

void handle_terminated_processes();

void createProcess(PCB *process);

int parse_args(int argc, char *argv[], SchedulingParams *params)
{
    params->quantum = 0;
    params->filename = NULL;
    params->algorithm = HPF;

    if (argc < 5)
    {
        fprintf(stderr, "Usage: %s -s <scheduling-algorithm> -f <processes-text-file> [-q <quantum>]\n", argv[0]);
        return 0;
    }

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc)
        {
            if (strcmp(argv[i + 1], "rr") == 0)
            {
                params->algorithm = RR;
            }
            else if (strcmp(argv[i + 1], "hpf") == 0)
            {
                params->algorithm = HPF;
            }
            else if (strcmp(argv[i + 1], "srtn") == 0)
            {
                params->algorithm = SRTN;
            }
            else
            {
                fprintf(stderr, "Unknown scheduling algorithm: %s\n", argv[i + 1]);
                return 0;
            }
            i++;
        }
        else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
        {
            params->filename = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "-q") == 0 && i + 1 < argc)
        {
            params->quantum = atoi(argv[i + 1]);
            i++;
        }
    }

    if (params->filename == NULL)
    {
        fprintf(stderr, "Missing process file (-f <filename>)\n");
        return 0;
    }
    if (params->algorithm == RR && params->quantum <= 0)
    {
        fprintf(stderr, "Round Robin requires a positive quantum (-q <quantum>)\n");
        return 0;
    }
    return 1;
}
void clearMemory(int signum, siginfo_t *info, void *context)
{

    if (info)
    {
        pid_t sender_pid = info->si_pid;
        printf("Signal %d received from PID: %d\n", signum, sender_pid);
        // free(mp[sender_pid]);

        if (deallocate_memory(sender_pid))
            log_message(LOG_INFO, "Memory deallocated successfully");
        else
            log_message(LOG_ERROR, "Failed to deallocate memory");
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, clear_resources);

    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = clearMemory;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_SIGINFO;

    sigaction(SIGUSR1, &sa_usr1, NULL);
    initialize_memory_Segment();
    SchedulingParams params;
    if (!parse_args(argc, argv, &params))
    {
        exit(EXIT_FAILURE);
    }

    print_divider("OS Scheduler Simulation");

    char algorithm_name[50];
    if (params.algorithm == HPF)
    {
        strcpy(algorithm_name, "HPF");
    }
    else if (params.algorithm == SRTN)
    {
        strcpy(algorithm_name, "SRTN");
    }
    else if (params.algorithm == RR)
    {
        sprintf(algorithm_name, "Round Robin (q=%d)", params.quantum);
    }
    else
    {
        strcpy(algorithm_name, "Unknown");
    }

    log_message(LOG_SYSTEM, "Starting simulation with %s algorithm", algorithm_name);
    log_message(LOG_INFO, "Loading processes from %s", params.filename);

    int process_count = 0;
    pq_init(&pq, 20, SORT_BY_ARRIVAL_TIME);
    waiting_queue = queue_init(100);

    read_processes(params.filename, &pq);
    initializeIPC();

    semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("semget failed");
        exit(1);
    }

    union semun arg;
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1)
    {
        perror("semctl failed");
        exit(1);
    }

    scheduler_pid = fork();
    if (scheduler_pid == 0)
    {
        char algorithm_str[10], semid_str[10];
        sprintf(algorithm_str, "%d", params.algorithm);
        sprintf(semid_str, "%d", semid);

        if (params.algorithm == RR)
        {
            char quantum_str[10];
            sprintf(quantum_str, "%d", params.quantum);
            execl("./bin/scheduler", "scheduler", algorithm_str, semid_str, quantum_str, NULL);
        }
        else
        {
            execl("./bin/scheduler", "scheduler", algorithm_str, semid_str, NULL);
        }
        perror("Error executing scheduler");
        exit(EXIT_FAILURE);
    }

    pid_t clk_pid = fork();
    if (clk_pid == 0)
    {
        signal(SIGINT, clear_resources);
        init_clk();
        sync_clk();
        run_clk();
    }

    sync_clk();

    Process p;
    while (1)
    {

        if (pq_empty(&pq) && queue_empty(waiting_queue))
        {
            log_message(LOG_INFO, "No more processes to schedule");
            break;
        }

        int current_time = get_clk();
        if (!pq_empty(&pq) && pq_top(&pq).arrival_time <= current_time)
        {
            down(semid);

            while (!queue_empty(waiting_queue))
            {
                printf("DAMNNNNNNNNNNNNNNN\n");
                PCB waiting_pcb = queue_front(waiting_queue);

                if (allocate_memory(waiting_pcb.id_from_file, waiting_pcb.memory_size))
                {
                    createProcess(&waiting_pcb);
                    update_id(waiting_pcb.id_from_file, waiting_pcb.pid);
                    queue_dequeue(waiting_queue);
                }
                else
                    break;

                MsgBuffer message;
                message.mtype = 1;
                message.pcb = waiting_pcb;

                if (msgsnd(msgq_id, &message, sizeof(message.pcb), !IPC_NOWAIT) == -1)
                {
                    perror("Error sending process to scheduler");
                    exit(EXIT_FAILURE);
                }
            }

            while (!pq_empty(&pq) && pq_top(&pq).arrival_time <= current_time)
            {
                p = pq_top(&pq);

                PCB *new_pcb = (PCB *)malloc(sizeof(PCB));
                new_pcb->id_from_file = p.id;
                new_pcb->arrival_time = p.arrival_time;
                new_pcb->execution_time = p.execution_time;
                new_pcb->priority = p.priority;
                new_pcb->waiting_time = 0;
                new_pcb->start_time = -1;
                new_pcb->remaining_time = p.execution_time;
                new_pcb->memory_size = p.memory_size;

                if (allocate_memory(new_pcb->id_from_file, new_pcb->memory_size))
                {
                    createProcess(new_pcb);

                    update_id(new_pcb->id_from_file, new_pcb->pid);
                    printf("\n\nESRARARARA %d\n\n", new_pcb->pid);
                }
                else
                    queue_enqueue(waiting_queue, *new_pcb);

                pq_pop(&pq);

                log_message(LOG_PROCESS, "Process %d arrived at time %d, Runtime: %d, Priority: %d",
                            new_pcb->id_from_file, current_time, new_pcb->execution_time, new_pcb->priority);

                MsgBuffer message;
                message.mtype = 1;
                message.pcb = *new_pcb;

                if (msgsnd(msgq_id, &message, sizeof(message.pcb), !IPC_NOWAIT) == -1)
                {
                    perror("Error sending process to scheduler");
                    exit(EXIT_FAILURE);
                }

                process_count++;
            }

            up(semid);
        }

        usleep(100000);
    }

    print_divider("Process Generation Complete");
    log_message(LOG_SYSTEM, "All %d processes sent to scheduler", process_count);
    kill(scheduler_pid, SIGUSR1);

    int status;
    log_message(LOG_SYSTEM, "Waiting for scheduler process %d to terminate", scheduler_pid);
    waitpid(scheduler_pid, &status, 0);
    log_message(LOG_SYSTEM, "Scheduler process %d terminated", scheduler_pid);

    clear_resources(0);
    printf("a333333333333333\n");
    return 0;
}

void createProcess(PCB *process)
{
    int shmid;
    int *shm_remaining_time;
    pid_t pid;

    shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("shmget failed");
        exit(-1);
    }

    shm_remaining_time = (int *)shmat(shmid, NULL, 0);
    if (shm_remaining_time == (int *)-1)
    {
        perror("shmat failed");
        exit(-1);
    }

    *shm_remaining_time = process->remaining_time;
    process->shm_id = shmid;

    shmdt(shm_remaining_time);
    pid_t process_generator_pid = getpid();
    pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(-1);
    }

    if (pid == 0)
    {
        char runtime_str[10], shmid_str[20], scheduler_pid_str[20], process_generator_pid_str[20];
        sprintf(runtime_str, "%d", process->remaining_time);
        sprintf(shmid_str, "%d", shmid);
        sprintf(scheduler_pid_str, "%d", scheduler_pid);
        sprintf(process_generator_pid_str, "%d", process_generator_pid);

        execl("./bin/process.out", "process.out", runtime_str, shmid_str, scheduler_pid_str, process_generator_pid_str, NULL);
        perror("execl failed");
        exit(1);
    }
    else
    {
        process->pid = pid;
        kill(pid, SIGSTOP);
    }
}

void initializeIPC()
{
    msgq_key = ftok("keyfile", 'A');
    if (msgq_key == -1)
    {
        perror("Error creating message queue key");
        exit(EXIT_FAILURE);
    }

    msgq_id = msgget(msgq_key, IPC_CREAT | 0666);
    if (msgq_id == -1)
    {
        perror("Error creating message queue");
        exit(EXIT_FAILURE);
    }

    log_message(LOG_SYSTEM, "Message queue created with ID: %d", msgq_id);
}

void read_processes(const char *filename, PriorityQueue *pq)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open processes.txt");
        exit(EXIT_FAILURE);
    }

    char line[256];
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file))
    {
        if (line[0] == '#')
        {
            continue;
        }

        Process p;
        if (sscanf(line, "%d %d %d %d %d", &p.id, &p.arrival_time, &p.execution_time, &p.priority,
                   &p.memory_size) == 5)
        {
            pq_push(pq, p);
        }
    }

    fclose(file);
}

void clear_resources(int signum)
{
    (void)signum;
    pq_free(&pq);
    msgctl(msgq_id, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    destroy_clk(1);
    log_message(LOG_SYSTEM, "Cleaned up and exiting");
    exit(0);
}
