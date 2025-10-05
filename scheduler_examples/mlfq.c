#include "fifo.h"   // for pcb_t, queue_t, enqueue_pcb, dequeue_pcb, etc.
#include "queue.h"
#include "msg.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define NUM_LEVELS      3
#define TIME_SLICE_MS   500

/**
 * @brief Multi-Level Feedback Queue (MLFQ) scheduler.
 *
 * - Drains incoming ready_queue into level 0 preserving arrival order.
 * - Each level has the same time slice (500 ms). If a task exhausts its slice
 *   it is demoted one level (until the lowest).
 * - Higher priority levels (0 is highest) are always selected first.
 *
 * @param current_time_ms Current simulation time in milliseconds.
 * @param ready_queue Pointer to the global ready queue that receives new processes.
 * @param cpu_task Double pointer to the PCB currently running on the CPU.
 */
void mlfq_scheduler(uint32_t current_time_ms, queue_t *ready_queue, pcb_t **cpu_task) {
    /* Internal static state for the MLFQ instance */
    static queue_t levels[NUM_LEVELS];
    static int initialized = 0;
    /* Start time for the currently executing time slice */
    static uint32_t slice_start_time = 0;
    /* Priority level of the currently executing task (valid only if *cpu_task != NULL) */
    static int current_level = 0;

    /* Lazy initialization of internal queues */
    if (!initialized) {
        for (int i = 0; i < NUM_LEVELS; i++) {
            levels[i].head = NULL;
            levels[i].tail = NULL;
        }
        initialized = 1;
    }

    /* 1) Move any newly-arrived PCBs from the external ready_queue into level 0.
     *    This preserves arrival order: dequeue from ready_queue and enqueue into levels[0].
     */
    while (ready_queue->head != NULL) {
        pcb_t *p = dequeue_pcb(ready_queue);
        if (p) {
            /* New arrivals always enter at highest priority (level 0) */
            enqueue_pcb(&levels[0], p);
        } else {
            break;
        }
    }

    /* 2) If there is a running task, advance its elapsed time and check completion/time-slice */
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        /* Task finished? */
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
        /* Time slice exhausted but task not finished -> demote and enqueue */
        else if ((current_time_ms - slice_start_time) >= TIME_SLICE_MS) {
            int next_level = current_level;
            if (next_level < (NUM_LEVELS - 1)) {
                next_level++;
            }
            enqueue_pcb(&levels[next_level], *cpu_task);
            *cpu_task = NULL;
        }
    }

    /* 3) If CPU is idle, select the highest-priority non-empty level and dequeue */
    if (*cpu_task == NULL) {
        for (int lvl = 0; lvl < NUM_LEVELS; lvl++) {
            if (levels[lvl].head != NULL) {
                pcb_t *next = dequeue_pcb(&levels[lvl]);
                if (next) {
                    *cpu_task = next;
                    current_level = lvl;
                    slice_start_time = current_time_ms;
                }
                break;
            }
        }
    }
}