#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <windows.h>
#include <intrin.h>

extern void ConsoleLog(const char* format, ...);

namespace RecRoom {

    using u64 = uintptr_t;

    static constexpr u64 OFF_get_LocalPlayer = 0x17D0700;     // Player::get_LocalPlayer
    static constexpr u64 OFF_Player_Self = 0x17C97E0;         // Player::Self (static backing)
    static constexpr u64 OFF_GetAllPlayers = 0x17B0080;      // Player::GetAllPlayers
    static constexpr u64 OFF_get_IsLocal = 0x17CF800;        // Player::get_IsLocal (instance)

    inline bool SafeReadPtr(u64 addr, u64& out) {
        __try {
            out = *(u64*)addr;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool SafeReadInt(u64 addr, int& out) {
        __try {
            out = *(int*)addr;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline void* CallFunctionRVA(u64 rva) {
        HMODULE h = GetModuleHandleA("GameAssembly.dll");
        if (!h) return nullptr;
        u64 base = (u64)h;
        void* fn = (void*)(base + rva);
        if (!fn) return nullptr;

        typedef void*(__fastcall* FnT)();
        FnT f = (FnT)fn;
        __try {
            return f();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    inline void* GetLocalPlayer() {
        HMODULE h = GetModuleHandleA("GameAssembly.dll");
        if (!h) {
            ConsoleLog("[RecRoom] GameAssembly.dll not loaded");
            return nullptr;
        }
        u64 base = (u64)h;

        void* res = CallFunctionRVA(OFF_get_LocalPlayer);
        if (res) return res;

        u64 selfAddr = base + OFF_Player_Self;
        u64 selfPtr = 0;
        if (SafeReadPtr(selfAddr, selfPtr) && selfPtr) {
            ConsoleLog("[RecRoom] Found local player via static Self: 0x%llX", selfPtr);
            return (void*)selfPtr;
        }

        void* arr = CallFunctionRVA(OFF_GetAllPlayers);
        if (!arr) {
            ConsoleLog("[RecRoom] GetAllPlayers returned null");
            return nullptr;
        }

        int count = 0;
        if (!SafeReadInt((u64)arr + 0x10, count) || count <= 0) {
            ConsoleLog("[RecRoom] Player array empty or unreadable");
            return nullptr;
        }

        u64 itemsBase = (u64)arr + 0x20;
        void* isLocalFnPtr = (void*)(base + OFF_get_IsLocal);
        typedef bool(__fastcall* IsLocalFn)(void*);
        IsLocalFn isLocalFn = (IsLocalFn)isLocalFnPtr;

        for (int i = 0; i < count; ++i) {
            u64 candidate = 0;
            if (!SafeReadPtr(itemsBase + (u64)i * sizeof(u64), candidate) || !candidate) continue;
            bool ok = false;
            if (isLocalFn) {
                __try { ok = isLocalFn((void*)candidate); } __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }
            }
            if (ok) {
                ConsoleLog("[RecRoom] Found local player from array: 0x%llX", candidate);
                return (void*)candidate;
            }
        }

        ConsoleLog("[RecRoom] Could not resolve local player");
        return nullptr;
    }

    inline std::vector<void*> GetAllPlayers() {
        std::vector<void*> out;
        HMODULE h = GetModuleHandleA("GameAssembly.dll");
        if (!h) return out;
        u64 base = (u64)h;

        void* arr = CallFunctionRVA(OFF_GetAllPlayers);
        if (!arr) return out;

        int count = 0;
        if (!SafeReadInt((u64)arr + 0x10, count) || count <= 0) return out;

        u64 itemsBase = (u64)arr + 0x20;
        for (int i = 0; i < count; ++i) {
            u64 candidate = 0;
            if (SafeReadPtr(itemsBase + (u64)i * sizeof(u64), candidate) && candidate) out.push_back((void*)candidate);
        }
        return out;
    }

}
