#pragma once
#include "v8.h"

namespace Runtime {
    void ConsoleLog(const v8::FunctionCallbackInfo<v8::Value>& args);
    void ConsoleWarn(const v8::FunctionCallbackInfo<v8::Value>& args);
    void ConsoleError(const v8::FunctionCallbackInfo<v8::Value>& args);
}
