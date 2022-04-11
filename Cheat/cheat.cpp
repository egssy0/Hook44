#include "cheat.h"
#include <filesystem>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_win32.h>
#include <HookLib/HookLib.h>

uintptr_t milliseconds_now() {
    static LARGE_INTEGER s_frequency;
    static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
    if (s_use_qpc) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (1000LL * now.QuadPart) / s_frequency.QuadPart;
    }
    else {
        return GetTickCount64();
    }
}

float CachedFramerates[1000];

float MaxArr(float arr[], int n)
{
    int i;
    float max = arr[0];
    for (i = 1; i < n; i++)
    {
        if (arr[i] > max)
        {
            max = arr[i];
        }
    }
    return max;
}

float MinArr(float arr[], int n)
{
    int i;
    float min = arr[0];
    for (i = 1; i < n; i++)
    {
        if (arr[i] < min)
        {
            min = arr[i];
        }
    }
    return min;
}

#define STEAM
namespace fs = std::filesystem;

void Cheat::Hacks::OnWeaponFiredHook(UINT64 arg1, UINT64 arg2)
{
    Logger::Log("arg1: %p, arg2: %p\n", arg1, arg2);
    auto& cameraCache = cache.localCamera->CameraCache.POV;
    auto prev = cameraCache.Rotation;
    cameraCache.Rotation = { -cameraCache.Rotation.Pitch, -cameraCache.Rotation.Yaw, 0.f };
    return OnWeaponFiredOriginal(arg1, arg2);
}

void Cheat::Hacks::Init()
{

}

inline void Cheat::Hacks::Remove()
{

}

void Cheat::Renderer::Drawing::RenderText(const char* text, const FVector2D& pos, const ImVec4& color, const bool outlined = true, const bool centered = true)
{
    if (!text) return;
    auto ImScreen = *reinterpret_cast<const ImVec2*>(&pos);
    if (centered)
    {
        auto size = ImGui::CalcTextSize(text);
        ImScreen.x -= size.x * 0.5f;
        ImScreen.y -= size.y;
    }
    auto window = ImGui::GetCurrentWindow();
    if (outlined) { window->DrawList->AddText(nullptr, 0.f, ImVec2(ImScreen.x - 1.f, ImScreen.y + 1.f), ImGui::GetColorU32(IM_COL32_BLACK), text); }
    window->DrawList->AddText(nullptr, 0.f, ImScreen, ImGui::GetColorU32(color), text);
}

void Cheat::Renderer::Drawing::Render2DBox(const FVector2D& top, const FVector2D& bottom, const float height, const float width, const ImVec4& color)
{
    ImGui::GetCurrentWindow()->DrawList->AddRect({ top.X - width * 0.5f, top.Y }, { top.X + width * 0.5f, bottom.Y }, ImGui::GetColorU32(color), 0.f, 15, 1.5f);
}

bool Cheat::Renderer::Drawing::Render3DBox(AController* const controller, const FVector& origin, const FVector& extent, const FRotator& rotation, const ImVec4& color)
{
    FVector vertex[2][4];
    vertex[0][0] = { -extent.X, -extent.Y,  -extent.Z };
    vertex[0][1] = { extent.X, -extent.Y,  -extent.Z };
    vertex[0][2] = { extent.X, extent.Y,  -extent.Z };
    vertex[0][3] = { -extent.X, extent.Y, -extent.Z };
    vertex[1][0] = { -extent.X, -extent.Y, extent.Z };
    vertex[1][1] = { extent.X, -extent.Y, extent.Z };
    vertex[1][2] = { extent.X, extent.Y, extent.Z };
    vertex[1][3] = { -extent.X, extent.Y, extent.Z };
    FVector2D screen[2][4];
    FTransform const Transform(rotation);
    for (auto k = 0; k < 2; k++)
    {
        for (auto i = 0; i < 4; i++)
        {
            auto& vec = vertex[k][i];
            vec = Transform.TransformPosition(vec) + origin;
            if (!controller->ProjectWorldLocationToScreen(vec, screen[k][i])) return false;
        }

    }
    auto ImScreen = reinterpret_cast<ImVec2(&)[2][4]>(screen);
    auto window = ImGui::GetCurrentWindow();
    for (auto i = 0; i < 4; i++)
    {
        window->DrawList->AddLine(ImScreen[0][i], ImScreen[0][(i + 1) % 4], ImGui::GetColorU32(color));
        window->DrawList->AddLine(ImScreen[1][i], ImScreen[1][(i + 1) % 4], ImGui::GetColorU32(color));
        window->DrawList->AddLine(ImScreen[0][i], ImScreen[1][i], ImGui::GetColorU32(color));
    }
    return true;
}

bool Cheat::Renderer::Drawing::RenderSkeleton(AController* const controller, USkeletalMeshComponent* const mesh, const FMatrix& comp2world, const std::pair<const BYTE*, const BYTE>* skeleton, int size, const ImVec4& color)
{
    for (auto s = 0; s < size; s++)
    {
        auto& bone = skeleton[s];
        FVector2D previousBone;
        for (auto i = 0; i < skeleton[s].second; i++)
        {
            FVector loc;
            if (!mesh->GetBone(bone.first[i], comp2world, loc)) return false;
            FVector2D screen;
            if (!controller->ProjectWorldLocationToScreen(loc, screen)) return false;
            if (previousBone.Size() != 0) {
                auto ImScreen1 = *reinterpret_cast<ImVec2*>(&previousBone);
                auto ImScreen2 = *reinterpret_cast<ImVec2*>(&screen);
                ImGui::GetCurrentWindow()->DrawList->AddLine(ImScreen1, ImScreen2, ImGui::GetColorU32(color));
            }
            previousBone = screen;
        }
    }
    return true;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI Cheat::Renderer::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam) && bIsOpen) return true;
    if (bIsOpen)
    {
        ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
        case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
        case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
        case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
        case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
        case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
        case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
        case ImGuiMouseCursor_NotAllowed:   win32_cursor = IDC_NO; break;
        }
        SetCursorOriginal(LoadCursorA(nullptr, win32_cursor));
    }
    if (!bIsOpen || uMsg == WM_KEYUP) return CallWindowProcA(WndProcOriginal, hwnd, uMsg, wParam, lParam);
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

HCURSOR WINAPI Cheat::Renderer::SetCursorHook(HCURSOR hCursor)
{
    if (bIsOpen) return 0;
    return SetCursorOriginal(hCursor);
}

BOOL WINAPI Cheat::Renderer::SetCursorPosHook(int X, int Y)
{
    if (bIsOpen) return FALSE;
    return SetCursorPosOriginal(X, Y);
}

void Cheat::Renderer::HookInput()
{
    RemoveInput();
    WndProcOriginal = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(gameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
    Logger::Log("WndProcOriginal = %p\n", WndProcOriginal);
}

void Cheat::Renderer::RemoveInput()
{
    if (WndProcOriginal)
    {
        SetWindowLongPtrA(gameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcOriginal));
        WndProcOriginal = nullptr;
    }
}

bool raytrace(UWorld* world, const struct FVector& start, const struct FVector& end, struct FHitResult* hit)
{
    if (world == nullptr || world->PersistentLevel == nullptr)
        return false;
    return UKismetMathLibrary::LineTraceSingle_NEW((UObject*)world, start, end, ETraceTypeQuery::TraceTypeQuery4, true, TArray<AActor*>(), EDrawDebugTrace::EDrawDebugTrace__None, true, hit);
}

bool GetProjectilePath(std::vector<FVector>& v, FVector& Vel, FVector& Pos, float Gravity, int count, UWorld* world)
{
    float interval = 0.033f;
    for (unsigned int i = 0; i < count; ++i)
    {
        v.push_back(Pos);
        FVector move;
        move.X = (Vel.X) * interval;
        move.Y = (Vel.Y) * interval;
        float newZ = Vel.Z - (Gravity * interval);
        move.Z = ((Vel.Z + newZ) * 0.5f) * interval;
        Vel.Z = newZ;
        FVector nextPos = Pos + move;
        bool res = true;
        {
            FHitResult hit_result;
            res = !raytrace(world, Pos, nextPos, &hit_result);
        }
        Pos = nextPos;
        if (!res) return true;
    }
    return false;
}

#define _USE_MATH_DEFINES
#include <math.h>

FRotator ToFRotator(FVector vec)
{
    FRotator rot;
    float RADPI = (float)(180 / M_PI);
    rot.Yaw = (float)(atan2f(vec.Y, vec.X) * RADPI);
    rot.Pitch = (float)atan2f(vec.Z, sqrt((vec.X * vec.X) + (vec.Y * vec.Y))) * RADPI;
    rot.Roll = 0;
    return rot;
}

#include <complex>
void SolveQuartic(const std::complex<float> coefficients[5], std::complex<float> roots[4]) {
    const std::complex<float> a = coefficients[4];
    const std::complex<float> b = coefficients[3] / a;
    const std::complex<float> c = coefficients[2] / a;
    const std::complex<float> d = coefficients[1] / a;
    const std::complex<float> e = coefficients[0] / a;
    const std::complex<float> Q1 = c * c - 3.f * b * d + 12.f * e;
    const std::complex<float> Q2 = 2.f * c * c * c - 9.f * b * c * d + 27.f * d * d + 27.f * b * b * e - 72.f * c * e;
    const std::complex<float> Q3 = 8.f * b * c - 16.f * d - 2.f * b * b * b;
    const std::complex<float> Q4 = 3.f * b * b - 8.f * c;
    const std::complex<float> Q5 = std::pow(Q2 / 2.f + std::sqrt(Q2 * Q2 / 4.f - Q1 * Q1 * Q1), 1.f / 3.f);
    const std::complex<float> Q6 = (Q1 / Q5 + Q5) / 3.f;
    const std::complex<float> Q7 = 2.f * std::sqrt(Q4 / 12.f + Q6);
    roots[0] = (-b - Q7 - std::sqrt(4.f * Q4 / 6.f - 4.f * Q6 - Q3 / Q7)) / 4.f;
    roots[1] = (-b - Q7 + std::sqrt(4.f * Q4 / 6.f - 4.f * Q6 - Q3 / Q7)) / 4.f;
    roots[2] = (-b + Q7 - std::sqrt(4.f * Q4 / 6.f - 4.f * Q6 + Q3 / Q7)) / 4.f;
    roots[3] = (-b + Q7 + std::sqrt(4.f * Q4 / 6.f - 4.f * Q6 + Q3 / Q7)) / 4.f;
}

int AimAtStaticTarget(const FVector& oTargetPos, float fProjectileSpeed, float fProjectileGravityScalar, const FVector& oSourcePos, FRotator& oOutLow, FRotator& oOutHigh) {
    const float gravity = 981.f * fProjectileGravityScalar;
    const FVector diff(oTargetPos - oSourcePos);
    const FVector oDiffXY(diff.X, diff.Y, 0.0f);
    const float fGroundDist = oDiffXY.Size();
    const float s2 = fProjectileSpeed * fProjectileSpeed;
    const float s4 = s2 * s2;
    const float y = diff.Z;
    const float x = fGroundDist;
    const float gx = gravity * x;
    float root = s4 - (gravity * ((gx * x) + (2 * y * s2)));
    if (root < 0)
        return 0;
    root = std::sqrtf(root);
    const float fLowAngle = std::atan2f((s2 - root), gx);
    const float fHighAngle = std::atan2f((s2 + root), gx);
    int nSolutions = fLowAngle != fHighAngle ? 2 : 1;
    const FVector oGroundDir(oDiffXY.unit());
    oOutLow = ToFRotator(oGroundDir * std::cosf(fLowAngle) * fProjectileSpeed + FVector(0.f, 0.f, 1.f) * std::sinf(fLowAngle) * fProjectileSpeed);
    if (nSolutions == 2)
        oOutHigh = ToFRotator(oGroundDir * std::cosf(fHighAngle) * fProjectileSpeed + FVector(0.f, 0.f, 1.f) * std::sinf(fHighAngle) * fProjectileSpeed);
    return nSolutions;
}

#include <limits>
int AimAtMovingTarget(const FVector& oTargetPos, const FVector& oTargetVelocity, float fProjectileSpeed, float fProjectileGravityScalar, const FVector& oSourcePos, const FVector& oSourceVelocity, FRotator& oOutLow, FRotator& oOutHigh) {
    const FVector v(oTargetVelocity - oSourceVelocity);
    const FVector g(0.f, 0.f, -981.f * fProjectileGravityScalar);
    const FVector p(oTargetPos - oSourcePos);
    const float c4 = g | g * 0.25f;
    const float c3 = v | g;
    const float c2 = (p | g) + (v | v) - (fProjectileSpeed * fProjectileSpeed);
    const float c1 = 2.f * (p | v);
    const float c0 = p | p;
    std::complex<float> pOutRoots[4];
    const std::complex<float> pInCoeffs[5] = { c0, c1, c2, c3, c4 };
    SolveQuartic(pInCoeffs, pOutRoots);
    float fBestRoot = FLT_MAX;
    for (int i = 0; i < 4; i++) {
        if (pOutRoots[i].real() > 0.f && std::abs(pOutRoots[i].imag()) < 0.0001f && pOutRoots[i].real() < fBestRoot) {
            fBestRoot = pOutRoots[i].real();
        }
    }
    if (fBestRoot == FLT_MAX)
        return 0;
    const FVector oAimAt = oTargetPos + (v * fBestRoot);
    return AimAtStaticTarget(oAimAt, fProjectileSpeed, fProjectileGravityScalar, oSourcePos, oOutLow, oOutHigh);
}

HRESULT Cheat::Renderer::PresentHook(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
    if (!device)
    {
        ID3D11Texture2D* surface = nullptr;
        goto init;
    cleanup:
        Cheat::Remove();
        if (surface) surface->Release();
        return fnPresent(swapChain, syncInterval, flags);
    init:
        if (FAILED(swapChain->GetBuffer(0, __uuidof(surface), reinterpret_cast<PVOID*>(&surface)))) { goto cleanup; };
        Logger::Log("ID3D11Texture2D* surface = %p\n", surface);
        if (FAILED(swapChain->GetDevice(__uuidof(device), reinterpret_cast<PVOID*>(&device)))) goto cleanup;
        Logger::Log("ID3D11Device* device = %p\n", device);
        if (FAILED(device->CreateRenderTargetView(surface, nullptr, &renderTargetView))) goto cleanup;
        Logger::Log("ID3D11RenderTargetView* renderTargetView = %p\n", renderTargetView);
        surface->Release();
        surface = nullptr;
        device->GetImmediateContext(&context);
        Logger::Log("ID3D11DeviceContext* context = %p\n", context);
        ImGui::CreateContext();
        {
            ImGuiIO& io = ImGui::GetIO();
            ImFontConfig config;
            config.GlyphRanges = io.Fonts->GetGlyphRangesCyrillic();
            config.RasterizerMultiply = 1.125f;
            io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 16.0f, &config);
            io.IniFilename = nullptr;
        }
#ifdef STEAM
        DXGI_SWAP_CHAIN_DESC desc;
        swapChain->GetDesc(&desc);
        auto& window = desc.OutputWindow;
        gameWindow = window;
#else
        auto window = FindWindowA(NULL, "Sea of Thieves");
        gameWindow = window;
#endif
        Logger::Log("gameWindow = %p\n", window);
        if (!ImGui_ImplWin32_Init(window)) goto cleanup;
        if (!ImGui_ImplDX11_Init(device, context)) goto cleanup;
        if (!ImGui_ImplDX11_CreateDeviceObjects()) goto cleanup;
        Logger::Log("ImGui initialized successfully!\n");
        HookInput();
    }
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::Begin("#1", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar);
    auto& io = ImGui::GetIO();
    ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);
    auto drawList = ImGui::GetCurrentWindow()->DrawList;
    try
    {
        do
        {
            memset(&cache, 0, sizeof(Cache));
            auto const world = *UWorld::GWorld;
            if (!world) break;
            auto const game = world->GameInstance;
            if (!game) break;
            auto const gameState = world->GameState;
            if (!gameState) break;
            cache.gameState = gameState;
            if (!game->LocalPlayers.Data) break;
            auto const localPlayer = game->LocalPlayers[0];
            if (!localPlayer) break;
            auto const localController = localPlayer->PlayerController;
            if (!localController) break;
            cache.localController = localController;
            auto const camera = localController->PlayerCameraManager;
            if (!camera) break;
            cache.localCamera = camera;
            const auto cameraLoc = camera->GetCameraLocation();
            const auto cameraRot = camera->GetCameraRotation();
            auto const localCharacter = localController->Character;
            if (!localCharacter) break;
            auto const localPlayerCharacter = (AAthenaPlayerCharacter*)localController->K2_GetPawn();
            if (!localPlayerCharacter) break;
            auto const AACharacter = (AAthenaPlayerCharacter*)localCharacter;
            if (!localPlayerCharacter) break;
            const auto levels = world->Levels;
            if (!levels.Data) break;
            const auto localLoc = localCharacter->K2_GetActorLocation();
            bool isWieldedWeapon = false;
            auto item = localCharacter->GetWieldedItem();
            if (item) isWieldedWeapon = item->isWeapon();
            auto const localWeapon = *reinterpret_cast<AProjectileWeapon**>(&item);
            ACharacter* attachObject = localCharacter->GetAttachParentActor();;
            bool isHarpoon = false;
            if (attachObject)
            {
                if (cfg.aim.harpoon.bEnable && attachObject->isHarpoon()) { isHarpoon = true; }
            }
            cache.good = true;
            static struct {
                ACharacter* target = nullptr;
                FVector location;
                FRotator delta;
                float best = FLT_MAX;
                float smoothness = 1.f;
            } aimBest;
            aimBest.target = nullptr;
            aimBest.best = FLT_MAX;
            for (auto l = 0u; l < levels.Count; l++)
            {
                auto const level = levels[l];
                if (!level) continue;
                const auto actors = level->AActors;
                if (!actors.Data) continue;
                for (auto a = 0u; a < actors.Count; a++)
                {
                    auto const actor = actors[a];
                    if (!actor) continue;
                    if (cfg.aim.bEnable)
                    {
                        if (isHarpoon)
                        {
                            if (actor->isItem())
                            {
                                do {
                                } while (false);
                            }
                        }

                        // PLAYER AIM

                        else if (!attachObject && isWieldedWeapon)
                        {
                            if (cfg.aim.players.bEnable && actor->isPlayer() && actor != localCharacter && !actor->IsDead())
                            {
                                do {
                                    FVector playerLoc = actor->K2_GetActorLocation();
                                    float dist = localLoc.DistTo(playerLoc);
                                    if (dist > localWeapon->WeaponParameters.ProjectileMaximumRange) { break; }
                                    if (cfg.aim.players.bVisibleOnly) if (!localController->LineOfSightTo(actor, cameraLoc, false)) { break; }
                                    if (!cfg.aim.players.bTeam) if (UCrewFunctions::AreCharactersInSameCrew(actor, localCharacter)) break;
                                    FRotator rotationDelta = UKismetMathLibrary::NormalizedDeltaRotator(UKismetMathLibrary::FindLookAtRotation(cameraLoc, playerLoc), cameraRot);
                                    float absYaw = abs(rotationDelta.Yaw);
                                    float absPitch = abs(rotationDelta.Pitch);
                                    if (absYaw > cfg.aim.players.fYaw || absPitch > cfg.aim.players.fPitch) { break; }
                                    float sum = absYaw + absPitch;
                                    if (sum < aimBest.best)
                                    {
                                        aimBest.target = actor;
                                        aimBest.location = playerLoc;
                                        aimBest.delta = rotationDelta;
                                        aimBest.best = sum;
                                        aimBest.smoothness = cfg.aim.players.fSmoothness;
                                    }
                                } while (false);
                            }
                        }
                    }
                    
                    // DEBUG
                    
                    if (cfg.visuals.bEnable)
                    {
                        if (cfg.visuals.client.bDebug)
                        {
                            const FVector location = actor->K2_GetActorLocation();
                            const float dist = localLoc.DistTo(location) * 0.01f;
                            if (dist < cfg.visuals.client.fDebug)
                            {
                                auto const actorClass = actor->Class;
                                if (!actorClass) continue;
                                auto super = actorClass->SuperField;
                                if (!super) continue;
                                FVector2D screen;
                                if (localController->ProjectWorldLocationToScreen(location, screen))
                                {
                                    auto superName = super->GetNameFast();
                                    auto className = actorClass->GetNameFast();
                                    if (superName && className)
                                    {
                                        char buf[0x128];
                                        sprintf_s(buf, "%s %s [%d] (%p)", className, superName, (int)dist, actor);
                                        Drawing::RenderText(buf, screen, ImVec4(1.f, 1.f, 1.f, 1.f));
                                    }
                                }
                            }
                        }

                        // ITEMS

                        else {
                            if (cfg.visuals.items.bEnable && actor->isItem()) {
                                if (cfg.visuals.items.bName)
                                {
                                    auto location = actor->K2_GetActorLocation();
                                    FVector2D screen;
                                    if (localController->ProjectWorldLocationToScreen(location, screen))
                                    {
                                        auto const desc = actor->GetItemInfo()->Desc;
                                        if (!desc) continue;
                                        const int dist = localLoc.DistTo(location) * 0.01f;
                                        char name[0x64];
                                        const int len = desc->Title->multi(name, 0x50);
                                        snprintf(name + len, sizeof(name) - len, " [%d]", dist);
                                        Drawing::RenderText(name, screen, cfg.visuals.items.textCol);
                                    };
                                }
                                continue;
                            }

                            // PLAYER ESP

                            else if (cfg.visuals.players.bEnable && actor->isPlayer() && actor != localCharacter && !actor->IsDead())
                            {
                                const bool teammate = UCrewFunctions::AreCharactersInSameCrew(actor, localCharacter);
                                if (teammate && !cfg.visuals.players.bDrawTeam) continue;
                                FVector origin, extent;
                                actor->GetActorBounds(true, origin, extent);
                                const FVector location = actor->K2_GetActorLocation();
                                FVector2D headPos;
                                if (!localController->ProjectWorldLocationToScreen({ location.X, location.Y, location.Z + extent.Z }, headPos)) continue;
                                FVector2D footPos;
                                if (!localController->ProjectWorldLocationToScreen({ location.X, location.Y, location.Z - extent.Z }, footPos)) continue;
                                const float height = abs(footPos.Y - headPos.Y);
                                const float width = height * 0.4f;
                                const bool bVisible = localController->LineOfSightTo(actor, cameraLoc, false);
                                ImVec4 col;
                                if (teammate) col = bVisible ? cfg.visuals.players.teamColorVis : cfg.visuals.players.teamColorInv;
                                else  col = bVisible ? cfg.visuals.players.enemyColorVis : cfg.visuals.players.enemyColorInv;
                                switch (cfg.visuals.players.boxType)
                                {
                                case Config::EBox::E2DBoxes:
                                {
                                    Drawing::Render2DBox(headPos, footPos, height, width, col);
                                    break;
                                }
                                case Config::EBox::E3DBoxes:
                                {
                                    FRotator rotation = actor->K2_GetActorRotation();
                                    FVector ext = { 35.f, 35.f, extent.Z };
                                    if (!Drawing::Render3DBox(localController, location, ext, rotation, col)) continue;
                                    break;
                                  }
                                }
                               
                                // PLAYER NAME

                                if (cfg.visuals.players.bName)
                                {
                                    auto const playerState = actor->PlayerState;
                                    if (!playerState) continue;
                                    const auto playerName = playerState->PlayerName;
                                    if (!playerName.Data) continue;
                                    char name[0x30];
                                    const int len = playerName.multi(name, 0x20);
                                    const int dist = localLoc.DistTo(origin) * 0.01f;
                                    snprintf(name + len, sizeof(name) - len, " [%d]", dist);
                                    const float adjust = height * 0.05f;
                                    FVector2D pos = { headPos.X, headPos.Y - adjust };
                                    Drawing::RenderText(name, pos, cfg.visuals.players.textCol);
                                }

                                // HEALTH BAR
                                
                                if (cfg.visuals.players.barType != Config::EBar::ENone)
                                {
                                    auto const healthComp = actor->HealthComponent;
                                    if (!healthComp) continue;
                                    const float hp = healthComp->GetCurrentHealth() / healthComp->GetMaxHealth();
                                    const float width2 = width * 0.5f;
                                    const float adjust = height * 0.025f;
                                    switch (cfg.visuals.players.barType)
                                    {
                                    case Config::EBar::ELeft:
                                    {
                                        const float len = height * hp;
                                        drawList->AddRectFilled({ headPos.X - width2 - adjust * 2.f, headPos.Y }, { headPos.X - width2 - adjust, footPos.Y - len }, ImGui::GetColorU32(IM_COL32(255, 0, 0, 255)));
                                        drawList->AddRectFilled({ headPos.X - width2 - adjust * 2.f, footPos.Y - len }, { headPos.X - width2 - adjust, footPos.Y }, ImGui::GetColorU32(IM_COL32(0, 255, 0, 255)));
                                        break;
                                    }
                                    case Config::EBar::ERight:
                                    {
                                        const float len = height * hp;
                                        drawList->AddRectFilled({ headPos.X + width2 + adjust, headPos.Y }, { headPos.X + width2 + adjust * 2.f, footPos.Y - len }, ImGui::GetColorU32(IM_COL32(255, 0, 0, 255)));
                                        drawList->AddRectFilled({ headPos.X + width2 + adjust, footPos.Y - len }, { headPos.X + width2 + adjust * 2.f, footPos.Y }, ImGui::GetColorU32(IM_COL32(0, 255, 0, 255)));
                                        break;
                                    }
                                    case Config::EBar::EBottom:
                                    {
                                        const float len = width * hp;
                                        drawList->AddRectFilled({ headPos.X - width2, footPos.Y + adjust }, { headPos.X - width2 + len, footPos.Y + adjust * 2.f }, ImGui::GetColorU32(IM_COL32(0, 255, 0, 255)));
                                        drawList->AddRectFilled({ headPos.X - width2 + len, footPos.Y + adjust }, { headPos.X + width2, footPos.Y + adjust * 2.f }, ImGui::GetColorU32(IM_COL32(255, 0, 0, 255)));
                                        break;
                                    }
                                    case Config::EBar::ETop:
                                    {
                                        const float len = width * hp;
                                        drawList->AddRectFilled({ headPos.X - width2, headPos.Y - adjust * 2.f }, { headPos.X - width2 + len, headPos.Y - adjust }, ImGui::GetColorU32(IM_COL32(0, 255, 0, 255)));
                                        drawList->AddRectFilled({ headPos.X - width2 + len, headPos.Y - adjust * 2.f }, { headPos.X + width2, headPos.Y - adjust }, ImGui::GetColorU32(IM_COL32(255, 0, 0, 255)));
                                        break;
                                      }
                                   }
                                }
                            }
                           
                            // WORLD EVENTS

                            if (cfg.visuals.world.bEnable)
                            {
                                if (actor->isEvent())
                                {
                                    const FVector location = actor->K2_GetActorLocation();
                                    FVector2D screen;
                                    if (localController->ProjectWorldLocationToScreen(location, screen))
                                    {
                                        auto type = actor->GetName();
                                        const int dist = localLoc.DistTo(location) * 0.01f;
                                        if (dist > cfg.visuals.world.drawdistance) continue;
                                        char buf[0x64];
                                        if (type.find("ShipCloud") != std::string::npos)
                                            sprintf(buf, "Fleet [%dm]", dist);
                                        else if (type.find("AshenLord") != std::string::npos)
                                            sprintf(buf, "Ashen Lord [%dm]", dist);
                                        else if (type.find("Flameheart") != std::string::npos)
                                            sprintf(buf, "Flame Heart [%dm]", dist);
                                        else if (type.find("LegendSkellyFort") != std::string::npos)
                                            sprintf(buf, "Fort of Fortune [%dm]", dist);
                                        else if (type.find("SkellyFortOfTheDamned") != std::string::npos)
                                            sprintf(buf, "FOTD [%dm]", dist);
                                        else if (type.find("BP_SkellyFort") != std::string::npos)
                                            sprintf(buf, "Skull Fort [%dm]", dist);
                                        Drawing::RenderText(buf, screen, cfg.visuals.world.textCol);
                                    }
                                }
                            }

                            //SHIP ESP 1

                            if (cfg.visuals.ships.bEnable)
                            {
                                FVector location = actor->K2_GetActorLocation();
                                location.Z = 3000;
                                const int dist = localLoc.DistTo(location) * 0.01f;
                                if (cfg.visuals.ships.bName && actor->isShip() && dist < 1726)
                                {
                                    FVector2D screen;
                                    FVector velocity = actor->GetVelocity() / 100.f;
                                    auto speed = velocity.Size();
                                    if (localController->ProjectWorldLocationToScreen(location, screen))
                                    {
                                        auto type = actor->GetName();
                                        char buf[0x64];
                                        if (type.find("BP_Small") != std::string::npos)
                                            sprintf(buf, "Sloop [%dm] [%.0fm/s]", dist, speed);
                                        else if (type.find("BP_Medium") != std::string::npos)
                                            sprintf(buf, "Brig [%dm] [%.0fm/s]", dist, speed);
                                        else if (type.find("BP_Large") != std::string::npos)
                                            sprintf(buf, "Galleon [%dm] [%.0fm/s]", dist, speed);
                                        else if (type.find("AISmall") != std::string::npos)
                                            sprintf(buf, "Skeleton Sloop [%dm] [%.0fm/s]", dist, speed);
                                        else if (type.find("AILarge") != std::string::npos)
                                            sprintf(buf, "Skeleton Galleon [%dm] [%.0fm/s]", dist, speed);
                                        Drawing::RenderText(buf, screen, cfg.visuals.ships.textCol);
                                    }
                                }
                               
                                // SHIP ESP 2
                                
                                if (cfg.visuals.ships.bName && actor->isFarShip() && dist > 1725)
                                {
                                    int amount = 0;
                                    FVector2D screen;
                                    FVector velocity = actor->GetVelocity() / 100.f;
                                    auto speed = velocity.Size();
                                    if (localController->ProjectWorldLocationToScreen(location, screen))
                                    {
                                        auto type = actor->GetName();
                                        char buf[0x64];
                                        if (type.find("BP_Small") != std::string::npos)
                                            sprintf(buf, "Sloop [%dm]", dist);
                                        else if (type.find("BP_Medium") != std::string::npos)
                                            sprintf(buf, "Brig [%dm]", dist);
                                        else if (type.find("BP_Large") != std::string::npos)
                                            sprintf(buf, "Galleon [%dm]", dist);
                                        else if (type.find("BP_AISmall") != std::string::npos)
                                            sprintf(buf, "Skeleton Sloop [%dm]", dist);
                                        else if (type.find("BP_AILarge") != std::string::npos)
                                            sprintf(buf, "Skeleton Galleon [%dm]", dist);
                                        Drawing::RenderText(buf, screen, cfg.visuals.ships.textCol);
                                    }
                                }
                                if (actor->isShip())
                                   { 
                                }
                            }
                        }
                    }
                }
            }
            
            // CROSSHAIR
            
            if (cfg.visuals.bEnable)
            {
                if (cfg.visuals.client.bCrosshair)
                {
                    drawList->AddLine({ io.DisplaySize.x * 0.5f - cfg.visuals.client.fCrosshair, io.DisplaySize.y * 0.5f }, { io.DisplaySize.x * 0.5f + cfg.visuals.client.fCrosshair, io.DisplaySize.y * 0.5f }, ImGui::GetColorU32(cfg.visuals.client.crosshairColor));
                    drawList->AddLine({ io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f - cfg.visuals.client.fCrosshair }, { io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f + cfg.visuals.client.fCrosshair }, ImGui::GetColorU32(cfg.visuals.client.crosshairColor));
                }
                
                // OXYGEN 
                
                if (cfg.visuals.client.bOxygen && localCharacter->IsInWater())
                {
                    auto drownComp = localCharacter->DrowningComponent;
                    if (!drownComp) break;
                    auto level = drownComp->GetOxygenLevel();
                    auto posX = io.DisplaySize.x * 0.5f;
                    auto posY = io.DisplaySize.y * 0.85f;
                    auto barWidth2 = io.DisplaySize.x * 0.05f;
                    auto barHeight2 = io.DisplaySize.y * 0.0030f;
                    drawList->AddRectFilled({ posX - barWidth2, posY - barHeight2 }, { posX + barWidth2, posY + barHeight2 }, ImGui::GetColorU32(IM_COL32(0, 0, 0, 255)));
                    drawList->AddRectFilled({ posX - barWidth2, posY - barHeight2 }, { posX - barWidth2 + barWidth2 * level * 2.f, posY + barHeight2 }, ImGui::GetColorU32(IM_COL32(0, 200, 255, 255)));
                }
                
                // COMPASS
                
                if (cfg.visuals.client.bCompass)
                {
                    const char* directions[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
                    int yaw = ((int)cameraRot.Yaw + 450) % 360;
                    int index = int(yaw + 22.5f) % 360 * 0.0222222f;
                    FVector2D pos = { io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.02f };
                    auto col = ImVec4(1.f, 1.f, 1.f, 1.f);
                    Drawing::RenderText(const_cast<char*>(directions[index]), pos, col);
                    char buf[0x30];
                    int len = sprintf_s(buf, "%d", yaw);
                    pos.Y += 15.f;
                    Drawing::RenderText(buf, pos, col);
                }
            }

            // SHIP INFO
            
            if (cfg.misc.bEnable && !localCharacter->IsLoading())
            {
                if (cfg.misc.client.bEnable)
                {
                    if (cfg.misc.client.bShipInfo)
                    {
                        auto ship = localCharacter->GetCurrentShip();
                        if (ship)
                        {
                            FVector velocity = ship->GetVelocity() / 100.f;
                            char buf[0xFF];
                            FVector2D pos{ 1.f, 45.f };
                            ImVec4 col{ 1.f,1.f,1.f,1.f };
                            auto speed = velocity.Size();
                            sprintf(buf, "Speed: %.0fm/s", speed);
                            pos.Y += 5.f;
                            Drawing::RenderText(buf, pos, col, true, false);
                            int holes = ship->GetHullDamage()->ActiveHullDamageZones.Count;
                            sprintf(buf, "Holes: %d", holes);
                            pos.Y += 20.f;
                            Drawing::RenderText(buf, pos, col, true, false);
                            int amount = 0;
                            auto water = ship->GetInternalWater();
                            if (water) amount = water->GetNormalizedWaterAmount() * 100.f;
                            sprintf(buf, "Water: %d%%", amount);
                            pos.Y += 20.f;
                            Drawing::RenderText(buf, pos, col, true, false);
                            pos.Y += 22.f;
                            float internal_water_percent = ship->GetInternalWater()->GetNormalizedWaterAmount();
                            drawList->AddLine({ pos.X - 1, pos.Y }, { pos.X + 100 + 1, pos.Y }, 0xFF000000, 6);
                            drawList->AddLine({ pos.X, pos.Y }, { pos.X + 100, pos.Y }, 0xFF00FF00, 4);
                            drawList->AddLine({ pos.X, pos.Y }, { pos.X + (100.f * internal_water_percent), pos.Y }, 0xFF0000FF, 4);
                        }
                    }

                    // FOV

                    static std::uintptr_t desiredTime = 0;
                    if (milliseconds_now() >= desiredTime)
                    {
                        int return_value = (int)localCharacter->GetTargetFOV(AACharacter);;
                        localController->FOV(cfg.misc.client.fov);
                        if (return_value == 17.f)
                        {
                            localController->FOV(cfg.misc.client.fov * 0.2f);
                        }
                        desiredTime = milliseconds_now() + 100;
                    }
                }
            }

            // AIM FUNCTION

            if (aimBest.target != nullptr)
            {
                FVector2D screen;
                if (localController->ProjectWorldLocationToScreen(aimBest.location, screen))
                {
                    auto col = ImGui::GetColorU32(IM_COL32(0, 200, 0, 255));
                    drawList->AddLine({ io.DisplaySize.x * 0.5f , io.DisplaySize.y * 0.5f }, { screen.X, screen.Y }, col);
                    drawList->AddCircle({ screen.X, screen.Y }, 3.f, col);
                }
                if (ImGui::IsMouseDown(1))
                {
                    if (isHarpoon)
                    {
                      
                    }
                    else {
                        FVector LV = localCharacter->GetVelocity();
                        if (auto const localShip = localCharacter->GetCurrentShip()) LV += localShip->GetVelocity();
                        FVector TV = aimBest.target->GetVelocity();
                        if (auto const targetShip = aimBest.target->GetCurrentShip()) TV += targetShip->GetVelocity();
                        const FVector RV = TV - LV;
                        const float BS = localWeapon->WeaponParameters.AmmoParams.Velocity;
                        const FVector RL = localLoc - aimBest.location;
                        const float a = RV.Size() - BS * BS;
                        const float b = (RL * RV * 2.f).Sum();
                        const float c = RL.SizeSquared();
                        const float D = b * b - 4 * a * c;
                        if (D > 0)
                        {
                            const float DRoot = sqrtf(D);
                            const float x1 = (-b + DRoot) / (2 * a);
                            const float x2 = (-b - DRoot) / (2 * a);
                            if (x1 >= 0 && x1 >= x2) aimBest.location += RV * x1;
                            else if (x2 >= 0) aimBest.location += RV * x2;
                            aimBest.delta = UKismetMathLibrary::NormalizedDeltaRotator(UKismetMathLibrary::FindLookAtRotation(cameraLoc, aimBest.location), cameraRot);
                            auto smoothness = 1.f / aimBest.smoothness;
                            localController->AddYawInput(aimBest.delta.Yaw * smoothness);
                            localController->AddPitchInput(aimBest.delta.Pitch * -smoothness);
                        }
                    }
                }

            }
        } while (false);
    }
    catch (...)
    {
        Logger::Log("Exception\n");
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
#ifdef STEAM
    if (ImGui::IsKeyPressed(VK_INSERT)) bIsOpen = !bIsOpen;
#else
    static const FKey insert("Insert");
    if (cache.localController && cache.localController->WasInputKeyJustPressed(insert)) { bIsOpen = !bIsOpen; }
#endif

    // IMGUI

        if (bIsOpen) {
            ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.3f, io.DisplaySize.y * 0.7f), ImGuiCond_Once);
            ImGui::Begin("M-Hook | 2.5.1 | Cut-Down Edition", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

            ImGuiStyle* style = &ImGui::GetStyle();
            style->WindowPadding = ImVec2(8, 8);
            style->WindowRounding = 7.0f;
            style->FramePadding = ImVec2(4, 3);
            style->FrameRounding = 0.0f;
            style->ItemSpacing = ImVec2(6, 4);
            style->ItemInnerSpacing = ImVec2(4, 4);
            style->IndentSpacing = 20.0f;
            style->ScrollbarSize = 14.0f;
            style->ScrollbarRounding = 9.0f;
            style->GrabMinSize = 5.0f;
            style->GrabRounding = 0.0f;
            style->WindowBorderSize = 0;
            style->WindowTitleAlign = ImVec2(0.0f, 0.5f);
            style->FramePadding = ImVec2(4, 3);
            style->Colors[ImGuiCol_TitleBg] = ImColor(1, 32, 255, 225);
            style->Colors[ImGuiCol_TitleBgActive] = ImColor(1, 32, 255, 225);
            style->Colors[ImGuiCol_Button] = ImColor(1, 32, 255, 225);
            style->Colors[ImGuiCol_ButtonActive] = ImColor(1, 32, 255, 225);
            style->Colors[ImGuiCol_ButtonHovered] = ImColor(41, 40, 41, 255);
            style->Colors[ImGuiCol_Separator] = ImColor(70, 70, 70, 255);
            style->Colors[ImGuiCol_SeparatorActive] = ImColor(76, 76, 76, 255);
            style->Colors[ImGuiCol_SeparatorHovered] = ImColor(76, 76, 76, 255);
            style->Colors[ImGuiCol_Tab] = ImColor(1, 32, 230, 225);
            style->Colors[ImGuiCol_TabHovered] = ImColor(1, 32, 238, 225);
            style->Colors[ImGuiCol_TabActive] = ImColor(1, 32, 238, 225);
            style->Colors[ImGuiCol_SliderGrab] = ImColor(1, 32, 255, 225);
            style->Colors[ImGuiCol_SliderGrabActive] = ImColor(1, 32, 255, 225);
            style->Colors[ImGuiCol_MenuBarBg] = ImColor(76, 76, 76, 255);
            style->Colors[ImGuiCol_FrameBg] = ImColor(37, 36, 37, 255);
            style->Colors[ImGuiCol_FrameBgActive] = ImColor(37, 36, 37, 255);
            style->Colors[ImGuiCol_FrameBgHovered] = ImColor(37, 36, 37, 255);
            style->Colors[ImGuiCol_Header] = ImColor(0, 0, 0, 0);
            style->Colors[ImGuiCol_HeaderActive] = ImColor(0, 0, 0, 0);
            style->Colors[ImGuiCol_HeaderHovered] = ImColor(46, 46, 46, 255);

        if (ImGui::BeginTabBar("Bars")) {
            
            // VISUALS TAB BEGINS
            
            if (ImGui::BeginTabItem("Visuals")) {

                // Global Visuals

                ImGui::Text("Global Visuals");
                if (ImGui::BeginChild("Global", ImVec2(0.f, 40.f), true, 0))
                {
                    ImGui::Checkbox("Enable", &cfg.visuals.bEnable);
                }
                ImGui::EndChild();
                ImGui::Columns(2, "CLM1", false);
                const char* boxes[] = { "None", "2DBox", "3DBox" };
                
                // Player ESP

                ImGui::Text("Players");
                if (ImGui::BeginChild("PlayersSettings", ImVec2(0.f, 290.f), true, 0))
                {
                    const char* bars[] = { "None", "2DRectLeft", "2DRectRight", "2DRectBottom", "2DRectTop" };
                    ImGui::Checkbox("Enable", &cfg.visuals.players.bEnable);
                    ImGui::Checkbox("Draw Name", &cfg.visuals.players.bName);
                    ImGui::Checkbox("Draw Team", &cfg.visuals.players.bDrawTeam);
                    ImGui::Combo("Box Type", reinterpret_cast<int*>(&cfg.visuals.players.boxType), boxes, IM_ARRAYSIZE(boxes));
                    ImGui::Combo("H Bar Type", reinterpret_cast<int*>(&cfg.visuals.players.barType), bars, IM_ARRAYSIZE(bars));
                    ImGui::ColorEdit4("Enemy Color", &cfg.visuals.players.enemyColorVis.x, 0);
                    ImGui::ColorEdit4("Enemy Color", &cfg.visuals.players.enemyColorInv.x, 0);
                    ImGui::ColorEdit4("Team Color", &cfg.visuals.players.teamColorVis.x, 0);
                    ImGui::ColorEdit4("Team Color", &cfg.visuals.players.teamColorInv.x, 0);
                    ImGui::ColorEdit4("Text Color", &cfg.visuals.players.textCol.x, 0);
                }
                ImGui::EndChild();
                ImGui::NextColumn();

                // SHIP ESP

                ImGui::Text("Ships");
                if (ImGui::BeginChild("ShipsSettings", ImVec2(0.f, 290.f), true, 0)) {
                    ImGui::Checkbox("Enable", &cfg.visuals.ships.bEnable);
                    ImGui::Checkbox("Draw Name", &cfg.visuals.ships.bName);
                    ImGui::ColorEdit4("Hole Color", &cfg.visuals.ships.damageColor.x, 0);
                    ImGui::ColorEdit4("Text Color", &cfg.visuals.ships.textCol.x, 0);
                }
                ImGui::EndChild();
                ImGui::NextColumn();

                // WORLD EVENTS

                ImGui::Text("World Events");
                if (ImGui::BeginChild("WorldSettings", ImVec2(0.f, 120.f), true, 0 | ImGuiWindowFlags_NoScrollWithMouse))
                {
                    ImGui::Checkbox("Enable", &cfg.visuals.world.bEnable);
                    ImGui::ColorEdit4("Text Color", &cfg.visuals.world.textCol.x, 0);
                    ImGui::SliderFloat("Draw Distance", &cfg.visuals.world.drawdistance, 0.f, 20000.f, "%.0f");
                }
                ImGui::EndChild();
                
                // CLIENT MAIN
                
                ImGui::Text("Client");
                if (ImGui::BeginChild("ClientSettings", ImVec2(0.f, 140.f), true, 0))
                {
                    ImGui::Checkbox("Crosshair", &cfg.visuals.client.bCrosshair);
                    if (cfg.visuals.client.bCrosshair)
                    {
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(75.f);
                        ImGui::SliderFloat("Radius##1", &cfg.visuals.client.fCrosshair, 1.f, 100.f);
                    }
                    ImGui::ColorEdit4("Crosshair color", &cfg.visuals.client.crosshairColor.x, ImGuiColorEditFlags_DisplayRGB);
                    ImGui::Checkbox("Oxygen Level", &cfg.visuals.client.bOxygen);
                    ImGui::Checkbox("Compass", &cfg.visuals.client.bCompass);
                }
                ImGui::EndChild();
                ImGui::NextColumn();

                // ITEM ESP
                
                ImGui::Text("Items");
                if (ImGui::BeginChild("ItemsSettings", ImVec2(0.f, 120.f), true, 0))
                {
                    ImGui::Checkbox("Enable", &cfg.visuals.items.bEnable);
                    ImGui::Checkbox("Draw Name", &cfg.visuals.items.bName);
                    ImGui::ColorEdit4("Text Color", &cfg.visuals.items.textCol.x, 0);
                }
                ImGui::EndChild();
                ImGui::Columns();
                ImGui::EndTabItem();
            }
            
            // AIM/MISC TAB BEGINS
            
            if (ImGui::BeginTabItem("Aim/Misc")) {
               
                // GLOBAL AIM

                ImGui::Text("Global Aim");
                if (ImGui::BeginChild("Global", ImVec2(0.f, 40.f), true, 0))
                {
                    ImGui::Checkbox("Enable", &cfg.aim.bEnable);
                }
                ImGui::EndChild();
                ImGui::Columns(2, "CLM1", false);
               
                // PLAYER AIM
                
                ImGui::Text("Players");
                if (ImGui::BeginChild("PlayersSettings", ImVec2(0.f, 270.f), true, 0))
                {
                    ImGui::Checkbox("Enable", &cfg.aim.players.bEnable);
                    ImGui::Checkbox("Aim at teammates", &cfg.aim.players.bTeam);
                    ImGui::SliderFloat("Yaw", &cfg.aim.players.fYaw, 1.f, 180.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::SliderFloat("Pitch", &cfg.aim.players.fPitch, 1.f, 180.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::SliderFloat("Smoothness", &cfg.aim.players.fSmoothness, 1.f, 100.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                }
                ImGui::EndChild();
                ImGui::Columns();
                ImGui::EndTabItem();
               
                // GLOBAL MISC

                ImGui::Text("Global Misc");
                if (ImGui::BeginChild("Global", ImVec2(0.f, 40.f), true, 0))
                {
                    ImGui::Checkbox("Enable", &cfg.misc.bEnable);
                }
                ImGui::EndChild();

                // CLIENT

                ImGui::Text("Client");
                if (ImGui::BeginChild("ClientSettings", ImVec2(0.f, 250.f), true, 0))
                {
                    ImGui::Checkbox("Enable", &cfg.misc.client.bEnable);
                    ImGui::Checkbox("Ship Info", &cfg.misc.client.bShipInfo);
                    ImGui::SliderFloat("FOV", &cfg.misc.client.fov, 20.f, 160.f, "%.0f");
                    ImGui::Separator();
                    if (ImGui::Button("Save settings"))
                    {
                        do {
                            wchar_t buf[MAX_PATH];
                            GetModuleFileNameW(hinstDLL, buf, MAX_PATH);
                            fs::path path = fs::path(buf).remove_filename() / ".settings";
                            auto file = CreateFileW(path.wstring().c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                            if (file == INVALID_HANDLE_VALUE) break;
                            DWORD written;
                            if (WriteFile(file, &cfg, sizeof(cfg), &written, 0)) ImGui::OpenPopup("##SettingsSaved");
                            CloseHandle(file);
                        } while (false);
                    }
                    
                    // SETTINGS/SAVING

                    ImGui::SameLine();
                    if (ImGui::Button("Load settings")) 
                    {
                        do {
                            wchar_t buf[MAX_PATH];
                            GetModuleFileNameW(hinstDLL, buf, MAX_PATH);
                            fs::path path = fs::path(buf).remove_filename() / ".settings";
                            auto file = CreateFileW(path.wstring().c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                            if (file == INVALID_HANDLE_VALUE) break;
                            DWORD readed;
                            if (ReadFile(file, &cfg, sizeof(cfg), &readed, 0))  ImGui::OpenPopup("##SettingsLoaded");
                            CloseHandle(file);
                        } while (false);
                    }
                    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                    if (ImGui::BeginPopupModal("##SettingsSaved", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
                    {
                        ImGui::Text("\nSettings have been saved\n\n");
                        ImGui::Separator();
                        if (ImGui::Button("OK", { 170.f , 0.f })) { ImGui::CloseCurrentPopup(); }
                        ImGui::EndPopup();
                    }
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                    if (ImGui::BeginPopupModal("##SettingsLoaded", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
                    {
                        ImGui::Text("\nSettings have been loaded\n\n");
                        ImGui::Separator();
                        if (ImGui::Button("OK", { 170.f , 0.f })) { ImGui::CloseCurrentPopup(); }
                        ImGui::EndPopup();
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        };
        ImGui::End();
    }
    context->OMSetRenderTargets(1, &renderTargetView, nullptr);
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    return PresentOriginal(swapChain, syncInterval, flags);
}

HRESULT Cheat::Renderer::ResizeHook(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
{
    if (renderTargetView)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui::DestroyContext();
        renderTargetView->Release();
        renderTargetView = nullptr;
    }
    if (context)
    {
        context->Release();
        context = nullptr;
    }
    if (device)
    {
        device->Release();
        device = nullptr;
    }
    return ResizeOriginal(swapChain, bufferCount, width, height, newFormat, swapChainFlags);
}

inline bool Cheat::Renderer::Init()
{
    HMODULE dxgi = GetModuleHandleA("dxgi.dll");
    Logger::Log("dxgi: %p\n", dxgi);
    static BYTE PresentSig[] = { 0x55, 0x57, 0x41, 0x56, 0x48, 0x8d, 0x6c, 0x24, 0x90, 0x48, 0x81, 0xec, 0x70, 0x01 };
    fnPresent = reinterpret_cast<decltype(fnPresent)>(Tools::FindFn(dxgi, PresentSig, sizeof(PresentSig)));
    Logger::Log("IDXGISwapChain::Present: %p\n", fnPresent);
    if (!fnPresent) return false;
    static BYTE ResizeSig[] = { 0x48, 0x81, 0xec, 0xc0, 0x00, 0x00, 0x00, 0x48, 0xc7, 0x45, 0x1f };
    fnResize = reinterpret_cast<decltype(fnResize)>(Tools::FindFn(dxgi, ResizeSig, sizeof(ResizeSig)));
    Logger::Log("IDXGISwapChain::ResizeBuffers: %p\n", fnResize);
    if (!fnResize) return false;
    if (!SetHook(fnPresent, PresentHook, reinterpret_cast<void**>(&PresentOriginal)))
    {
        return false;
    };
    Logger::Log("PresentHook: %p\n", PresentHook);
    Logger::Log("PresentOriginal: %p\n", PresentOriginal);
    if (!SetHook(fnResize, ResizeHook, reinterpret_cast<void**>(&ResizeOriginal)))
    {
        return false;
    };
    Logger::Log("ResizeHook: %p\n", ResizeHook);
    Logger::Log("ResizeOriginal: %p\n", ResizeOriginal);
    if (!SetHook(SetCursorPos, SetCursorPosHook, reinterpret_cast<void**>(&SetCursorPosOriginal)))
    {
        Logger::Log("Can't hook SetCursorPos\n");
        return false;
    };
    if (!SetHook(SetCursor, SetCursorHook, reinterpret_cast<void**>(&SetCursorOriginal)))
    {
        Logger::Log("Can't hook SetCursor\n");
        return false;
    };
    return true;
}

inline bool Cheat::Renderer::Remove()
{
    Renderer::RemoveInput(); 
    if (!RemoveHook(PresentOriginal) || !RemoveHook(ResizeOriginal) || !RemoveHook(SetCursorPosOriginal) || !RemoveHook(SetCursorOriginal))
    {
        return false;
    }
    if (renderTargetView)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui::DestroyContext();
        renderTargetView->Release();
        renderTargetView = nullptr;
    }
    if (context)
    {
        context->Release();
        context = nullptr;
    }
    if (device)
    {
        device->Release();
        device = nullptr;
    }
    return true;
}

inline bool Cheat::Tools::CompareByteArray(BYTE* data, BYTE* sig, SIZE_T size)
{
    for (SIZE_T i = 0; i < size; i++) {
        if (data[i] != sig[i]) {
            if (sig[i] == 0x00) continue;
            return false;
        }
    }
    return true;
}

inline BYTE* Cheat::Tools::FindSignature(BYTE* start, BYTE* end, BYTE* sig, SIZE_T size)
{
    for (BYTE* it = start; it < end - size; it++) {
        if (CompareByteArray(it, sig, size)) {
            return it;
        };
    }
    return 0;
}

void* Cheat::Tools::FindPointer(BYTE* sig, SIZE_T size, int addition = 0)
{
    auto base = static_cast<BYTE*>(gBaseMod.lpBaseOfDll);
    auto address = FindSignature(base, base + gBaseMod.SizeOfImage - 1, sig, size);
    if (!address) return nullptr;
    auto k = 0;
    for (; sig[k]; k++);
    auto offset = *reinterpret_cast<UINT32*>(address + k);
    return address + k + 4 + offset + addition;
}

inline BYTE* Cheat::Tools::FindFn(HMODULE mod, BYTE* sig, SIZE_T sigSize)
{
    if (!mod || !sig || !sigSize) return 0;
    MODULEINFO modInfo;
    if (!K32GetModuleInformation(GetCurrentProcess(), mod, &modInfo, sizeof(MODULEINFO))) return 0;
    auto base = static_cast<BYTE*>(modInfo.lpBaseOfDll);
    auto fn = Tools::FindSignature(base, base + modInfo.SizeOfImage - 1, sig, sigSize);
    if (!fn) return 0;
    for (; *fn != 0xCC && *fn != 0xC3; fn--);
    fn++;
    return fn;
}

inline bool Cheat::Tools::PatchMem(void* address, void* bytes, SIZE_T size)
{
    DWORD oldProtection;
    if (VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtection))
    {
        memcpy(address, bytes, size);
        return VirtualProtect(address, size, oldProtection, &oldProtection);
    };
    return false;
}

inline BYTE* Cheat::Tools::PacthFn(HMODULE mod, BYTE* sig, SIZE_T sigSize, BYTE* bytes, SIZE_T bytesSize)
{
    if (!mod || !sig || !sigSize || !bytes || !bytesSize) return 0;
    auto fn = FindFn(mod, sig, sigSize);
    if (!fn) return 0;
    return Tools::PatchMem(fn, bytes, bytesSize) ? fn : 0;
}

inline bool Cheat::Tools::FindNameArray()
{
    static BYTE sig[] = { 0x48, 0x8b, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xff, 0x75, 0x3c };
    auto address = reinterpret_cast<decltype(FName::GNames)*>(FindPointer(sig, sizeof(sig)));
    if (!address) return 0;
    Logger::Log("%p\n", address);
    FName::GNames = *address;
    return FName::GNames;
}

inline bool Cheat::Tools::FindObjectsArray()
{
    static BYTE sig[] = { 0x89, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0xDF, 0x48, 0x89, 0x5C, 0x24 };
    UObject::GObjects = reinterpret_cast<decltype(UObject::GObjects)>(FindPointer(sig, sizeof(sig), 16));
    return UObject::GObjects;
}

inline bool Cheat::Tools::FindWorld()
{
    static BYTE sig[] = { 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x88, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC9, 0x74, 0x06, 0x48, 0x8B, 0x49, 0x70 };
    UWorld::GWorld = reinterpret_cast<decltype(UWorld::GWorld)>(FindPointer(sig, sizeof(sig)));
    return UWorld::GWorld;
}

inline bool Cheat::Tools::InitSDK()
{
    if (!UCrewFunctions::Init()) return false;
    if (!UKismetMathLibrary::Init()) return false;
    return true;
}

inline bool Cheat::Logger::Init()
{
    fs::path log;
#ifdef STEAM
    wchar_t buf[MAX_PATH];
    if (!GetModuleFileNameW(hinstDLL, buf, MAX_PATH)) return false;
    log = fs::path(buf).remove_filename() / "log.txt";
#else
#ifdef UWPDEBUG
    log = "C:\\Users\\Maxi\\AppData\\Local\\Packages\\Microsoft.SeaofThieves_8wekyb3d8bbwe\\TempState\\log.txt";
#else
    return true;
#endif
#endif
    file = CreateFileW(log.wstring().c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    return file != INVALID_HANDLE_VALUE;
}

inline bool Cheat::Logger::Remove()
{
    if (!file) return true;
    return CloseHandle(file);
}

void Cheat::Logger::Log(const char* format, ...)
{
#if defined STEAM || defined UWPDEBUG
    SYSTEMTIME rawtime;
    GetSystemTime(&rawtime);
    char buf[MAX_PATH];
    auto size = GetTimeFormatA(LOCALE_CUSTOM_DEFAULT, 0, &rawtime, "[HH':'mm':'ss] ", buf, MAX_PATH) - 1;
    size += snprintf(buf + size, sizeof(buf) - size, "[TID: 0x%X] ", GetCurrentThreadId());
    va_list argptr;
    va_start(argptr, format);
    size += vsnprintf(buf + size, sizeof(buf) - size, format, argptr);
#if defined LOGFILE
    WriteFile(file, buf, size, NULL, NULL);
#endif
    printf("%s", buf);
    va_end(argptr);
#endif
}

bool Cheat::Init(HINSTANCE _hinstDLL)
{
    hinstDLL = _hinstDLL;
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    if (!Logger::Init())
    {
        return false;
    };
    if (!K32GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(nullptr), &gBaseMod, sizeof(MODULEINFO)))
    {
        return false;
    };
    Logger::Log("Base address: %p\n", gBaseMod.lpBaseOfDll);
    if (!Tools::FindNameArray())
    {
        Logger::Log("Can't find NameArray!\n");
        return false;
    }
    Logger::Log("NameArray: %p\n", FName::GNames);
    if (!Tools::FindObjectsArray())
    {
        Logger::Log("Can't find ObjectsArray!\n");
        return false;
    }
    Logger::Log("ObjectsArray: %p\n", UObject::GObjects);
    if (!Tools::FindWorld())
    {
        Logger::Log("Can't find World!\n");
        return false;
    }
    Logger::Log("World: %p\n", UWorld::GWorld);
    if (!Tools::InitSDK())
    {
        Logger::Log("Can't find important objects!\n");
        return false;
    };
    if (!Renderer::Init())
    {
        Logger::Log("Can't initialize renderer\n");
        return false;
    }
    Hacks::Init();
#ifdef STEAM
    auto t = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ClearingThread), nullptr, 0, nullptr);
    if (t) CloseHandle(t);
#endif
    return true;
}

void Cheat::ClearingThread()
{
    while (true) {
        if (GetAsyncKeyState(VK_END) & 1) {
            FreeLibraryAndExitThread(hinstDLL, 0);
        }
        Sleep(20);
    }
}

bool Cheat::Remove()
{
    Logger::Log("Removing cheat...\n");
    if (!Renderer::Remove() || !Logger::Remove())
    {
        return false;
    };
    Hacks::Remove();
    FreeConsole();
    return true;
}
