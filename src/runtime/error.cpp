#include "error.h"
#include <cstdio>

namespace Runtime {
    void HandleRuntimeException(v8::Isolate *isolate, const v8::Local<v8::Context> context, const v8::TryCatch &try_catch) {
        v8::String::Utf8Value exception_string(isolate, try_catch.Exception());

        if (const v8::Local<v8::Message> message = try_catch.Message(); !message.IsEmpty()) {
            // get error location metadata
            const int line_number = message->GetLineNumber(context).FromJust();
            const int column_number = message->GetStartColumn(context).FromJust();
            v8::String::Utf8Value file(isolate, message->GetScriptResourceName());

            fprintf(stderr, "\n[Runtime Error] %s (Line %d, Col %d)\n", *file, line_number, column_number);
            fprintf(stderr, " > %s\n", *exception_string);

            // get stack trace if available
            v8::Local<v8::Value> stack_trace;
            if (try_catch.StackTrace(context).ToLocal(&stack_trace)) {
                v8::String::Utf8Value stack_trace_string(isolate, stack_trace);
                fprintf(stderr, "\nStack Trace:\n%s\n", *stack_trace_string);
            }
        } else {
            // fallback
            fprintf(stderr, "\n[Runtime Error] %s\n", *exception_string);
        }
    }
}
