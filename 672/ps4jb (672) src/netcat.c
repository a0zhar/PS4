#include <ps4/mmap.h>
#include <librop/pthread_create.h>
#include <librop/extcall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stddef.h>
#include <unistd.h>
#include <time.h>

#define MIRA_BLOB_1     "$(window.mira_blob||0)"
#define MIRA_BLOB_2     "$(window.mira_blob_2||0)"

#define MIRA_BLOB_1_LEN "$(window.mira_blob_len||0)"
#define MIRA_BLOB_2_LEN "$(window.mira_blob_2_len||0)"
void *sender_thread(void *_) {
	// mira_blob_2 is a pointer to the address of MIRA_BLOB_2
	char *mira_blob_2_ptr = __builtin_gadget_addr(MIRA_BLOB_2);
	// mira_blob_2_len is an integer containing the address of MIRA_BLOB_2_LEN
	int mira_blob_2_len = __builtin_gadget_addr(MIRA_BLOB_2_LEN);
	// If mira_blob_2_ptr is NULL (0), return NULL
	if (!mira_blob_2_ptr) return NULL;

	// Sleep for nanoseconds specified in the string "\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	nanosleep("\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", NULL);
	// Create a new socket with AF_INET (IPv4), SOCK_STREAM (TCP), and default protocol
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	// Set up a sockaddr_in structure with the following values:
	// - sin_family: AF_INET (IPv4)
	// - sin_addr: {.s_addr = 0x100007f} (inet_aton("127.0.0.1"))
	// - sin_port: 0x3d23 (htons(9021))
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr = {.s_addr = 0x100007f},
		.sin_port = 0x3d23,
	};
	// Connect to the socket specified by addr
	connect(socket_fd, &addr, sizeof(addr));
	// Set mirabuf to the address of mira_blob_2
	char *mirabuf = mira_blob_2_ptr;
	// Set lenleft to the value of mira_blob_2_len
	int lenleft = mira_blob_2_len;

	// While lenleft is greater than 0
	while (lenleft) {
		// Write the contents of mirabuf to the socket specified by socket_fd
		// chunk will be the number of bytes written
		int chunk = write(socket_fd, mirabuf, lenleft);
		// If chunk is less than or equal to 0, exit the loop
		if (chunk <= 0)
			break;
		// Move mirabuf to the next chunk of data
		mirabuf += chunk;
		// Decrement lenleft by the number of bytes written
		lenleft -= chunk;
	}
	// Close the socket
	close(socket_fd);
	// Return NULL
	return NULL;
}


int main() {
	if (setuid(0))
		return 1; //jailbreak failed or not run yet
	char *mapping = mmap(NULL, 131072, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	char *mira_blob = __builtin_gadget_addr("$(window.mira_blob||0)");
	if (mira_blob) {
		for (int i = 0; i < 131072; i++)
			mapping[i] = mira_blob[i];
	}
	else {
		int q = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in addr = {
			.sin_family = AF_INET,
			.sin_addr = {.s_addr = 0},
			.sin_port = 0x3c23, // htons(9020)
		};
		bind(q, &addr, sizeof(addr));
		listen(q, 1);
		q = accept(q, NULL, NULL);
		char *x = mapping;
		int l = 131072;
		while (l) {
			int chk = read(q, x, l);
			if (chk <= 0)
				break;
			x += chk;
			l -= chk;
		}
	}
	int sender[512];
	pthread_create(sender, NULL, sender_thread, NULL);
	rop_call_funcptr(mapping);
	return 0;
}
