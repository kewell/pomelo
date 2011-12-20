#include "fb.h"

static int fd;

int main()
{
	int i;
	
	unsigned int v,p;
	
	char buf[4];
	
	fd = open("/dev/fishb", O_RDWR);
	if (-1 == fd) {
		printf("can not open fishb!\n");
		return 1;
	}

while(1) {	
	printf ("Action!\n1: read\n2: write\n3: quit\nPlease select:");
	scanf ("%x", &i);
	switch (i) {
		case 1:
			printf("Input phy addr:");
			scanf ("%x", &p);
			memcpy(buf, &p, 4);
			//lseek(fd, (loff_t)p, SEEK_SET);
			v = read (fd, buf, 4);
			if (-1 == v) {
				perror("read error!\n");
				printf("read fishb error!\n");
			}
			for (i=0; i<4; i++) {
				printf ("0x%x:0x%02x\t", v+i, buf[i]);
			}
			printf("\n");
			break;
		case 2:
			printf("Input phy addr:");
			scanf ("%x", &p);
			memcpy(buf, &p, 4);
			printf("Input data(u32 type):");
			scanf ("%x", &p);
			v = write (fd, buf, (unsigned int)p);
			if (-1 == v) {
				perror("read error!\n");
				printf("write fishb error!\n");
			}
			for (i=0; i<4; i++) {
				printf ("0x%x:0x%02x\t", v+i, buf[i]);
			}
			printf("\n");
			break;
		case 3:
			goto out;
			break;
		default:
			printf("Wrong selection!\n");
	}
}			

out:	
	close(fd);
	return 0;
}
