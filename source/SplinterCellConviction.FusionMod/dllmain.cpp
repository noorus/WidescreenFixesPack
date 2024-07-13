#include "stdafx.h"
#include <LEDEffects.h>

float gVisibility = 1.0f;
int32_t gBlacklistIndicators = 0;
bool bDisableBlackAndWhiteFilter = false;

injector::hook_back<void (__cdecl*)(float a1, float a2, int a3)> hb_8330DB;
void __cdecl sub_8330DB(float a1, float a2, int a3)
{
    gVisibility = a1;

    if (!bDisableBlackAndWhiteFilter)
        return hb_8330DB.fun(a1, a2, a3);
}

injector::hook_back<void(__cdecl*)(void* a1, int a2, int a3)> hb_100177B7;
void __cdecl sub_100177B7(void* a1, int a2, int a3)
{
    if (gBlacklistIndicators) //enables bloom in dark areas
    {
        if (gVisibility == 1.0f)
            return hb_100177B7.fun(a1, a2, a3);
        else
            return;
    }
    return hb_100177B7.fun(a1, a2, a3);
}

// Distortion during movement (sonar)
injector::hook_back<void(__fastcall*)(void* self, int edx)> hb_1002581C;
void __fastcall sub_1002581C(void* self, int edx)
{
    //return hb_1002581C.fun(self, edx);
}

SafetyHookInline sub_78C666_hook{};
int __cdecl sub_78C666(char* a1, int a2)
{
    return sub_78C666_hook.unsafe_ccall<int>(a1, a2);
}

void Init()
{
    CIniReader iniReader("");
    auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    auto bSkipSystemDetection = iniReader.ReadInteger("MAIN", "SkipSystemDetection", 1) != 0;
    bDisableBlackAndWhiteFilter = iniReader.ReadInteger("MAIN", "DisableBlackAndWhiteFilter", 0) != 0;
    auto bDisableCharacterLighting = iniReader.ReadInteger("MAIN", "DisableCharacterLighting", 0) != 0;

    if (bSkipIntro)
    {
        auto pattern = hook::pattern("55 8D 6C 24 88 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 74 53 56 57 BE ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? E8 ? ? ? ? 50");
        injector::MakeRET(pattern.get_first());
    }

    if (bSkipSystemDetection)
    {
        auto pattern = hook::pattern("55 8D AC 24 ? ? ? ? 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 85 ? ? ? ? 0F 57 C0 53 56 57 33 F6 33 DB 46 68");
        injector::WriteMemory<uint32_t>(pattern.get_first(0), 0xC3C0940F, true);
        injector::MakeRET(pattern.get_first(3));
    }

    //if (bDisableBlackAndWhiteFilter) //light and shadow
    {
        auto pattern = hook::pattern("E8 ? ? ? ? D9 05 ? ? ? ? 83 C4 0C 33 F6 56 51");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C 83 3D ? ? ? ? ? 0F 8C ? ? ? ? 7F 0D 83 3D ? ? ? ? ? 0F 86");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C 83 3D ? ? ? ? ? 7C 77 7F 09 83 3D ? ? ? ? ? 76 6C");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("D9 5C 24 04 D9 40 08 D9 1C 24 E8 ? ? ? ? 83 C4 0C C3");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(10), sub_8330DB, true).get();
    }

    if (bDisableCharacterLighting)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 33 C9 38 48 1C");
        static SafetyHookMid test = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& ctx)
        {
            sub_78C666_hook.unsafe_ccall<int>("l3d-char-lighting");
        });

        pattern = hook::pattern("E9 ? ? ? ? 8D 45 90 68 ? ? ? ? 50 E8 ? ? ? ? 85 C0 59 59 74 0B 81 C6 ? ? ? ? E9 ? ? ? ? 8D 45 88 50 68 ? ? ? ? FF 75 90 E8 ? ? ? ? 83 C4 0C 85 C0 74 12 F3 0F 10 45 ? F3 0F 11 86 ? ? ? ? E9 ? ? ? ? 8D 45 88");
        static SafetyHookMid test2 = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& ctx)
        {
            *(uint8_t*)ctx.esi ^= 1;
        });

        pattern = hook::pattern("55 8D AC 24 ? ? ? ? 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 85 ? ? ? ? 8B 85 ? ? ? ? 56 57 8B BD");
        sub_78C666_hook = safetyhook::create_inline(pattern.get_first(0), sub_78C666);
    }
}

void InitLeadD3DRender()
{
    CIniReader iniReader("");
    auto bDisableDOF = iniReader.ReadInteger("MAIN", "DisableDOF", 1) != 0;
    auto bDisableBlackAndWhiteFilter = iniReader.ReadInteger("MAIN", "DisableBlackAndWhiteFilter", 0) != 0;
    auto bEnhancedSonarVision = iniReader.ReadInteger("MAIN", "EnhancedSonarVision", 0) != 0;
    gBlacklistIndicators = iniReader.ReadInteger("MAIN", "BlacklistIndicators", 0);

    if (bDisableDOF)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "E8 ? ? ? ? 8B 06 8B 08 83 C4 18 6A 00 68");
        injector::MakeNOP(pattern.get_first(0), 5);
    }

    if (bDisableBlackAndWhiteFilter)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "E8 ? ? ? ? 8B 06 8B 08 83 C4 34");
        hb_100177B7.fun = injector::MakeCALL(pattern.get_first(), sub_100177B7, true).get();
    }

    if (bEnhancedSonarVision)
    {
        static bool bNightVision = false;
        static auto loc_1002E95F = (uintptr_t)hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "89 B8 ? ? ? ? E8 ? ? ? ? 5F 5E 5B 83 C5 78 C9 C3").get_first(13);

        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "E8 ? ? ? ? 89 5D 78");
        hb_1002581C.fun = injector::MakeCALL(pattern.get_first(), sub_1002581C, true).get();

        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "8B CE 88 86");
        struct SonarVisionHook
        {
            void operator()(injector::reg_pack& regs)
            {
                bNightVision = regs.eax & 0xff;
                //if (false)
                //    *(uint8_t*)(regs.esi + 0x642C) = regs.eax & 0xff;
                //else
                *(uint8_t*)(regs.esi + 0x642C) = 0;
            }
        }; injector::MakeInline<SonarVisionHook>(pattern.get_first(2), pattern.get_first(8));

        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "80 B9 ? ? ? ? ? 0F 84 ? ? ? ? 56 57 E8 ? ? ? ? 8B 03 33 FF 47");
        struct NVCheck
        {
            void operator()(injector::reg_pack& regs)
            {
                if (!bNightVision)
                    *(uintptr_t*)(regs.esp - 4) = loc_1002E95F;
            }
        }; injector::MakeInline<NVCheck>(pattern.get_first(0), pattern.get_first(13));

        //DrawVisibleOpaque
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "68 ? ? ? ? 57 8D 43 0C");
        injector::MakeNOP(pattern.get_first(0), 17);
        
        //to do: add bNightVision check
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "75 15 8B 80");
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);
        
        //to do: add bNightVision check
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "80 B8 ? ? ? ? ? 74 65");
        injector::MakeNOP(pattern.get_first(0), 7);
        
        //to do: add bNightVision check
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "8B 03 80 B8");
        injector::MakeNOP(pattern.get_first(2), 7);
    }
}

void InitLED()
{
    CIniReader iniReader("");
    auto bLightSyncRGB = iniReader.ReadInteger("LOGITECH", "LightSyncRGB", 1);

    if (bLightSyncRGB)
    {
        LEDEffects::Inject([]()
        {
            static auto fPlayerVisibility = gVisibility;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto gVisCmp = static_cast<float>(static_cast<int>(gVisibility * 10.0f)) / 10.0f;
            auto fPlVisCmp = static_cast<float>(static_cast<int>(fPlayerVisibility * 10.0f)) / 10.0f;
            
            if (fPlVisCmp > gVisCmp)
                fPlayerVisibility -= 0.05f;
            else if (fPlVisCmp < gVisCmp)
                fPlayerVisibility += 0.05f;
            
            constexpr auto minBrightness = 0.3f;
            constexpr auto maxBrightness = 1.0f;

            fPlayerVisibility = std::clamp(fPlayerVisibility, minBrightness, maxBrightness);
            
            auto [R, G, B] = LEDEffects::RGBtoPercent(255, 39, 26, gBlacklistIndicators ? fPlayerVisibility : ((maxBrightness + minBrightness) - fPlayerVisibility));
            LEDEffects::SetLighting(R, G, B, false, false, false);
        });
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("D9 1C 24 E8 ? ? ? ? D9 5E 0C"));
        CallbackHandler::RegisterCallback(L"LeadD3DRender.dll", InitLeadD3DRender);
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitLED, hook::pattern("D9 1C 24 E8 ? ? ? ? D9 5E 0C"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
