#include <stdio.h>
#include <memory>
#include <cstdlib>
#include "v8.h"
#include "libplatform/libplatform.h"
#include "file/read.h"
#include "runtime/console.h"
#include "runtime/error.h"
#include "runtime/module.h"

extern char **environ;

static int ExecuteScript(v8::Isolate *isolate, const std::string &script_name) {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    const v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);

    struct ModuleCacheGuard {
        ~ModuleCacheGuard() { ClearModuleCache(); }
    } cache_guard;

    // attach environment variables to the context object
    const v8::Local<v8::Object> environment_variables = v8::Object::New(isolate);
    for (char **env = environ; *env != nullptr; env++) {
        std::string entry(*env);
        const size_t delimiter = entry.find('=');
        if (delimiter == std::string::npos) {
            continue;
        }

        std::string key = entry.substr(0, delimiter);
        std::string value = entry.substr(delimiter + 1);

        const v8::Local<v8::String> isolate_key = v8::String::NewFromUtf8(isolate, key.c_str(), v8::NewStringType::kNormal).
                ToLocalChecked();
        const v8::Local<v8::String> isolate_value = v8::String::NewFromUtf8(isolate, value.c_str(), v8::NewStringType::kNormal).
                ToLocalChecked();

        environment_variables->Set(context, isolate_key, isolate_value).Check();
    }

    const v8::Local<v8::Object> context_object = v8::Object::New(isolate);
    context_object->Set(context, v8::String::NewFromUtf8(isolate, "env").ToLocalChecked(), environment_variables).Check();
    context->Global()->Set(context, v8::String::NewFromUtf8(isolate, "context").ToLocalChecked(), context_object).Check();

    // create console object
    const v8::Local<v8::Object> console = v8::Object::New(isolate);

    // add console.log function
    const v8::Local<v8::FunctionTemplate> console_log_template = v8::FunctionTemplate::New(isolate, Runtime::ConsoleLog);
    const v8::Local<v8::Function> console_log_function = console_log_template->GetFunction(context).ToLocalChecked();
    console->Set(context, v8::String::NewFromUtf8(isolate, "log").ToLocalChecked(), console_log_function).Check();

    // add console.warn function
    const v8::Local<v8::FunctionTemplate> console_warn_template = v8::FunctionTemplate::New(isolate, Runtime::ConsoleWarn);
    const v8::Local<v8::Function> console_warn_function = console_warn_template->GetFunction(context).ToLocalChecked();
    console->Set(context, v8::String::NewFromUtf8(isolate, "warn").ToLocalChecked(), console_warn_function).Check();

    // add console.error function
    const v8::Local<v8::FunctionTemplate> console_error_template = v8::FunctionTemplate::New(isolate, Runtime::ConsoleError);
    const v8::Local<v8::Function> console_error_function = console_error_template->GetFunction(context).ToLocalChecked();
    console->Set(context, v8::String::NewFromUtf8(isolate, "error").ToLocalChecked(), console_error_function).Check();

    // attach console object
    context->Global()->Set(context, v8::String::NewFromUtf8(isolate, "console").ToLocalChecked(), console).Check();

    // create try-catch scope for runtime exception handling
    const v8::TryCatch try_catch(isolate);

    // compile root script as a module
    const v8::Local<v8::Module> module = CompileModule(isolate, script_name);
    if (module.IsEmpty()) {
        Runtime::HandleRuntimeException(isolate, context, try_catch);
        return 1;
    }

    // instantiate and link dependencies
    if (!module->InstantiateModule(context, ResolveModuleCallback).FromMaybe(false)) {
        Runtime::HandleRuntimeException(isolate, context, try_catch);
        return 1;
    }

    // execute script
    v8::Local<v8::Value> result;
    if (!module->Evaluate(context).ToLocal(&result)) {
        Runtime::HandleRuntimeException(isolate, context, try_catch);
        return 1;
    }

    // handle promise
    if (result->IsPromise()) {
        if (const auto promise = result.As<v8::Promise>(); promise->State() == v8::Promise::kRejected) {
            isolate->ThrowException(promise->Result());
            Runtime::HandleRuntimeException(isolate, context, try_catch);
            return 1;
        }
    }

    return 0;
}

int main(const int argc, char *argv[]) {
    // parse script file
    if (argc < 2) {
        fprintf(stderr, "usage: %s <path>\n", argv[0]);
        return 1;
    }

    // parse script
    if (const std::string script_string = ReadFile(argv[1]); script_string.empty()) {
        fprintf(stderr, "Could not read %s\n", argv[1]);
        return 1;
    }

    // init v8
    const std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::SetFlagsFromString("--harmony-temporal");
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();

    // create a v8 isolate
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate *isolate = v8::Isolate::New(create_params);

    // execute the script
    const int exit_code = ExecuteScript(isolate, argv[1]);

    // dispose the isolate
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
    delete create_params.array_buffer_allocator;

    return exit_code;
}
