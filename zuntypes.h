#ifndef _ZUNTYPES_H
#define _ZUNTYPES_H

#include <stdint.h>

struct __attribute__((__packed__)) th06_replay_header_t {
    char magic[4];
    uint16_t version;
    uint8_t player;
    uint8_t rank;
    uint32_t checksum;
    uint16_t unknown2;
    uint8_t key;
    unsigned char crypted_data[1]; // Janky hack
};

struct __attribute__((__packed__)) th06_replay_t {
	uint8_t unknown; //TODO: seems to be ignored by the game. Padding?
    char date[9]; // null-terminated string
    char name[9]; // null-terminated string
    uint16_t unknown2; //TODO: seems to be ignored by the game. Padding?
    uint32_t score; //TODO: Total score. seems to be ignored by the game.
    uint32_t unknown3; //TODO: seems to be ignored by the game.
    float slowdown; // As a percentage, not a proper rate
    uint32_t unknown4; //TODO: seems to be ignored by the game.
    uint32_t stage_offsets[7];
};

struct __attribute__((__packed__)) th06_replay_stage_t {
	uint32_t score;
    uint16_t random_seed;
    uint16_t unknown1;
    uint8_t power;
    int8_t lives;
    int8_t bombs;
    uint8_t rank;	
};

struct __attribute__((__packed__)) th17_replay_header_t {
	uint32_t magic;
	uint32_t version;
	char unused[4];
	uint32_t userdata_offset;
	char unused1[12];
	uint32_t comp_size;
	uint32_t size;
};

struct __attribute__((__packed__)) th17_replay_t {
	
};

struct th_replay_userdata_header_t {
	uint32_t magic; // "USER"
	uint32_t length;
	uint32_t section_type;
};
#endif