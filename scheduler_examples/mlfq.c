#ifndef PCB_H
#define PCB_H


#include <stdint.h>
#include <sys/types.h>

typedef struct {
    pid_t pid;
    int status;
    uint32_t slice_start_ms;
    uint32_t sockfd;
    uint32_t time_ms;
    uint32_t ellapsed_time_ms;
    int priority; // usado no MLFQ
} pcb_t;

#endif // PCB_H

#include "queue.h"
#include "pcb.h"         // <--- aqui está a correção
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define PROCESS_REQUEST_DONE 1
#define TIME_SLICE_MS 500
#define TICKS_MS 100
#define NUM_QUEUES 3  // Prioridades: 0 (alta), 1 (média), 2 (baixa)

typedef struct {
    pid_t pid;
    int request;
    uint32_t time_ms;
} msg_t;


void mlfq_scheduler(uint32_t current_time_ms, queue_t *queues[], pcb_t **cpu_task) {

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
            int next_priority = (*cpu_task)->priority + 1;
            if (next_priority >= NUM_QUEUES) next_priority = NUM_QUEUES - 1;
            (*cpu_task)->priority = next_priority;
            enqueue_pcb(queues[next_priority], *cpu_task);
            *cpu_task = NULL;
        }
    }

    // Se o CPU estiver livre, procurar tarefa na fila de maior prioridade
    if (*cpu_task == NULL) {
        for (int i = 0; i < NUM_QUEUES; i++) {
            if (queues[i]->head != NULL) {
                *cpu_task = dequeue_pcb(queues[i]);
                if (*cpu_task) {
                    (*cpu_task)->slice_start_ms = current_time_ms;
                    (*cpu_task)->priority = i;
                }
                break;
            }
        }
    }
}