#pragma once

#include "safetyhook/easy.hpp"
#include "safetyhook/inline_hook.hpp"
#include "safetyhook/mid_hook.hpp"
#include "safetyhook/vmt_hook.hpp"

using SafetyHookContext = safetyhook::Context;
using SafetyHookInline = safetyhook::InlineHook;
using SafetyHookMid = safetyhook::MidHook;
using SafetyInlineHook [[deprecated("Use SafetyHookInline instead.")]] = safetyhook::InlineHook;
using SafetyMidHook [[deprecated("Use SafetyHookMid instead.")]] = safetyhook::MidHook;
using SafetyHookVmt = safetyhook::VmtHook;
using SafetyHookVm = safetyhook::VmHook;
