#include "module.h"
#include <filesystem>
#include <unordered_map>
#include <ranges>
#include "v8.h"
#include "../file/read.h"

namespace {
    std::unordered_map<int, std::string> g_module_paths;
    inline std::unordered_map<std::string, v8::Global<v8::Module>> g_module_cache;
}

void ClearModuleCache() {
    for (auto &mod: g_module_cache | std::views::values) {
        mod.Reset();
    }

    g_module_cache.clear();
    g_module_paths.clear();
}

v8::Local<v8::Module> CompileModule(v8::Isolate *isolate, const std::string &raw_path) {
    const std::string path = std::filesystem::path(raw_path).lexically_normal().string();

    // cache hit
    if (g_module_cache.contains(path)) {
        return g_module_cache[path].Get(isolate);
    }

    const std::string script = ReadFile(path);
    const v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, script.c_str(), v8::NewStringType::kNormal).ToLocalChecked();

    const v8::Local<v8::String> file_name = v8::String::NewFromUtf8(isolate, path.c_str(), v8::NewStringType::kNormal).ToLocalChecked();

    const v8::ScriptOrigin origin(file_name, 0, 0, false, -1, v8::Local<v8::Value>(), false, false, true);
    v8::ScriptCompiler::Source source_obj(source, origin);

    // compile the module
    v8::Local<v8::Module> module;
    if (!v8::ScriptCompiler::CompileModule(isolate, &source_obj).ToLocal(&module)) {
        return {}; // return empty handle if compilation fails
    }

    // save the module path for referrer
    g_module_paths[module->GetIdentityHash()] = path;
    g_module_cache[path].Reset(isolate, module);

    return module;
}

v8::MaybeLocal<v8::Module> ResolveModuleCallback(const v8::Local<v8::Context> context, const v8::Local<v8::String> specifier,
                                                 v8::Local<v8::FixedArray> import_assertions, const v8::Local<v8::Module> referrer) {
    v8::Isolate *isolate = context->GetIsolate();

    // convert specifier string to standard string
    v8::String::Utf8Value specifier_str(isolate, specifier);
    const std::string specifier_path(*specifier_str, specifier_str.length());

    std::string target_path;

    if (referrer.IsEmpty() || !g_module_paths.contains(referrer->GetIdentityHash())) {
        // relative to cwd
        target_path = specifier_path;
    } else {
        // absolute path
        const std::string referrer_path = g_module_paths[referrer->GetIdentityHash()];
        const std::filesystem::path base_directory = std::filesystem::path(referrer_path).parent_path();

        target_path = (base_directory / specifier_path).lexically_normal().string();
    }

    // compile module
    const v8::Local<v8::Module> module = CompileModule(isolate, target_path);

    return module;
}
