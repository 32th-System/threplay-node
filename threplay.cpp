#include <napi.h>
#include "zuntypes.h"

void th_decrypt(unsigned char * buffer, int length, int block_size, unsigned char base, unsigned char add);
unsigned int th_unlzss(unsigned char * buffer, unsigned char * decode, unsigned int length);

Napi::Object get_replay_data(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	
	Napi::Object obj = Napi::Object::New(env);
	obj.Set(Napi::String::New(env, "Deez"), Napi::String::New(env, "nUTS"));
	obj.Set(Napi::String::New(env, "Truth"), Napi::Number::New(env, 42));
	
	
	return obj;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	exports.Set(Napi::String::New(env, "get_replay_data"),
				Napi::Function::New(env, get_replay_data));
	return exports;
}

NODE_API_MODULE(threplay, Init)