#include <napi.h>
#include "cam.hpp"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return cam::Init(env, exports);
}

NODE_API_MODULE(camAddon, InitAll)