#include <iostream>
#include <string>
#include <napi.h>
#include <libcamera/libcamera.h>

namespace cam
{
    std::string getCameraInfos(libcamera::Camera *camera);

    int cameraStream();
    Napi::Number cameraStreamWrapped(const Napi::CallbackInfo& info);

    Napi::Object Init(Napi::Env env, Napi::Object exports);
}
