#ifndef _ZUNTYPES_H
#define _ZUNTYPES_H

#include <stdint.h>

#pragma pack(push,1)

struct th_timer_t {
    int prev_time;
    int time;
    float time_f;
    int game_speed_unused;
    int control;
};

struct th06_replay_header_t {
    char magic[4];
    uint16_t version;
    uint8_t shot;
    uint8_t rank;
    uint32_t checksum;
    uint16_t unknown2;
    uint8_t key;
    unsigned char crypted_data[1]; // Janky hack
};

struct th06_replay_t {
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

struct th06_replay_stage_t {
	uint32_t score;
    uint16_t random_seed;
    uint16_t unknown1;
    uint8_t power;
    int8_t lives;
    int8_t bombs;
    uint8_t rank;	
};

struct th17_replay_header_t {
	uint32_t magic;
	uint32_t version;
	char unused[4];
	uint32_t userdata_offset;
	char unused1[12];
	uint32_t comp_size;
	uint32_t size;
};

struct th17_stage_global_t {
    int stage_num;
    int field_4;
    int chapter;
    int stage_time[3];
    int chara;
    int goast;
    int score;
    int diff;
    int continues;
    int rank_unused;
    int graze;
    int field_34;
    int spell_practice_id;
    int miss_count;
    int field_40;
    int point_items_collected;
    int piv;
    int piv_min;
    int piv_max;
    int power;
    int power_max;
    int power_levelup;
    int field_60;
    int lifes;
    int life_pieces;
    int field_6C;
    int bombs;
    int bomb_pieces;
    int bomb_restock_on_death;
    int field_7C;
    int field_80;
    int hyper_fill;
    int tokens[5];
    int field_9C;
    int field_A0;
    int field_A4;
    int field_A8;
    int field_AC;
    int field_B0;
    th_timer_t field_B4;
    th_timer_t hyper_time;
    int field_DC;
    int field_E0;
    int field_E4;
    int hyper_flags;
};

struct th17_replay_t {
    char name[16];
    int timestamp;
    char unk3[4];
    int score;
    char unk1[100];
    float slowdown;
    int stage_count;
    int chara;
    int goast;
    int difficulty;
    char unk2[8];
    int spell_practice_id;
};

struct th17_replay_stage_t {
    uint16_t stage;
    uint16_t rng;
    int frame_count;
    int end_off; // Relative to the start of this structure!!!
    int pos_subpixel[2];
    th17_stage_global_t stagedata;
    int player_is_focused;
    int spellcard_real_times[21];
};

struct th_replay_userdata_header_t {
	uint32_t magic; // "USER"
	uint32_t length;
	uint32_t section_type;
};

#pragma pack(pop)

#endif