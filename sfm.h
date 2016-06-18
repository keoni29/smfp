/*
 * sfm.h
 *
 *  Created on: Jun 18, 2016
 *      Author: koen
 */

#ifndef SFM_H_
#define SFM_H_

#include <stdint.h>

struct SFM_header
{
	char str[4];
	uint32_t length;
	uint16_t format;
	uint16_t n;
	uint16_t division;
};

struct SFM_chunk
{
	char str[4];
	uint32_t length;
};

int SFM_ReadHeader(FILE *file ,struct SFM_header* header);
uint8_t *SFM_ReadChunk(FILE *in, struct SFM_chunk* chunk);
int SFM_ParseChunk(FILE *out, struct SFM_chunk* chunk, uint8_t *data);


#endif /* SFM_H_ */
