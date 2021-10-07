#include "eurobeat.h"
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
		int savedError = errno;
        cerr << "Error opening socket - " << strerror(savedError) << " (" << savedError << ")\n";
        exit(savedError);
    }

    strncpy(ifr.ifr_name, IFACE, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
		int savedError = errno;
        cerr << "Error in ioctl: SIOCGIFINDEX - " << strerror(savedError) << " (" << savedError << ")\n";
        exit(savedError);
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		int savedError = errno;
        cerr << "Error connecting to CAN interface - " << strerror(savedError) << " (" << savedError << ")\n";
        exit(savedError);
    }
	
	#if 0
	/* Get interface flags */
	int tempsock = socket(AF_PACKET, SOCK_RAW, 0);
	if (ioctl(tempsock, SIOCGIFFLAGS, &ifr) < 0) {
		int savedError = errno;
        cerr << "Error in ioctl: SIOCGIFFLAGS - " << strerror(savedError) << " (" << savedError << ")\n";
        exit(savedError);
    }
	
	#if DEBUG
	cout << "IFFLAGS: " << ifr.ifr_flags;
	#endif
	
	/* Set the interface to up */
	ifr.ifr_flags |= IFF_UP | IFF_NOARP;
	#if DEBUG
	cout << " -> " << ifr.ifr_flags << endl;
	#endif
	if (ioctl(tempsock, SIOCSIFFLAGS, &ifr) < 0) {
		int savedError = errno;
		cerr << "Error in ioctl: SIOCSIFFLAGS - " << strerror(savedError) << " (" << savedError << ")\n";
		exit(savedError);
	}
	
	close(tempsock);
	#endif

    /* Get socket flags */
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
		int savedError = errno;
        cerr << "Error in fcntl: F_GETFL - \n" << strerror(savedError) << " (" << savedError << ")\n";
        exit(savedError);
    }
	
	/* Set socket to non-blocking */
    /*if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
		int savedError = errno;
        cerr << "Error in fcntl: F_SETFL - " << strerror(savedError) << " (" << savedError << ")\n";
        exit(savedError);
    }*/
	
	/* Set socket to blocking */
	if (fcntl(sock, F_SETFL, flags & ~O_NONBLOCK) < 0) {
		int savedError = errno;
        cerr << "Error in fcntl: F_SETFL - " << strerror(savedError) << " (" << savedError << ")\n";
        exit(savedError);
    }
	
	/* Setup code */
    sigval = 0;
	can_msg msg;
    msg.msg_head.opcode  = RX_SETUP;
    msg.msg_head.can_id  = CAN_PEDALS_ID;
    msg.msg_head.flags   = 0;
    msg.msg_head.nframes = 0;
    if (write(sock, &msg, sizeof(msg)) < 0) {
		int savedError = errno;
        cerr << "Error in write: RX_SETUP - " << strerror(savedError) << " (" << savedError << ")\n";
        exit(savedError);
    }
	
	return sock;
}

void closeCan(int sock) {
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(struct ifreq));
	
	strncpy(ifr.ifr_name, IFACE, IFNAMSIZ);
    /*if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
		int savedError = errno;
        cerr << "Error in ioctl: SIOCGIFINDEX!\n";
        exit(savedError);
    }*/
	
	#if 0
	/* Get interface flags */
	int tempsock = socket(AF_PACKET, SOCK_RAW, 0);
	if (ioctl(tempsock, SIOCGIFFLAGS, &ifr) < 0) {
		int savedError = errno;
        cerr << "Error in ioctl: SIOCGIFFLAGS - " << strerror(savedError) << " (" << savedError << ")\n";
        exit(savedError);
    }
	
	/* Set the interface to down */
	ifr.ifr_flags &= ~IFF_UP;
	if (ioctl(tempsock, SIOCSIFFLAGS, &ifr) < 0) {
		int savedError = errno;
		cerr << "Error in ioctl: SIOCSIFFLAGS - " << strerror(savedError) << " (" << savedError << ")\n";
		exit(savedError);
	}
	
	close(tempsock);
	#endif
	
	if (close(sock) < 0) {
		int savedError = errno;
        cerr << "Error closing CAN interface - " << strerror(savedError) << " (" << savedError << ")\n";
        exit(savedError);
    }
}

bool readCan(struct can_frame** frame, int sock) {
	(void)frame;
	can_msg msg;
	ssize_t nbytes = read(sock, &msg, sizeof(msg));
	if (nbytes < 0) {
		if (errno != EAGAIN) {
			int savedError = errno;
			cout << endl;
			cerr << "Error in CAN read - " << strerror(savedError) << " (" << savedError << ")\n";
		}

		return false;
	}
	else if (nbytes < (ssize_t)sizeof(msg)) {
		int savedError = errno;
		cout << endl;
		cerr << "Error in CAN read: incomplete BCM message - " << strerror(savedError) << " (" << savedError << ")\n";
		return false;
	}
	else {
		*frame = msg.frame;
		
		#if DEBUG
		/* Print the received CAN frame */
		cout << "\nRX:  ";
		print_can_frame(msg.frame);
		cout << endl;
		#endif
		
		return true;
	}
}