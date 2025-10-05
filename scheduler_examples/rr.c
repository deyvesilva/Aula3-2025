#include "fifo.h"
#include <stdio.h>
#include <stdlib.h>
#include "msg.h"
#include <unistd.h>

#define TIME_SLICE_MS 500  // Quantum of 500 milliseconds

/**
 * @brief Round-Robin (RR) scheduling algorithm.
 *
 * Each task runs for up to 500ms. If it doesn't finish, it goes back to the end of the queue.
 * If it finishes, it is removed.
 *
 * @param current_time_ms Current time in milliseconds.
 * @param rq Pointer to the ready queue.
 * @param cpu_task Double pointer to the currently running task.
 */
void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    static uint32_t slice_start_time = 0;  // Start current time-slice

    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        // Verify is task finished
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
        // Check if reached time-slice, but task not finished
        else if ((current_time_ms - slice_start_time) >= TIME_SLICE_MS) {
            enqueue_pcb(rq, *cpu_task);  // Task go back to the queue
            *cpu_task = NULL;
        }
    }

    if (*cpu_task == NULL) { // If CPU is idle
        *cpu_task = dequeue_pcb(rq); //removed  from queue
        slice_start_time = current_time_ms;  // restart the time-slice
    }
}

