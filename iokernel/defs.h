/*
 * defs.h - shared definitions local to the iokernel
 */

#include <base/bitmap.h>
#include <base/gen.h>
#include <base/lrpc.h>
#include <base/mem.h>
#include <base/stddef.h>
#include <iokernel/control.h>
#include <net/ethernet.h>

/*
 * Constant limits
 */
#define IOKERNEL_MAX_PROC		1024
#define IOKERNEL_NUM_MBUFS		8191
#define IOKERNEL_TX_BURST_SIZE		64
#define IOKERNEL_CMD_BURST_SIZE		64
#define IOKERNEL_RX_BURST_SIZE		64
#define IOKERNEL_CONTROL_BURST_SIZE	4


/*
 * Process Support
 */

struct thread {
	struct lrpc_chan_out	rxq;
	struct lrpc_chan_in		txpktq;
	struct lrpc_chan_in		txcmdq;
	pid_t					tid;
	int32_t					park_efd;
	struct gen_num			rq_gen;
	struct gen_num			rxq_gen; /* currently unused */
	/* only valid if this thread's bit in available_threads is not set */
	unsigned int			core;
};

struct proc {
	pid_t			pid;
	struct shm_region	region;
	bool			removed;

	/* scheduler data */
	struct sched_spec	sched_cfg;

	/* runtime threads */
	unsigned int		thread_count;
	struct thread		threads[NCPU];
	unsigned int		active_thread_count;
	DEFINE_BITMAP(available_threads, NCPU);

	/* network data */
	struct eth_addr		mac;

	/* table of physical addresses for shared memory */
	physaddr_t		page_paddrs[];
};

/*
 * Communication between control plane and data-plane in the I/O kernel
 */
#define CONTROL_DATAPLANE_QUEUE_SIZE	128
struct lrpc_params {
	struct lrpc_msg *buffer;
	uint32_t *wb;
};
extern struct lrpc_params lrpc_control_to_data_params;
extern struct lrpc_params lrpc_data_to_control_params;

/*
 * Commands from control plane to dataplane.
 */
enum {
	DATAPLANE_ADD_CLIENT,		/* points to a struct proc */
	DATAPLANE_REMOVE_CLIENT,	/* points to a struct proc */
	DATAPLANE_NR,			/* number of commands */
};

/*
 * Commands from dataplane to control plane.
 */
enum {
	CONTROL_PLANE_REMOVE_CLIENT,	/* points to a struct proc */
	CONTROL_PLANE_NR,				/* number of commands */
};

/*
 * Dataplane state
 */
struct dataplane {
	uint8_t				port;
	struct rte_mempool	*rx_mbuf_pool;

	struct proc			*clients[IOKERNEL_MAX_PROC];
	int					nr_clients;
	struct rte_hash		*mac_to_proc;
	struct rte_hash		*pid_to_proc;
};

extern struct dataplane dp;

/*
 * Logical cores assigned to linux and the control and dataplane threads
 */
struct core_assignments {
	uint8_t linux_core;
	uint8_t ctrl_core;
	uint8_t dp_core;
};

extern struct core_assignments core_assign;

/*
 * Initialization
 */

extern int cores_init(void);
extern int control_init(void);
extern int dpdk_init();
extern int rx_init();
extern int tx_init();
extern int dp_clients_init();
extern int dpdk_late_init();

/*
 * dataplane RX/TX functions
 */
extern void rx_burst();
extern void tx_burst();
extern bool tx_send_completion(void *obj);

/*
 * dataplane functions for communicating with runtimes and the control plane
 */
extern void dp_clients_rx_control_lrpcs();
extern void commands_rx();

/*
 * functions for manipulating core assignments
 */
extern void cores_init_proc(struct proc *p);
extern void cores_free_proc(struct proc *p);
extern int cores_pin_thread(pid_t tid, int core);
extern void cores_park_kthread(struct proc *p, int kthread);
extern int cores_reserve_core(struct proc *p);
extern void cores_wake_kthread(struct proc *p, int kthread);
extern void cores_adjust_assignments();
