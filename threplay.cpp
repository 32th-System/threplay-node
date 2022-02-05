#include <napi.h>
#include <stdlib.h>
#include "zuntypes.h"

#include <string>
#include <string_view>
#include <unordered_map>

unsigned int th06_decrypt(unsigned char* buf, char key, unsigned int length);
void th_decrypt(unsigned char * buffer, int length, int block_size, unsigned char base, unsigned char add);
unsigned int th_unlzss(unsigned char * buffer, unsigned char * decode, unsigned int length);

void get_th06(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("gameid", 0);
	
	if(len < sizeof(th06_replay_header_t)) return;
	th06_replay_header_t* rep_raw = (th06_replay_header_t*)buf;

	char ver[5] = "    ";
	snprintf(ver, 5, "%.2hhx%.2hhx", rep_raw->version[0], rep_raw->version[1]);
	out.Set("version", ver);
	out.Set("shot", rep_raw->shot);
	out.Set("difficulty", rep_raw->difficulty);

	size_t size = len - offsetof(th06_replay_header_t, crypted_data);
	if(size < sizeof(th06_replay_t) + sizeof(th06_replay_stage_t) + 6) {
		//	TODO return some kind of status
		return;
	}
	
	uint8_t* rep_dec = (uint8_t*)malloc(size);
	memcpy(rep_dec, rep_raw->crypted_data, size);
	th06_decrypt(rep_dec, rep_raw->key, size);

	th06_replay_t* rep = (th06_replay_t*)rep_dec;
	
	// Replay name
	if(rep->name[8] != '\0') rep->name[8] = '\0';
	out.Set("name", rep->name);
	// Date
	char date[11] = "2000-01-01";
	memcpy(date+5, rep->date, 2);
	memcpy(date+8, &rep->date[3], 2);
	out.Set("date", date);
	// Score
	out.Set("score", rep->score);
	out.Set("slowdown", rep->slowdown);
	
	
	Napi::Array stages = Napi::Array::New(env);
	
	for(int i = 0, h = 0; i < 6; i++) {
		if(rep->stage_offsets[i]) {
			Napi::Object stage_ = Napi::Object::New(env);

			uint32_t stage_off = rep->stage_offsets[i] - offsetof(th06_replay_header_t, crypted_data);
			if(stage_off + sizeof(th06_replay_stage_t) > size) {
				// out.Set("invalid", "stage data out of bounds");
				// return;
			} else {
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
	}
	out.Set("stages", stages);	
	free(rep_dec);
}

void get_th07(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("gameid", 1);

	th07_replay_header_t* rep_raw = (th07_replay_header_t*)malloc(len);

	char ver[5] = "    ";
	snprintf(ver, 5, "%.2hhx%.2hhx", rep_raw->version[0], rep_raw->version[1]);
	out.Set("version", ver);

	memcpy(rep_raw, buf, len);
	th06_decrypt(
		(uint8_t*)rep_raw + offsetof(th07_replay_header_t, field_10),
		rep_raw->key,
		len - offsetof(th07_replay_header_t, field_10)
	);
	
	uint8_t* rep_dec = (uint8_t*)malloc(rep_raw->size);
	th_unlzss((uint8_t*)rep_raw + sizeof(th07_replay_header_t), rep_dec, rep_raw->comp_size);

	th07_replay_t* rep = (th07_replay_t*)rep_dec;

	if(rep->name[8] != '\0') rep->name[8] = '\0';
	out.Set("name", rep->name);
	char date[11] = "2000-01-01";
	memcpy(date+5, rep->date, 2);
	memcpy(date+8, &rep->date[3], 2);
	out.Set("date", date);
	out.Set("score", (uint64_t)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("difficulty", rep->difficulty);
	out.Set("shot", rep->shot);

	Napi::Array stages = Napi::Array::New(env);
	for(int i = 0, h = 0; i < 7; i++) {
		if(rep_raw->stage_offsets[i]) {
			Napi::Object stage_ = Napi::Object::New(env);

			uint32_t stage_off = rep_raw->stage_offsets[i] - sizeof(th07_replay_header_t);
			if(stage_off + sizeof(th07_replay_stage_t) > rep_raw->size) {
				// out.Set("invalid", "stage data out of bounds");
				// return;
			} else {
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
							stage_.Set("score", (uint64_t)s_->score * 10);
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
	}
	out.Set("stages", stages);	
	free(rep_dec);
	free(rep_raw);

	return;
}

std::unordered_map<std::string_view, const char*> th08_shots {
	{ "\x97\xEC\x96\xB2\x81\x95\x8E\x87\x81\x40\x81\x40\x81\x40\x81\x40", "Border Team"},
	{ "\x96\x82\x97\x9D\x8D\xB9\x81\x95\x83\x41\x83\x8A\x83\x58\x81\x40", "Magic Team"},
	{ "\x8D\xE7\x96\xE9\x81\x95\x83\x8C\x83\x7E\x83\x8A\x83\x41\x81\x40", "Scarlet Team"},
	{ "\x97\x64\x96\xB2\x81\x95\x97\x48\x81\x58\x8E\x71\x81\x40\x81\x40", "Ghost Team"},
	{ "\x94\x8E\x97\xED\x81\x40\x97\xEC\x96\xB2\x81\x40\x81\x40\x81\x40", "Reimu" },
	{ "\x94\xAA\x89\x5F\x81\x40\x8E\x87\x81\x40\x81\x40\x81\x40\x81\x40", "Yukari" },
	{ "\x96\xB6\x89\x4A\x81\x40\x96\x82\x97\x9D\x8D\xB9\x81\x40\x81\x40", "Marisa" },
	{ "\x83\x41\x83\x8A\x83\x58\x81\x45\x82\x6C\x81\x40\x81\x40\x81\x40", "Alice" },
	{ "\x8F\x5C\x98\x5A\x96\xE9\x81\x40\x8D\xE7\x96\xE9\x81\x40\x81\x40", "Sakuya" },
	{ "\x83\x8C\x83\x7E\x83\x8A\x83\x41\x81\x45\x82\x72\x81\x40\x81\x40", "Remilia" },
	{ "\x8D\xB0\xE9\xAE\x81\x40\x97\x64\x96\xB2\x81\x40\x81\x40\x81\x40", "Youmu" },
	{ "\x90\xBC\x8D\x73\x8E\x9B\x81\x40\x97\x48\x81\x58\x8E\x71\x81\x40", "Yuyuko" }
};

void get_th08(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("gameid", 2);

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
		userdata_txt[user_offset + l] = 0;
		const char* _ = th08_shots[std::string_view(&userdata_txt[user_offset])];
		if(!_) {
			out.Set("shot", Napi::Buffer<char>::New(env, &userdata_txt[user_offset], l));
		} else {
			out.Set("shot", _);
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
		out.Set("stage", Napi::String::New(env, &userdata_txt[user_offset], l));
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
						stage_.Set("score", (uint64_t)s_->score * 10);
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
	out.Set("gameid", 10);
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;

	if(rep_raw->size < sizeof(th13_replay_t) + sizeof(th13_replay_stage_t) * 2 + 6) {
		// out.Set("invalid", "too small");
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
	out.Set("score", (uint64_t)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("cleared", rep->cleared);
	out.Set("shot", rep->shot);
	// out.Set("subshot", rep->subshot_unused);

				
	Napi::Array stages = Napi::Array::New(env);
	size_t stage_off = sizeof(th13_replay_t);
	for(uint32_t i = 0; i < rep->stage_count; i++) {
		Napi::Object stage_ = Napi::Object::New(env);
		th13_replay_stage_t* stage = (th13_replay_stage_t*)((size_t)rep + stage_off);

		if(stage_off + sizeof(th13_replay_stage_t) > rep_raw->size) {
			// fuck, quit
			i = rep->stage_count;
		} else {
			stage_.Set("stage", stage->stage_num);
			stage_.Set("score", (uint64_t)stage->score * 10);
			stage_.Set("graze", stage->graze);
			//stage_.Set("misscount", stage->miss_count);
			stage_.Set("piv", stage->piv);
			stage_.Set("power", stage->power);
			stage_.Set("lives", stage->lives);
			stage_.Set("life_pieces", stage->life_pieces);
			stage_.Set("bombs", stage->bombs);
			stage_.Set("bomb_pieces", stage->bomb_pieces);
			stage_.Set("trance", stage->trance_gauge);
					
			stages.Set(i, stage_);
			stage_off += stage->end_off + sizeof(th13_replay_stage_t);
		}
				
	}
	out.Set("stages", stages);
	free(rep_dec);
}

void get_th14(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("gameid", 11);
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;

	if(rep_raw->size < sizeof(th14_replay_t) + sizeof(th14_replay_stage_t) * 2 + 6) {
		// out.Set("invalid", "too small");
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
	out.Set("score", (uint64_t)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("cleared", rep->cleared);
	out.Set("shot", rep->shot);
	out.Set("subshot", rep->subshot);
	
				
	Napi::Array stages = Napi::Array::New(env);
	size_t stage_off = sizeof(th14_replay_t);
	for(uint32_t i = 0; i < rep->stage_count; i++) {
		Napi::Object stage_ = Napi::Object::New(env);
		th14_replay_stage_t* stage = (th14_replay_stage_t*)((size_t)rep + stage_off);

		if(stage_off + sizeof(th14_replay_stage_t) > rep_raw->size) {
			i = rep->stage_count;
			// out.Set("invalid", "stage data out of bounds");
			return;
		} else {
			stage_.Set("stage", stage->stage_num);
			stage_.Set("score", (uint64_t)stage->score * 10);
			stage_.Set("graze", stage->graze);
			//stage_.Set("misscount", stage->miss_count);
			stage_.Set("piv", stage->piv);
			stage_.Set("power", stage->power);
			stage_.Set("lives", stage->lives);
			stage_.Set("life_pieces", stage->life_pieces);
			stage_.Set("bombs", stage->bombs);
			stage_.Set("bomb_pieces", stage->bomb_pieces);
			stage_.Set("poc_count", stage->poc_count);
					
			stages.Set(i, stage_);
			stage_off += stage->end_off + sizeof(th14_replay_stage_t);
		}				
	}
	out.Set("stages", stages);
	free(rep_dec);
}

void get_th143(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("gameid", 12);
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;

	if(rep_raw->size < sizeof(th143_replay_t)) {
		// out.Set("invalid", "too small");
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
	out.Set("score", (uint64_t)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("primary_item", rep->primary_item);
	out.Set("secondary_item", rep->secondary_item);
	out.Set("day", rep->day);
	out.Set("scene", rep->scene);

	free(rep_dec);
}

void get_th15(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("gameid", 13);
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;
	
	if(rep_raw->size < sizeof(th15_replay_t) + sizeof(th15_replay_stage_t) * 2 + 6) {
		// out.Set("invalid", "too small");
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
	out.Set("score", (uint64_t)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("cleared", rep->cleared);
	out.Set("shot", rep->shot);
				
	Napi::Array stages = Napi::Array::New(env);
	size_t stage_off = sizeof(th15_replay_t);
	for(uint32_t i = 0; i < rep->stage_count; i++) {
		Napi::Object stage_ = Napi::Object::New(env);
		th15_replay_stage_t* stage = (th15_replay_stage_t*)((size_t)rep + stage_off);

		if(stage_off + sizeof(th15_replay_stage_t) > rep_raw->size) {
			i = rep->stage_count;
			// out.Set("invalid", "stage data out of bounds");
			// return;
		} else {
			stage_.Set("stage", stage->stage_num);
			stage_.Set("score", (uint64_t)stage->score * 10);
			stage_.Set("graze", stage->graze);
			stage_.Set("misscount", stage->miss_count);
			stage_.Set("piv", stage->piv);
			stage_.Set("power", stage->power);
			stage_.Set("lives", stage->lives);
			stage_.Set("life_pieces", stage->life_pieces);
			stage_.Set("bombs", stage->bombs);
			stage_.Set("bomb_pieces", stage->bomb_pieces);
					
			stages.Set(i, stage_);
			stage_off += stage->end_off + sizeof(th15_replay_stage_t);
		}				
	}
	out.Set("stages", stages);
	free(rep_dec);
}

void get_th16(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("gameid", 14);
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;
	
	if(rep_raw->size < sizeof(th16_replay_t) + sizeof(th16_replay_stage_t) * 2 + 6) {
		// out.Set("invalid", "too small");
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
	out.Set("score", (uint64_t)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("cleared", rep->cleared);
	out.Set("shot", rep->shot);
	out.Set("subseason", rep->subseason);

	Napi::Array stages = Napi::Array::New(env);
	size_t stage_off = sizeof(th16_replay_t);
	for(uint32_t i = 0; i < rep->stage_count; i++) {
		Napi::Object stage_ = Napi::Object::New(env);
		th16_replay_stage_t* stage = (th16_replay_stage_t*)((size_t)rep + stage_off);
				
		if(stage_off + sizeof(th16_replay_stage_t) > rep_raw->size) {
			i = rep->stage_count;
			// out.Set("invalid", "stage data out of bounds");
			// return;
		} else {
			stage_.Set("stage", stage->stage_num);
			stage_.Set("score", (uint64_t)stage->score * 10);
			stage_.Set("graze", stage->graze);
			stage_.Set("misscount", stage->miss_count);
			stage_.Set("piv", stage->piv);
			stage_.Set("power", stage->power);
			stage_.Set("lives", stage->lives);
			stage_.Set("bombs", stage->bombs);
			stage_.Set("bomb_pieces", stage->bomb_pieces);
			stage_.Set("season", stage->season);
			
			// TODO: maybe check that this data stays the same every stage?
			// I don't know much about this game, of it's natural for it
			// to change between stages I wouldn't know
			out.Set("season_max", stage->season_max);
			Napi::Array season_required = Napi::Array::New(env);
			for(uint32_t i = 0; i < 6; i++) {
				season_required.Set(i, stage->season_required[i]);
			}
			out.Set("season_required", season_required);
					
			stages.Set(i, stage_);
			stage_off += stage->end_off + sizeof(th16_replay_stage_t);
		}
	}
	out.Set("stages", stages);
	free(rep_dec);
}

void get_th165(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("gameid", 15);
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;

	if(rep_raw->size < sizeof(th165_replay_t)) {
		// out.Set("invalid", "too small");
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
	out.Set("gameid", 16);
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;

	if(rep_raw->size < sizeof(th17_replay_t) + sizeof(th17_replay_stage_t) * 2 + 6) {
		// out.Set("invalid", "too small");
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
	out.Set("score", (uint64_t)rep->score * 10);
	out.Set("slowdown", rep->slowdown);
	out.Set("cleared", rep->cleared);
	out.Set("shot", rep->shot);
	out.Set("subshot", rep->subshot);

	Napi::Array stages = Napi::Array::New(env);
	size_t stage_off = sizeof(th17_replay_t);
	for(uint32_t i = 0; i < rep->stage_count; i++) {
		Napi::Object stage_ = Napi::Object::New(env);
		th17_replay_stage_t* stage = (th17_replay_stage_t*)((size_t)rep + stage_off);

		if(stage_off + sizeof(th17_replay_stage_t) > rep_raw->size) {
			i = rep->stage_count;
			// out.Set("invalid", "stage data out of bounds");
			// return;
		} else {
			stage_.Set("stage", stage->stage_num);
			stage_.Set("score", (uint64_t)stage->score * 10);
			stage_.Set("graze", stage->graze);
			stage_.Set("misscount", stage->miss_count);
			stage_.Set("piv", stage->piv);
			stage_.Set("power", stage->power);
			stage_.Set("lives", stage->lives);
			stage_.Set("life_pieces", stage->life_pieces);
			stage_.Set("bombs", stage->bombs);
			stage_.Set("bomb_pieces", stage->bomb_pieces);
			
			Napi::Array tokens = Napi::Array::New(env);
			for(uint32_t j = 0; j < 5; j++) {
				tokens.Set(j, stage->tokens[j]);
			}
			stage_.Set("tokens", tokens);
			
			stages.Set(i, stage_);
			stage_off += stage->end_off + sizeof(th17_replay_stage_t);
		}				
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
	out.Set("score", (uint64_t)rep->score * 10);
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
				
		stage_.Set("stage", stage->stage_num);
		stage_.Set("score", (uint64_t)stage->score * 10);
		stage_.Set("graze", stage->graze);
		stage_.Set("misses", stage->miss_count);
		stage_.Set("piv", stage->piv);
		stage_.Set("power", stage->power);
		stage_.Set("lives", stage->lives);
		stage_.Set("life_pieces", stage->life_pieces);
		stage_.Set("bombs", stage->bombs);
		stage_.Set("bomb_pieces", stage->bomb_pieces);
		
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
