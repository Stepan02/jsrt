#pragma once
#include <string>
#include <unordered_map>
#include "v8.h"

struct ModuleRegistry {
    std::unordered_map<std::string, std::string> modules;
    std::unordered_map<int, std::string> paths;
    std::unordered_map<std::string, v8::Global<v8::Module>> module_cache;
};

void ClearModuleCache();

v8::Local<v8::Module> CompileModule(v8::Isolate *isolate, const std::string &path);

v8::MaybeLocal<v8::Module> ResolveModuleCallback(v8::Local<v8::Context> context, v8::Local<v8::String> specifier,
                                                 v8::Local<v8::FixedArray> import_assertions, v8::Local<v8::Module> referrer);
