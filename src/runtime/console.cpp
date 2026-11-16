#include "console.h"
#include "v8.h"

namespace Runtime {
    void ConsoleLog(const v8::FunctionCallbackInfo<v8::Value>& args) {
        v8::Isolate* isolate = args.GetIsolate();
        v8::HandleScope handle_scope(isolate);

        // output to stdout
        for (int i = 0; i < args.Length(); i++) {
            if (const v8::String::Utf8Value str(isolate, args[i]); *str) {
                fprintf(stdout, "%s", *str);

                if (i + 1 < args.Length()) {
                    fprintf(stdout, " ");
                }
            }
        }

        fprintf(stdout, "\n");
    }

    void ConsoleWarn(const v8::FunctionCallbackInfo<v8::Value>& args) {
        v8::Isolate* isolate = args.GetIsolate();
        v8::HandleScope handle_scope(isolate);

        // output to stderrr
        fprintf(stderr, "\033[33m");
        for (int i = 0; i < args.Length(); i++) {
            if (const v8::String::Utf8Value str(isolate, args[i]); *str) {
                fprintf(stderr, "%s", *str);

                if (i + 1 < args.Length()) {
                    fprintf(stderr, " ");
                }
            }
        }

        fprintf(stderr, "\033[0m\n");
    }

    void ConsoleError(const v8::FunctionCallbackInfo<v8::Value>& args) {
        v8::Isolate* isolate = args.GetIsolate();
        v8::HandleScope handle_scope(isolate);

        // output to stderrr
        fprintf(stderr, "\033[31m");
        for (int i = 0; i < args.Length(); i++) {
            if (const v8::String::Utf8Value str(isolate, args[i]); *str) {
                fprintf(stderr, "%s", *str);

                if (i + 1 < args.Length()) {
                    fprintf(stderr, " ");
                }
            }
        }

        fprintf(stderr, "\033[0m\n");
    }
}
