#include <stdio.h>
#include <stdlib.h>
#include "sfm.h"

static void ReverseInt16(uint16_t *d); /**< Helper function for reversing endianess */
static void ReverseInt32(uint32_t *d); /**< Helper function for reversing endianess */
static uint32_t VData(uint8_t **p, uint32_t *bytesleft);	/**< Parse variable length data field */
static uint8_t Data(uint8_t **p, uint32_t *bytesleft);	/**< Parse single byte data field */
static uint8_t Peek(uint8_t *p, uint32_t *bytesleft);	/**< Peek single byte data field without incrementing pointer */
static uint8_t SkipData(uint8_t **p, uint32_t *bytesleft, uint32_t skip);	/**< Skip over several fields */
static char *Terminate(char* dst, char *src, uint32_t len); /**< Terminate a character string of specified length. */

static float mpb = 500000, tpb, tick;

/** Read and verify header of a Standard MIDI File
 *  Returns -1 on failure, 0 on success. */
int SFM_ReadHeader(FILE *in ,struct SFM_header* header)
{
	fread((void *)&header->str, sizeof(header->str), 1, in);
	fread((void *)&header->length, sizeof(header->length), 1, in);
	fread((void *)&header->format, sizeof(header->format), 1, in);
	fread((void *)&header->n, sizeof(header->n), 1, in);
	fread((void *)&header->division, sizeof(header->division), 1, in);

	ReverseInt32(&header->length);
	ReverseInt16(&header->format);
	ReverseInt16(&header->n);
	ReverseInt16(&header->division);

	tpb = header->division;

	return 0;
}

/** Read and verify track chunk of a Standard MIDI File *
 *	Returns NULL on failure, pointer to chunk data on success. */
uint8_t *SFM_ReadChunk(FILE *in, struct SFM_chunk* chunk)
{
	uint8_t *data = NULL;

	fread((void *)&chunk->str, sizeof(chunk->str), 1, in);
	fread((void *)&chunk->length, sizeof(chunk->length), 1, in);

	ReverseInt32(&chunk->length);

	data = (uint8_t *)malloc(chunk->length);
	if (data != NULL)
	{
		fread((void *)data, chunk->length, 1, in);
	}

	return data;
}

int SFM_ParseChunk(FILE *out, struct SFM_chunk* chunk, uint8_t *data)
{
	float time;
	uint32_t dt, length, left;
	uint8_t event, type, p1, p2;
	char str[100];

	left = chunk->length;

	while (left)
	{
		/* Calculate time offset */
		if (Peek(data, &left))
		{
			p1 = p2; /* Todo remove this. Variable length field! */
		}

		dt = VData(&data, &left);
		time += dt * tick / 1000000;

		if (!(Peek(data, &left) & 0x80))
		{
			/* Running byte! Do not change the state. */
		}
		else
		{
			event = Data(&data, &left);
		}

		fprintf(out, "%.1f s, ", time);

		switch (event)
		{
		case 0xF0 :
		case 0xF7 :
			length = 0;
			while (Data(&data, &left) != 0xF7)
			{
				++length;
			}
			fprintf(out, "SYEX 0x%02x[%u]\r\n", event, length);
			break;

		case 0xFF :
			type = Data(&data, &left);
			length = VData(&data, &left);
			fprintf(out, "META 0x%02x[%u] \"%s\"\r\n", type, length, Terminate(str, (char *)data, length));
			if (type == 0x51 && length == 3)
			{
				mpb = 0;
				mpb = (Data(&data, &left) << 16) | (Data(&data, &left) << 8) | (Data(&data, &left) << 0);
				tick = mpb / tpb;
			}
			else
			{
				SkipData(&data, &left, length);
			}
			break;

		default :
			fprintf(out, "MIDI 0x%02x[], : ", event);

			switch (event & 0xF0)
			{
			case 0xF0:
				fprintf(out, "Unhandled midi event 0x%02x.\r\n", event);
				return -1;
				break;
			case 0x80:
				p1 = Data(&data, &left);
				p2 = Data(&data, &left);
				fprintf(out, "Note Off %u, %u\r\n", p1, p2);
				break;
			case 0x90:
				p1 = Data(&data, &left);
				p2 = Data(&data, &left);
				fprintf(out, "Note On %u, %u\r\n", p1, p2);
				break;
			case 0xA0:
				p1 = Data(&data, &left);
				p2 = Data(&data, &left);
				fprintf(out, "p1 pressure %u, %u\r\n", p1, p2);
				break;
			case 0xB0:
				p1 = Data(&data, &left);
				p2 = Data(&data, &left);
				fprintf(out, "Control change %u, %u\r\n", p1, p2);
				break;
			case 0xC0:
				p1 = Data(&data, &left);
				fprintf(out, "Program change %u\r\n", p1);
				break;
			case 0xD0:
				p1 = Data(&data, &left);
				fprintf(out, "Channel pressure %u\r\n", p1);
				break;
			case 0xE0:
				p1 = Data(&data, &left);
				p2 = Data(&data, &left);
				fprintf(out, "Pitch wheel change %u, %u\r\n", p1, p2);
				break;
			}

			break;
		}
	}

	if (!(event == 0xFF && type == 0x2F))
	{
		fprintf(out, "Notice: Last message was not META 0x2F (End of track)\r\n");
	}
	return 0;
}

static void ReverseInt16(uint16_t *d)
{
	uint8_t help;
	help = *(((uint8_t*)d)+0);
	*(((uint8_t*)d)+0) = *(((uint8_t*)d)+1);
	*(((uint8_t*)d)+1) = help;
}

static void ReverseInt32(uint32_t *d)
{
	uint8_t help1, help2;
	help1 = *(((uint8_t*)d) + 0);
	help2 = *(((uint8_t*)d) + 1);
	*(((uint8_t*)d) + 0) = *(((uint8_t*)d) + 3);
	*(((uint8_t*)d) + 1) = *(((uint8_t*)d) + 2);
	*(((uint8_t*)d) + 2) = help2;
	*(((uint8_t*)d) + 3) = help1;
}

static uint32_t VData(uint8_t **p, uint32_t *bytesleft)
{
	uint32_t data = 0;
	int i;

	if (bytesleft)
	{
		for (i = 0; i < 4; i++)
		{
			data = (data << 7) +  (**p & 0x7F);

			if (!(**p & 0x80))
			{
				++*p;
				--*bytesleft;
				break;
			}
			++*p;
			--*bytesleft;
		}
	}

	return data;
}

static uint8_t Data(uint8_t **p, uint32_t *bytesleft)
{
	uint8_t data = **p;
	if (bytesleft)
	{
		++*p;
		--*bytesleft;
		return data;
	}
	return 0;
}

static uint8_t Peek(uint8_t *p, uint32_t *bytesleft)
{
	return *p;
}

static uint8_t SkipData(uint8_t **p, uint32_t *bytesleft, uint32_t skip)
{
	while (skip && *bytesleft)
	{
		Data(p, bytesleft);
		--skip;
	}
	return 0;
}

static char *Terminate(char* dst, char *src, uint32_t len)
{
	uint32_t i;
	char *result = dst;
	/* Copy string */
	for (i = 0; i < len; i++)
	{
		*dst = *src;
		++src;
		++dst;
	}
	/* Append termination character */
	*dst = 0;
	return result;
}
