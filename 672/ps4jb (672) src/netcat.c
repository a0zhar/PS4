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
	char *mira_blob_2 = __builtin_gadget_addr(MIRA_BLOB_2);
	int mira_blob_2_len = __builtin_gadget_addr(MIRA_BLOB_2_LEN);
	if (!mira_blob_2) return NULL;

	nanosleep("\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", NULL);
	int q = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr = {.s_addr = 0x100007f}, // inet_aton(127.0.0.1)
		.sin_port = 0x3d23, // htons(9021)
	};
	connect(q, &addr, sizeof(addr));
	char *mirabuf = mira_blob_2;
	int lenleft = mira_blob_2_len;

	// While lenleft is greater than 0 aka FALSE
	while (lenleft) {
		int chunk = write(q, mirabuf, lenleft);
		if (chunk <= 0) // if written is less than or equal to 0
			break;// exit  loop
		mirabuf += chunk;
		lenleft -= chunk;
	}
	close(q);
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
