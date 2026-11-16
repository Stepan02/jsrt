#pragma once
#include <string>
#include "v8.h"

void ClearModuleCache();

v8::Local<v8::Module> CompileModule(v8::Isolate *isolate, const std::string &path);

v8::MaybeLocal<v8::Module> ResolveModuleCallback(v8::Local<v8::Context> context, v8::Local<v8::String> specifier,
                                                 v8::Local<v8::FixedArray> import_assertions, v8::Local<v8::Module> referrer);
