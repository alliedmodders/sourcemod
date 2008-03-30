#include <stdio.h>
#include <stdlib.h>
#include <sm_crc32.h>

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: crc32 <file>\n");
		exit(1);
	}

	FILE *fp = fopen(argv[1], "rb");
	if (!fp)
	{
		fprintf(stderr, "Could not open file: %s\n", argv[1]);
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);	
	if (!size)
	{
		fprintf(stderr, "Cannot checksum an empty file.\n");
		exit(1);
	}

	fseek(fp, 0, SEEK_SET);

	void *buffer = malloc(size);
	if (!buffer)
	{
		fprintf(stderr, "Unable to allocate %d bytes of memory.\n", size);
		exit(1);
	}

	fread(buffer, size, 1, fp);
	unsigned int crc32 = UTIL_CRC32(buffer, size);
	free(buffer);
	fclose(fp);

	fprintf(stdout, "%08X\n", crc32);

	return 0;
}

