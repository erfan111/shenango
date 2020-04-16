#include "defs.h"
#include <iokernel/queue.h>
#include <base/time.h>
#include <base/log.h>

#define LB_PERIOD 512

void update_load_balancing_counters(struct thread *t, unsigned long load)
{
    /* store the thread load statistics */
    log_info("received load from %lu %d-------", t, load);
    t->load = load;
    if(load > t->p->overload_count) {
        t->p->overloaded_thread = t;
        t->p->overload_count = load;
    }
    if(load <= t->p->underload_count) {
        t->p->underloaded_thread = t;
        t->p->underload_count = load;
    }
}

bool check_load_balancing_conditions(struct thread *t)
{
    uint64_t now;
    if(t->p->rebalancing_inprogress)
        return 0;
    log_info("lb cond1");
    now = microtime();
    if(now - t->p->last_rebalance_complete < LB_PERIOD)
        return 0;
    log_info("lb cond2");
    // TODO: implement timeout
    if(t->p->overload_count > 0 && t->p->underload_count < ((t->p->overload_count/2) - 1))
    {
        log_info("lb cond satisfied sending");
        t->p->rebalancing_inprogress = true;
        t->p->last_rebalance_sent = now;
        lrpc_send(&t->p->underloaded_thread->rxq, RX_STEAL, t->p->overload_count);
        return 1;
    }
    return 0;
}

void reset_load_balancing_counters(struct thread *t, unsigned long payload)
{
    log_info("resetting counters");
    t->p->overload_count = 0;
    t->p->underload_count = -1;
    t->p->last_rebalance_complete = microtime();
    t->p->rebalancing_inprogress = false;
}