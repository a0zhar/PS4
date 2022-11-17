#include <ps4/mmap.h>
#include <librop/pthread_create.h>
#include <librop/extcall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stddef.h>
#include <unistd.h>
#include <time.h>

void* sender_thread(void* _) {
	char* mira_blob_2 = __builtin_gadget_addr("$(window.mira_blob_2||0)");
	int mira_blob_2_len = __builtin_gadget_addr("$(window.mira_blob_2_len||0)");
	if (!mira_blob_2)
		return NULL;

	nanosleep("\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", NULL);

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock != -1) {
		struct sockaddr_in addr = { 0 };
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = 0x100007f;// inet_aton(127.0.0.1)
		addr.sin_port = 0x3d23; // htons(9021)

		int conn = connect(sock, &addr, sizeof(addr));

		if (conn != -1) {
			char* blob = mira_blob_2;
			int bloblen = mira_blob_2_len;
			while (bloblen) {
				int chk = write(sock, blob, bloblen);
				if (chk <= 0) break;

				blob += chk;
				bloblen -= chk;
			}
		}
		close(sock);
	}
	return NULL;
}

int main() {
	if (setuid(0))return 1; //jailbreak failed or not run yet
	char* mapping = mmap(NULL, 131072, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	char* mira_blob = __builtin_gadget_addr("$(window.mira_blob||0)");
	if (mira_blob) {
		for (int i = 0; i < 131072; i++)
			mapping[i] = mira_blob[i];
	} else {
		int sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock != -1) {
			struct sockaddr_in addr = { 0 };
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = 0;
			addr.sin_port = 0x3c23;// htons(9020)

			bind(sock, &addr, sizeof(addr));
			listen(sock, 1);

			sock = accept(sock, 0, 0);
			char* x = mapping;
			int len = 131072;
			while (len) {
				int chk = read(sock, x, len);
				if (chk <= 0) break;
				x += chk;
				len -= chk;
			}
		}
	}
	int sender[512];
	pthread_create(sender, NULL, sender_thread, NULL);
	rop_call_funcptr(mapping);
	return 0;
}
