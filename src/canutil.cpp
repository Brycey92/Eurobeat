#include "canutil.h"

#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <net/if.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

static void onsig(int val) {
    sigval = (sig_atomic_t)val;
}

void print_can_frame(struct can_frame* const frame) {
    const unsigned char *data = frame->data;
    const unsigned int dlc = frame->can_dlc;
    unsigned int i;

    printf("%03X  [%u] ", frame->can_id, dlc);
    for (i = 0; i < dlc; ++i)
    {
        printf(" %02X", data[i]);
    }
}

int openCan() {
	struct sockaddr_can addr;
	struct ifreq ifr;
	
	/* Open the CAN interface */
    int sock = socket(PF_CAN, SOCK_DGRAM, CAN_BCM);
    if (sock < 0) {
        cerr << "Error opening socket!\n";
        exit(errno);
    }

    strncpy(ifr.ifr_name, IFACE, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        cerr << "Error in ioctl!\n";
        exit(errno);
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "Error connecting to CAN interface!\n";
        exit(errno);
    }

    /* Set socket to non-blocking */
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        cerr << "Error in fcntl: F_GETFL!\n";
        exit(errno);
    }

    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        cerr << "Error in fcntl: F_SETFL!\n";
        exit(errno);
    }
	
	/* Setup code */
    sigval = 0;
	can_msg msg;
    msg.msg_head.opcode  = RX_SETUP;
    msg.msg_head.can_id  = CAN_PEDALS_ID;
    msg.msg_head.flags   = 0;
    msg.msg_head.nframes = 0;
    if (write(sock, &msg, sizeof(msg)) < 0) {
        cerr << "Error in write: RX_SETUP!\n";
        exit(errno);
    }
	
	return sock;
}

void closeCan(int sock) {
	if (close(sock) < 0) {
        cerr << "Error closing CAN interface!\n";
        exit(errno);
    }
}

bool readCan(struct can_frame* frame, int sock) {
	can_msg msg;
	ssize_t nbytes = read(sock, &msg, sizeof(msg));
	if (nbytes < 0) {
		if (errno != EAGAIN) {
			cerr << "Error in CAN read:" << errno << endl;
		}

		return false;
	}
	else if (nbytes < (ssize_t)sizeof(msg)) {
		cerr << "Error in CAN read: incomplete BCM message:" << errno << endl;
		return false;
	}
	else {
		frame = msg.frame;
		
		#if DEBUG
		/* Print the received CAN frame */
		cout << "RX:  ";
		print_can_frame(msg.frame);
		cout << endl;
		#endif
		
		return true;
	}
}