#include "module.h"
#include <filesystem>
#include <unordered_map>
#include <ranges>
#include "v8.h"

void ClearModuleCache() {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    if (!isolate) {
        return;
    }

    auto* registry = static_cast<ModuleRegistry*>(isolate->GetData(0));
    if (!registry) {
        return;
    }

    for (auto &mod: registry->module_cache | std::views::values) {
        mod.Reset();
    }

    registry->module_cache.clear();
    registry->paths.clear();
}

v8::Local<v8::Module> CompileModule(v8::Isolate *isolate, const std::string &raw_path) {
    const std::string path = std::filesystem::path(raw_path).lexically_normal().generic_string();

    // fetch module registry from isolate data slot 0
    auto* registry = static_cast<ModuleRegistry*>(isolate->GetData(0));

    // registry cache hit
    if (registry->module_cache.contains(path)) {
        return registry->module_cache[path].Get(isolate);
    }

    // handle missing module
    if (!registry->modules.contains(path)) {
        const std::string error_message = "Module not found: " + path;
        const v8::Local<v8::String> message = v8::String::NewFromUtf8(isolate, error_message.c_str(), v8::NewStringType::kNormal).ToLocalChecked();

        // throw isolate error
        isolate->ThrowException(v8::Exception::Error(message));
        return {};
    }

    const std::string &script = registry->modules[path];
    const v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, script.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
    const v8::Local<v8::String> file_name = v8::String::NewFromUtf8(isolate, path.c_str(), v8::NewStringType::kNormal).ToLocalChecked();

    const v8::ScriptOrigin origin(file_name, 0, 0, false, -1, v8::Local<v8::Value>(), false, false, true);
    v8::ScriptCompiler::Source source_obj(source, origin);

    // compile the module
    v8::Local<v8::Module> module;
    if (!v8::ScriptCompiler::CompileModule(isolate, &source_obj).ToLocal(&module)) {
        return {}; // return empty handle if compilation fails
    }

    // save the module to registry cache
    registry->paths[module->GetIdentityHash()] = path;
    registry->module_cache[path].Reset(isolate, module);

    return module;
}

v8::MaybeLocal<v8::Module> ResolveModuleCallback(const v8::Local<v8::Context> context, const v8::Local<v8::String> specifier,
                                                 v8::Local<v8::FixedArray> import_assertions, const v8::Local<v8::Module> referrer) {
    v8::Isolate *isolate = context->GetIsolate();

    // fetch module registry from isolate data slot 0
    auto* registry = static_cast<ModuleRegistry*>(isolate->GetData(0));

    // convert specifier string to standard string
    v8::String::Utf8Value specifier_str(isolate, specifier);
    const std::string specifier_path(*specifier_str, specifier_str.length());

    std::string target_path;

    if (referrer.IsEmpty() || !registry->paths.contains(referrer->GetIdentityHash())) {
        // relative to cwd or entrypoint
        target_path = specifier_path;
    } else {
        // absolute path
        const std::string referrer_path = registry->paths[referrer->GetIdentityHash()];
        const std::filesystem::path base_directory = std::filesystem::path(referrer_path).parent_path();

        target_path = (base_directory / specifier_path).lexically_normal().generic_string();
    }

    // compile module
    const v8::Local<v8::Module> module = CompileModule(isolate, target_path);

    return module;
}
