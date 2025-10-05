#include "fifo.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"

/**
 * @brief Shortest Job First (SJF) scheduling algorithm.
 *
 * This function implements the non-preemptive Shortest Job First scheduler.
 * It selects the task with the shortest total burst time (time_ms) from the ready queue.
 * Once a task starts running, it continues until completion.
 *
 * @param current_time_ms Current time in milliseconds.
 * @param rq Pointer to the ready queue containing ready tasks.
 * @param cpu_task Double pointer to the currently running task.
 */
void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    // If there is a task currently running
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        // If the running task is finished
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }

            // Free the finished task and clear CPU
            free(*cpu_task);
            *cpu_task = NULL;
        }
    }

    // If the CPU is idle, select the shortest job from the ready queue
    if (*cpu_task == NULL && rq->head != NULL) {
        pcb_t *shortest = NULL;
        queue_elem_t *prev = NULL, *shortest_prev = NULL;
        queue_elem_t *curr = rq->head;

        // Find the task with the smallest total burst time
        while (curr) {
            pcb_t *task = curr->pcb;
            if (!shortest || task->time_ms < shortest->time_ms) {
                shortest = task;
                shortest_prev = prev;
            }
            prev = curr;
            curr = curr->next;
        }

        // Remove the shortest task from the queue
        if (shortest) {
            queue_elem_t *to_remove;

            // If the shortest is at the head
            if (shortest_prev == NULL) {
                to_remove = rq->head;
            } else {
                to_remove = shortest_prev->next;
            }

            // Remove the element (your queue API already has a function for that)
            remove_queue_elem(rq, to_remove);

            // Assign the selected task to the CPU
            *cpu_task = shortest;
        }
    }
}
