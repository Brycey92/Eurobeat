#ifndef CANUTIL
#define CANUTIL

#include <linux/can.h>
#include <linux/can/bcm.h>
#include <signal.h>

#define IFACE "can0"
#define NFRAMES 1
#define DELAY (10000)
#define CAN_PEDALS_ID 0x0329
#define ACCELERATOR_BYTE_INDEX 6 /*starts at 0*/
#define ACCELERATOR_BYTE_MIN 0
#define ACCELERATOR_BYTE_MAX 0xFD
#define ACTIVATION_PERCENT 80
#define DEACTIVATION_PERCENT 60

static sig_atomic_t sigval;

typedef struct {
	struct bcm_msg_head msg_head;
	struct can_frame frame[NFRAMES];
} can_msg;

void print_can_frame(struct can_frame* const frame);

int openCan();

void closeCan(int sock);

bool readCan(struct can_frame** frame, int sock);

#endif