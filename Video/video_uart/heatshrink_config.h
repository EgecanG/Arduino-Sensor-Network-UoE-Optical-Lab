// heatshrink_config.h
#ifndef HEATSHRINK_CONFIG_H
#define HEATSHRINK_CONFIG_H

/* Window bits are set to 8, which gives a 256-byte sliding window
   and can compress 2^8 = 256 bytes at a time. */
#define HEATSHRINK_STATIC_WINDOW_BITS 8

/* Number of bits used for back-references. */
#define HEATSHRINK_STATIC_LOOKAHEAD_BITS 4

/* Use static allocation to avoid dynamic memory usage. */
#define HEATSHRINK_STATIC_INPUT_BUFFER_SIZE BUFFER_SIZE
#define HEATSHRINK_STATIC_WINDOW_SIZE (1 << HEATSHRINK_STATIC_WINDOW_BITS)

/* Use indexing instead of pointer math for better performance on AVR. */
#define HEATSHRINK_USE_INDEX

#endif // HEATSHRINK_CONFIG_H