#pragma once
#include <v8.h>

namespace Runtime {
    void HandleRuntimeException(v8::Isolate* isolate, v8::Local<v8::Context> context, const v8::TryCatch& try_catch);
}
