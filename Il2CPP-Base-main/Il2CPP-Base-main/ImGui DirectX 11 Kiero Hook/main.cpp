#include "rrid.hpp"
#include "kiero/minhook/include/MinHook.h"
#include "RecRoomPlayer.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <tlhelp32.h>
#include <d3d11.h>
#include <psapi.h>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <cmath>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include "kiero/kiero.h"
#include <winternl.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "ntdll.lib")

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void ConsoleLog(const char* format, ...);

struct Vector3 {
    float x, y, z;
};

typedef HRESULT(__stdcall* Present) (IDXGISwapChain*, UINT, UINT);
Present oPresent = nullptr;

HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

// Anti-Cheat Offsets
UINT64 g_DetectionOffsets[5] = {
    0x1e11e45, 0x2745f7c, 0x2749b69, 0x274c009, 0x7B26FF0
};

const UINT64 FLY_OFFSET = 0x1820C20;
const UINT64 IS_DEVELOPER_OFFSET = 0x1DCDFC0;
const UINT64 PLAYER_INVINCIBLE_OFFSET = 0x211E640;
const UINT64 SET_LOCAL_SCALE_OFFSET = 0x9E664D0;

static constexpr uintptr_t get_MaxWalkSpeed = 0x18215A0;
static constexpr uintptr_t AddMaxWalkSpeed = 0x180D7B0;
static constexpr uintptr_t get_fieldOfView = 0x9D9F590;
static constexpr uintptr_t set_fieldOfView = 0x9DA2A40;
static constexpr uintptr_t get_HasVisibleRecRoomPlus = 0x17CF7B0;
static constexpr uintptr_t set_HasVisibleRecRoomPlus = 0x17D1B90;
static constexpr uintptr_t get_CurrentRecoilMultiplier = 0x1CC6290;
static constexpr uintptr_t IsItemUnlockedLocally = 0x1348880;
static constexpr uintptr_t get_DeveloperDisplayMode = 0x17CF5E0;
static constexpr uintptr_t set_DeveloperDisplayMode = 0x17D19D0;
static constexpr uintptr_t get_AimAssistEnabled = 0x1CC56C0;
static constexpr uintptr_t RpcFireShot = 0x1CBFAA0;
static constexpr uintptr_t SetHeadScale = 0x17C9970;
static constexpr uintptr_t get_IsSendChatOnCooldown = 0x12DC7E0;
static constexpr uintptr_t get_IsOnCooldown = 0x157FD60;
static constexpr uintptr_t get_LocalPlayerExists = 0x17D00F0;
static constexpr uintptr_t RpcPlayLevelUpFeedback = 0x1C0ACB0;
static constexpr uintptr_t CV2GoToRoom = 0x17A6FB0;
static constexpr uintptr_t get_CanUseStreamingCam = 0xDB3B20;
static constexpr uintptr_t CV2UnequipSlots = 0x17AAF20;
static constexpr uintptr_t set_eulerAngles = 0x9E1E5F0;
static constexpr uintptr_t AddMaxJumpHeight = 0x180D530;
static constexpr uintptr_t get_IsSprintOnCooldown = 0x1821080;
static constexpr uintptr_t AddSlideExpectedDuration = 0x180DF20;
static constexpr uintptr_t AddGravityDisabled = 0x180D410;
static constexpr uintptr_t AddSlideStartSpeedBoost = 0x180E100;
static constexpr uintptr_t AddAirControlPercentage = 0x180CDA0;
static constexpr uintptr_t AddWallRunMaxDuration = 0x180F0A0;
static constexpr uintptr_t AddWallRunStartSpeedBoost = 0x180F270;
static constexpr uintptr_t AddWallRunMaxJumpHeight = 0x180F170;
static constexpr uintptr_t AddCrouchSpeedModifier = 0x180D110;
static constexpr uintptr_t AddProneSpeedModifier = 0x180D9C0;
static constexpr uintptr_t AddAutoSprintRestriction = 0x180CF70;
static constexpr uintptr_t AddTeleportMaxHorizontalDistance = 0x180E920;
static constexpr uintptr_t AddTeleportCooldown = 0x180E6B0;
static constexpr uintptr_t AddTeleportCooldownModifier = 0x180E630;
static constexpr uintptr_t AddSelfScaleLocomotionScale = 0x180DB70;
static constexpr uintptr_t RequestAutomaticWallClimbStart = 0x181A920;
static constexpr uintptr_t AddCollisionParentingRestriction = 0x180D050;
static constexpr uintptr_t AddStationaryRestriction = 0x180E580;
static constexpr uintptr_t AddSitRestriction = 0x180DC60;
static constexpr uintptr_t AddShortenRestriction = 0x180DC30;
static constexpr uintptr_t AddCrouchInputRestriction = 0x180D080;
static constexpr uintptr_t AddSlideRestriction = 0x180E0D0;
static constexpr uintptr_t AddSprintInputRestriction = 0x180E400;
static constexpr uintptr_t AddWallClimbRestriction = 0x180EDC0;
static constexpr uintptr_t AddClamberRestriction = 0x180D020;
static constexpr uintptr_t AddJumpRestriction = 0x180D500;
static constexpr uintptr_t AddTeleportRestriction = 0x180EAC0;
static constexpr uintptr_t AddWallRunRestriction = 0x180F240;
static constexpr uintptr_t AddRoomScaleMovementRestriction = 0x180DB40;
static constexpr uintptr_t AddRoomScaleCollisionDisplacementRestriction = 0x180DAE0;
static constexpr uintptr_t AddRoomScaleConfinementEnabled = 0x180DB10;
static constexpr uintptr_t AddSteeringInputRestriction = 0x180E5B0;
static constexpr uintptr_t AddMaxSteeringSpeed = 0x180D600;
static constexpr uintptr_t AddWalkAccelerationDuration = 0x180EAF0;
static constexpr uintptr_t AddSlideAccelerateDownSlopesEnabled = 0x180DD80;
static constexpr uintptr_t AddSlideStartSpeedModifier = 0x180E1D0;
static constexpr uintptr_t AddSlideStopSpeedModifier = 0x180E360;
static constexpr uintptr_t AddSlideMaxSpeedSprintSpeedMultiplier = 0x180E000;
static constexpr uintptr_t AddSlideSteeringParameter = 0x180E270;
static constexpr uintptr_t AddSlideAirControlMultiplier = 0x180DE50;
static constexpr uintptr_t AddJumpCutoffRestriction = 0x180D440;
static constexpr uintptr_t AddJumpInputRestriction = 0x180D470;
static constexpr uintptr_t AddCanGetPushed = 0x180CFA0;
static constexpr uintptr_t SetCanRam = 0x181CAC0;
static constexpr uintptr_t SetPlayerNormalizedHeight = 0x181CB00;

bool g_FlyEnabled = false;
bool g_FlyHookReady = false;
bool g_GodModeEnabled = false;
bool g_GodModeHookReady = false;
bool g_ScaleModified = false;
float g_PlayerScale = 1.0f;
bool g_ScaleHookReady = false;
bool g_FOVModified = false;
float g_PlayerFOV = 60.0f;
bool g_FOVHookReady = false;
uintptr_t g_CameraInstance = 0;
bool g_WalkSpeedModified = false;
float g_WalkSpeed = 5.0f;
bool g_WalkSpeedHookReady = false;
bool g_RecRoomPlusEnabled = false;
bool g_RecRoomPlusHookReady = false;
bool g_RecoilModified = false;
float g_RecoilMultiplier = 0.0f;
bool g_RecoilHookReady = false;
bool g_UnlockItemsEnabled = false;
bool g_UnlockItemsHookReady = false;
bool g_AimAssistEnabled = false;
bool g_AimAssistHookReady = false;
bool g_BlockShots = false;
bool g_BlockShotsHookReady = false;
bool g_HeadScaleModified = false;
float g_HeadScale = 1.0f;
bool g_HeadScaleHookReady = false;
bool g_NoChatCooldown = false;
bool g_NoChatCooldownHookReady = false;
bool g_RapidFire = false;
bool g_RapidFireHookReady = false;
bool g_FreezeSelf = false;
bool g_FreezeSelfHookReady = false;
bool g_BlockLevelUp = false;
bool g_BlockLevelUpHookReady = false;
bool g_BlockRoomTeleport = false;
bool g_BlockRoomTeleportHookReady = false;
bool g_StreamerCamEnabled = false;
bool g_StreamerCamHookReady = false;
bool g_BlockUnequipSlots = false;
bool g_BlockUnequipSlotsHookReady = false;
bool g_SpinbotEnabled = false;
bool g_SpinbotHookReady = false;
bool g_SuperJumpEnabled = false;
float g_JumpHeight = 10.0f;
bool g_JumpHookReady = false;
bool g_InfiniteSprintEnabled = false;
bool g_InfiniteSprintHookReady = false;
bool g_ExtendedSlideEnabled = false;
float g_SlideDuration = 5.0f;
bool g_SlideHookReady = false;
bool g_NoGravityEnabled = false;
bool g_NoGravityHookReady = false;
bool g_SlideSpeedBoostEnabled = false;
float g_SlideSpeedBoost = 2.0f;
bool g_SlideSpeedHookReady = false;
bool g_AirControlEnabled = false;
float g_AirControlAmount = 1.0f;
bool g_AirControlHookReady = false;
bool g_WallRunDurationEnabled = false;
float g_WallRunDuration = 5.0f;
bool g_WallRunDurationHookReady = false;
bool g_WallRunSpeedEnabled = false;
float g_WallRunSpeed = 2.0f;
bool g_WallRunSpeedHookReady = false;
bool g_WallRunJumpEnabled = false;
float g_WallRunJumpHeight = 10.0f;
bool g_WallRunJumpHookReady = false;
bool g_CrouchSpeedEnabled = false;
float g_CrouchSpeed = 2.0f;
bool g_CrouchSpeedHookReady = false;
bool g_ProneSpeedEnabled = false;
float g_ProneSpeed = 2.0f;
bool g_ProneSpeedHookReady = false;
bool g_AutoSprintEnabled = false;
bool g_AutoSprintHookReady = false;
bool g_TeleportDistanceEnabled = false;
float g_TeleportDistance = 100.0f;
bool g_TeleportDistanceHookReady = false;
bool g_NoTeleportCooldownEnabled = false;
bool g_NoTeleportCooldownHookReady = false;
bool g_TeleportCooldownReductionEnabled = false;
float g_TeleportCooldownReduction = 0.5f;
bool g_TeleportCooldownReductionHookReady = false;
bool g_FlightSpeedEnabled = false;
float g_FlightSpeed = 2.0f;
bool g_FlightSpeedHookReady = false;
bool g_AutoWallClimbEnabled = false;
bool g_AutoWallClimbHookReady = false;
bool g_NoCollisionParenting = false;
bool g_NoCollisionParentingHookReady = false;
bool g_StationaryEnabled = false;
bool g_StationaryHookReady = false;
bool g_NoSitEnabled = false;
bool g_NoSitHookReady = false;
bool g_NoShortenEnabled = false;
bool g_NoShortenHookReady = false;
bool g_NoCrouchInput = false;
bool g_NoCrouchInputHookReady = false;
bool g_NoSlideEnabled = false;
bool g_NoSlideHookReady = false;
bool g_NoSprintInput = false;
bool g_NoSprintInputHookReady = false;
bool g_NoWallClimb = false;
bool g_NoWallClimbHookReady = false;
bool g_NoClamber = false;
bool g_NoClamberHookReady = false;
bool g_NoJump = false;
bool g_NoJumpHookReady = false;
bool g_NoTeleport = false;
bool g_NoTeleportHookReady = false;
bool g_NoWallRun = false;
bool g_NoWallRunHookReady = false;
bool g_NoRoomScaleMovement = false;
bool g_NoRoomScaleMovementHookReady = false;

std::string g_PlayerName = "Unknown";
std::string g_AccountId = "Unknown";
int g_PlayerId = -1;
int g_ActorNumber = -1;
bool g_IsLocal = false;
bool g_PlayerInfoReady = false;

void* g_CachedTransform = nullptr;
bool g_BypassActive = false;
bool g_ShowMenu = true;
bool g_ConsoleCreated = false;
bool g_ShowPlayerListWindow = false;
std::ofstream g_LogFile;
std::atomic<bool> g_KillSwitch{ false };
std::vector<HANDLE> g_RefereeThreads;
std::vector<uint8_t> g_OriginalBytes[4];
std::vector<uint8_t> g_PatchBytes[5];

typedef bool(__fastcall* IsFlyingEnabled_t)();
typedef bool(__fastcall* IsDeveloper_t)();
typedef bool(__fastcall* PlayerIsInvincible_t)();
typedef void(__fastcall* SetLocalScale_t)(void* __this, void* value, void* method);
typedef float(__fastcall* get_fieldOfView_t)(void* __this, void* method);
typedef void(__fastcall* set_fieldOfView_t)(void* __this, float value, void* method);
typedef float(__fastcall* get_MaxWalkSpeed_t)(void* __this, void* method);
typedef void(__fastcall* AddMaxWalkSpeed_t)(void* __this, float value, void* method);
typedef bool(__fastcall* get_HasVisibleRecRoomPlus_t)(void* __this, void* method);
typedef void(__fastcall* set_HasVisibleRecRoomPlus_t)(void* __this, bool value, void* method);
typedef float(__fastcall* get_CurrentRecoilMultiplier_t)(void* __this, void* method);
typedef bool(__fastcall* IsItemUnlockedLocally_t)(void* __this, void* method);
typedef bool(__fastcall* get_DeveloperDisplayMode_t)(void* __this, void* method);
typedef void(__fastcall* set_DeveloperDisplayMode_t)(void* __this, bool value, void* method);
typedef bool(__fastcall* get_AimAssistEnabled_t)(void* __this, void* method);
typedef void(__fastcall* RpcFireShot_t)(void* __this, void* method);
typedef void(__fastcall* SetHeadScale_t)(void* __this, float scale, void* method);
typedef bool(__fastcall* get_IsSendChatOnCooldown_t)(void* __this, void* method);
typedef bool(__fastcall* get_IsOnCooldown_t)(void* __this, void* method);
typedef bool(__fastcall* get_LocalPlayerExists_t)(void* __this, void* method);
typedef void(__fastcall* RpcPlayLevelUpFeedback_t)(void* __this, void* method);
typedef void(__fastcall* CV2GoToRoom_t)(void* __this, void* method);
typedef bool(__fastcall* get_CanUseStreamingCam_t)(void* __this, void* method);
typedef void(__fastcall* CV2UnequipSlots_t)(void* __this, void* method);
typedef void(__fastcall* set_eulerAngles_t)(void* __this, Vector3 value, void* method);
typedef void(__fastcall* AddMaxJumpHeight_t)(void* __this, float height, void* method);
typedef bool(__fastcall* get_IsSprintOnCooldown_t)(void* __this, void* method);
typedef void(__fastcall* AddSlideExpectedDuration_t)(void* __this, float duration, void* method);
typedef void(__fastcall* AddGravityDisabled_t)(void* __this, void* method);
typedef void(__fastcall* AddSlideStartSpeedBoost_t)(void* __this, float boost, void* method);
typedef void(__fastcall* AddAirControlPercentage_t)(void* __this, float percentage, void* method);
typedef void(__fastcall* AddWallRunMaxDuration_t)(void* __this, float duration, void* method);
typedef void(__fastcall* AddWallRunStartSpeedBoost_t)(void* __this, float boost, void* method);
typedef void(__fastcall* AddWallRunMaxJumpHeight_t)(void* __this, float height, void* method);
typedef void(__fastcall* AddCrouchSpeedModifier_t)(void* __this, float modifier, void* method);
typedef void(__fastcall* AddProneSpeedModifier_t)(void* __this, float modifier, void* method);
typedef void(__fastcall* AddAutoSprintRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddTeleportMaxHorizontalDistance_t)(void* __this, float distance, void* method);
typedef void(__fastcall* AddTeleportCooldown_t)(void* __this, void* method);
typedef void(__fastcall* AddTeleportCooldownModifier_t)(void* __this, float modifier, void* method);
typedef void(__fastcall* AddSelfScaleLocomotionScale_t)(void* __this, float scale, void* method);
typedef void(__fastcall* RequestAutomaticWallClimbStart_t)(void* __this, void* method);
typedef void(__fastcall* AddCollisionParentingRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddStationaryRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddSitRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddShortenRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddCrouchInputRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddSlideRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddSprintInputRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddWallClimbRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddClamberRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddJumpRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddTeleportRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddWallRunRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddRoomScaleMovementRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddRoomScaleCollisionDisplacementRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddRoomScaleConfinementEnabled_t)(void* __this, void* method);
typedef void(__fastcall* AddSteeringInputRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddMaxSteeringSpeed_t)(void* __this, float speed, void* method);
typedef void(__fastcall* AddWalkAccelerationDuration_t)(void* __this, float duration, void* method);
typedef void(__fastcall* AddSlideAccelerateDownSlopesEnabled_t)(void* __this, void* method);
typedef void(__fastcall* AddSlideStartSpeedModifier_t)(void* __this, float modifier, void* method);
typedef void(__fastcall* AddSlideStopSpeedModifier_t)(void* __this, float modifier, void* method);
typedef void(__fastcall* AddSlideMaxSpeedSprintSpeedMultiplier_t)(void* __this, float multiplier, void* method);
typedef void(__fastcall* AddSlideSteeringParameter_t)(void* __this, float parameter, void* method);
typedef void(__fastcall* AddSlideAirControlMultiplier_t)(void* __this, float multiplier, void* method);
typedef void(__fastcall* AddJumpCutoffRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddJumpInputRestriction_t)(void* __this, void* method);
typedef void(__fastcall* AddCanGetPushed_t)(void* __this, void* method);
typedef void(__fastcall* SetCanRam_t)(void* __this, bool value, void* method);
typedef void(__fastcall* SetPlayerNormalizedHeight_t)(void* __this, float height, void* method);

get_CanUseStreamingCam_t oGetCanUseStreamingCam = nullptr;
CV2UnequipSlots_t oCV2UnequipSlots = nullptr;
set_eulerAngles_t oSetEulerAngles = nullptr;
IsFlyingEnabled_t oIsFlyingEnabled = nullptr;
IsDeveloper_t oIsDeveloper = nullptr;
PlayerIsInvincible_t oPlayerIsInvincible = nullptr;
SetLocalScale_t oSetLocalScale = nullptr;
get_fieldOfView_t oGetFieldOfView = nullptr;
set_fieldOfView_t oSetFieldOfView = nullptr;
get_MaxWalkSpeed_t oGetMaxWalkSpeed = nullptr;
AddMaxWalkSpeed_t oAddMaxWalkSpeed = nullptr;
get_HasVisibleRecRoomPlus_t oGetHasVisibleRecRoomPlus = nullptr;
set_HasVisibleRecRoomPlus_t oSetHasVisibleRecRoomPlus = nullptr;
get_CurrentRecoilMultiplier_t oGetCurrentRecoilMultiplier = nullptr;
IsItemUnlockedLocally_t oIsItemUnlockedLocally = nullptr;
get_DeveloperDisplayMode_t oGetDeveloperDisplayMode = nullptr;
set_DeveloperDisplayMode_t oSetDeveloperDisplayMode = nullptr;
get_AimAssistEnabled_t oGetAimAssistEnabled = nullptr;
RpcFireShot_t oRpcFireShot = nullptr;
SetHeadScale_t oSetHeadScale = nullptr;
get_IsSendChatOnCooldown_t oGetIsSendChatOnCooldown = nullptr;
get_IsOnCooldown_t oGetIsOnCooldown = nullptr;
get_LocalPlayerExists_t oGetLocalPlayerExists = nullptr;
RpcPlayLevelUpFeedback_t oRpcPlayLevelUpFeedback = nullptr;
CV2GoToRoom_t oCV2GoToRoom = nullptr;
AddMaxJumpHeight_t oAddMaxJumpHeight = nullptr;
get_IsSprintOnCooldown_t oGetIsSprintOnCooldown = nullptr;
AddSlideExpectedDuration_t oAddSlideExpectedDuration = nullptr;
AddGravityDisabled_t oAddGravityDisabled = nullptr;
AddSlideStartSpeedBoost_t oAddSlideStartSpeedBoost = nullptr;
AddAirControlPercentage_t oAddAirControlPercentage = nullptr;
AddWallRunMaxDuration_t oAddWallRunMaxDuration = nullptr;
AddWallRunStartSpeedBoost_t oAddWallRunStartSpeedBoost = nullptr;
AddWallRunMaxJumpHeight_t oAddWallRunMaxJumpHeight = nullptr;
AddCrouchSpeedModifier_t oAddCrouchSpeedModifier = nullptr;
AddProneSpeedModifier_t oAddProneSpeedModifier = nullptr;
AddAutoSprintRestriction_t oAddAutoSprintRestriction = nullptr;
AddTeleportMaxHorizontalDistance_t oAddTeleportMaxHorizontalDistance = nullptr;
AddTeleportCooldown_t oAddTeleportCooldown = nullptr;
AddTeleportCooldownModifier_t oAddTeleportCooldownModifier = nullptr;
AddSelfScaleLocomotionScale_t oAddSelfScaleLocomotionScale = nullptr;
RequestAutomaticWallClimbStart_t oRequestAutomaticWallClimbStart = nullptr;
AddCollisionParentingRestriction_t oAddCollisionParentingRestriction = nullptr;
AddStationaryRestriction_t oAddStationaryRestriction = nullptr;
AddSitRestriction_t oAddSitRestriction = nullptr;
AddShortenRestriction_t oAddShortenRestriction = nullptr;
AddCrouchInputRestriction_t oAddCrouchInputRestriction = nullptr;
AddSlideRestriction_t oAddSlideRestriction = nullptr;
AddSprintInputRestriction_t oAddSprintInputRestriction = nullptr;
AddWallClimbRestriction_t oAddWallClimbRestriction = nullptr;
AddClamberRestriction_t oAddClamberRestriction = nullptr;
AddJumpRestriction_t oAddJumpRestriction = nullptr;
AddTeleportRestriction_t oAddTeleportRestriction = nullptr;
AddWallRunRestriction_t oAddWallRunRestriction = nullptr;
AddRoomScaleMovementRestriction_t oAddRoomScaleMovementRestriction = nullptr;

bool __fastcall hkGetCanUseStreamingCam(void* __this, void* method) {
    if (g_StreamerCamEnabled) return true;
    return oGetCanUseStreamingCam ? oGetCanUseStreamingCam(__this, method) : false;
}

void __fastcall hkCV2UnequipSlots(void* __this, void* method) {
    if (g_BlockUnequipSlots) return;
    if (oCV2UnequipSlots) oCV2UnequipSlots(__this, method);
}

void __fastcall hkSetEulerAngles(void* __this, Vector3 value, void* method) {
    if (g_SpinbotEnabled) {
        static float yaw = 0.0f;
        yaw += 1.0f;
        if (yaw > 360.0f) yaw -= 360.0f;
        Vector3 spinAngles = { 0.0f, yaw, 0.0f };
        if (oSetEulerAngles) oSetEulerAngles(__this, spinAngles, method);
        return;
    }
    if (oSetEulerAngles) oSetEulerAngles(__this, value, method);
}

bool __fastcall hkIsFlyingEnabled() {
    if (g_FlyEnabled) return true;
    return oIsFlyingEnabled ? oIsFlyingEnabled() : false;
}

bool __fastcall hkIsDeveloper() {
    return true;
}

bool __fastcall hkPlayerIsInvincible() {
    if (g_GodModeEnabled) return true;
    return oPlayerIsInvincible ? oPlayerIsInvincible() : false;
}

void __fastcall hkSetLocalScale(void* __this, void* value, void* method) {
    g_CachedTransform = __this;
    if (g_ScaleModified && value) {
        float newVec[3] = { g_PlayerScale, g_PlayerScale, g_PlayerScale };
        if (oSetLocalScale) oSetLocalScale(__this, newVec, method);
        return;
    }
    if (oSetLocalScale) oSetLocalScale(__this, value, method);
}

float __fastcall hkGetFieldOfView(void* __this, void* method) {
    if (g_FOVModified && g_CameraInstance == (uintptr_t)__this) return g_PlayerFOV;
    return oGetFieldOfView ? oGetFieldOfView(__this, method) : 60.0f;
}

void __fastcall hkSetFieldOfView(void* __this, float value, void* method) {
    if (g_CameraInstance == 0) {
        g_CameraInstance = (uintptr_t)__this;
        ConsoleLog("[+] Camera instance cached at: 0x%llX", g_CameraInstance);
    }
    if (g_FOVModified && g_CameraInstance == (uintptr_t)__this) {
        if (oSetFieldOfView) oSetFieldOfView(__this, g_PlayerFOV, method);
        return;
    }
    if (oSetFieldOfView) oSetFieldOfView(__this, value, method);
}

float __fastcall hkGetMaxWalkSpeed(void* __this, void* method) {
    if (g_WalkSpeedModified) return g_WalkSpeed;
    return oGetMaxWalkSpeed ? oGetMaxWalkSpeed(__this, method) : 5.0f;
}

void __fastcall hkAddMaxWalkSpeed(void* __this, float value, void* method) {
    if (g_WalkSpeedModified) {
        if (oAddMaxWalkSpeed) oAddMaxWalkSpeed(__this, g_WalkSpeed, method);
        return;
    }
    if (oAddMaxWalkSpeed) oAddMaxWalkSpeed(__this, value, method);
}

bool __fastcall hkGetHasVisibleRecRoomPlus(void* __this, void* method) {
    if (g_RecRoomPlusEnabled) return true;
    return oGetHasVisibleRecRoomPlus ? oGetHasVisibleRecRoomPlus(__this, method) : false;
}

void __fastcall hkSetHasVisibleRecRoomPlus(void* __this, bool value, void* method) {
    if (g_RecRoomPlusEnabled) {
        if (oSetHasVisibleRecRoomPlus) oSetHasVisibleRecRoomPlus(__this, true, method);
        return;
    }
    if (oSetHasVisibleRecRoomPlus) oSetHasVisibleRecRoomPlus(__this, value, method);
}

float __fastcall hkGetCurrentRecoilMultiplier(void* __this, void* method) {
    if (g_RecoilModified) return g_RecoilMultiplier;
    return oGetCurrentRecoilMultiplier ? oGetCurrentRecoilMultiplier(__this, method) : 1.0f;
}

bool __fastcall hkIsItemUnlockedLocally(void* __this, void* method) {
    if (g_UnlockItemsEnabled) return true;
    return oIsItemUnlockedLocally ? oIsItemUnlockedLocally(__this, method) : false;
}

bool __fastcall hkGetDeveloperDisplayMode(void* __this, void* method) {
    return true;
}

void __fastcall hkSetDeveloperDisplayMode(void* __this, bool value, void* method) {
    if (oSetDeveloperDisplayMode) oSetDeveloperDisplayMode(__this, true, method);
}

bool __fastcall hkGetAimAssistEnabled(void* __this, void* method) {
    if (g_AimAssistEnabled) return true;
    return oGetAimAssistEnabled ? oGetAimAssistEnabled(__this, method) : false;
}

void __fastcall hkRpcFireShot(void* __this, void* method) {
    if (g_BlockShots) return;
    if (oRpcFireShot) oRpcFireShot(__this, method);
}

void __fastcall hkSetHeadScale(void* __this, float scale, void* method) {
    if (g_HeadScaleModified) {
        if (oSetHeadScale) oSetHeadScale(__this, g_HeadScale, method);
        return;
    }
    if (oSetHeadScale) oSetHeadScale(__this, scale, method);
}

bool __fastcall hkGetIsSendChatOnCooldown(void* __this, void* method) {
    if (g_NoChatCooldown) return false;
    return oGetIsSendChatOnCooldown ? oGetIsSendChatOnCooldown(__this, method) : false;
}

bool __fastcall hkGetIsOnCooldown(void* __this, void* method) {
    if (g_RapidFire) return false;
    return oGetIsOnCooldown ? oGetIsOnCooldown(__this, method) : false;
}

bool __fastcall hkGetLocalPlayerExists(void* __this, void* method) {
    if (g_FreezeSelf) return false;
    return oGetLocalPlayerExists ? oGetLocalPlayerExists(__this, method) : false;
}

void __fastcall hkRpcPlayLevelUpFeedback(void* __this, void* method) {
    if (g_BlockLevelUp) return;
    if (oRpcPlayLevelUpFeedback) oRpcPlayLevelUpFeedback(__this, method);
}

void __fastcall hkCV2GoToRoom(void* __this, void* method) {
    if (g_BlockRoomTeleport) return;
    if (oCV2GoToRoom) oCV2GoToRoom(__this, method);
}

void __fastcall hkAddMaxJumpHeight(void* __this, float height, void* method) {
    if (g_SuperJumpEnabled) {
        if (oAddMaxJumpHeight) oAddMaxJumpHeight(__this, g_JumpHeight, method);
        return;
    }
    if (oAddMaxJumpHeight) oAddMaxJumpHeight(__this, height, method);
}

bool __fastcall hkGetIsSprintOnCooldown(void* __this, void* method) {
    if (g_InfiniteSprintEnabled) return false;
    return oGetIsSprintOnCooldown ? oGetIsSprintOnCooldown(__this, method) : false;
}

void __fastcall hkAddSlideExpectedDuration(void* __this, float duration, void* method) {
    if (g_ExtendedSlideEnabled) {
        if (oAddSlideExpectedDuration) oAddSlideExpectedDuration(__this, g_SlideDuration, method);
        return;
    }
    if (oAddSlideExpectedDuration) oAddSlideExpectedDuration(__this, duration, method);
}

void __fastcall hkAddGravityDisabled(void* __this, void* method) {
    if (g_NoGravityEnabled) {
        if (oAddGravityDisabled) oAddGravityDisabled(__this, method);
        return;
    }
}

void __fastcall hkAddSlideStartSpeedBoost(void* __this, float boost, void* method) {
    if (g_SlideSpeedBoostEnabled) {
        if (oAddSlideStartSpeedBoost) oAddSlideStartSpeedBoost(__this, g_SlideSpeedBoost, method);
        return;
    }
    if (oAddSlideStartSpeedBoost) oAddSlideStartSpeedBoost(__this, boost, method);
}

void __fastcall hkAddAirControlPercentage(void* __this, float percentage, void* method) {
    if (g_AirControlEnabled) {
        if (oAddAirControlPercentage) oAddAirControlPercentage(__this, g_AirControlAmount, method);
        return;
    }
    if (oAddAirControlPercentage) oAddAirControlPercentage(__this, percentage, method);
}

void __fastcall hkAddWallRunMaxDuration(void* __this, float duration, void* method) {
    if (g_WallRunDurationEnabled) {
        if (oAddWallRunMaxDuration) oAddWallRunMaxDuration(__this, g_WallRunDuration, method);
        return;
    }
    if (oAddWallRunMaxDuration) oAddWallRunMaxDuration(__this, duration, method);
}

void __fastcall hkAddWallRunStartSpeedBoost(void* __this, float boost, void* method) {
    if (g_WallRunSpeedEnabled) {
        if (oAddWallRunStartSpeedBoost) oAddWallRunStartSpeedBoost(__this, g_WallRunSpeed, method);
        return;
    }
    if (oAddWallRunStartSpeedBoost) oAddWallRunStartSpeedBoost(__this, boost, method);
}

void __fastcall hkAddWallRunMaxJumpHeight(void* __this, float height, void* method) {
    if (g_WallRunJumpEnabled) {
        if (oAddWallRunMaxJumpHeight) oAddWallRunMaxJumpHeight(__this, g_WallRunJumpHeight, method);
        return;
    }
    if (oAddWallRunMaxJumpHeight) oAddWallRunMaxJumpHeight(__this, height, method);
}

void __fastcall hkAddCrouchSpeedModifier(void* __this, float modifier, void* method) {
    if (g_CrouchSpeedEnabled) {
        if (oAddCrouchSpeedModifier) oAddCrouchSpeedModifier(__this, g_CrouchSpeed, method);
        return;
    }
    if (oAddCrouchSpeedModifier) oAddCrouchSpeedModifier(__this, modifier, method);
}

void __fastcall hkAddProneSpeedModifier(void* __this, float modifier, void* method) {
    if (g_ProneSpeedEnabled) {
        if (oAddProneSpeedModifier) oAddProneSpeedModifier(__this, g_ProneSpeed, method);
        return;
    }
    if (oAddProneSpeedModifier) oAddProneSpeedModifier(__this, modifier, method);
}

void __fastcall hkAddAutoSprintRestriction(void* __this, void* method) {
    if (g_AutoSprintEnabled) {
        if (oAddAutoSprintRestriction) oAddAutoSprintRestriction(__this, method);
    }
}

void __fastcall hkAddTeleportMaxHorizontalDistance(void* __this, float distance, void* method) {
    if (g_TeleportDistanceEnabled) {
        if (oAddTeleportMaxHorizontalDistance) oAddTeleportMaxHorizontalDistance(__this, g_TeleportDistance, method);
        return;
    }
    if (oAddTeleportMaxHorizontalDistance) oAddTeleportMaxHorizontalDistance(__this, distance, method);
}

void __fastcall hkAddTeleportCooldown(void* __this, void* method) {
    if (g_NoTeleportCooldownEnabled) {
        if (oAddTeleportCooldown) oAddTeleportCooldown(__this, method);
    }
}

void __fastcall hkAddTeleportCooldownModifier(void* __this, float modifier, void* method) {
    if (g_TeleportCooldownReductionEnabled) {
        if (oAddTeleportCooldownModifier) oAddTeleportCooldownModifier(__this, g_TeleportCooldownReduction, method);
        return;
    }
    if (oAddTeleportCooldownModifier) oAddTeleportCooldownModifier(__this, modifier, method);
}

void __fastcall hkAddSelfScaleLocomotionScale(void* __this, float scale, void* method) {
    if (g_FlightSpeedEnabled) {
        if (oAddSelfScaleLocomotionScale) oAddSelfScaleLocomotionScale(__this, g_FlightSpeed, method);
        return;
    }
    if (oAddSelfScaleLocomotionScale) oAddSelfScaleLocomotionScale(__this, scale, method);
}

void __fastcall hkRequestAutomaticWallClimbStart(void* __this, void* method) {
    if (g_AutoWallClimbEnabled) {
        if (oRequestAutomaticWallClimbStart) oRequestAutomaticWallClimbStart(__this, method);
    }
}

void __fastcall hkAddCollisionParentingRestriction(void* __this, void* method) {
    if (g_NoCollisionParenting) {
        if (oAddCollisionParentingRestriction) oAddCollisionParentingRestriction(__this, method);
    }
}

void __fastcall hkAddStationaryRestriction(void* __this, void* method) {
    if (g_StationaryEnabled) {
        if (oAddStationaryRestriction) oAddStationaryRestriction(__this, method);
    }
}

void __fastcall hkAddSitRestriction(void* __this, void* method) {
    if (g_NoSitEnabled) {
        if (oAddSitRestriction) oAddSitRestriction(__this, method);
    }
}

void __fastcall hkAddShortenRestriction(void* __this, void* method) {
    if (g_NoShortenEnabled) {
        if (oAddShortenRestriction) oAddShortenRestriction(__this, method);
    }
}

void __fastcall hkAddCrouchInputRestriction(void* __this, void* method) {
    if (g_NoCrouchInput) {
        if (oAddCrouchInputRestriction) oAddCrouchInputRestriction(__this, method);
    }
}

void __fastcall hkAddSlideRestriction(void* __this, void* method) {
    if (g_NoSlideEnabled) {
        if (oAddSlideRestriction) oAddSlideRestriction(__this, method);
    }
}

void __fastcall hkAddSprintInputRestriction(void* __this, void* method) {
    if (g_NoSprintInput) {
        if (oAddSprintInputRestriction) oAddSprintInputRestriction(__this, method);
    }
}

void __fastcall hkAddWallClimbRestriction(void* __this, void* method) {
    if (g_NoWallClimb) {
        if (oAddWallClimbRestriction) oAddWallClimbRestriction(__this, method);
    }
}

void __fastcall hkAddClamberRestriction(void* __this, void* method) {
    if (g_NoClamber) {
        if (oAddClamberRestriction) oAddClamberRestriction(__this, method);
    }
}

void __fastcall hkAddJumpRestriction(void* __this, void* method) {
    if (g_NoJump) {
        if (oAddJumpRestriction) oAddJumpRestriction(__this, method);
    }
}

void __fastcall hkAddTeleportRestriction(void* __this, void* method) {
    if (g_NoTeleport) {
        if (oAddTeleportRestriction) oAddTeleportRestriction(__this, method);
    }
}

void __fastcall hkAddWallRunRestriction(void* __this, void* method) {
    if (g_NoWallRun) {
        if (oAddWallRunRestriction) oAddWallRunRestriction(__this, method);
    }
}

void __fastcall hkAddRoomScaleMovementRestriction(void* __this, void* method) {
    if (g_NoRoomScaleMovement) {
        if (oAddRoomScaleMovementRestriction) oAddRoomScaleMovementRestriction(__this, method);
    }
}

std::string GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm timeInfo;
    localtime_s(&timeInfo, &time);
    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "[%H:%M:%S]", &timeInfo);
    return std::string(timeStr);
}

void ConsoleLog(const char* format, ...) {
    if (!g_ConsoleCreated) return;
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    std::string logMsg = GetCurrentTimeString() + " " + buffer;
    std::cout << logMsg << std::endl;
    if (g_LogFile.is_open()) {
        g_LogFile << logMsg << std::endl;
        g_LogFile.flush();
    }
}

void CreateConsole() {
    if (g_ConsoleCreated) return;
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    SetConsoleTitleA("Project Encryptic");
    g_LogFile.open("referee_killer.log", std::ios::out | std::ios::app);
    g_ConsoleCreated = true;
    ConsoleLog("================================================");
    ConsoleLog("Project Encryptic Initialized");
    ConsoleLog("================================================");
}

bool WaitForModule(const char* moduleName, DWORD timeoutMs = 30000) {
    auto start = GetTickCount64();
    while (GetTickCount64() - start < timeoutMs) {
        HMODULE hMod = GetModuleHandleA(moduleName);
        if (hMod) {
            __try {
                PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hMod;
                if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
                    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hMod + dosHeader->e_lfanew);
                    if (ntHeaders->Signature == IMAGE_NT_SIGNATURE) return true;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
        Sleep(100);
    }
    return false;
}

void EnumerateRefereeThreads() {
    g_RefereeThreads.clear();
    DWORD processId = GetCurrentProcessId();
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;

    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);
    if (Thread32First(hSnapshot, &te)) {
        do {
            if (te.th32OwnerProcessID == processId) {
                HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME | THREAD_TERMINATE, FALSE, te.th32ThreadID);
                if (hThread) {
                    HMODULE hReferee = GetModuleHandleA("Referee.dll");
                    if (hReferee) {
                        PVOID startAddr = nullptr;
                        if (NT_SUCCESS(NtQueryInformationThread(hThread, (THREADINFOCLASS)0x9, &startAddr, sizeof(startAddr), nullptr))) {
                            MODULEINFO modInfo = { 0 };
                            if (GetModuleInformation(GetCurrentProcess(), hReferee, &modInfo, sizeof(modInfo))) {
                                if (startAddr >= modInfo.lpBaseOfDll && startAddr < (PVOID)((UINT64)modInfo.lpBaseOfDll + modInfo.SizeOfImage)) {
                                    g_RefereeThreads.push_back(hThread);
                                    continue;
                                }
                            }
                        }
                    }
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hSnapshot, &te));
    }
    CloseHandle(hSnapshot);
}

void KillReferee() {
    EnumerateRefereeThreads();
    for (auto hThread : g_RefereeThreads) SuspendThread(hThread);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        if (Process32First(snapshot, &pe)) {
            do {
                if (_stricmp(pe.szExeFile, "RefereeClientApp.exe") == 0) {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (hProcess) {
                        TerminateProcess(hProcess, 0);
                        CloseHandle(hProcess);
                    }
                    break;
                }
            } while (Process32Next(snapshot, &pe));
        }
        CloseHandle(snapshot);
    }

    for (auto hThread : g_RefereeThreads) {
        TerminateThread(hThread, 0);
        CloseHandle(hThread);
    }
    g_RefereeThreads.clear();
}

bool SafeRead(void* addr, void* buf, size_t size) {
    __try { memcpy(buf, addr, size); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

bool SafeWrite(void* addr, void* buf, size_t size) {
    __try { memcpy(addr, buf, size); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

bool PatchMemory(uintptr_t addr, std::vector<uint8_t>& patch, std::vector<uint8_t>& original, int idx) {
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi))) return false;
    if (mbi.State != MEM_COMMIT) return false;

    original.resize(patch.size());
    if (!SafeRead((void*)addr, original.data(), patch.size())) return false;

    DWORD oldProtect;
    if (!VirtualProtect((LPVOID)addr, patch.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) return false;

    bool success = SafeWrite((void*)addr, patch.data(), patch.size());

    DWORD temp;
    VirtualProtect((LPVOID)addr, patch.size(), oldProtect, &temp);
    return success;
}

bool ApplyBypass() {
    uintptr_t base = (uintptr_t)GetModuleHandleA("GameAssembly.dll");
    if (!base) return false;

    g_PatchBytes[0] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
    g_PatchBytes[1] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
    g_PatchBytes[2] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
    g_PatchBytes[3] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
    g_PatchBytes[4] = { 0xB0, 0x01, 0xC3, 0x90, 0x90 };

    int success = 0;
    for (int i = 0; i < 4; i++) {
        if (PatchMemory(base + g_DetectionOffsets[i], g_PatchBytes[i], g_OriginalBytes[i], i))
            success++;
    }

    g_BypassActive = (success > 0);
    ConsoleLog("[*] Bypass applied, success count: %d", success);
    return g_BypassActive;
}

bool InitFeatureHooks() {
    uintptr_t base = (uintptr_t)GetModuleHandleA("GameAssembly.dll");
    if (!base) {
        ConsoleLog("[-] Failed to get GameAssembly base!");
        return false;
    }

    ConsoleLog("[*] Initializing hooks at base: 0x%llX", base);

    struct HookResult { std::string name; bool success; };
    std::vector<HookResult> results;

#define TRY_HOOK(offset, hookFunc, originalPtr, name, readyFlag) \
    do { \
        uintptr_t addr = base + offset; \
        if (MH_CreateHook((LPVOID)addr, hookFunc, (LPVOID*)&originalPtr) == MH_OK) { \
            if (MH_EnableHook((LPVOID)addr) == MH_OK) { \
                results.push_back({name, true}); \
                readyFlag = true; \
            } else { \
                results.push_back({name, false}); \
                ConsoleLog("[-] Failed to enable %s hook", name); \
                readyFlag = false; \
            } \
        } else { \
            results.push_back({name, false}); \
            ConsoleLog("[-] Failed to create %s hook at 0x%llX", name, offset); \
            readyFlag = false; \
        } \
    } while(0)

    TRY_HOOK(FLY_OFFSET, hkIsFlyingEnabled, oIsFlyingEnabled, "Fly", g_FlyHookReady);
    TRY_HOOK(IS_DEVELOPER_OFFSET, hkIsDeveloper, oIsDeveloper, "Developer", g_FlyHookReady);
    TRY_HOOK(get_DeveloperDisplayMode, hkGetDeveloperDisplayMode, oGetDeveloperDisplayMode, "Developer Display Get", g_FlyHookReady);
    TRY_HOOK(set_DeveloperDisplayMode, hkSetDeveloperDisplayMode, oSetDeveloperDisplayMode, "Developer Display Set", g_FlyHookReady);
    TRY_HOOK(PLAYER_INVINCIBLE_OFFSET, hkPlayerIsInvincible, oPlayerIsInvincible, "God Mode", g_GodModeHookReady);
    TRY_HOOK(SET_LOCAL_SCALE_OFFSET, hkSetLocalScale, oSetLocalScale, "Scale", g_ScaleHookReady);
    TRY_HOOK(get_MaxWalkSpeed, hkGetMaxWalkSpeed, oGetMaxWalkSpeed, "Get Walk Speed", g_WalkSpeedHookReady);
    TRY_HOOK(AddMaxWalkSpeed, hkAddMaxWalkSpeed, oAddMaxWalkSpeed, "Add Walk Speed", g_WalkSpeedHookReady);
    TRY_HOOK(get_fieldOfView, hkGetFieldOfView, oGetFieldOfView, "Get FOV", g_FOVHookReady);
    TRY_HOOK(set_fieldOfView, hkSetFieldOfView, oSetFieldOfView, "Set FOV", g_FOVHookReady);
    TRY_HOOK(get_HasVisibleRecRoomPlus, hkGetHasVisibleRecRoomPlus, oGetHasVisibleRecRoomPlus, "RecRoomPlus Get", g_RecRoomPlusHookReady);
    TRY_HOOK(set_HasVisibleRecRoomPlus, hkSetHasVisibleRecRoomPlus, oSetHasVisibleRecRoomPlus, "RecRoomPlus Set", g_RecRoomPlusHookReady);
    TRY_HOOK(get_CurrentRecoilMultiplier, hkGetCurrentRecoilMultiplier, oGetCurrentRecoilMultiplier, "Recoil", g_RecoilHookReady);
    TRY_HOOK(IsItemUnlockedLocally, hkIsItemUnlockedLocally, oIsItemUnlockedLocally, "Unlock Items", g_UnlockItemsHookReady);
    TRY_HOOK(get_AimAssistEnabled, hkGetAimAssistEnabled, oGetAimAssistEnabled, "Aim Assist", g_AimAssistHookReady);
    TRY_HOOK(RpcFireShot, hkRpcFireShot, oRpcFireShot, "Block Shots", g_BlockShotsHookReady);
    TRY_HOOK(SetHeadScale, hkSetHeadScale, oSetHeadScale, "Head Scale", g_HeadScaleHookReady);
    TRY_HOOK(get_IsSendChatOnCooldown, hkGetIsSendChatOnCooldown, oGetIsSendChatOnCooldown, "No Chat Cooldown", g_NoChatCooldownHookReady);
    TRY_HOOK(get_IsOnCooldown, hkGetIsOnCooldown, oGetIsOnCooldown, "Rapid Fire", g_RapidFireHookReady);
    TRY_HOOK(get_LocalPlayerExists, hkGetLocalPlayerExists, oGetLocalPlayerExists, "Freeze Self", g_FreezeSelfHookReady);
    TRY_HOOK(RpcPlayLevelUpFeedback, hkRpcPlayLevelUpFeedback, oRpcPlayLevelUpFeedback, "Block Level Up", g_BlockLevelUpHookReady);
    TRY_HOOK(CV2GoToRoom, hkCV2GoToRoom, oCV2GoToRoom, "Block Room Teleport", g_BlockRoomTeleportHookReady);
    TRY_HOOK(get_CanUseStreamingCam, hkGetCanUseStreamingCam, oGetCanUseStreamingCam, "Streamer Cam", g_StreamerCamHookReady);
    TRY_HOOK(CV2UnequipSlots, hkCV2UnequipSlots, oCV2UnequipSlots, "Block Unequip Slots", g_BlockUnequipSlotsHookReady);
    TRY_HOOK(set_eulerAngles, hkSetEulerAngles, oSetEulerAngles, "Spinbot", g_SpinbotHookReady);
    TRY_HOOK(AddMaxJumpHeight, hkAddMaxJumpHeight, oAddMaxJumpHeight, "Super Jump", g_JumpHookReady);
    TRY_HOOK(get_IsSprintOnCooldown, hkGetIsSprintOnCooldown, oGetIsSprintOnCooldown, "Infinite Sprint", g_InfiniteSprintHookReady);
    TRY_HOOK(AddSlideExpectedDuration, hkAddSlideExpectedDuration, oAddSlideExpectedDuration, "Extended Slide", g_SlideHookReady);
    TRY_HOOK(AddGravityDisabled, hkAddGravityDisabled, oAddGravityDisabled, "No Gravity", g_NoGravityHookReady);
    TRY_HOOK(AddSlideStartSpeedBoost, hkAddSlideStartSpeedBoost, oAddSlideStartSpeedBoost, "Slide Speed Boost", g_SlideSpeedHookReady);
    TRY_HOOK(AddAirControlPercentage, hkAddAirControlPercentage, oAddAirControlPercentage, "Air Control", g_AirControlHookReady);
    TRY_HOOK(AddWallRunMaxDuration, hkAddWallRunMaxDuration, oAddWallRunMaxDuration, "Wall Run Duration", g_WallRunDurationHookReady);
    TRY_HOOK(AddWallRunStartSpeedBoost, hkAddWallRunStartSpeedBoost, oAddWallRunStartSpeedBoost, "Wall Run Speed", g_WallRunSpeedHookReady);
    TRY_HOOK(AddWallRunMaxJumpHeight, hkAddWallRunMaxJumpHeight, oAddWallRunMaxJumpHeight, "Wall Run Jump", g_WallRunJumpHookReady);
    TRY_HOOK(AddCrouchSpeedModifier, hkAddCrouchSpeedModifier, oAddCrouchSpeedModifier, "Crouch Speed", g_CrouchSpeedHookReady);
    TRY_HOOK(AddProneSpeedModifier, hkAddProneSpeedModifier, oAddProneSpeedModifier, "Prone Speed", g_ProneSpeedHookReady);
    TRY_HOOK(AddAutoSprintRestriction, hkAddAutoSprintRestriction, oAddAutoSprintRestriction, "Auto Sprint", g_AutoSprintHookReady);
    TRY_HOOK(AddTeleportMaxHorizontalDistance, hkAddTeleportMaxHorizontalDistance, oAddTeleportMaxHorizontalDistance, "Teleport Distance", g_TeleportDistanceHookReady);
    TRY_HOOK(AddTeleportCooldown, hkAddTeleportCooldown, oAddTeleportCooldown, "No Teleport Cooldown", g_NoTeleportCooldownHookReady);
    TRY_HOOK(AddTeleportCooldownModifier, hkAddTeleportCooldownModifier, oAddTeleportCooldownModifier, "Teleport Cooldown Reduction", g_TeleportCooldownReductionHookReady);
    TRY_HOOK(AddSelfScaleLocomotionScale, hkAddSelfScaleLocomotionScale, oAddSelfScaleLocomotionScale, "Flight Speed", g_FlightSpeedHookReady);
    TRY_HOOK(RequestAutomaticWallClimbStart, hkRequestAutomaticWallClimbStart, oRequestAutomaticWallClimbStart, "Auto Wall Climb", g_AutoWallClimbHookReady);
    TRY_HOOK(AddCollisionParentingRestriction, hkAddCollisionParentingRestriction, oAddCollisionParentingRestriction, "No Collision Parenting", g_NoCollisionParentingHookReady);
    TRY_HOOK(AddStationaryRestriction, hkAddStationaryRestriction, oAddStationaryRestriction, "Stationary", g_StationaryHookReady);
    TRY_HOOK(AddSitRestriction, hkAddSitRestriction, oAddSitRestriction, "No Sit", g_NoSitHookReady);
    TRY_HOOK(AddShortenRestriction, hkAddShortenRestriction, oAddShortenRestriction, "No Shorten", g_NoShortenHookReady);
    TRY_HOOK(AddCrouchInputRestriction, hkAddCrouchInputRestriction, oAddCrouchInputRestriction, "No Crouch Input", g_NoCrouchInputHookReady);
    TRY_HOOK(AddSlideRestriction, hkAddSlideRestriction, oAddSlideRestriction, "No Slide", g_NoSlideHookReady);
    TRY_HOOK(AddSprintInputRestriction, hkAddSprintInputRestriction, oAddSprintInputRestriction, "No Sprint Input", g_NoSprintInputHookReady);
    TRY_HOOK(AddWallClimbRestriction, hkAddWallClimbRestriction, oAddWallClimbRestriction, "No Wall Climb", g_NoWallClimbHookReady);
    TRY_HOOK(AddClamberRestriction, hkAddClamberRestriction, oAddClamberRestriction, "No Clamber", g_NoClamberHookReady);
    TRY_HOOK(AddJumpRestriction, hkAddJumpRestriction, oAddJumpRestriction, "No Jump", g_NoJumpHookReady);
    TRY_HOOK(AddTeleportRestriction, hkAddTeleportRestriction, oAddTeleportRestriction, "No Teleport", g_NoTeleportHookReady);
    TRY_HOOK(AddWallRunRestriction, hkAddWallRunRestriction, oAddWallRunRestriction, "No Wall Run", g_NoWallRunHookReady);
    TRY_HOOK(AddRoomScaleMovementRestriction, hkAddRoomScaleMovementRestriction, oAddRoomScaleMovementRestriction, "No Room Scale Movement", g_NoRoomScaleMovementHookReady);

#undef TRY_HOOK

    ConsoleLog("\n========================================");
    ConsoleLog("HOOK INITIALIZATION SUMMARY");
    ConsoleLog("========================================");
    int successCount = 0, failCount = 0;
    for (const auto& r : results) {
        if (r.success) {
            successCount++;
            ConsoleLog("[+] %s: SUCCESS", r.name.c_str());
        }
        else {
            failCount++;
            ConsoleLog("[-] %s: FAILED", r.name.c_str());
        }
    }
    ConsoleLog("========================================");
    ConsoleLog("Total: %d successful, %d failed", successCount, failCount);
    ConsoleLog("========================================\n");

    return successCount > 0;
}

void BypassThread() {
    ConsoleLog("[*] Starting bypass thread...");

    if (!WaitForModule("GameAssembly.dll", 60000)) {
        ConsoleLog("[-] GameAssembly.dll timeout");
        return;
    }
    Sleep(500);

    WaitForModule("Referee.dll", 10000);
    KillReferee();
    Sleep(200);

    for (int i = 0; i < 3 && !g_BypassActive; i++) {
        ApplyBypass();
        Sleep(500);
    }

    if (!g_BypassActive) {
        ConsoleLog("[-] Failed to apply bypass");
        return;
    }

    ConsoleLog("[+] Bypass applied successfully!");

    Sleep(8000);

    if (InitFeatureHooks()) {
        ConsoleLog("[+] Feature hooks initialized!");
    }

    Sleep(3000);
    if (rrid::init()) {
        ConsoleLog("[+] RRID initialized successfully after bypass");
    }
    else {
        ConsoleLog("[-] RRID init failed!");
    }

    while (!g_KillSwitch) {
        Sleep(1000);

        uintptr_t base = (uintptr_t)GetModuleHandleA("GameAssembly.dll");
        if (base) {
            bool intact = true;
            for (int i = 0; i < 4; i++) {
                if (memcmp((void*)(base + g_DetectionOffsets[i]), g_PatchBytes[i].data(), g_PatchBytes[i].size()) != 0) {
                    intact = false;
                    break;
                }
            }
            if (!intact) ApplyBypass();
        }
    }
}

void InitImGui() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(8, 6);
    style.ItemSpacing = ImVec2(10, 8);
    style.ItemInnerSpacing = ImVec2(8, 4);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.98f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.12f, 0.80f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.28f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.33f, 0.40f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.38f, 0.45f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.30f, 0.36f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.17f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.80f, 0.40f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.30f, 0.32f, 0.38f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.42f, 0.48f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.35f, 0.40f, 1.00f);

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;
    return CallWindowProcA(oWndProc, hWnd, uMsg, wParam, lParam);
}

extern "C" {
    static bool SafeReadIl2CppStringRaw(void* strObj, wchar_t** outChars, int* outLen) {
        if (!outChars || !outLen) return false;
        __try {
            if (!strObj) return false;
            int len = *(int*)((char*)strObj + 0x10);
            *outLen = len;
            *outChars = (wchar_t*)((char*)strObj + 0x14);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static bool SafeCall_Player_GetName(void* instance, void** outStr) {
        if (!outStr) return false;
        __try {
            *outStr = nullptr;
            HMODULE h = GetModuleHandleA("GameAssembly.dll");
            if (!h) return false;
            uintptr_t base = (uintptr_t)h;
            auto fn = (void*(__fastcall*)(void*))(base + 0x845BA0);
            *outStr = fn(instance);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            *outStr = nullptr;
            return false;
        }
    }

    static bool SafeCall_Player_GetId(void* instance, int* outId) {
        if (!outId) return false;
        __try {
            *outId = -1;
            HMODULE h = GetModuleHandleA("GameAssembly.dll");
            if (!h) return false;
            uintptr_t base = (uintptr_t)h;
            auto fn = (int(__fastcall*)(void*))(base + 0x17D0D40);
            *outId = fn(instance);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            *outId = -1;
            return false;
        }
    }

    static bool SafeCall_Player_IsLocal(void* instance, bool* outBool) {
        if (!outBool) return false;
        __try {
            *outBool = false;
            HMODULE h = GetModuleHandleA("GameAssembly.dll");
            if (!h) return false;
            uintptr_t base = (uintptr_t)h;
            auto fn = (bool(__fastcall*)(void*))(base + 0x17CF800);
            *outBool = fn(instance);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            *outBool = false;
            return false;
        }
    }
}

static std::string Il2CppStringToString(void* strObj) {
    if (!strObj) return std::string("<null>");
    wchar_t* wchars = nullptr;
    int len = 0;
    if (!SafeReadIl2CppStringRaw(strObj, &wchars, &len) || !wchars || len <= 0) return std::string();
    std::wstring ws(wchars, len);
    int needed = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
    if (needed <= 0) return std::string();
    std::string out; out.resize(needed);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &out[0], needed, NULL, NULL);
    return out;
}

static std::string GetPlayerNameFromInstance(void* playerInstance) {
    if (!playerInstance) return std::string("<null>");
    void* str = nullptr;
    if (!SafeCall_Player_GetName(playerInstance, &str) || !str) return std::string("<null>");
    return Il2CppStringToString(str);
}

static int GetPlayerIdFromInstance(void* playerInstance) {
    if (!playerInstance) return -1;
    int id = -1;
    if (!SafeCall_Player_GetId(playerInstance, &id)) return -1;
    return id;
}

static bool IsPlayerLocalInstance(void* playerInstance) {
    if (!playerInstance) return false;
    bool res = false;
    if (!SafeCall_Player_IsLocal(playerInstance, &res)) return false;
    return res;
}

static bool IsRVAValid(uintptr_t rva) {
    HMODULE h = GetModuleHandleA("GameAssembly.dll");
    if (!h) return false;
    uintptr_t base = (uintptr_t)h;
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
    uintptr_t size = nt->OptionalHeader.SizeOfImage;
    uintptr_t addr = base + rva;
    return (addr >= base && addr < base + size);
}
static bool SafeCall_Player_GetCurrentBodyPosition(void* instance, Vector3* outPos) {
    if (!outPos) return false;
    __try {
        HMODULE h = GetModuleHandleA("GameAssembly.dll");
        if (!h) return false;
        uintptr_t base = (uintptr_t)h;
        auto fn = (Vector3(__fastcall*)(void*))(base + 0x17CEB70); // Player::get_CurrentBodyPosition
        *outPos = fn(instance);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static bool SafeCall_Camera_get_main(void** outCamera) {
    if (!outCamera) return false;
    __try {
        *outCamera = nullptr;
        HMODULE h = GetModuleHandleA("GameAssembly.dll");
        if (!h) return false;
        uintptr_t base = (uintptr_t)h;
        auto fn = (void*(__fastcall*)())(base + 0x9D9FBB0);
        *outCamera = fn();
        return *outCamera != nullptr;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *outCamera = nullptr;
        return false;
    }
}

static bool SafeCall_Camera_WorldToScreenPoint(void* cameraInstance, Vector3 worldPos, Vector3* outScreen) {
    if (!outScreen) return false;
    __try {
        HMODULE h = GetModuleHandleA("GameAssembly.dll");
        if (!h) return false;
        uintptr_t base = (uintptr_t)h;
        auto fn = (Vector3(__fastcall*)(void*, Vector3))(base + 0x9D9DE70);
        *outScreen = fn(cameraInstance, worldPos);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool init = false;
bool g_TracersEnabled = true;

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!init) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice))) {
            pDevice->GetImmediateContext(&pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            window = sd.OutputWindow;
            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
            InitImGui();
            init = true;
        } else {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static std::vector<void*> cachedPlayers;
    static uint64_t lastRefreshTick = 0;
    static bool autoRefreshEnabled = true;
    static uint32_t refreshIntervalMs = 5000;

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(780, 920), ImGuiCond_FirstUseEver);
        ImGui::Begin("Project Encryptic", &g_ShowMenu,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.90f, 0.50f, 1.00f));
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("PROJECT ENCRYPTIC").x) * 0.5f);
        ImGui::Text("PROJECT ENCRYPTIC");
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, g_BypassActive ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0.5f, 0, 1));
        ImGui::Text("STATUS: %s", g_BypassActive ? "ACTIVE" : "INITIALIZING");
        ImGui::PopStyleColor();
        ImGui::Separator();

        if (ImGui::Button("Player List")) {
            g_ShowPlayerListWindow = !g_ShowPlayerListWindow;
        }
        ImGui::SameLine();
        ImGui::TextColored(g_ShowPlayerListWindow ? ImVec4(0, 1, 0, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1), g_ShowPlayerListWindow ? "OPEN" : "CLOSED");

        if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_None)) {

            if (ImGui::BeginTabItem("SELF INFO")) {
                ImGui::Spacing();
                ImGui::Text("PLAYER INFORMATION");
                ImGui::Separator();

                if (g_PlayerInfoReady) {
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: Connected");
                    ImGui::Text("Name: %s", g_PlayerName.c_str());
                    ImGui::Text("Account ID: %s", g_AccountId.c_str());
                    ImGui::Text("Player ID: %d", g_PlayerId);
                    ImGui::Text("Actor Number: %d", g_ActorNumber);
                }
                else {
                    ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Status: Loading...");
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("COMBAT")) {
                ImGui::Spacing();
                ImGui::Text("MOVEMENT");
                ImGui::Separator();

                if (g_FlyHookReady) ImGui::Checkbox("Fly Mode", &g_FlyEnabled);
                else ImGui::TextDisabled("Fly Mode (Unavailable)");

                if (g_GodModeHookReady) ImGui::Checkbox("God Mode", &g_GodModeEnabled);
                else ImGui::TextDisabled("God Mode (Unavailable)");

                if (g_WalkSpeedHookReady) {
                    ImGui::Checkbox("Walk Speed Modified", &g_WalkSpeedModified);
                    if (g_WalkSpeedModified) ImGui::SliderFloat("Speed", &g_WalkSpeed, 1.0f, 50.0f, "%.1fx");
                }
                else ImGui::TextDisabled("Walk Speed (Unavailable)");

                if (g_JumpHookReady) {
                    ImGui::Checkbox("Super Jump", &g_SuperJumpEnabled);
                    if (g_SuperJumpEnabled) ImGui::SliderFloat("Jump Height", &g_JumpHeight, 5.0f, 50.0f, "%.1f units");
                }
                else ImGui::TextDisabled("Super Jump (Unavailable)");

                if (g_InfiniteSprintHookReady) {
                    ImGui::Checkbox("Infinite Sprint", &g_InfiniteSprintEnabled);
                    ImGui::SameLine(150);
                    ImGui::TextColored(g_InfiniteSprintEnabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), g_InfiniteSprintEnabled ? "NO COOLDOWN" : "OFF");
                }
                else ImGui::TextDisabled("Infinite Sprint (Unavailable)");

                if (g_SlideHookReady) {
                    ImGui::Checkbox("Extended Slide", &g_ExtendedSlideEnabled);
                    if (g_ExtendedSlideEnabled) ImGui::SliderFloat("Slide Duration", &g_SlideDuration, 1.0f, 15.0f, "%.1f seconds");
                }
                else ImGui::TextDisabled("Extended Slide (Unavailable)");

                if (g_NoGravityHookReady) {
                    ImGui::Checkbox("No Gravity", &g_NoGravityEnabled);
                    ImGui::SameLine(150);
                    ImGui::TextColored(g_NoGravityEnabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), g_NoGravityEnabled ? "ZERO G" : "OFF");
                }
                else ImGui::TextDisabled("No Gravity (Unavailable)");

                if (g_SlideSpeedHookReady) {
                    ImGui::Checkbox("Slide Speed Boost", &g_SlideSpeedBoostEnabled);
                    if (g_SlideSpeedBoostEnabled) ImGui::SliderFloat("Boost", &g_SlideSpeedBoost, 1.0f, 5.0f, "%.1fx");
                }
                else ImGui::TextDisabled("Slide Speed Boost (Unavailable)");

                if (g_AirControlHookReady) {
                    ImGui::Checkbox("Air Control", &g_AirControlEnabled);
                    if (g_AirControlEnabled) ImGui::SliderFloat("Control", &g_AirControlAmount, 0.0f, 1.0f, "%.0f%%");
                }
                else ImGui::TextDisabled("Air Control (Unavailable)");

                if (g_WallRunDurationHookReady) {
                    ImGui::Checkbox("Wall Run Duration", &g_WallRunDurationEnabled);
                    if (g_WallRunDurationEnabled) ImGui::SliderFloat("Duration", &g_WallRunDuration, 1.0f, 30.0f, "%.1f sec");
                }
                else ImGui::TextDisabled("Wall Run Duration (Unavailable)");

                if (g_WallRunSpeedHookReady) {
                    ImGui::Checkbox("Wall Run Speed", &g_WallRunSpeedEnabled);
                    if (g_WallRunSpeedEnabled) ImGui::SliderFloat("Speed", &g_WallRunSpeed, 1.0f, 5.0f, "%.1fx");
                }
                else ImGui::TextDisabled("Wall Run Speed (Unavailable)");

                if (g_WallRunJumpHookReady) {
                    ImGui::Checkbox("Wall Run Jump", &g_WallRunJumpEnabled);
                    if (g_WallRunJumpEnabled) ImGui::SliderFloat("Jump Height", &g_WallRunJumpHeight, 5.0f, 30.0f, "%.1f units");
                }
                else ImGui::TextDisabled("Wall Run Jump (Unavailable)");

                if (g_CrouchSpeedHookReady) {
                    ImGui::Checkbox("Crouch Speed", &g_CrouchSpeedEnabled);
                    if (g_CrouchSpeedEnabled) ImGui::SliderFloat("Crouch Speed", &g_CrouchSpeed, 0.5f, 5.0f, "%.1fx");
                }
                else ImGui::TextDisabled("Crouch Speed (Unavailable)");

                if (g_ProneSpeedHookReady) {
                    ImGui::Checkbox("Prone Speed", &g_ProneSpeedEnabled);
                    if (g_ProneSpeedEnabled) ImGui::SliderFloat("Prone Speed", &g_ProneSpeed, 0.5f, 5.0f, "%.1fx");
                }
                else ImGui::TextDisabled("Prone Speed (Unavailable)");

                if (g_AutoSprintHookReady) {
                    ImGui::Checkbox("Auto Sprint", &g_AutoSprintEnabled);
                    ImGui::SameLine(150);
                    ImGui::TextColored(g_AutoSprintEnabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), g_AutoSprintEnabled ? "ON" : "OFF");
                }
                else ImGui::TextDisabled("Auto Sprint (Unavailable)");

                if (g_SpinbotHookReady) {
                    ImGui::Checkbox("Spinbot", &g_SpinbotEnabled);
                    ImGui::SameLine(150);
                    ImGui::TextColored(g_SpinbotEnabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), g_SpinbotEnabled ? "ON" : "OFF");
                }
                else ImGui::TextDisabled("Spinbot (Unavailable)");

                ImGui::Spacing();
                ImGui::Text("WEAPONS");
                ImGui::Separator();

                if (g_RecoilHookReady) ImGui::Checkbox("No Recoil", &g_RecoilModified);
                else ImGui::TextDisabled("No Recoil (Unavailable)");

                if (g_RapidFireHookReady) ImGui::Checkbox("Rapid Fire", &g_RapidFire);
                else ImGui::TextDisabled("Rapid Fire (Unavailable)");

                if (g_AimAssistHookReady) ImGui::Checkbox("Aim Assist", &g_AimAssistEnabled);
                else ImGui::TextDisabled("Aim Assist (Unavailable)");

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("TELEPORT")) {
                ImGui::Spacing();

                if (g_TeleportDistanceHookReady) {
                    ImGui::Checkbox("Teleport Distance", &g_TeleportDistanceEnabled);
                    if (g_TeleportDistanceEnabled) ImGui::SliderFloat("Max Distance", &g_TeleportDistance, 10.0f, 500.0f, "%.0f units");
                }
                else ImGui::TextDisabled("Teleport Distance (Unavailable)");

                if (g_NoTeleportCooldownHookReady) {
                    ImGui::Checkbox("No Teleport Cooldown", &g_NoTeleportCooldownEnabled);
                    ImGui::SameLine(150);
                    ImGui::TextColored(g_NoTeleportCooldownEnabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), g_NoTeleportCooldownEnabled ? "ACTIVE" : "OFF");
                }
                else ImGui::TextDisabled("No Teleport Cooldown (Unavailable)");

                if (g_TeleportCooldownReductionHookReady) {
                    ImGui::Checkbox("Teleport Cooldown Reduction", &g_TeleportCooldownReductionEnabled);
                    if (g_TeleportCooldownReductionEnabled) ImGui::SliderFloat("Reduction", &g_TeleportCooldownReduction, 0.0f, 1.0f, "%.0f%%");
                }
                else ImGui::TextDisabled("Teleport Cooldown Reduction (Unavailable)");

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("FLIGHT")) {
                ImGui::Spacing();

                if (g_FlyHookReady) ImGui::Checkbox("Fly Mode", &g_FlyEnabled);
                else ImGui::TextDisabled("Fly Mode (Unavailable)");

                if (g_FlightSpeedHookReady) {
                    ImGui::Checkbox("Flight Speed", &g_FlightSpeedEnabled);
                    if (g_FlightSpeedEnabled) ImGui::SliderFloat("Speed Multiplier", &g_FlightSpeed, 0.5f, 10.0f, "%.1fx");
                }
                else ImGui::TextDisabled("Flight Speed (Unavailable)");

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("CLIMB")) {
                ImGui::Spacing();

                if (g_AutoWallClimbHookReady) {
                    ImGui::Checkbox("Auto Wall Climb", &g_AutoWallClimbEnabled);
                    ImGui::SameLine(150);
                    ImGui::TextColored(g_AutoWallClimbEnabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), g_AutoWallClimbEnabled ? "ACTIVE" : "OFF");
                }
                else ImGui::TextDisabled("Auto Wall Climb (Unavailable)");

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("RESTRICTIONS")) {
                ImGui::Spacing();

                if (g_NoCollisionParentingHookReady) ImGui::Checkbox("No Collision Parenting", &g_NoCollisionParenting);
                if (g_StationaryHookReady) ImGui::Checkbox("Stationary", &g_StationaryEnabled);
                if (g_NoSitHookReady) ImGui::Checkbox("No Sit", &g_NoSitEnabled);
                if (g_NoShortenHookReady) ImGui::Checkbox("No Shorten", &g_NoShortenEnabled);
                if (g_NoCrouchInputHookReady) ImGui::Checkbox("No Crouch Input", &g_NoCrouchInput);
                if (g_NoSlideHookReady) ImGui::Checkbox("No Slide", &g_NoSlideEnabled);
                if (g_NoSprintInputHookReady) ImGui::Checkbox("No Sprint Input", &g_NoSprintInput);
                if (g_NoWallClimbHookReady) ImGui::Checkbox("No Wall Climb", &g_NoWallClimb);
                if (g_NoClamberHookReady) ImGui::Checkbox("No Clamber", &g_NoClamber);
                if (g_NoJumpHookReady) ImGui::Checkbox("No Jump", &g_NoJump);
                if (g_NoTeleportHookReady) ImGui::Checkbox("No Teleport", &g_NoTeleport);
                if (g_NoWallRunHookReady) ImGui::Checkbox("No Wall Run", &g_NoWallRun);
                if (g_NoRoomScaleMovementHookReady) ImGui::Checkbox("No Room Scale Movement", &g_NoRoomScaleMovement);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("ANTI-CRASH")) {
                ImGui::Spacing();
                ImGui::Text("PROTECTION");
                ImGui::Separator();

                if (g_BlockShotsHookReady) ImGui::Checkbox("Block Shots", &g_BlockShots);
                else ImGui::TextDisabled("Block Shots (Unavailable)");

                if (g_BlockLevelUpHookReady) ImGui::Checkbox("Block Level Up", &g_BlockLevelUp);
                else ImGui::TextDisabled("Block Level Up (Unavailable)");

                if (g_BlockRoomTeleportHookReady) ImGui::Checkbox("Block Room Teleport", &g_BlockRoomTeleport);
                else ImGui::TextDisabled("Block Room Teleport (Unavailable)");

                if (g_FreezeSelfHookReady) ImGui::Checkbox("Freeze Self", &g_FreezeSelf);
                else ImGui::TextDisabled("Freeze Self (Unavailable)");

                if (g_BlockUnequipSlotsHookReady) {
                    ImGui::Checkbox("Block Unequip Slots (Anti-Crash)", &g_BlockUnequipSlots);
                    ImGui::SameLine(220);
                    ImGui::TextColored(g_BlockUnequipSlots ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), g_BlockUnequipSlots ? "ACTIVE" : "INACTIVE");
                }
                else ImGui::TextDisabled("Block Unequip Slots (Unavailable)");

                ImGui::Spacing();
                ImGui::Text("CHAT");
                ImGui::Separator();

                if (g_NoChatCooldownHookReady) ImGui::Checkbox("No Chat Cooldown", &g_NoChatCooldown);
                else ImGui::TextDisabled("No Chat Cooldown (Unavailable)");

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("VISUAL")) {
                ImGui::Spacing();

                if (g_FOVHookReady) {
                    ImGui::Checkbox("FOV Modified", &g_FOVModified);
                    if (g_FOVModified) ImGui::SliderFloat("FOV", &g_PlayerFOV, 1.0f, 1000.0f, "%.0f");
                }
                else ImGui::TextDisabled("FOV (Unavailable)");

                if (g_ScaleHookReady) {
                    ImGui::Checkbox("Scale Modified", &g_ScaleModified);
                    if (g_ScaleModified) ImGui::SliderFloat("Scale", &g_PlayerScale, 0.1f, 10.0f, "%.1fx");
                }
                else ImGui::TextDisabled("Scale (Unavailable)");

                if (g_HeadScaleHookReady) {
                    ImGui::Checkbox("Head Scale Modified", &g_HeadScaleModified);
                    if (g_HeadScaleModified) ImGui::SliderFloat("Head Scale", &g_HeadScale, 0.1f, 5.0f, "%.1fx");
                }
                else ImGui::TextDisabled("Head Scale (Unavailable)");

                ImGui::Checkbox("Tracers (bottom->player)", &g_TracersEnabled);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("UTILITY")) {
                ImGui::Spacing();

                if (g_StreamerCamHookReady) {
                    ImGui::Checkbox("Streamer Camera", &g_StreamerCamEnabled);
                    ImGui::SameLine(150);
                    ImGui::TextColored(g_StreamerCamEnabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), g_StreamerCamEnabled ? "ON" : "OFF");
                }
                else ImGui::TextDisabled("Streamer Camera (Unavailable)");

                if (g_RecRoomPlusHookReady) {
                    ImGui::Checkbox("RecRoom Plus", &g_RecRoomPlusEnabled);
                    ImGui::SameLine(150);
                    ImGui::TextColored(g_RecRoomPlusEnabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), g_RecRoomPlusEnabled ? "ON" : "OFF");
                }
                else ImGui::TextDisabled("RecRoom Plus (Unavailable)");

                if (g_UnlockItemsHookReady) {
                    ImGui::Checkbox("Unlock All Items", &g_UnlockItemsEnabled);
                    ImGui::SameLine(150);
                    ImGui::TextColored(g_UnlockItemsEnabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1), g_UnlockItemsEnabled ? "ON" : "OFF");
                }
                else ImGui::TextDisabled("Unlock Items (Unavailable)");

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();
        ImGui::TextDisabled("INSERT - Toggle Menu | END - Unload");
        ImGui::End();
    }

    if (g_ShowPlayerListWindow) {
        ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
        ImGui::Begin("Player List", &g_ShowPlayerListWindow, ImGuiWindowFlags_NoCollapse);

        if (!IsRVAValid(0x17A0E00) || !IsRVAValid(0x17C1150) || !IsRVAValid(0x17C1000)) {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Player functions not mapped (JIT/Delayed). Try again after game loaded.");
            ImGui::End();
        }
        else {
            static bool prev_open = false;
            static bool need_log = true;
            if (!prev_open && g_ShowPlayerListWindow) need_log = true;
            prev_open = g_ShowPlayerListWindow;

            ImGui::BeginGroup();
            if (ImGui::Button("Refresh")) {
                need_log = true;
                lastRefreshTick = 0;
            }
            ImGui::SameLine();
            if (need_log) ImGui::TextColored(ImVec4(0, 1, 0, 1), "Last: Pending");
            else ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "Last: Idle");
            ImGui::EndGroup();

            HMODULE hGA = GetModuleHandleA("GameAssembly.dll");
            uintptr_t base = (uintptr_t)hGA;
            ImGui::Text("GameAssembly base: 0x%llX", (unsigned long long)base);
            ImGui::Text("OFF_GetAllPlayers mapped: %s", IsRVAValid(0x17A0E00) ? "YES" : "NO");
            ImGui::Text("Player::get_Name mapped: %s", IsRVAValid(0x17C1150) ? "YES" : "NO");
            ImGui::Text("Player::get_Id mapped: %s", IsRVAValid(0x17C1000) ? "YES" : "NO");

            void* local = RecRoom::GetLocalPlayer();
            ImGui::Text("Local player ptr: %p", local);
            if (local) {
                std::string lname = GetPlayerNameFromInstance(local);
                int lid = GetPlayerIdFromInstance(local);
                ImGui::Text("Local player name: %s (id: %d)", lname.c_str(), lid);
            }

            if (need_log) {
                ConsoleLog("[PlayerList] Refresh requested");
                ConsoleLog("[PlayerList] GameAssembly base: 0x%llX", (unsigned long long)base);
                ConsoleLog("[PlayerList] OFF_GetAllPlayers mapped: %s", IsRVAValid(0x17A0E00) ? "YES" : "NO");
                ConsoleLog("[PlayerList] Player::get_Name mapped: %s", IsRVAValid(0x17C1150) ? "YES" : "NO");
                ConsoleLog("[PlayerList] Player::get_Id mapped: %s", IsRVAValid(0x17C1000) ? "YES" : "NO");
                ConsoleLog("[PlayerList] Local player ptr: %p", local);
            }

            uint64_t now = GetTickCount64();
            bool shouldRefresh = false;
            if (autoRefreshEnabled) {
                if (lastRefreshTick == 0 || now - lastRefreshTick >= refreshIntervalMs) shouldRefresh = true;
            }

            if (shouldRefresh) {
                cachedPlayers.clear();
                void* rawArr = RecRoom::CallFunctionRVA(RecRoom::OFF_GetAllPlayers);
                ConsoleLog("[PlayerList] Raw GetAllPlayers returned: %p", rawArr);
                if (rawArr) {
                    int len = 0;
                    if (SafeRead((char*)rawArr + 0x10, &len, sizeof(len)) && len > 0) {
                        ConsoleLog("[PlayerList] Array length read at +0x10 = %d", len);
                        uintptr_t itemsBase = (uintptr_t)rawArr + 0x20;
                        for (int i = 0; i < len; ++i) {
                            uintptr_t elem = 0;
                            if (SafeRead((void*)(itemsBase + (uintptr_t)i * sizeof(uintptr_t)), &elem, sizeof(elem)) && elem) {
                                cachedPlayers.push_back((void*)elem);
                            }
                        }
                    }
                    else {
                        ConsoleLog("[PlayerList] Failed to read array length at +0x10 or length == 0");
                    }
                }
                else {
                    ConsoleLog("[PlayerList] GetAllPlayers returned NULL");
                }
                lastRefreshTick = now;
            }

            ImGui::Text("Players found (cached): %zu", cachedPlayers.size());
            ImGui::Separator();
            ImGui::BeginChild("player_list_child", ImVec2(0, 0), false);

            if (cachedPlayers.empty()) {
                void* localPtr = RecRoom::GetLocalPlayer();
                if (localPtr) {
                    std::string name = GetPlayerNameFromInstance(localPtr);
                    int id = GetPlayerIdFromInstance(localPtr);
                    ImGui::Text("ID: %d    Name: %s    (Local - fallback)", id, name.c_str());
                    ConsoleLog("[PlayerList] Fallback: showed local player ptr=%p id=%d name=%s", localPtr, id, name.c_str());
                }
            }

            for (size_t i = 0; i < cachedPlayers.size(); ++i) {
                void* p = cachedPlayers[i];
                void* rawNamePtr = nullptr;
                SafeCall_Player_GetName(p, &rawNamePtr);
                std::string name = Il2CppStringToString(rawNamePtr);
                int id = GetPlayerIdFromInstance(p);
                bool isLocal = IsPlayerLocalInstance(p);
                ImGui::Text("ID: %d    Name: %s    %s", id, name.c_str(), isLocal ? "(Local)" : "");

                if (need_log) {
                    ConsoleLog("[PlayerList] Player[%zu] instance=%p rawNamePtr=%p id=%d name=%s local=%d", i, p, rawNamePtr, id, name.c_str(), isLocal ? 1 : 0);
                }
            }

            ImGui::EndChild();
            ImGui::End();
            if (need_log) {
                need_log = false;
                ConsoleLog("[PlayerList] Total players cached: %zu", cachedPlayers.size());
            }
        }
    }

    ImGui::Render();
    if (g_TracersEnabled) {
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        ImGuiIO& io = ImGui::GetIO();
        float cx = io.DisplaySize.x * 0.5f;
        float by = io.DisplaySize.y - 5.0f;

        void* camera = nullptr;
        if (SafeCall_Camera_get_main(&camera) && camera) {
            if (cachedPlayers.empty()) {
                void* rawArr = RecRoom::CallFunctionRVA(RecRoom::OFF_GetAllPlayers);
                if (rawArr) {
                    int len = 0;
                    if (SafeRead((char*)rawArr + 0x10, &len, sizeof(len)) && len > 0) {
                        uintptr_t itemsBase = (uintptr_t)rawArr + 0x20;
                        for (int i = 0; i < len; ++i) {
                            uintptr_t elem = 0;
                            if (SafeRead((void*)(itemsBase + (uintptr_t)i * sizeof(uintptr_t)), &elem, sizeof(elem)) && elem) {
                                cachedPlayers.push_back((void*)elem);
                            }
                        }
                    }
                }
            }

            for (size_t i = 0; i < cachedPlayers.size(); ++i) {
                void* p = cachedPlayers[i];
                if (!p) continue;
                Vector3 worldPos;
                if (!SafeCall_Player_GetCurrentBodyPosition(p, &worldPos)) continue;
                Vector3 screenPos;
                if (!SafeCall_Camera_WorldToScreenPoint(camera, worldPos, &screenPos)) continue;
                if (screenPos.z <= 0.0f) continue;
                ImVec2 pt(screenPos.x, io.DisplaySize.y - screenPos.y);
                bool isLocal = IsPlayerLocalInstance(p);
                ImU32 col = isLocal ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 255, 255, 200);
                draw->AddLine(ImVec2(cx, by), pt, col, 2.0f);
                draw->AddCircleFilled(pt, 4.0f, col, 12);
            }
        }
    }
    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (GetAsyncKeyState(VK_INSERT) & 1) {
        g_ShowMenu = !g_ShowMenu;
        ImGui::GetIO().MouseDrawCursor = g_ShowMenu;
    }

    if (GetAsyncKeyState(VK_END) & 1) {
        g_KillSwitch = true;
        g_ShowMenu = false;
        g_FlyEnabled = false;
        g_GodModeEnabled = false;
        g_ScaleModified = false;
        g_PlayerScale = 1.0f;
        g_FOVModified = false;
        g_PlayerFOV = 60.0f;
        g_WalkSpeedModified = false;
        g_WalkSpeed = 5.0f;
        g_RecRoomPlusEnabled = false;
        g_RecoilModified = false;
        g_UnlockItemsEnabled = false;
        g_AimAssistEnabled = false;
        g_BlockShots = false;
        g_HeadScaleModified = false;
        g_HeadScale = 1.0f;
        g_NoChatCooldown = false;
        g_RapidFire = false;
        g_FreezeSelf = false;
        g_BlockLevelUp = false;
        g_BlockRoomTeleport = false;
        g_StreamerCamEnabled = false;
        g_BlockUnequipSlots = false;
        g_SpinbotEnabled = false;
        g_SuperJumpEnabled = false;
        g_JumpHeight = 10.0f;
        g_InfiniteSprintEnabled = false;
        g_ExtendedSlideEnabled = false;
        g_SlideDuration = 5.0f;
        g_NoGravityEnabled = false;
        g_SlideSpeedBoostEnabled = false;
        g_SlideSpeedBoost = 2.0f;
        g_AirControlEnabled = false;
        g_AirControlAmount = 1.0f;
        g_WallRunDurationEnabled = false;
        g_WallRunDuration = 5.0f;
        g_WallRunSpeedEnabled = false;
        g_WallRunSpeed = 2.0f;
        g_WallRunJumpEnabled = false;
        g_WallRunJumpHeight = 10.0f;
        g_CrouchSpeedEnabled = false;
        g_CrouchSpeed = 2.0f;
        g_ProneSpeedEnabled = false;
        g_ProneSpeed = 2.0f;
        g_AutoSprintEnabled = false;
        g_TeleportDistanceEnabled = false;
        g_TeleportDistance = 100.0f;
        g_NoTeleportCooldownEnabled = false;
        g_TeleportCooldownReductionEnabled = false;
        g_TeleportCooldownReduction = 0.5f;
        g_FlightSpeedEnabled = false;
        g_FlightSpeed = 2.0f;
        g_AutoWallClimbEnabled = false;
        g_NoCollisionParenting = false;
        g_StationaryEnabled = false;
        g_NoSitEnabled = false;
        g_NoShortenEnabled = false;
        g_NoCrouchInput = false;
        g_NoSlideEnabled = false;
        g_NoSprintInput = false;
        g_NoWallClimb = false;
        g_NoClamber = false;
        g_NoJump = false;
        g_NoTeleport = false;
        g_NoWallRun = false;
        g_NoRoomScaleMovement = false;

        Sleep(100);
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}

void initchair() {
    CreateConsole();
    MH_Initialize();
    CreateThread(nullptr, 0, [](LPVOID) -> DWORD { BypassThread(); return 0; }, nullptr, 0, nullptr);
    kiero::bind(8, (void**)&oPresent, hkPresent);
}

DWORD WINAPI MainThread(LPVOID lpReserved) {
    bool init_hook = false;
    do {
        if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success) {
            initchair();
            init_hook = true;
        }
        else {
            Sleep(1000);
        }
    } while (!init_hook);
    return TRUE;
}

BOOL WINAPI DllMain(HMODULE mod, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(mod);
        CreateThread(nullptr, 0, MainThread, mod, 0, nullptr);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        g_KillSwitch = true;
        uintptr_t base = (uintptr_t)GetModuleHandleA("GameAssembly.dll");
        if (base) {
            MH_DisableHook((LPVOID)(base + FLY_OFFSET));
            MH_RemoveHook((LPVOID)(base + FLY_OFFSET));
            MH_DisableHook((LPVOID)(base + IS_DEVELOPER_OFFSET));
            MH_RemoveHook((LPVOID)(base + IS_DEVELOPER_OFFSET));
            MH_DisableHook((LPVOID)(base + PLAYER_INVINCIBLE_OFFSET));
            MH_RemoveHook((LPVOID)(base + PLAYER_INVINCIBLE_OFFSET));
            MH_DisableHook((LPVOID)(base + SET_LOCAL_SCALE_OFFSET));
            MH_RemoveHook((LPVOID)(base + SET_LOCAL_SCALE_OFFSET));
            MH_DisableHook((LPVOID)(base + get_MaxWalkSpeed));
            MH_RemoveHook((LPVOID)(base + get_MaxWalkSpeed));
            MH_DisableHook((LPVOID)(base + AddMaxWalkSpeed));
            MH_RemoveHook((LPVOID)(base + AddMaxWalkSpeed));
            MH_DisableHook((LPVOID)(base + get_fieldOfView));
            MH_RemoveHook((LPVOID)(base + get_fieldOfView));
            MH_DisableHook((LPVOID)(base + set_fieldOfView));
            MH_RemoveHook((LPVOID)(base + set_fieldOfView));
            MH_DisableHook((LPVOID)(base + get_HasVisibleRecRoomPlus));
            MH_RemoveHook((LPVOID)(base + get_HasVisibleRecRoomPlus));
            MH_DisableHook((LPVOID)(base + set_HasVisibleRecRoomPlus));
            MH_RemoveHook((LPVOID)(base + set_HasVisibleRecRoomPlus));
            MH_DisableHook((LPVOID)(base + get_CurrentRecoilMultiplier));
            MH_RemoveHook((LPVOID)(base + get_CurrentRecoilMultiplier));
            MH_DisableHook((LPVOID)(base + IsItemUnlockedLocally));
            MH_RemoveHook((LPVOID)(base + IsItemUnlockedLocally));
            MH_DisableHook((LPVOID)(base + get_DeveloperDisplayMode));
            MH_RemoveHook((LPVOID)(base + get_DeveloperDisplayMode));
            MH_DisableHook((LPVOID)(base + set_DeveloperDisplayMode));
            MH_RemoveHook((LPVOID)(base + set_DeveloperDisplayMode));
            MH_DisableHook((LPVOID)(base + get_AimAssistEnabled));
            MH_RemoveHook((LPVOID)(base + get_AimAssistEnabled));
            MH_DisableHook((LPVOID)(base + RpcFireShot));
            MH_RemoveHook((LPVOID)(base + RpcFireShot));
            MH_DisableHook((LPVOID)(base + SetHeadScale));
            MH_RemoveHook((LPVOID)(base + SetHeadScale));
            MH_DisableHook((LPVOID)(base + get_IsSendChatOnCooldown));
            MH_RemoveHook((LPVOID)(base + get_IsSendChatOnCooldown));
            MH_DisableHook((LPVOID)(base + get_IsOnCooldown));
            MH_RemoveHook((LPVOID)(base + get_IsOnCooldown));
            MH_DisableHook((LPVOID)(base + get_LocalPlayerExists));
            MH_RemoveHook((LPVOID)(base + get_LocalPlayerExists));
            MH_DisableHook((LPVOID)(base + RpcPlayLevelUpFeedback));
            MH_RemoveHook((LPVOID)(base + RpcPlayLevelUpFeedback));
            MH_DisableHook((LPVOID)(base + CV2GoToRoom));
            MH_RemoveHook((LPVOID)(base + CV2GoToRoom));
            MH_DisableHook((LPVOID)(base + get_CanUseStreamingCam));
            MH_RemoveHook((LPVOID)(base + get_CanUseStreamingCam));
            MH_DisableHook((LPVOID)(base + CV2UnequipSlots));
            MH_RemoveHook((LPVOID)(base + CV2UnequipSlots));
            MH_DisableHook((LPVOID)(base + set_eulerAngles));
            MH_RemoveHook((LPVOID)(base + set_eulerAngles));
            MH_DisableHook((LPVOID)(base + AddMaxJumpHeight));
            MH_RemoveHook((LPVOID)(base + AddMaxJumpHeight));
            MH_DisableHook((LPVOID)(base + get_IsSprintOnCooldown));
            MH_RemoveHook((LPVOID)(base + get_IsSprintOnCooldown));
            MH_DisableHook((LPVOID)(base + AddSlideExpectedDuration));
            MH_RemoveHook((LPVOID)(base + AddSlideExpectedDuration));
            MH_DisableHook((LPVOID)(base + AddGravityDisabled));
            MH_RemoveHook((LPVOID)(base + AddGravityDisabled));
            MH_DisableHook((LPVOID)(base + AddSlideStartSpeedBoost));
            MH_RemoveHook((LPVOID)(base + AddSlideStartSpeedBoost));
            MH_DisableHook((LPVOID)(base + AddAirControlPercentage));
            MH_RemoveHook((LPVOID)(base + AddAirControlPercentage));
            MH_DisableHook((LPVOID)(base + AddWallRunMaxDuration));
            MH_RemoveHook((LPVOID)(base + AddWallRunMaxDuration));
            MH_DisableHook((LPVOID)(base + AddWallRunStartSpeedBoost));
            MH_RemoveHook((LPVOID)(base + AddWallRunStartSpeedBoost));
            MH_DisableHook((LPVOID)(base + AddWallRunMaxJumpHeight));
            MH_RemoveHook((LPVOID)(base + AddWallRunMaxJumpHeight));
            MH_DisableHook((LPVOID)(base + AddCrouchSpeedModifier));
            MH_RemoveHook((LPVOID)(base + AddCrouchSpeedModifier));
            MH_DisableHook((LPVOID)(base + AddProneSpeedModifier));
            MH_RemoveHook((LPVOID)(base + AddProneSpeedModifier));
            MH_DisableHook((LPVOID)(base + AddAutoSprintRestriction));
            MH_RemoveHook((LPVOID)(base + AddAutoSprintRestriction));
            MH_DisableHook((LPVOID)(base + AddTeleportMaxHorizontalDistance));
            MH_RemoveHook((LPVOID)(base + AddTeleportMaxHorizontalDistance));
            MH_DisableHook((LPVOID)(base + AddTeleportCooldown));
            MH_RemoveHook((LPVOID)(base + AddTeleportCooldown));
            MH_DisableHook((LPVOID)(base + AddTeleportCooldownModifier));
            MH_RemoveHook((LPVOID)(base + AddTeleportCooldownModifier));
            MH_DisableHook((LPVOID)(base + AddSelfScaleLocomotionScale));
            MH_RemoveHook((LPVOID)(base + AddSelfScaleLocomotionScale));
            MH_DisableHook((LPVOID)(base + RequestAutomaticWallClimbStart));
            MH_RemoveHook((LPVOID)(base + RequestAutomaticWallClimbStart));
            MH_DisableHook((LPVOID)(base + AddCollisionParentingRestriction));
            MH_RemoveHook((LPVOID)(base + AddCollisionParentingRestriction));
            MH_DisableHook((LPVOID)(base + AddStationaryRestriction));
            MH_RemoveHook((LPVOID)(base + AddStationaryRestriction));
            MH_DisableHook((LPVOID)(base + AddSitRestriction));
            MH_RemoveHook((LPVOID)(base + AddSitRestriction));
            MH_DisableHook((LPVOID)(base + AddShortenRestriction));
            MH_RemoveHook((LPVOID)(base + AddShortenRestriction));
            MH_DisableHook((LPVOID)(base + AddCrouchInputRestriction));
            MH_RemoveHook((LPVOID)(base + AddCrouchInputRestriction));
            MH_DisableHook((LPVOID)(base + AddSlideRestriction));
            MH_RemoveHook((LPVOID)(base + AddSlideRestriction));
            MH_DisableHook((LPVOID)(base + AddSprintInputRestriction));
            MH_RemoveHook((LPVOID)(base + AddSprintInputRestriction));
            MH_DisableHook((LPVOID)(base + AddWallClimbRestriction));
            MH_RemoveHook((LPVOID)(base + AddWallClimbRestriction));
            MH_DisableHook((LPVOID)(base + AddClamberRestriction));
            MH_RemoveHook((LPVOID)(base + AddClamberRestriction));
            MH_DisableHook((LPVOID)(base + AddJumpRestriction));
            MH_RemoveHook((LPVOID)(base + AddJumpRestriction));
            MH_DisableHook((LPVOID)(base + AddTeleportRestriction));
            MH_RemoveHook((LPVOID)(base + AddTeleportRestriction));
            MH_DisableHook((LPVOID)(base + AddWallRunRestriction));
            MH_RemoveHook((LPVOID)(base + AddWallRunRestriction));
            MH_DisableHook((LPVOID)(base + AddRoomScaleMovementRestriction));
            MH_RemoveHook((LPVOID)(base + AddRoomScaleMovementRestriction));
        }
        MH_Uninitialize();
    }
    return TRUE;
}