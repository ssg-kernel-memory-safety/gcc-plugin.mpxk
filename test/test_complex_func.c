/*
 * Author: Hans Liljestrand <LiljestrandH@gmail.com>
 * Copyright: Secure Systems Group, Aalto University, ssg.aalto.fi
 */
#include "test.h"
#include "mock_kernel.h"

#define noinline __attribute__((noinline))

static int fails = 0;

#define XMIT_TIME_BOUNDS 123
#define TCPCB_RETRANS 1
#define TCPCB_SACKED_ACKED 1

struct tcp_sacktag_state {
	int fack_count;
	int reord;
};

struct tcp_sock {
	int undo_marker;
	int undo_retrans;
	int snd_una;
};

struct sock {
	void *ptr;
};

static struct tcp_sock tcp_sk_retval = { '\0' };

noinline
static struct tcp_sock *tcp_sk(struct sock *sk) {
	tcp_sk_retval.undo_marker++;
	tcp_sk_retval.undo_retrans++;
	tcp_sk_retval.snd_una++;
	fails += !assert_bnds(sk, sizeof(struct sock));
	return &tcp_sk_retval;
}

noinline
static int after(int end, int undo_marker) {
	return end + 1;
}

noinline
static int min(int a, int b) {
	if (a<b) return a;
	return b;
}

noinline
static void tcp_rack_advance(struct tcp_sock *tp,
		void *xmit_time, int sacked)
{
	fails += !assert_bnds(xmit_time, XMIT_TIME_BOUNDS);
	fails += !assert_bnds(tp, sizeof(struct tcp_sock));
}

/* from net/ipv4/tcp_input.c */
noinline
static int tcp_sacktag_one(struct sock *sk,
		struct tcp_sacktag_state *state, int sacked,
		int start_seq, int end_seq,
		int dup_sack, int pcount,
		void *xmit_time)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int fack_count = state->fack_count;

	/* Account D-SACK for retransmitted packet. */
	if (dup_sack && (sacked & TCPCB_RETRANS)) {
		if (tp->undo_marker && tp->undo_retrans > 0 &&
		    after(end_seq, tp->undo_marker))
			tp->undo_retrans--;
		if (sacked & TCPCB_SACKED_ACKED)
			state->reord = min(fack_count, state->reord);
	}

	/* Nothing to do; acked frame is about to be dropped (was ACKed). */
	if (!after(end_seq, tp->snd_una))
		return sacked;

	if (!(sacked & TCPCB_SACKED_ACKED)) {
		tcp_rack_advance(tp, xmit_time, sacked);
	}
	return 0;
}

int test_complex_func(void)
{
	void *sock, *state, *xmit_time;
	printf("%s", __func__);

	sock = kmalloc(sizeof(struct sock), GFP_KERNEL);
	state = kmalloc(sizeof(struct tcp_sacktag_state), GFP_KERNEL);
	xmit_time = kmalloc(XMIT_TIME_BOUNDS, GFP_KERNEL);

	tcp_sacktag_one(sock, state, 0, 0, 1, 0, 0, xmit_time);

	kfree(sock);
	kfree(state);
	kfree(xmit_time);

	printf(" %s\n", (fails ? "FAILED" : "ok"));
	return !fails;
}

