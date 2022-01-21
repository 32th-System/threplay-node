#include <napi.h>
#include <stdlib.h>
#include "zuntypes.h"

#include <string>

unsigned int th06_decrypt(unsigned char* buf, char key, unsigned int length);
void th_decrypt(unsigned char * buffer, int length, int block_size, unsigned char base, unsigned char add);
unsigned int th_unlzss(unsigned char * buffer, unsigned char * decode, unsigned int length);

void get_th06(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th06");
	
	th06_replay_header_t* rep_raw = (th06_replay_header_t*)buf;
	size_t size = len - offsetof(th06_replay_header_t, crypted_data);
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
	
	for(int i = 0; i < 6; i++) {
		if(rep->stage_offsets[i]) {
			Napi::Object stage_ = Napi::Object::New(env);
			th06_replay_stage_t* stage = (th06_replay_stage_t*)(rep_dec + rep->stage_offsets[i] - offsetof(th06_replay_header_t, crypted_data));
				
			stage_.Set("stage", i + 1);
			stage_.Set("score", stage->score);
			stage_.Set("power", stage->power);
			stage_.Set("lives", stage->lives);
			stage_.Set("bombs", stage->bombs);

			stages.Set(i, stage_);
		}
	}
	out.Set("stages", stages);	
	free(rep_dec);
}

void get_th13(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th13");
	
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;
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

void get_th15(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th15");
	
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;
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
	}
	out.Set("stages", stages);
	free(rep_dec);
}

void get_th17(Napi::Object& out, uint8_t* buf, size_t len, Napi::Env& env) {
	out.Set("game", "th17");
	
	th13_replay_header_t* rep_raw = (th13_replay_header_t*)buf;
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
	case 0x72333174: // "t13r"
		_ver = buf[_th13_rep->userdata_offset + 16];
		if(_ver == 0x90) {			
			get_th13(ret, buf, len, env);
		} else if(_ver == 0x8b) {
			get_th14(ret, buf, len, env);
		}
		break;
	case 0x72353174: // "t15r"
		get_th15(ret, buf, len, env);
		break;
	case 0x72363174: // "t16r"
		get_th16(ret, buf, len, env);
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