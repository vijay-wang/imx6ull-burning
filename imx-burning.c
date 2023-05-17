#include <stdio.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
#include <stdlib.h>

#define DCD_ADDRESS "dcd_address"
#define DCD_VALUE "dcd_value"
#define DCD_PAIRS 60 //all clocks,addr and value

/*About setting values of the members
 *you can refer to IMX6ULL REFERENCE
 *MANNUAL Chapter 8 Program image
 *+---------------------------------------------------------------------------------------
 *| IVT 32B | BOOT DATA 12B | 2540B zero | DCD 488B |        Application data            |
 *+---------------------------------------------------------------------------------------
 *|---------------> 3K <----------------------------|------> 0x87800000-end ------------>|
 */
struct image_IVT {
	unsigned long header;
	unsigned long entry;
	unsigned long reserved1;
	unsigned long dcd;
	unsigned long boot_data;
	unsigned long self;
	unsigned long csf;
	unsigned long reserved2;
};

struct boot_data {
	unsigned long start;
	unsigned long length;
	unsigned long plugin_flag;
};

struct addr_val {
	unsigned address;
	unsigned val_mask;
};

struct image_DCD {
	unsigned long header;
	unsigned command_format;
	struct addr_val pairs[DCD_PAIRS];
};

struct firmware_header {
	struct image_IVT ivt;
	struct boot_data boot_data;
	struct image_DCD dcd;
};

int cmd_get(const char *cmd, char *out)
{
	FILE *fcmd;	

	fcmd = popen(cmd, "r");

	if (!cmd) {
		printf("popen error, exited\n");
		return -1;
	}

	if (!fgets(out, sizeof(out), fcmd)) {
		printf("fgets error, exited\n");
		return -1;
	}

	pclose(fcmd);
	return 0;
}

int dcd_get_addr(struct firmware_header *header)
{
	FILE *fd;	
	char *line = NULL;
	size_t len = 0;
	int i = 0;

	fd = fopen(DCD_ADDRESS, "r");

	if (!fd) {
		printf("fopen error, exited\n");
		return -1;
	}

	while (getline(&line, &len, fd) > 0) {
		header->dcd.pairs[i].address = strtol(line, NULL, 16);
//		printf("%x\n", header->dcd.pairs[i].address);
//		fwrite(&header->dcd.pairs[i].address, 4, 1, bfd);
		i++;
	}

	fclose(fd);
	return 0;
}

int dcd_get_val(struct firmware_header *header)
{
	FILE *fd;	
	char *line = NULL;
	size_t len = 0;
	int i = 0;

	fd = fopen(DCD_VALUE, "r");

	if (!fd) {
		printf("fopen error, exited\n");
		return -1;
	}

	while (getline(&line, &len, fd) > 0) {
		header->dcd.pairs[i].val_mask = strtol(line, NULL, 16);
//		printf("%x\n", header->dcd.pairs[i].address);
//		fwrite(&header->dcd.pairs[i].val_mask, 4, 1, bfd);
		i++;
	}

	fclose(fd);
	return 0;
}

int main(int argc, char *argv[])
{
	int i = 0;
	char cmd[32] = { 0 };
	char cmd_out[32] = { 0 };
	char zero_buf[2540] = { 0 };
	unsigned long dcd_addr[60];
	unsigned long dcd_val[60];

	sprintf(cmd, "ls -l %s | awk '{ print $5}'", argv[1]);
	cmd_get(cmd, cmd_out);

//	sprintf(cmd, "cat dcd_data | awk '{ print $5}'| ");
//	cmd_get(cmd, cmd_out);

	struct firmware_header header = {
		.ivt = {
			.header	  = 0X402000D1,
			.entry	  = 0X87800000,
			.reserved1 = 0X00000000,
			.dcd	  = 0X877FFE18,
			.boot_data = 0X877FF420,
			.self	  = 0X877FF400,
			.csf	  = 0X00000000,
			.reserved2 = 0X00000000,	
		},

		.boot_data = {
			.start = 0X877FF000,
			.plugin_flag = 0X00000000,
		},

		.dcd = {
			.header = 0X40E801D2,
			.command_format = 0X04E401CC,
		},
	};

	header.boot_data.length = atoi(cmd_out) + 4096;


	FILE *fd = fopen("spl", "wrba");

	fwrite(&header.ivt.header, 4, 1, fd);
	fwrite(&header.ivt.entry, 4, 1, fd);
	fwrite(&header.ivt.reserved1, 4, 1, fd);
	fwrite(&header.ivt.dcd, 4, 1, fd);
	fwrite(&header.ivt.boot_data, 4, 1, fd);
	fwrite(&header.ivt.self, 4, 1, fd);
	fwrite(&header.ivt.csf, 4, 1, fd);
	fwrite(&header.ivt.reserved2, 4, 1, fd);

	fwrite(&header.boot_data.start, 4, 1, fd);
	fwrite(&header.boot_data.length, 4, 1, fd);
	fwrite(&header.boot_data.plugin_flag, 4, 1, fd);

	fwrite(zero_buf, 2540, 1, fd);

	fwrite(&header.dcd.header, 4, 1, fd);
	fwrite(&header.dcd.command_format, 4, 1, fd);

	dcd_get_addr(&header);
	dcd_get_val(&header);

	while (i < DCD_PAIRS) {
		fwrite(&header.dcd.pairs[i].address, 4, 1, fd);
		fwrite(&header.dcd.pairs[i].val_mask, 4, 1, fd);
		i++;
	}

	pclose(fd);

	return 0;	
}
