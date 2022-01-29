#include <napi.h>
#include <stdlib.h>
#include "zuntypes.h"

#include <string>
#include <string_view>
#include <unordered_map>

unsigned int th06_decrypt(unsigned char* buf, char key, unsigned int length);
void th_decrypt(unsigned char * userdata_txt, int length, int block_size, unsigned char base, unsigned char add);
unsigned int th_unlzss(unsigned char * userdata_txt, unsigned char * decode, unsigned int length);

void get_th06(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th06");
	th06_replay_header_t* rep_raw = (th06_replay_header_t*)buf;

	size_t size = len - offsetof(th06_replay_header_t, crypted_data);
	if(size < sizeof(th06_replay_t) + sizeof(th06_replay_stage_t) + 6) {
		out.Set("invalid", "too small");
		return;
	}
	
	uint8_t* rep_dec = (uint8_t*)malloc(size);
	memcpy(rep_dec, rep_raw->crypted_data, size);
	th06_decrypt(rep_dec, rep_raw->key, size);
	
	th06_replay_t* rep = (th06_replay_t*)rep_dec;
	
	// Replay name
	out.Set("name", rep->name);
	// Date
	out.Set("date", rep->date);
	// Score
	out.Set("score", rep->score);
	out.Set("slowdown", rep->slowdown);
	
	const char* shots[] = {
		"ReimuA",
		"ReimuB",
		"MarisaA",
		"MarisaB"
	};
	
	if(rep_raw->shot < 4) {
		out.Set("shot", shots[rep_raw->shot]);
	} else {
		// If typeof(rep.shot) === "Number" the shottype in the replay data is invalid
		out.Set("shot", rep_raw->shot);	
	}
	
	Napi::Array stages = Napi::Array::New(env);
	
	for(int i = 0, h = 0; i < 6; i++) {
		if(rep->stage_offsets[i]) {
			Napi::Object stage_ = Napi::Object::New(env);

			uint32_t stage_off = rep->stage_offsets[i] - offsetof(th06_replay_header_t, crypted_data);
			if(stage_off + sizeof(th06_replay_stage_t) > size) {
				out.Set("invalid", "stage data out of bounds");
				return;
			}
			th06_replay_stage_t* stage = (th06_replay_stage_t*)(rep_dec + stage_off);
				
			stage_.Set("stage", i + 1);
			stage_.Set("score", stage->score);
			stage_.Set("power", stage->power);
			stage_.Set("lives", stage->lives);
			stage_.Set("bombs", stage->bombs);

			stages.Set(h, stage_);
			h++;
		}
	}
	out.Set("stages", stages);	
	free(rep_dec);
}

void get_th07(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th07");

	th07_replay_header_t* rep_raw = (th07_replay_header_t*)malloc(len);
	memcpy(rep_raw, buf, len);
	th06_decrypt(
		(uint8_t*)rep_raw + offsetof(th07_replay_header_t, field_10),
		rep_raw->key,
		len - offsetof(th07_replay_header_t, field_10)
	);
	
	uint8_t* rep_dec = (uint8_t*)malloc(rep_raw->size);
	th_unlzss((uint8_t*)rep_raw + sizeof(th07_replay_header_t), rep_dec, rep_raw->comp_size);

	th07_replay_t* rep = (th07_replay_t*)rep_dec;

	out.Set("name", rep->name);
	char date[11] = "2000-01-01";
	memcpy(date+5, rep->date, 2);
	memcpy(date+8, &rep->date[3], 2);
	out.Set("date", date);
	out.Set("score", (double)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("difficulty", rep->difficulty);

	const char* shots[] = {
		"ReimuA",
		"ReimuB",
		"MarisaA",
		"MarisaB",
		"SakuyaA",
		"SakuyaB"
	};

	if(rep->shot < 6) {
		out.Set("shot", shots[rep->shot]);
	} else {
		out.Set("shot", rep->shot);
	}

	Napi::Array stages = Napi::Array::New(env);
	for(int i = 0, h = 0; i < 7; i++) {
		if(rep_raw->stage_offsets[i]) {
			Napi::Object stage_ = Napi::Object::New(env);

			uint32_t stage_off = rep_raw->stage_offsets[i] - sizeof(th07_replay_header_t);
			if(stage_off + sizeof(th07_replay_stage_t) > rep_raw->size) {
				out.Set("invalid", "stage data out of bounds");
				return;
			}
			th07_replay_stage_t* stage = (th07_replay_stage_t*)(rep_dec + stage_off);
				
			stage_.Set("stage", i + 1);
			
			// ZUN decided to store the stage end score in the score field for some reason despite storing the
			// start score in every other game. I don't know how ZUN determines the stage start score like that
			// in his game, but this is how I am doing it.
			if(i == 0) {
				stage_.Set("score", 0);
			} else {
				for(int j = 1; j <= i; j++) {
					if(rep_raw->stage_offsets[i - j]) {
						uint32_t stage_off = rep_raw->stage_offsets[i - j] - sizeof(th07_replay_header_t);
						// If this stage_off was out of bounds, it would've been caught earlier
						th07_replay_stage_t* s_ = (th07_replay_stage_t*)(rep_dec + stage_off);
						stage_.Set("score", (double)s_->score * 10);
						break;
					}
				}
			}
			stage_.Set("point_items", stage->point_items);
			stage_.Set("piv", stage->piv);
			stage_.Set("cherrymax", stage->cherrymax);
			stage_.Set("cherry", stage->cherry);
			stage_.Set("graze", stage->graze);
			stage_.Set("power", stage->power);
			stage_.Set("lives", stage->lives);
			stage_.Set("bombs", stage->bombs);

			stages.Set(h, stage_);
			h++;
		}
	}
	out.Set("stages", stages);	
	free(rep_dec);
	free(rep_raw);

	return;
}

std::unordered_map<std::string_view, const char*> th08_shots = {
	{ "\x97\x64\x96\xB2\x81\x95\x97\x48\x81\x58\x8E\x71\x81\x40\x81\x40", "Border Team"}
};

void get_th08(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th08");

	th08_replay_header_t* rep_raw = (th08_replay_header_t*)malloc(len);
	memcpy(rep_raw, buf, len);

	size_t userdata_len = len - rep_raw->comp_size;
	if(userdata_len >= len) {
		// Underflow occured, replay corrupt
		out.Set("invalid", "comp_size larger than file");
		return;
	}
	if(userdata_len <= sizeof(th_replay_userdata_header_t)) {
		out.Set("invalid", "userdata section too small");
		return;
	}
	th_replay_userdata_header_t* userdata = (th_replay_userdata_header_t*)((size_t)rep_raw + rep_raw->comp_size);
	if(userdata->magic != 0x52455355) {
		out.Set("invalid", "invalid magic for userdata section");
		return;
	}
	char* userdata_txt = (char*)userdata + sizeof(th_replay_userdata_header_t);
	size_t userdata_txtlen = userdata_len - sizeof(th_replay_userdata_header_t);
	th06_decrypt(
		(uint8_t*)rep_raw + offsetof(th08_replay_header_t, field_18),
		rep_raw->key,
		rep_raw->comp_size - offsetof(th08_replay_header_t, field_18)
	);

	uint8_t* rep_dec = (uint8_t*)malloc(rep_raw->size);
	th_unlzss((uint8_t*)rep_raw + sizeof(th08_replay_header_t), rep_dec, rep_raw->comp_size - sizeof(th08_replay_header_t));
	th08_replay_t* rep = (th08_replay_t*)rep_dec;

	size_t user_offset = 13;
	size_t l = 0;

	#define OR_ELSE else { out.Set("invalid", "userdata section too small"); return; }

	for(uint16_t crlf = *(uint16_t*)&userdata_txt[user_offset + l]; crlf!=0x0a0d && user_offset + l <= userdata_txtlen;crlf = *(uint16_t*)&userdata_txt[user_offset + ++l]);
	if(user_offset + l <= userdata_txtlen) {
		out.Set("name", Napi::String::New(env, &userdata_txt[user_offset], l));
	} OR_ELSE
	
	user_offset += 13 + l;
	l = 0;
	
	for(uint16_t crlf = *(uint16_t*)&userdata_txt[user_offset + l]; crlf!=0x0a0d && user_offset + l <= userdata_txtlen;crlf = *(uint16_t*)&userdata_txt[user_offset + ++l]);
	if(user_offset + l <= userdata_txtlen) {
		out.Set("date", Napi::String::New(env, &userdata_txt[user_offset], l));
	} OR_ELSE
	
	user_offset += 11 + l;
	l = 0;
	
	for(uint16_t crlf = *(uint16_t*)&userdata_txt[user_offset + l]; crlf!=0x0a0d && user_offset + l <= userdata_txtlen;crlf = *(uint16_t*)&userdata_txt[user_offset + ++l]);
	if(user_offset + l <= userdata_txtlen) {
		//size_t l_ = sj2utf8(&userdata_txt[user_offset], l, _, 230);
		userdata_txt[user_offset + l] = 0;
		const char* __ = th08_shots[std::string_view(&userdata_txt[user_offset])];
		if(!__) {
			out.Set("shot", Napi::Buffer<char>::New(env, &userdata_txt[user_offset], l));
		} else {
			out.Set("shot", __);
		}
	} OR_ELSE
	
	user_offset += 10 + l;
	l = 0;
	
	for(uint16_t crlf = *(uint16_t*)&userdata_txt[user_offset + l]; crlf!=0x0a0d && user_offset + l <= userdata_txtlen;crlf = *(uint16_t*)&userdata_txt[user_offset + ++l]);
	if(user_offset + l <= userdata_txtlen) {
		out.Set("score", Napi::String::New(env, &userdata_txt[user_offset], l));
	} OR_ELSE
	
	user_offset += 10 + l;
	l = 0;
	
	for(uint16_t crlf = *(uint16_t*)&userdata_txt[user_offset + l]; crlf!=0x0a0d && user_offset + l <= userdata_txtlen;crlf = *(uint16_t*)&userdata_txt[user_offset + ++l]);
	if(user_offset + l <= userdata_txtlen) {
		out.Set("difficulty", Napi::String::New(env, &userdata_txt[user_offset], l));
	} OR_ELSE
	
	user_offset += 15 + l;
	l = 0;
	
	for(uint16_t crlf = *(uint16_t*)&userdata_txt[user_offset + l]; crlf!=0x0a0d && user_offset + l <= userdata_txtlen;crlf = *(uint16_t*)&userdata_txt[user_offset + ++l]);
	if(user_offset + l <= userdata_txtlen) {
		out.Set("cleared", Napi::String::New(env, &userdata_txt[user_offset], l));
	} OR_ELSE

	user_offset += 11 + l;
	l = 0;

	for(uint16_t crlf = *(uint16_t*)&userdata_txt[user_offset + l]; crlf!=0x0a0d && user_offset + l <= userdata_txtlen;crlf = *(uint16_t*)&userdata_txt[user_offset + ++l]);
	if(user_offset + l <= userdata_txtlen) {
		out.Set("misses", Napi::String::New(env, &userdata_txt[user_offset], l));
	} OR_ELSE

	user_offset += 11 + l;
	l = 0;

	for(uint16_t crlf = *(uint16_t*)&userdata_txt[user_offset + l]; crlf!=0x0a0d && user_offset + l <= userdata_txtlen;crlf = *(uint16_t*)&userdata_txt[user_offset + ++l]);
	if(user_offset + l <= userdata_txtlen) {
		out.Set("bombs", Napi::String::New(env, &userdata_txt[user_offset], l));
	} OR_ELSE

	user_offset += 13 + l;
	l = 0;

	for(uint16_t crlf = *(uint16_t*)&userdata_txt[user_offset + l]; crlf!=0x0a0d && user_offset + l <= userdata_txtlen;crlf = *(uint16_t*)&userdata_txt[user_offset + ++l]);
	if(user_offset + l <= userdata_txtlen) {
		out.Set("slowdown", Napi::String::New(env, &userdata_txt[user_offset], l));
	} OR_ELSE

	user_offset += 10 + l;
	l = 0;

	//	it's labelled as lives in the replay, but i think its the rating or something
	//	also its not written properly
	for(uint16_t crlf = *(uint16_t*)&userdata_txt[user_offset + l]; crlf!=0x0a0d && user_offset + l <= userdata_txtlen;crlf = *(uint16_t*)&userdata_txt[user_offset + ++l]);
	if(user_offset + l <= userdata_txtlen) {} OR_ELSE

	user_offset += 21 + l;
	l = 0;

	for(uint16_t crlf = *(uint16_t*)&userdata_txt[user_offset + l]; crlf!=0x0a0d && user_offset + l <= userdata_txtlen;crlf = *(uint16_t*)&userdata_txt[user_offset + ++l]);
	if(user_offset + l <= userdata_txtlen) {
		out.Set("version", Napi::String::New(env, (const char*)&userdata_txt[user_offset], l));
	} OR_ELSE

	#undef OR_ELSE

	uint8_t th08_stagenum_real[] = {
		1, 2, 3, 4, 4, 5, 6, 6, 7
	};

	Napi::Array stages = Napi::Array::New(env);
	for(int i = 0, h = 0; i < 9; i++) {
		if(rep_raw->stage_offsets[i]) {
			Napi::Object stage_ = Napi::Object::New(env);

			uint32_t stage_off = rep_raw->stage_offsets[i] - sizeof(th08_replay_header_t);
			if(stage_off + sizeof(th08_replay_stage_t) > rep_raw->size) {
				out.Set("invalid", "stage data out of bounds");
				return;
			}
			th08_replay_stage_t* stage = (th08_replay_stage_t*)(rep_dec + stage_off);
				
			stage_.Set("stage", th08_stagenum_real[i]);
			
			// ZUN decided to store the stage end score in the score field for some reason despite storing the
			// start score in every other game. I don't know how ZUN determines the stage start score like that
			// in his game, but this is how I am doing it.
			if(i == 0) {
				stage_.Set("score", 0);
			} else {
				for(int j = 1; j <= i; j++) {
					if(rep_raw->stage_offsets[i - j]) {
						uint32_t stage_off = rep_raw->stage_offsets[i - j] - sizeof(th08_replay_header_t);
						// If this stage_off was out of bounds, it would've been caught earlier
						th08_replay_stage_t* s_ = (th08_replay_stage_t*)(rep_dec + stage_off);
						stage_.Set("score", (double)s_->score * 10);
						break;
					}
				}
			}
			stage_.Set("point_items", stage->point_items);
			stage_.Set("piv", stage->piv);
			stage_.Set("human_youkai", stage->human_youkai);
			stage_.Set("graze", stage->graze);
			stage_.Set("power", stage->power);
			stage_.Set("lives", stage->lives);
			stage_.Set("bombs", stage->bombs);

			stages.Set(h, stage_);
			h++;
		}
	}
	out.Set("stages", stages);	
	free(rep_dec);
	free(rep_raw);
}

void get_th13(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th13");
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;

	if(rep_raw->size < sizeof(th13_replay_t) + sizeof(th13_replay_stage_t) * 2 + 6) {
		out.Set("invalid", "too small");
		return;
	}

	uint8_t* comp_buf = (uint8_t*)malloc(rep_raw->comp_size);
	uint8_t* rep_dec = (uint8_t*)malloc(rep_raw->size);
	memcpy((void*)comp_buf, (void*)(buf + sizeof(th13_replay_header_t)), rep_raw->comp_size);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x400, 0x5c, 0xe1);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x100, 0x7d, 0x3a);
	th_unlzss(comp_buf, rep_dec, rep_raw->comp_size);
	free(comp_buf);
	
	th13_replay_t* rep = (th13_replay_t*)rep_dec;
	
	out.Set("name", rep->name);
	out.Set("date", rep->timestamp);
	out.Set("difficulty", rep->difficulty);
	out.Set("score", (double)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("cleared", rep->cleared);
	
	const char* charas[] = {
		"Reimu",
		"Marisa",
		"Sanae",
		"Youmu"
	};
	
	if(rep->chara < 4 && rep->subshot_unused < 1) {
		out.Set("chara", charas[rep->chara]);
	} else {
		out.Set("chara", rep->chara);
		out.Set("subshot", rep->subshot_unused);
	}
				
	Napi::Array stages = Napi::Array::New(env);
	size_t stage_off = sizeof(th13_replay_t);
	for(uint32_t i = 0; i < rep->stage_count; i++) {
		Napi::Object stage_ = Napi::Object::New(env);
		th13_replay_stage_t* stage = (th13_replay_stage_t*)((size_t)rep + stage_off);

		if(stage_off + sizeof(th13_replay_stage_t) > rep_raw->size) {
			out.Set("invalid", "stage data out of bounds");
			return;
		}
				
		stage_.Set("stage", stage->stage_num);
		stage_.Set("score", (double)stage->stagedata.score * 10);
		stage_.Set("graze", stage->stagedata.graze);
		//stage_.Set("misscount", stage->stagedata.miss_count);
		stage_.Set("piv", stage->stagedata.piv);
		stage_.Set("power", stage->stagedata.power);
		stage_.Set("lives", stage->stagedata.lives);
		stage_.Set("life_pieces", stage->stagedata.life_pieces);
		stage_.Set("bombs", stage->stagedata.bombs);
		stage_.Set("bomb_pieces", stage->stagedata.bomb_pieces);
		stage_.Set("trance_gauge", stage->stagedata.trance_gauge);
				
		stages.Set(i, stage_);
		stage_off += stage->end_off + sizeof(th13_replay_stage_t);
	}
	out.Set("stages", stages);
	free(rep_dec);
}

void get_th14(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th14");
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;

	if(rep_raw->size < sizeof(th14_replay_t) + sizeof(th14_replay_stage_t) * 2 + 6) {
		out.Set("invalid", "too small");
		return;
	}

	uint8_t* comp_buf = (uint8_t*)malloc(rep_raw->comp_size);
	uint8_t* rep_dec = (uint8_t*)malloc(rep_raw->size);
	memcpy((void*)comp_buf, (void*)(buf + sizeof(th13_replay_header_t)), rep_raw->comp_size);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x400, 0x5c, 0xe1);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x100, 0x7d, 0x3a);
	th_unlzss(comp_buf, rep_dec, rep_raw->comp_size);
	free(comp_buf);
	
	th14_replay_t* rep = (th14_replay_t*)rep_dec;
	
	out.Set("name", rep->name);
	out.Set("date", rep->timestamp);
	out.Set("difficulty", rep->difficulty);
	out.Set("score", (double)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("cleared", rep->cleared);
	
	const char* charas[] = {
		"Reimu",
		"Marisa",
		"Sakuya"
	};
	const char* shots = "AB";
	
	if(rep->chara < 3 && rep->subshot < 2) {
		#define ch rep->chara
		size_t charlen = strlen(charas[ch]);
		char truechara[16] = {};
		memcpy(truechara, charas[ch], charlen);
		truechara[charlen] = shots[rep->subshot];
		out.Set("shot", truechara);
		#undef ch
	} else {
		out.Set("chara", rep->chara);
		out.Set("subshot", rep->subshot);
	}
				
	Napi::Array stages = Napi::Array::New(env);
	size_t stage_off = sizeof(th14_replay_t);
	for(uint32_t i = 0; i < rep->stage_count; i++) {
		Napi::Object stage_ = Napi::Object::New(env);
		th14_replay_stage_t* stage = (th14_replay_stage_t*)((size_t)rep + stage_off);

		if(stage_off + sizeof(th14_replay_stage_t) > rep_raw->size) {
			out.Set("invalid", "stage data out of bounds");
			return;
		}
				
		stage_.Set("stage", stage->stage_num);
		stage_.Set("score", (double)stage->stagedata.score * 10);
		stage_.Set("graze", stage->stagedata.graze);
		//stage_.Set("misscount", stage->stagedata.miss_count);
		stage_.Set("piv", stage->stagedata.piv);
		stage_.Set("power", stage->stagedata.power);
		stage_.Set("lives", stage->stagedata.lives);
		stage_.Set("life_pieces", stage->stagedata.life_pieces);
		stage_.Set("bombs", stage->stagedata.bombs);
		stage_.Set("bomb_pieces", stage->stagedata.bomb_pieces);
		stage_.Set("poc_count", stage->stagedata.poc_count);
				
		stages.Set(i, stage_);
		stage_off += stage->end_off + sizeof(th14_replay_stage_t);
	}
	out.Set("stages", stages);
	free(rep_dec);
}

void get_th143(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th143");
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;

	if(rep_raw->size < sizeof(th143_replay_t)) {
		out.Set("invalid", "too small");
		return;
	}

	uint8_t* comp_buf = (uint8_t*)malloc(rep_raw->comp_size);
	uint8_t* rep_dec = (uint8_t*)malloc(rep_raw->size);
	memcpy((void*)comp_buf, (void*)(buf + sizeof(th13_replay_header_t)), rep_raw->comp_size);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x400, 0x5c, 0xe1);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x100, 0x7d, 0x3a);
	th_unlzss(comp_buf, rep_dec, rep_raw->comp_size);
	free(comp_buf);

	th143_replay_t* rep = (th143_replay_t*)rep_dec;

	out.Set("name", rep->name);
	out.Set("date", rep->timestamp);
	out.Set("score", (double)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("primary_item", rep->primary_item);
	out.Set("secondary_item", rep->secondary_item);
	out.Set("day", rep->day);
	out.Set("scene", rep->scene);

	free(rep_dec);
}

void get_th15(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th15");
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;
	
	if(rep_raw->size < sizeof(th15_replay_t) + sizeof(th15_replay_stage_t) * 2 + 6) {
		out.Set("invalid", "too small");
		return;
	}

	uint8_t* comp_buf = (uint8_t*)malloc(rep_raw->comp_size);
	uint8_t* rep_dec = (uint8_t*)malloc(rep_raw->size);
	memcpy((void*)comp_buf, (void*)(buf + sizeof(th13_replay_header_t)), rep_raw->comp_size);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x400, 0x5c, 0xe1);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x100, 0x7d, 0x3a);
	th_unlzss(comp_buf, rep_dec, rep_raw->comp_size);
	free(comp_buf);
	
	th15_replay_t* rep = (th15_replay_t*)rep_dec;
	
	out.Set("name", rep->name);
	out.Set("date", rep->timestamp);
	out.Set("difficulty", rep->difficulty);
	out.Set("score", (double)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("cleared", rep->cleared);
	
	const char* charas[] = {
		"Reimu",
		"Marisa",
		"Sanae",
		"Reisen"
	};
		
	if(rep->chara < 4) {
		out.Set("chara", charas[rep->chara]);
	} else {
		out.Set("chara", rep->chara);
	}
				
	Napi::Array stages = Napi::Array::New(env);
	size_t stage_off = sizeof(th15_replay_t);
	for(uint32_t i = 0; i < rep->stage_count; i++) {
		Napi::Object stage_ = Napi::Object::New(env);
		th15_replay_stage_t* stage = (th15_replay_stage_t*)((size_t)rep + stage_off);

		if(stage_off + sizeof(th15_replay_stage_t) > rep_raw->size) {
			out.Set("invalid", "stage data out of bounds");
			return;
		}
				
		stage_.Set("stage", stage->stagedata.stage_num);
		stage_.Set("score", (double)stage->stagedata.score * 10);
		stage_.Set("graze", stage->stagedata.graze);
		stage_.Set("misscount", stage->stagedata.miss_count);
		stage_.Set("piv", stage->stagedata.piv);
		stage_.Set("power", stage->stagedata.power);
		stage_.Set("lives", stage->stagedata.lives);
		stage_.Set("life_pieces", stage->stagedata.life_pieces);
		stage_.Set("bombs", stage->stagedata.bombs);
		stage_.Set("bomb_pieces", stage->stagedata.bomb_pieces);
				
		stages.Set(i, stage_);
		stage_off += stage->end_off + sizeof(th15_replay_stage_t);
	}
	out.Set("stages", stages);
	free(rep_dec);
}

void get_th16(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th16");
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;
	
	if(rep_raw->size < sizeof(th16_replay_t) + sizeof(th16_replay_stage_t) * 2 + 6) {
		out.Set("invalid", "too small");
		return;
	}
	
	uint8_t* comp_buf = (uint8_t*)malloc(rep_raw->comp_size);
	uint8_t* rep_dec = (uint8_t*)malloc(rep_raw->size);
	memcpy((void*)comp_buf, (void*)(buf + sizeof(th13_replay_header_t)), rep_raw->comp_size);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x400, 0x5c, 0xe1);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x100, 0x7d, 0x3a);
	th_unlzss(comp_buf, rep_dec, rep_raw->comp_size);
	free(comp_buf);
	
	th16_replay_t* rep = (th16_replay_t*)rep_dec;
	
	if(rep->spell_practice_id != -1) {
		out.Set("spell_practice_id", rep->spell_practice_id);
		return;
	}
	
	out.Set("name", rep->name);
	out.Set("date", rep->timestamp);
	out.Set("difficulty", rep->difficulty);
	out.Set("score", (double)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("cleared", rep->cleared);
	
	const char* charas[] = {
		"Reimu",
		"Cirno",
		"Aya",
		"Marisa"
	};
	const char* seasons[] = {
		"Spring",
		"Summer",
		"Autumn",
		"Winter",
		"Extra"
	};
	
	if(rep->chara < 4 && rep->subseason < 5) {
		out.Set("shot", std::string(charas[rep->chara]) + seasons[rep->subseason]);
	} else {
		out.Set("chara", rep->chara);
		out.Set("subseason", rep->subseason);
	}

	Napi::Array stages = Napi::Array::New(env);
	size_t stage_off = sizeof(th16_replay_t);
	for(uint32_t i = 0; i < rep->stage_count; i++) {
		Napi::Object stage_ = Napi::Object::New(env);
		th16_replay_stage_t* stage = (th16_replay_stage_t*)((size_t)rep + stage_off);
				
		stage_.Set("stage", stage->stagedata.stage_num);
		stage_.Set("score", (double)stage->stagedata.score * 10);
		stage_.Set("graze", stage->stagedata.graze);
		stage_.Set("misscount", stage->stagedata.miss_count);
		stage_.Set("piv", stage->stagedata.piv);
		stage_.Set("power", stage->stagedata.power);
		stage_.Set("lives", stage->stagedata.lives);
		stage_.Set("bombs", stage->stagedata.bombs);
		stage_.Set("bomb_pieces", stage->stagedata.bomb_pieces);
		stage_.Set("season_power", stage->stagedata.season_power);
		
		// TODO: maybe check that this data stays the same every stage?
		// I don't know much about this game, of it's natural for it
		// to change between stages I wouldn't know
		out.Set("season_power_max", stage->stagedata.season_power_max);
		Napi::Array season_power_required = Napi::Array::New(env);
		for(uint32_t i = 0; i < 6; i++) {
			season_power_required.Set(i, stage->stagedata.season_power_required[i]);
		}
		out.Set("season_power_required", season_power_required);
				
		stages.Set(i, stage_);
		stage_off += stage->end_off + sizeof(th16_replay_stage_t);

		if(stage_off + sizeof(th16_replay_stage_t) > rep_raw->size) {
			out.Set("invalid", "stage data out of bounds");
			return;
		}
	}
	out.Set("stages", stages);
	free(rep_dec);
}

void get_th165(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th165");
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;

	if(rep_raw->size < sizeof(th165_replay_t)) {
		out.Set("invalid", "too small");
		return;
	}

	uint8_t* comp_buf = (uint8_t*)malloc(rep_raw->comp_size);
	uint8_t* rep_dec = (uint8_t*)malloc(rep_raw->size);
	memcpy((void*)comp_buf, (void*)(buf + sizeof(th13_replay_header_t)), rep_raw->comp_size);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x400, 0x5c, 0xe1);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x100, 0x7d, 0x3a);
	th_unlzss(comp_buf, rep_dec, rep_raw->comp_size);
	free(comp_buf);

	th165_replay_t* rep = (th165_replay_t*)rep_dec;

	out.Set("name", rep->name);
	out.Set("date", rep->timestamp);
	out.Set("score", rep->score);
	out.Set("slowdown", rep->slowdown);
	out.Set("day", rep->day);
	out.Set("scene", rep->scene);

	free(rep_dec);
}

void get_th17(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th17");
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;

	if(rep_raw->size < sizeof(th17_replay_t) + sizeof(th17_replay_stage_t) * 2 + 6) {
		out.Set("invalid", "too small");
		return;
	}
	
	uint8_t* comp_buf = (uint8_t*)malloc(rep_raw->comp_size);
	uint8_t* rep_dec = (uint8_t*)malloc(rep_raw->size);
	memcpy((void*)comp_buf, (void*)(buf + sizeof(th13_replay_header_t)), rep_raw->comp_size);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x400, 0x5c, 0xe1);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x100, 0x7d, 0x3a);
	th_unlzss(comp_buf, rep_dec, rep_raw->comp_size);
	free(comp_buf);
	
	th17_replay_t* rep = (th17_replay_t*)rep_dec;
	
	if(rep->spell_practice_id != -1) {
		out.Set("spell_practice_id", rep->spell_practice_id);
		return;
	}
	
	out.Set("name", rep->name);
	out.Set("date", rep->timestamp);
	out.Set("difficulty", rep->difficulty);
	out.Set("score", (double)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("cleared", rep->cleared);
	
	const char* charas[] = {
		"Reimu",
		"Marisa",
		"Youmu"
	};
	const char* goasts[] = {
		"Wolf",
		"Otter",
		"Eagle"
	};
	if(rep->chara < 3 && rep->goast < 3) {
		out.Set("shot", std::string(charas[rep->chara]) + goasts[rep->goast]);
	} else {
		out.Set("chara", rep->chara);
		out.Set("goast", rep->goast);
	}

	Napi::Array stages = Napi::Array::New(env);
	size_t stage_off = sizeof(th17_replay_t);
	for(uint32_t i = 0; i < rep->stage_count; i++) {
		Napi::Object stage_ = Napi::Object::New(env);
		th17_replay_stage_t* stage = (th17_replay_stage_t*)((size_t)rep + stage_off);

		if(stage_off + sizeof(th17_replay_stage_t) > rep_raw->size) {
			out.Set("invalid", "stage data out of bounds");
			return;
		}
				
		stage_.Set("stage", stage->stagedata.stage_num);
		stage_.Set("score", (double)stage->stagedata.score * 10);
		stage_.Set("graze", stage->stagedata.graze);
		stage_.Set("misscount", stage->stagedata.miss_count);
		stage_.Set("piv", stage->stagedata.piv);
		stage_.Set("power", stage->stagedata.power);
		stage_.Set("lives", stage->stagedata.lives);
		stage_.Set("life_pieces", stage->stagedata.life_pieces);
		stage_.Set("bombs", stage->stagedata.bombs);
		stage_.Set("bomb_pieces", stage->stagedata.bomb_pieces);
		
		Napi::Array tokens = Napi::Array::New(env);
		for(uint32_t j = 0; j < 5; j++) {
			tokens.Set(j, stage->stagedata.tokens[j]);
		}
		stage_.Set("tokens", tokens);
		
		stages.Set(i, stage_);
		stage_off += stage->end_off + sizeof(th17_replay_stage_t);
	}
	out.Set("stages", stages);
	free(rep_dec);
}

void get_th18(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th18");
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;

	if(rep_raw->size < sizeof(th18_replay_t) + sizeof(th18_replay_stage_t) * 2 + 6) {
		out.Set("invalid", "too small");
		return;
	}

	uint8_t* comp_buf = (uint8_t*)malloc(rep_raw->comp_size);
	uint8_t* rep_dec = (uint8_t*)malloc(rep_raw->size);
	memcpy((void*)comp_buf, (void*)(buf + sizeof(th13_replay_header_t)), rep_raw->comp_size);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x400, 0x5c, 0xe1);
	th_decrypt(comp_buf, rep_raw->comp_size, 0x100, 0x7d, 0x3a);
	th_unlzss(comp_buf, rep_dec, rep_raw->comp_size);
	free(comp_buf);
	
	th18_replay_t* rep = (th18_replay_t*)rep_dec;
	
	if(rep->spell_practice_id != -1) {
		out.Set("spell_practice_id", rep->spell_practice_id);
		return;
	}
	
	out.Set("name", rep->name);
	out.Set("date", rep->timestamp);
	out.Set("difficulty", rep->difficulty);
	out.Set("score", (double)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("cleared", rep->cleared);
	
	const char* charas[] = {
		"Reimu",
		"Marisa",
		"Sakuya",
		"Sanae"
	};
	
	if(rep->chara > 4) {
		out.Set("chara", rep->chara);
	} else {
		out.Set("chara", charas[rep->chara]);	
	}
	
	Napi::Array stages = Napi::Array::New(env);
	size_t stage_off = sizeof(th18_replay_t);
	for(uint32_t i = 0; i < rep->stage_count; i++) {
		Napi::Object stage_ = Napi::Object::New(env);
		th18_replay_stage_t* stage = (th18_replay_stage_t*)((size_t)rep + stage_off);

		if(stage_off + sizeof(th18_replay_stage_t) > rep_raw->size) {
			out.Set("invalid", "stage data out of bounds");
			return;
		}
				
		stage_.Set("stage", stage->stagedata.stage_num);
		stage_.Set("score", (double)stage->stagedata.score * 10);
		stage_.Set("graze", stage->stagedata.graze);
		stage_.Set("misscount", stage->stagedata.miss_count);
		stage_.Set("piv", stage->stagedata.piv);
		stage_.Set("power", stage->stagedata.power);
		stage_.Set("lives", stage->stagedata.lives);
		stage_.Set("life_pieces", stage->stagedata.life_pieces);
		stage_.Set("bombs", stage->stagedata.bombs);
		stage_.Set("bomb_pieces", stage->stagedata.bomb_pieces);
		
		Napi::Array cards = Napi::Array::New(env);
		for(int i = 0; i < 256; i++) {
			if(stage->cards[i] == -1) {
				break;
			}
			cards.Set(i, stage->cards[i]);
		}
		stage_.Set("cards", cards);
		stage_.Set("card_active", stage->card_active);
		
		stages.Set(i, stage_);
		stage_off += stage->end_off + sizeof(th18_replay_stage_t);
	}
	out.Set("stages", stages);
	free(rep_dec);
}

Napi::Value get_replay_data(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::Object ret = Napi::Object::New(env);
	
	if(!info[0].IsBuffer()) {
		Napi::TypeError::New(env, "Wrong argument type: argument 0 must be a Buffer").ThrowAsJavaScriptException();
    	return env.Null();	
	}
	
	uint8_t* buf = info[0].As<Napi::Buffer<uint8_t>>().Data();
	size_t len = info[0].As<Napi::Buffer<uint8_t>>().Length();
	
	uint32_t magic = *(uint32_t*)buf;
	th13_replay_header_t* _th13_rep = (th13_replay_header_t*)buf;
	uint8_t _ver;
	switch(magic) {	
	case 0x50523654: // "T6RP"
		get_th06(ret, buf, len, env);
		break;
	case 0x50523754: // "T7RP"
		get_th07(ret, buf, len, env);
		break;
	case 0x50523854: // "T8RP"
		get_th08(ret, buf, len, env);
		break;
	case 0x72333174: // "t13r"
		_ver = buf[_th13_rep->userdata_offset + 16];
		if(_ver == 0x90) {			
			get_th13(ret, buf, len, env);
		} else if(_ver == 0x8b) {
			get_th14(ret, buf, len, env);
		}
		break;
	case 0x33343174: // "t143"
		get_th143(ret, buf, len, env);
		break;
	case 0x72353174: // "t15r"
		get_th15(ret, buf, len, env);
		break;
	case 0x72363174: // "t16r"
		get_th16(ret, buf, len, env);
		break;
	case 0x36353174: // "t156". Really ZUN?
		get_th165(ret, buf, len, env);
		break;
	case 0x72373174: // "t17r"
		get_th17(ret, buf, len, env);
		break;
	case 0x72383174: // "t18r"
		get_th18(ret, buf, len, env);
		break;
	
	default:
		break;
	}
	return ret;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	exports.Set(Napi::String::New(env, "get_replay_data"),
				Napi::Function::New(env, get_replay_data));
	return exports;
}

NODE_API_MODULE(threplay, Init)
