#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "sfm.h"

int main(int argc, char **argv) {
	int i;
	uint32_t s;
	uint8_t *data;
	struct SFM_header header;
	struct SFM_chunk chunk;
	char buff[100];

	FILE *in = stdin;
	FILE *out = stdout;

	if (argc >= 2)
	{
		in = fopen(argv[1],"r");
	}

	if (argc >= 3)
	{
		out = fopen(argv[2],"w");
	}

	if (in == NULL)
	{
		fprintf(out, "Could not open file. \r\n");
		exit(1);
	}

	if (SFM_ReadHeader(in, &header) == -1)
	{
		fprintf(stderr, "Bad SFM header.\r\n");
		exit(1);
	}

	fprintf(out, "---- SMF header ---\r\n");
	fprintf(out, 	"%c%c%c%c\r\n",
			header.str[0],
			header.str[1],
			header.str[2],
			header.str[3]);
	fprintf(out, "Length:\t\t%u\r\n", header.length);
	fprintf(out, "Format:\t\t%u\r\n", header.format);
	fprintf(out, "Tracks:\t\t%u\r\n", header.n);
	fprintf(out, "Division:\t%u\r\n", header.division);

	for (i = 0; i < header.n; i++)
	{
		data = SFM_ReadChunk(in, &chunk);
		if (data == NULL)
		{
			fprintf(stderr, "SFM chunk no %u is bad.\r\n", i);
			exit(1);
		}

		fprintf(out, "---- SMF chunk (%u / %u) ---\r\n", i + 1, header.n);
		fprintf(out, 	"%c%c%c%c\r\n",
				chunk.str[0],
				chunk.str[1],
				chunk.str[2],
				chunk.str[3]);
		fprintf(out, "Length:\t\t%u\r\n", chunk.length);
		if (SFM_ParseChunk(out, &chunk, data) == -1)
		{
			fprintf(out, "Invalid midi chunk detected.\r\n");
			exit(1);
		}

		free(data);
	}

	s = fread((void *)buff, sizeof(buff), sizeof(buff[0]), in);

	if (s == 0 && feof(in))
	{
		fprintf(out, "EOF\r\n");
	}
	else
	{
		fprintf(stderr, "Invalid filesize\r\n");
		fprintf(stderr, "Extra %u bytes at end of file\r\n", s);
		exit(1);
	}

	fclose(in);
	fclose(out);

	exit(0);
}
