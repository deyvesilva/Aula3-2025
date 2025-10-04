#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TIME_SLICE_MS 500
#define TICKS_MS 100  // Ajuste conforme o seu simulador
#define PROCESS_REQUEST_DONE 1

typedef struct {
    pid_t pid;
    int request;
    uint32_t time_ms;
} msg_t;


void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        // Verifica se a tarefa terminou
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            free(*cpu_task);
            *cpu_task = NULL;
        }
        // Verifica se o time slice expirou
        else if ((current_time_ms - (*cpu_task)->slice_start_ms) >= TIME_SLICE_MS) {
            enqueue_pcb(rq, *cpu_task);
            *cpu_task = NULL;
        }
    }

    // Se o CPU estiver livre, buscar próxima tarefa
    if (*cpu_task == NULL && rq->head != NULL) {
        *cpu_task = dequeue_pcb(rq);
        if (*cpu_task) {
            (*cpu_task)->slice_start_ms = current_time_ms;
        }
    }
}