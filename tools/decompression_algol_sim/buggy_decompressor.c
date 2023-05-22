#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#include "example_inputs.h"

#define COMPRESSED_BUF_LEN 128
uint8_t* decompress(uint8_t* command, uint8_t command_len, uint8_t* decompressed_buf_len, uint64_t* __decompressed_buf_actual_len) {
  if (command_len > COMPRESSED_BUF_LEN) return NULL;
  uint8_t* mem = malloc(1000000); // represents RAM
  for (int i = 0; i < command_len; i += 1) mem[i] = command[i];

  uint8_t* compressed_buf_start = mem;
  uint8_t* decompressed_buf_start = mem + COMPRESSED_BUF_LEN;

  uint8_t compression_type = mem[0]; // unused
  uint8_t expected_decompressed_len = mem[1];

  // we already parsed the compression_type and expected_decompressed_len,
  // so skip 2 bytes + the 2 unused bytes after compression_type and expected_decompressed_len
  uint8_t* compressed_buf = compressed_buf_start + 4; 
  uint8_t* decompressed_buf = decompressed_buf_start;

  uint8_t bytes_written = 0;
  uint64_t __actual_bytes_written = 0;
  while (bytes_written != expected_decompressed_len) {
    uint8_t map = *compressed_buf;
    compressed_buf += 1;

    for (uint8_t block = 0; block < 8 && bytes_written != expected_decompressed_len; block += 1) {
      if ((map >> (8 - block - 1)) & 0x1) {
        uint8_t byte_one = *compressed_buf;
        uint8_t byte_two = *(compressed_buf + 1);
        uint8_t copy_len = (byte_one >> 4) + 3;
        uint16_t offset = ((byte_one & 0xF) << 8) + byte_two + 1;

        uint8_t* p = decompressed_buf - offset;
        for (int j = 0; j < copy_len; j += 1) {
          *decompressed_buf = *(p + j);
          decompressed_buf += 1;
          bytes_written += 1;
          __actual_bytes_written += 1;
        }
        compressed_buf += 2;
      } else {
        *decompressed_buf = *compressed_buf;
        decompressed_buf += 1;
        bytes_written += 1;
        compressed_buf += 1;
         __actual_bytes_written += 1;
      }
    }
  }

  *decompressed_buf_len = decompressed_buf - decompressed_buf_start;
  *__decompressed_buf_actual_len = __actual_bytes_written;

  return decompressed_buf_start;
}

int main(int argc, char** argv) {
  uint8_t decompressed_buf_len;
  uint64_t decompressed_buf_actual_len;
  uint8_t* decompressed_buf = decompress(dmitrys_attack, sizeof(dmitrys_attack), &decompressed_buf_len, &decompressed_buf_actual_len);

  for (int i = 0; i < decompressed_buf_actual_len; i += 1)
    printf("%04d %02X\n", i, decompressed_buf[i]);

  printf("Written Bytes (bugged): %d\n", decompressed_buf_len);
  printf("Written Bytes (actual): %d\n", decompressed_buf_actual_len);
}