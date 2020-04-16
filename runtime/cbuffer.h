#include "defs.h"
#include <base/log.h>


// void initialize(thread_t ** rq, uint64_t *head, uint64_t *tail, uint64_t size)
// {

// }

void cbuffer_push(struct kthread *kt, thread_t *item)
{
    if(kt->load == RUNTIME_RQ_SIZE) {
        log_err("RQ is full");
        return;
    }
    kt->rq[kt->rq_head++] = item;
    if(kt->rq_head == RUNTIME_RQ_SIZE)
        kt->rq_head = 0;
    kt->q_ptrs->rq_head++;
    if(kt->q_ptrs->rq_head == RUNTIME_RQ_SIZE)
        kt->q_ptrs->rq_head = 0;
    kt->load++;
    // log_info("pushing %lu, new size = %d", item, kt->load);
}

thread_t * cbuffer_pop(struct kthread *kt)
{
    thread_t *item;
    if(kt->load == 0) {
        log_info("RQ is empty");
        return;
    }
    item = kt->rq[kt->rq_tail++];
    if(kt->rq_tail == RUNTIME_RQ_SIZE)
        kt->rq_tail = 0;
    kt->q_ptrs->rq_tail++;
    if(kt->q_ptrs->rq_tail == RUNTIME_RQ_SIZE)
        kt->q_ptrs->rq_tail = 0;
    if(kt->rq_head == kt->rq_tail) {
        kt->rq_tail = 0;
        kt->rq_head = 0;
    }

    kt->load--;
    // log_info("popping %lu, new size = %d", item, kt->load);
    return item;
}
