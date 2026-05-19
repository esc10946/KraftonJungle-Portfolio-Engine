#include "EditorViewerWindowWidget.h"
#include "Editor/EditorEngine.h"
#include "Editor/UI/EditorChromeConstants.h"
#include "Editor/Viewer/AnimationViewer.h"
#include "Editor/Viewer/EditorViewer.h"
#include "Animation/AnimationSequenceBase.h"
#include "Asset/AnimationSequence.h"
#include "Viewport/ViewportLayout.h"
#include "GameFramework/PrimitiveActors.h"
#include "Component/SkeletalMeshComponent.h"
#include "Component/StaticMeshComponent.h"
#include "Core/ResourceManager.h"
#include "Component/GizmoComponent.h"
#include "Component/TransformProxy.h"
#include "Editor/Viewport/EditorViewportClient.h"
#include "Engine/Runtime/WindowsWindow.h"
#include "imgui.h"
#include "ImGui/imgui_impl_win32.h"

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <functional>

namespace
{
void SetOpaqueBlendStateCallback(const ImDrawList*, const ImDrawCmd* Cmd)
{
    ID3D11DeviceContext* DeviceContext = static_cast<ID3D11DeviceContext*>(Cmd->UserCallbackData);
    if (!DeviceContext)
        return;

    const float BlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    DeviceContext->OMSetBlendState(nullptr, BlendFactor, 0xffffffff);
}

bool UsesAbsoluteImGuiCoordinates()
{
    return (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
}

POINT ImGuiScreenToClientPoint(FWindowsWindow* Window, const ImVec2& Point)
{
    POINT Result =
    {
        static_cast<LONG>(std::lround(Point.x)),
        static_cast<LONG>(std::lround(Point.y))
    };
    if (Window && Window->GetHWND() && UsesAbsoluteImGuiCoordinates())
    {
        ::ScreenToClient(Window->GetHWND(), &Result);
    }
    return Result;
}

FString GetBaseFileNameWithoutExtension(const FString& Path)
{
    if (Path.empty())
    {
        return "Viewer";
    }

    const size_t SlashPos = Path.find_last_of("/\\");
    const size_t NameBegin = SlashPos == FString::npos ? 0 : SlashPos + 1;
    FString Name = Path.substr(NameBegin);

    const size_t DotPos = Name.find_last_of('.');
    if (DotPos != FString::npos && DotPos > 0)
    {
        Name = Name.substr(0, DotPos);
    }

    return Name.empty() ? "Viewer" : Name;
}

FString GetViewerAssetLabel(FEditorViewer* Viewer)
{
    return Viewer ? GetBaseFileNameWithoutExtension(Viewer->GetFileName()) : FString("Viewer");
}

void ApplyDetachedDocumentWindowClass()
{
    ImGuiWindowClass WindowClass;
    WindowClass.ClassId = 0x4A534457u; // "JSDW" - detached document window class
    WindowClass.ViewportFlagsOverrideSet =
        ImGuiViewportFlags_NoAutoMerge |
        ImGuiViewportFlags_NoDecoration;
    WindowClass.ViewportFlagsOverrideClear = ImGuiViewportFlags_NoTaskBarIcon;
    ImGui::SetNextWindowClass(&WindowClass);
}

HWND GetCurrentViewportHwnd()
{
    ImGuiViewport* Viewport = ImGui::GetWindowViewport();
    if (!Viewport)
    {
        return nullptr;
    }
    return static_cast<HWND>(Viewport->PlatformHandleRaw ? Viewport->PlatformHandleRaw : Viewport->PlatformHandle);
}

ImGui_ImplWin32_CustomChromeRect MakeChromeRect(const ImVec2& Min, const ImVec2& Max, const ImVec2& WindowPos)
{
    return ImGui_ImplWin32_CustomChromeRect{
        static_cast<int>(Min.x - WindowPos.x),
        static_cast<int>(Min.y - WindowPos.y),
        static_cast<int>(Max.x - WindowPos.x),
        static_cast<int>(Max.y - WindowPos.y)
    };
}

void AddChromeRect(ImGui_ImplWin32_CustomChromeRect* Rects, int& Count, const ImVec2& Min, const ImVec2& Max, const ImVec2& WindowPos)
{
    if (Count >= 16)
    {
        return;
    }
    Rects[Count++] = MakeChromeRect(Min, Max, WindowPos);
}

bool IsViewportMaximized(HWND Hwnd)
{
    return Hwnd && ::IsZoomed(Hwnd) != FALSE;
}

void ToggleViewportMaximize(HWND Hwnd)
{
    if (!Hwnd)
    {
        return;
    }
    ::PostMessageW(Hwnd, WM_SYSCOMMAND, IsViewportMaximized(Hwnd) ? SC_RESTORE : SC_MAXIMIZE, 0);
}

bool DrawDetachedWindowButton(
    const char* Id,
    const char* Tooltip,
    const ImVec2& Size,
    const ImVec4& HoverColor,
    const ImVec4& ActiveColor,
    const std::function<void(ImDrawList*, const ImVec2&, const ImVec2&, ImU32)>& DrawIcon)
{
    ImGui::PushID(Id);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HoverColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ActiveColor);

    const bool bClicked = ImGui::InvisibleButton("##Button", Size);
    const bool bHovered = ImGui::IsItemHovered();
    const bool bActive = ImGui::IsItemActive();
    const ImVec2 Min = ImGui::GetItemRectMin();
    const ImVec2 Max = ImGui::GetItemRectMax();
    const ImU32 BgColor = ImGui::GetColorU32(
        bActive ? ActiveColor : (bHovered ? HoverColor : ImVec4(0.0f, 0.0f, 0.0f, 0.0f)));

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    DrawList->AddRectFilled(Min, Max, BgColor, 0.0f);
    DrawIcon(DrawList, Min, Max, ImGui::GetColorU32(ImVec4(0.82f, 0.85f, 0.90f, 1.0f)));

    if (bHovered && Tooltip)
    {
        ImGui::SetTooltip("%s", Tooltip);
    }

    ImGui::PopStyleColor(3);
    ImGui::PopID();
    return bClicked;
}

constexpr uint64 MeshEditHashOffset = 14695981039346656037ull;
constexpr uint64 MeshEditHashPrime = 1099511628211ull;

uint64 HashBytes(uint64 Seed, const void* Data, size_t Size)
{
    const unsigned char* Bytes = static_cast<const unsigned char*>(Data);
    for (size_t Index = 0; Index < Size; ++Index)
    {
        Seed ^= static_cast<uint64>(Bytes[Index]);
        Seed *= MeshEditHashPrime;
    }
    return Seed;
}

template <typename T>
uint64 HashValue(uint64 Seed, const T& Value)
{
    return HashBytes(Seed, &Value, sizeof(T));
}

uint64 HashString(uint64 Seed, const FString& Value)
{
    const uint64 Length = static_cast<uint64>(Value.size());
    Seed = HashValue(Seed, Length);
    return Value.empty() ? Seed : HashBytes(Seed, Value.data(), Value.size());
}

uint64 HashMatrix(uint64 Seed, const FMatrix& Matrix)
{
    return HashBytes(Seed, Matrix.M, sizeof(Matrix.M));
}
}

void FEditorViewerWindowWidget::Initialize(UEditorEngine* InEditorEngine)
{
    FEditorWidget::Initialize(InEditorEngine);
}

void FEditorViewerWindowWidget::Shutdown()
{
    Children.clear();
    BoneToSocketIndices.clear();
    CachedMesh = nullptr;
    CachedSkComp = nullptr; 

    PendingPreviewPickerSocketIdx = -1; 
    RenameSocketIdx = -1;               
    bMeshDirty = false; 
    CleanMeshEditSignature = 0;
    bHasCleanMeshEditSignature = false;

    Viewer = nullptr;
    bOpen = false;
}

FString FEditorViewerWindowWidget::GetWindowName() const
{
    char WindowName[64];
    sprintf_s(WindowName, "###ViewerWindow_%p", Viewer);
    return GetViewerAssetLabel(Viewer) + WindowName;
}

bool FEditorViewerWindowWidget::CanSaveMesh() const
{
	if (!Viewer)
	{
		return false;
	}

	ASkeletalMeshActor* ViewTarget = Viewer->GetViewTarget();
	USkeletalMeshComponent* SkelComp = ViewTarget ? ViewTarget->GetSkeletalMeshComponent() : nullptr;
	return SkelComp && SkelComp->GetSkeletalMesh() && HasMeshAssetEdits();
}

bool FEditorViewerWindowWidget::IsMeshDirty() const
{
	return HasMeshAssetEdits();
}

void FEditorViewerWindowWidget::RequestSaveMesh()
{
	if (!Viewer)
	{
		return;
	}

	ASkeletalMeshActor* ViewTarget = Viewer->GetViewTarget();
	USkeletalMeshComponent* SkelComp = ViewTarget ? ViewTarget->GetSkeletalMeshComponent() : nullptr;
	USkeletalMesh* Mesh = SkelComp ? SkelComp->GetSkeletalMesh() : nullptr;
	if (!Mesh)
	{
		return;
	}

	if (FResourceManager::Get().SaveSkeletalMesh(Mesh))
	{
		ResetMeshDirtyBaseline();
	}
}

FSkeletalMesh* FEditorViewerWindowWidget::ResolveCurrentMeshData() const
{
	if (CachedMesh)
	{
		return CachedMesh;
	}

	if (!Viewer)
	{
		return nullptr;
	}

	ASkeletalMeshActor* ViewTarget = Viewer->GetViewTarget();
	USkeletalMeshComponent* SkelComp = ViewTarget ? ViewTarget->GetSkeletalMeshComponent() : nullptr;
	USkeletalMesh* Mesh = SkelComp ? SkelComp->GetSkeletalMesh() : nullptr;
	return Mesh ? Mesh->GetMeshData() : nullptr;
}

uint64 FEditorViewerWindowWidget::ComputeEditableMeshSignature(const FSkeletalMesh* MeshData) const
{
	if (!MeshData)
	{
		return 0;
	}

	uint64 Hash = MeshEditHashOffset;

	Hash = HashValue(Hash, static_cast<uint64>(MeshData->Bones.size()));
	for (const FBoneInfo& Bone : MeshData->Bones)
	{
		Hash = HashString(Hash, Bone.Name);
		Hash = HashValue(Hash, Bone.ParentIndex);
		Hash = HashMatrix(Hash, Bone.LocalBindTransform);
		Hash = HashMatrix(Hash, Bone.GlobalBindTransform);
		Hash = HashMatrix(Hash, Bone.InverseBindPose);
	}

	Hash = HashValue(Hash, static_cast<uint64>(MeshData->Sockets.size()));
	for (const FSkeletalMeshSocket& Socket : MeshData->Sockets)
	{
		Hash = HashString(Hash, Socket.Name.ToString());
		Hash = HashValue(Hash, Socket.BoneIndex);
		Hash = HashValue(Hash, Socket.RelativeLocation.X);
		Hash = HashValue(Hash, Socket.RelativeLocation.Y);
		Hash = HashValue(Hash, Socket.RelativeLocation.Z);
		Hash = HashValue(Hash, Socket.RelativeRotation.Pitch);
		Hash = HashValue(Hash, Socket.RelativeRotation.Yaw);
		Hash = HashValue(Hash, Socket.RelativeRotation.Roll);
		Hash = HashValue(Hash, Socket.RelativeScale.X);
		Hash = HashValue(Hash, Socket.RelativeScale.Y);
		Hash = HashValue(Hash, Socket.RelativeScale.Z);
	}

	return Hash;
}

void FEditorViewerWindowWidget::ResetMeshDirtyBaseline()
{
	FSkeletalMesh* MeshData = ResolveCurrentMeshData();
	if (!MeshData)
	{
		CleanMeshEditSignature = 0;
		bHasCleanMeshEditSignature = false;
		bMeshDirty = false;
		return;
	}

	CleanMeshEditSignature = ComputeEditableMeshSignature(MeshData);
	bHasCleanMeshEditSignature = true;
	bMeshDirty = false;
}

bool FEditorViewerWindowWidget::HasMeshAssetEdits() const
{
	FSkeletalMesh* MeshData = ResolveCurrentMeshData();
	if (!MeshData)
	{
		return false;
	}

	if (bMeshDirty)
	{
		return true;
	}

	return bHasCleanMeshEditSignature && ComputeEditableMeshSignature(MeshData) != CleanMeshEditSignature;
}

void FEditorViewerWindowWidget::Render(float DeltaTime)
{
    if (!bOpen)
        return;

    if (!EditorEngine)
        return;

	if (!Viewer)
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(13.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(9.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.055f, 0.060f, 0.072f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.055f, 0.060f, 0.072f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.20f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.15f, 0.17f, 0.22f, 1.0f));

	FString WindowName = GetWindowName();
	bool bDockRequested = false;
	bool bCloseRequested = false;

    // Detached document는 borderless secondary viewport로 띄우고,
    // Win32 backend에 titlebar hit-test 정보를 넘겨 native window처럼 움직이게 한다.
    ApplyDetachedDocumentWindowClass();
	// Make the viewer window reasonably large on first creation.
	ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
    if (const ImGuiViewport* MainViewport = ImGui::GetMainViewport())
    {
        ImGui::SetNextWindowPos(
            ImVec2(MainViewport->Pos.x + 120.0f, MainViewport->Pos.y + 90.0f),
            ImGuiCond_FirstUseEver);
    }
	constexpr ImGuiWindowFlags WindowFlags =
		ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse;
	if (ImGui::Begin(WindowName.c_str(), &bOpen, WindowFlags))
	{
        RenderDetachedDocumentChrome(bDockRequested, bCloseRequested);
        RenderDetachedDocumentToolbar(bDockRequested);
		RenderContent(DeltaTime);
	}
	ImGui::End();

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(5);

	if (bDockRequested)
	{
		EditorEngine->GetMainPanel().RequestDockViewer(Viewer);
		return;
	}
	if (bCloseRequested)
	{
		bOpen = false;
	}

	if (!bOpen)
    {
        EditorEngine->RemoveViewer(Viewer);
        Shutdown();
    }
}

void FEditorViewerWindowWidget::RenderDetachedDocumentChrome(bool& bDockRequested, bool& bCloseRequested)
{
    if (!Viewer || !ImGui::BeginMenuBar())
    {
        return;
    }

    constexpr float WindowButtonWidth = 48.0f;
    constexpr float TitleBarHeight = FEditorChromeMetrics::ApplicationTitleBarHeight;
    constexpr float LogoSize = 28.0f;
    constexpr float LogoPaddingX = 4.0f;
    constexpr float MenuLogoGap = 8.0f;
    constexpr float MenuStartX = LogoPaddingX + LogoSize + MenuLogoGap;

    HWND ViewportHwnd = GetCurrentViewportHwnd();
    const ImVec2 WindowPos = ImGui::GetWindowPos();
    const ImVec2 WindowSize = ImGui::GetWindowSize();
    const float ButtonStartX = std::max(0.0f, WindowSize.x - WindowButtonWidth * 3.0f);

    ImGui_ImplWin32_CustomChromeRect ChromeRects[16] = {};
    int ChromeRectCount = 0;

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    ID3D11ShaderResourceView* HomeIcon = EditorEngine ? EditorEngine->GetMainPanel().GetHomeIconResource() : nullptr;
    const ImVec2 LogoMin(WindowPos.x + LogoPaddingX, WindowPos.y + (TitleBarHeight - LogoSize) * 0.5f);
    const ImVec2 LogoMax(LogoMin.x + LogoSize, LogoMin.y + LogoSize);
    if (HomeIcon)
    {
        DrawList->AddImage(reinterpret_cast<ImTextureID>(HomeIcon), LogoMin, LogoMax);
    }
    else
    {
        DrawList->AddRectFilled(LogoMin, LogoMax, ImGui::GetColorU32(ImVec4(0.95f, 0.78f, 0.12f, 1.0f)), 0.0f);
        DrawList->AddText(
            ImVec2(LogoMin.x + 4.0f, LogoMin.y + 5.0f),
            ImGui::GetColorU32(ImVec4(0.08f, 0.09f, 0.11f, 1.0f)),
            "JS");
    }
    AddChromeRect(ChromeRects, ChromeRectCount, LogoMin, LogoMax, WindowPos);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 8.0f));

    ImGui::SetCursorPosX(MenuStartX);

    const bool bCanSaveMesh = CanSaveMesh();
    const char* SaveMeshLabel = IsMeshDirty() ? "Save Mesh *" : "Save Mesh";

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem(SaveMeshLabel, "Ctrl+S", false, bCanSaveMesh))
        {
            RequestSaveMesh();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Close"))
        {
            bCloseRequested = true;
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit"))
    {
        ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
        ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, false);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Asset"))
    {
        if (ImGui::MenuItem(SaveMeshLabel, nullptr, false, bCanSaveMesh))
        {
            RequestSaveMesh();
        }
        ImGui::MenuItem("Reimport Mesh", nullptr, false, false);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Window"))
    {
        if (ImGui::MenuItem("Dock Back"))
        {
            bDockRequested = true;
        }
        if (ImGui::MenuItem("Close"))
        {
            bCloseRequested = true;
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Tools"))
    {
        FSkeletalViewerShowFlags& ShowFlags = Viewer->GetClient().GetShowFlags();
        ImGui::MenuItem("Bones", nullptr, &ShowFlags.bShowBones);
        ImGui::MenuItem("Bounding Box", nullptr, &ShowFlags.bShowBoundingBox);
        ImGui::MenuItem("Outline", nullptr, &ShowFlags.bShowOutline);
        ImGui::Separator();
        ImGui::MenuItem("Bone Weight Heatmap", nullptr, &ShowFlags.bShowBoneWeightHeatmap);
        ImGui::BeginDisabled(!ShowFlags.bShowBoneWeightHeatmap);
        if (ImGui::SliderFloat("Opacity", &ShowFlags.BoneWeightHeatmapOpacity, 0.05f, 1.0f, "%.2f"))
        {
            ShowFlags.BoneWeightHeatmapOpacity = std::clamp(ShowFlags.BoneWeightHeatmapOpacity, 0.05f, 1.0f);
        }
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help"))
    {
        ImGui::TextDisabled("Skeletal Mesh Previewer");
        ImGui::EndMenu();
    }

    const float MenuEndX = std::min(ButtonStartX, ImGui::GetCursorScreenPos().x - WindowPos.x + 8.0f);
    AddChromeRect(
        ChromeRects,
        ChromeRectCount,
        ImVec2(WindowPos.x, WindowPos.y),
        ImVec2(WindowPos.x + MenuEndX, WindowPos.y + TitleBarHeight),
        WindowPos);

    const FString AssetLabel = GetViewerAssetLabel(Viewer);
    const ImVec2 TitleSize = ImGui::CalcTextSize(AssetLabel.c_str());
    const float TitleX = std::clamp(
        MenuEndX + (ButtonStartX - MenuEndX - TitleSize.x) * 0.5f,
        MenuEndX + 8.0f,
        std::max(MenuEndX + 8.0f, ButtonStartX - TitleSize.x - 8.0f));
    DrawList->AddText(
        ImVec2(WindowPos.x + TitleX, WindowPos.y + (TitleBarHeight - TitleSize.y) * 0.5f),
        ImGui::GetColorU32(ImVec4(0.72f, 0.76f, 0.84f, 1.0f)),
        AssetLabel.c_str());

    const ImVec2 ButtonSize(WindowButtonWidth, TitleBarHeight);
    ImGui::SetCursorPos(ImVec2(ButtonStartX, 0.0f));
    if (DrawDetachedWindowButton(
        "DetachedMinimize",
        "Minimize",
        ButtonSize,
        ImVec4(0.14f, 0.16f, 0.20f, 1.0f),
        ImVec4(0.18f, 0.20f, 0.25f, 1.0f),
        [](ImDrawList* InDrawList, const ImVec2& Min, const ImVec2& Max, ImU32 Color)
        {
            const float Y = (Min.y + Max.y) * 0.5f + 4.0f;
            InDrawList->AddLine(ImVec2(Min.x + 17.0f, Y), ImVec2(Max.x - 17.0f, Y), Color, 1.6f);
        }))
    {
        if (ViewportHwnd)
        {
            ::PostMessageW(ViewportHwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        }
    }
    AddChromeRect(ChromeRects, ChromeRectCount, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), WindowPos);

    ImGui::SameLine(0.0f, 0.0f);
    if (DrawDetachedWindowButton(
        "DetachedMaximize",
        IsViewportMaximized(ViewportHwnd) ? "Restore" : "Maximize",
        ButtonSize,
        ImVec4(0.14f, 0.16f, 0.20f, 1.0f),
        ImVec4(0.18f, 0.20f, 0.25f, 1.0f),
        [ViewportHwnd](ImDrawList* InDrawList, const ImVec2& Min, const ImVec2& Max, ImU32 Color)
        {
            const bool bMaximized = IsViewportMaximized(ViewportHwnd);
            const ImVec2 A(Min.x + 17.0f, Min.y + 12.0f);
            const ImVec2 B(Max.x - 17.0f, Max.y - 12.0f);
            if (bMaximized)
            {
                InDrawList->AddRect(ImVec2(A.x + 3.0f, A.y), ImVec2(B.x + 3.0f, B.y - 3.0f), Color, 0.0f, 0, 1.4f);
                InDrawList->AddRect(ImVec2(A.x, A.y + 3.0f), ImVec2(B.x, B.y), Color, 0.0f, 0, 1.4f);
            }
            else
            {
                InDrawList->AddRect(A, B, Color, 0.0f, 0, 1.4f);
            }
        }))
    {
        ToggleViewportMaximize(ViewportHwnd);
    }
    AddChromeRect(ChromeRects, ChromeRectCount, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), WindowPos);

    ImGui::SameLine(0.0f, 0.0f);
    if (DrawDetachedWindowButton(
        "DetachedClose",
        "Close",
        ButtonSize,
        ImVec4(0.62f, 0.18f, 0.20f, 1.0f),
        ImVec4(0.46f, 0.10f, 0.13f, 1.0f),
        [](ImDrawList* InDrawList, const ImVec2& Min, const ImVec2& Max, ImU32 Color)
        {
            InDrawList->AddLine(ImVec2(Min.x + 17.0f, Min.y + 12.0f), ImVec2(Max.x - 17.0f, Max.y - 12.0f), Color, 1.6f);
            InDrawList->AddLine(ImVec2(Max.x - 17.0f, Min.y + 12.0f), ImVec2(Min.x + 17.0f, Max.y - 12.0f), Color, 1.6f);
        }))
    {
        bCloseRequested = true;
    }
    AddChromeRect(ChromeRects, ChromeRectCount, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), WindowPos);

    ImGui_ImplWin32_SetCustomChrome(ViewportHwnd, static_cast<int>(TitleBarHeight), ChromeRects, ChromeRectCount);
    ImGui::PopStyleVar(3);
    ImGui::EndMenuBar();
}

void FEditorViewerWindowWidget::RenderDetachedDocumentToolbar(bool& bDockRequested)
{
    if (!Viewer || !EditorEngine)
    {
        return;
    }

    constexpr ImGuiWindowFlags ToolbarFlags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::BeginChild("##DetachedViewerToolbar", ImVec2(0.0f, 40.0f), false, ToolbarFlags);
    ImGui::SetCursorPos(ImVec2(8.0f, 6.0f));

    const bool bCanSaveMesh = CanSaveMesh();
    if (!bCanSaveMesh)
    {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(IsMeshDirty() ? "Save *" : "Save"))
    {
        RequestSaveMesh();
    }
    if (!bCanSaveMesh)
    {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Dock"))
    {
        bDockRequested = true;
    }
    ImGui::SameLine(0.0f, 12.0f);
    EditorEngine->GetMainPanel().RenderViewerToolbarControls(Viewer);
    ImGui::EndChild();
}

void FEditorViewerWindowWidget::RenderEmbedded(float DeltaTime)
{
	if (!bOpen || !EditorEngine || !Viewer)
	{
		return;
	}

	RenderContent(DeltaTime);
}

void FEditorViewerWindowWidget::RenderContent(float DeltaTime)
{
	(void)DeltaTime;

	FSceneViewport& SceneViewport = Viewer->GetViewport();

	ID3D11ShaderResourceView* SRV = SceneViewport.GetOutSRV();

    if (!SRV)
	{
		ImGui::TextDisabled("Viewer render target is not ready.");
        return;
	}

	ImVec2 FullSize = ImGui::GetContentRegionAvail();

	float CenterWidth = std::max(160.0f, FullSize.x - LeftPanelWidth - RightPanelWidth - (ImGui::GetStyle().ItemSpacing.x * 2.0f));
    float CenterHeight = std::max(160.0f, FullSize.y - DownPanelHeight - (ImGui::GetStyle().ItemSpacing.y));

		// =====================================================
		// LEFT: Skeleton Tree
		// =====================================================
		ImGui::BeginChild("SkeletonPanel", ImVec2(LeftPanelWidth, 0), true);

		ImGui::Text("Skeleton");

		ASkeletalMeshActor* ViewTarget = Viewer->GetViewTarget();
		USkeletalMeshComponent* SkelMeshComp = ViewTarget ? ViewTarget->GetSkeletalMeshComponent() : nullptr;
		USkeletalMesh* SkeletalMesh = SkelMeshComp ? SkelMeshComp->GetSkeletalMesh() : nullptr;
		FSkeletalMesh* MeshData = SkeletalMesh ? SkeletalMesh->GetMeshData() : nullptr;

		// 헬퍼들이 참조할 transient 캐시 (Render 호출 범위에서만 유효)
		CachedSkComp = SkelMeshComp;

		if (!MeshData)
		{
			CachedMesh = nullptr;
			Children.clear();
			BoneToSocketIndices.clear();
			if (Viewer)
			{
				Viewer->ClearSelection();
			}
			ResetMeshDirtyBaseline();
			ImGui::TextDisabled("No skeletal mesh");
		}
		else if (CachedMesh != MeshData)
		{
			CachedMesh = MeshData;
			if (Viewer)
			{
				Viewer->ClearSelection();
			}

			RebuildBoneTreeCaches(MeshData);
			ResetMeshDirtyBaseline();
		}

		if (MeshData)
		{
            ApplyPendingBoneTreeOpenState(MeshData);
			for (int32 j = 0; j < MeshData->Bones.size(); ++j)
			{
				if (MeshData->Bones[j].ParentIndex == -1)
				{
					DrawBoneNode(j, MeshData->Bones, Children);
				}
			}
		}

		// 컨텍스트 메뉴에서 PendingPreviewPickerSocketIdx가 셋되면 모달 오픈.
		// (BeginPopupContextItem 안에서 OpenPopup하지 않고 *바깥*에서 하는 것이 안전 — popup 스택 충돌 방지)
		if (PendingPreviewPickerSocketIdx >= 0 && !ImGui::IsPopupOpen("PickStaticMesh"))
		{
			ImGui::OpenPopup("PickStaticMesh");
		}
		DrawPreviewPickerModal();

		if (RenameSocketIdx >= 0 && !ImGui::IsPopupOpen("RenameSocket"))
		{
			ImGui::OpenPopup("RenameSocket");
		}
		DrawRenameModal();

		// 선택된 socket의 properties 편집기 + Save 버튼 (좌측 패널 하단)
		ImGui::Separator();
		DrawSocketInspector();

		ImGui::EndChild();

        // Left Splitter
        ImGui::SameLine();
        ImGui::Button("##left_splitter", ImVec2(2.0f, -1.0f));
        if (ImGui::IsItemActive())
        {
            LeftPanelWidth += ImGui::GetIO().MouseDelta.x;
            LeftPanelWidth = std::clamp(LeftPanelWidth, 100.0f, FullSize.x * 0.4f);
        }
        ImGui::SameLine();

		// =====================================================
		// CENTER: Viewport
		// =====================================================

        ImGui::BeginChild("CenterPanel", ImVec2(CenterWidth, 0), false);
        {
        ImGui::BeginChild("ViewportPanel", ImVec2(0, CenterHeight), false);

		ImVec2 Size = ImGui::GetContentRegionAvail();

		Size.x = std::max(Size.x, 1.0f);
		Size.y = std::max(Size.y, 1.0f);

		ImGui::Dummy(Size);
        ImVec2 Min = ImGui::GetItemRectMin();
        ImVec2 Max = ImGui::GetItemRectMax();
		const POINT ClientMin = ImGuiScreenToClientPoint(EditorEngine ? EditorEngine->GetWindow() : nullptr, Min);
		const bool bViewportHovered = ImGui::IsItemHovered();
		const bool bViewportClicked =
			bViewportHovered &&
			(ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
			 ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
			 ImGui::IsMouseClicked(ImGuiMouseButton_Middle));

		FViewportRect NewRect;
        NewRect.X = (int32)ClientMin.x;
        NewRect.Y = (int32)ClientMin.y;
        NewRect.Width = (int32)(Max.x - Min.x);
        NewRect.Height = (int32)(Max.y - Min.y);

		SceneViewport.SetRect(NewRect);

		if (auto* Client = SceneViewport.GetClient())
		{
			Client->SetViewportSize((float)NewRect.Width, (float)NewRect.Height);
		}
		if (bViewportClicked)
		{
			EditorEngine->FocusViewportInput(&SceneViewport);
		}

		ImDrawList* DrawList = ImGui::GetWindowDrawList();

		ID3D11DeviceContext* DC =
			EditorEngine->GetRenderer().GetFD3DDevice().GetDeviceContext();

		DrawList->AddCallback(SetOpaqueBlendStateCallback, DC);

		// Render viewport
        DrawList->AddImage((ImTextureID)SRV, Min, Max);

		DrawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

		ImGui::EndChild();

        ImGui::BeginChild("AnimationSequencePanel", ImVec2(0, DownPanelHeight), true);
        {
            RenderAnimationSequencePanelContent(
                dynamic_cast<FAnimationViewer*>(Viewer),
                TimelineState);
        }
        ImGui::EndChild();

        }
        ImGui::EndChild();

        // Right Splitter
        ImGui::SameLine();
        ImGui::Button("##right_splitter", ImVec2(2.0f, -1.0f));
        if (ImGui::IsItemActive())
        {
            RightPanelWidth -= ImGui::GetIO().MouseDelta.x;
            RightPanelWidth = std::clamp(RightPanelWidth, 100.0f, FullSize.x * 0.4f);
        }

        ImGui::SameLine();

		// =====================================================
		// RIGHT: Bone Details
		// =====================================================
		ImGui::BeginChild("BoneDetailsPanel", ImVec2(RightPanelWidth, 0), true);
		ImGui::Text("Details");
		ImGui::Separator();
		if (Viewer->GetSelectedBoneIndex() != -1 && SkelMeshComp)
		{
			RenderBoneDetails(SkelMeshComp);
		}
        else if (Viewer->GetSelectedSocketIndex() != -1 && SkelMeshComp)
        {
            // Socket details (Location, Rotation, Scale already in Left Panel, but showing something here is good)
            if (CachedMesh && Viewer->GetSelectedSocketIndex() < (int32)CachedMesh->Sockets.size())
            {
                ImGui::Text("Socket: %s", CachedMesh->Sockets[Viewer->GetSelectedSocketIndex()].Name.ToString().c_str());
                ImGui::Separator();
                ImGui::Text("Selected Socket for transformation.");
            }
        }
		else
		{
			ImGui::TextDisabled("No bone or socket selected.");
		}
	ImGui::EndChild();
}

void FEditorViewerWindowWidget::RenderAnimationSequencePanelContent(
    FAnimationViewer* AnimationViewer,
    FAnimTimelineUIState& State)
{
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const float PanelWidth = ImGui::GetContentRegionAvail().x;
    const float PanelHeight = ImGui::GetContentRegionAvail().y;

    const float TopBarHeight = 58.0f;
    const float HeaderHeight = 24.0f;
    const float RowHeight = 24.0f;
    constexpr float FPS = 30.0f;

    const FString EmptyAnimPath;
    const FString& AnimPath = AnimationViewer ? AnimationViewer->GetAnimationSequencePath() : EmptyAnimPath;
    UAnimationSequence* CurrentSequence = AnimationViewer ? AnimationViewer->GetAnimationSequence() : nullptr;
    const TArray<FAnimNotifyEvent>* Notifies = CurrentSequence ? &CurrentSequence->GetNotifies() : nullptr;
    const char* AnimName = AnimPath.empty() ? "None" : AnimPath.c_str();
    const float CurrentTime = AnimationViewer ? AnimationViewer->GetAnimationTime() : 0.0f;
    const float Duration = AnimationViewer ? AnimationViewer->GetAnimationLength() : 0.0f;
    const int32 TotalFrames = Duration > 0.0f
        ? std::max(1, static_cast<int32>(std::ceil(Duration * FPS)))
        : 0;
    const bool bHasAnimation = AnimationViewer && Duration > 0.0f;
    if (bHasAnimation && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        State.CurrentFrame = std::clamp(
            static_cast<int32>(std::round(CurrentTime * FPS)),
            0,
            TotalFrames);
    }

    // =====================================================
    // TOP BAR
    // =====================================================
    ImGui::BeginChild("AnimSeqTopBar", ImVec2(0, TopBarHeight), false);
    {
        ImGui::Text("Preview:");

        ImGui::SameLine();
        const char* AnimationLabel = AnimName;
        ImGui::SetNextItemWidth(std::max(180.0f, ImGui::CalcTextSize(AnimationLabel).x + 42.0f));
        if (ImGui::BeginCombo("##AnimationSequenceCombo", AnimationLabel))
        {
            const TArray<FString>& AnimationPaths = EditorEngine->GetAssetService().GetAnimationSequenceAssetPaths();
            bool bHasCompatibleAnimation = false;

            if (AnimationViewer)
            {
                for (const FString& AnimationPath : AnimationPaths)
                {
                    if (!AnimationViewer->IsAnimationSequenceCompatible(AnimationPath))
                    {
                        continue;
                    }

                    bHasCompatibleAnimation = true;
                    const bool bSelected = AnimationPath == AnimPath;
                    if (ImGui::MenuItem(AnimationPath.c_str(), nullptr, bSelected))
                    {
                        AnimationViewer->SetAnimationSequence(AnimationPath);
                        State.CurrentFrame = 0;
                        State.SelectedNotifyIndex = -1;
                    }
                }
            }

            if (AnimationPaths.empty() || !bHasCompatibleAnimation)
            {
                ImGui::TextDisabled("No compatible animation sequences");
            }

            if (AnimationViewer && !AnimPath.empty())
            {
                ImGui::Separator();
                if (ImGui::MenuItem("Clear"))
                {
                    AnimationViewer->ClearAnimationSequence();
                    State.CurrentFrame = 0;
                    State.SelectedNotifyIndex = -1;
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        ImGui::Text("Frame: %d / %d", State.CurrentFrame, TotalFrames);

        ImGui::SameLine();
        ImGui::Text("Time: %.3f / %.3f", CurrentTime, Duration);

        ImGui::SameLine();
        ImGui::Text("FPS: %.1f", FPS);

        ImGui::SameLine();

        if (!bHasAnimation)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button(AnimationViewer && AnimationViewer->IsAnimationPlaying() && !AnimationViewer->IsAnimationPaused() ? "Pause" : "Play"))
        {
            if (AnimationViewer && AnimationViewer->IsAnimationPlaying() && !AnimationViewer->IsAnimationPaused())
            {
                AnimationViewer->PauseAnimation();
            }
            else if (AnimationViewer)
            {
                AnimationViewer->PlayAnimation();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Stop"))
        {
            if (AnimationViewer)
            {
                AnimationViewer->StopAnimation();
            }
            State.CurrentFrame = 0;
        }

        if (!bHasAnimation)
        {
            ImGui::EndDisabled();
        }

        if (!AnimationViewer)
        {
            ImGui::BeginDisabled();
        }

        ImGui::SameLine();
        bool bLooping = AnimationViewer ? AnimationViewer->IsLooping() : false;
        if (ImGui::Checkbox("Loop", &bLooping) && AnimationViewer)
        {
            AnimationViewer->SetLooping(bLooping);
        }

        ImGui::SameLine();
        bool bReversePlay = AnimationViewer ? AnimationViewer->IsReversePlaying() : false;
        if (ImGui::Checkbox("Reverse", &bReversePlay) && AnimationViewer)
        {
            AnimationViewer->SetReversePlay(bReversePlay);
        }

        ImGui::SameLine();
        float PlayRate = AnimationViewer ? AnimationViewer->GetPlayRate() : 1.0f;
        ImGui::SetNextItemWidth(90.0f);
        if (ImGui::DragFloat("SpeedRate", &PlayRate, 0.01f, 0.001f, 4.0f, "%.2fx") && AnimationViewer)
        {
            AnimationViewer->SetPlayRate(PlayRate);
        }

        if (!AnimationViewer)
        {
            ImGui::EndDisabled();
        }
    }
    ImGui::EndChild();

    ImGui::Separator();

    // =====================================================
    // LEFT TRACK LIST
    // =====================================================
    ImGui::BeginChild("AnimTrackList", ImVec2(State.TrackListWidth, 0), true);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

        char Filter[128] = {};
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##AnimTrackFilter", "Filter", Filter, sizeof(Filter));

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Notifies", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputTextWithHint("##AnimNotifyName", "Notify name", State.NotifyNameBuffer, sizeof(State.NotifyNameBuffer));

            if (!CurrentSequence)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("Add Notify At Current Time", ImVec2(-1.0f, 0.0f)) && CurrentSequence)
            {
                FAnimNotifyEvent Notify;
                Notify.TriggerTime = std::clamp(CurrentTime, 0.0f, Duration);
                Notify.Duration = 0.0f;
                Notify.NotifyName = FName(State.NotifyNameBuffer);
                CurrentSequence->AddNotify(Notify);
                if (Notifies)
                {
                    State.SelectedNotifyIndex = static_cast<int32>(Notifies->size()) - 1;
                }
            }

            if (!CurrentSequence)
            {
                ImGui::EndDisabled();
            }

            ImGui::Separator();

            const int32 NotifyCount = Notifies ? static_cast<int32>(Notifies->size()) : 0;
            if (State.SelectedNotifyIndex >= NotifyCount)
            {
                State.SelectedNotifyIndex = -1;
            }

            const bool bDeleteNotifyDisabled = State.SelectedNotifyIndex < 0 || !CurrentSequence;
            if (bDeleteNotifyDisabled)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("Delete Selected Notify", ImVec2(-1.0f, 0.0f)) && CurrentSequence)
            {
                if (CurrentSequence->RemoveNotifyAt(State.SelectedNotifyIndex))
                {
                    State.SelectedNotifyIndex = -1;
                }
            }

            if (bDeleteNotifyDisabled)
            {
                ImGui::EndDisabled();
            }

            ImGui::Separator();

            if (Notifies && !Notifies->empty())
            {
                for (int32 NotifyIndex = 0; NotifyIndex < static_cast<int32>(Notifies->size()); ++NotifyIndex)
                {
                    const FAnimNotifyEvent& Notify = (*Notifies)[NotifyIndex];
                    FString NotifyLabel = Notify.NotifyName.IsValid() ? Notify.NotifyName.ToString() : "<None>";
                    NotifyLabel += " @ ";
                    NotifyLabel += std::to_string(Notify.TriggerTime);
                    if (ImGui::Selectable(NotifyLabel.c_str(), State.SelectedNotifyIndex == NotifyIndex))
                    {
                        State.SelectedNotifyIndex = NotifyIndex;
                    }
                }
            }
            else
            {
                ImGui::TextDisabled("No notifies");
            }
        }

        if (ImGui::CollapsingHeader("Curves", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Selectable("Curve_0", false);
        }

        if (ImGui::CollapsingHeader("Additive Layer Tracks", ImGuiTreeNodeFlags_DefaultOpen))
        {
        }

        if (ImGui::CollapsingHeader("Attributes", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::TreeNodeEx("pelvis", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::TreePop();
            }
        }

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    // =====================================================
    // SPLITTER
    // =====================================================
    ImGui::SameLine();

    ImGui::Button("##AnimTimelineSplitter", ImVec2(2.0f, -1.0f));
    if (ImGui::IsItemActive())
    {
        State.TrackListWidth += ImGui::GetIO().MouseDelta.x;
        State.TrackListWidth = std::clamp(State.TrackListWidth, 180.0f, PanelWidth * 0.5f);
    }

    ImGui::SameLine();

    // =====================================================
    // RIGHT TIMELINE
    // =====================================================
    ImGui::BeginChild(
        "AnimTimeline",
        ImVec2(0, 0),
        true,
        ImGuiWindowFlags_HorizontalScrollbar);
    {
        ImVec2 TimelineOrigin = ImGui::GetCursorScreenPos();
        ImVec2 Available = ImGui::GetContentRegionAvail();

        const float TimelineContentWidth =
            std::max(Available.x, TotalFrames * State.FrameWidth + 80.0f);

        const float TimelineContentHeight =
            std::max(Available.y, HeaderHeight + RowHeight * 8.0f);

        ImGui::InvisibleButton(
            "##AnimTimelineCanvas",
            ImVec2(TimelineContentWidth, TimelineContentHeight),
            ImGuiButtonFlags_MouseButtonLeft);

        const bool bHovered = ImGui::IsItemHovered();
        const bool bActive = ImGui::IsItemActive();

        ImVec2 CanvasMin = TimelineOrigin;
        ImVec2 CanvasMax = ImVec2(
            CanvasMin.x + TimelineContentWidth,
            CanvasMin.y + TimelineContentHeight);

        // Background
        DrawList->AddRectFilled(
            CanvasMin,
            CanvasMax,
            IM_COL32(24, 24, 24, 255));

        // Header background
        DrawList->AddRectFilled(
            CanvasMin,
            ImVec2(CanvasMax.x, CanvasMin.y + HeaderHeight),
            IM_COL32(34, 34, 34, 255));

        // Rows
        for (int32 Row = 0; Row < 8; ++Row)
        {
            float Y0 = CanvasMin.y + HeaderHeight + Row * RowHeight;
            float Y1 = Y0 + RowHeight;

            ImU32 RowColor = (Row % 2 == 0)
                                 ? IM_COL32(28, 28, 28, 255)
                                 : IM_COL32(32, 32, 32, 255);

            DrawList->AddRectFilled(
                ImVec2(CanvasMin.x, Y0),
                ImVec2(CanvasMax.x, Y1),
                RowColor);

            DrawList->AddLine(
                ImVec2(CanvasMin.x, Y1),
                ImVec2(CanvasMax.x, Y1),
                IM_COL32(48, 48, 48, 255));
        }

        // Frame grid + numbers
        for (int32 Frame = 0; Frame <= TotalFrames; ++Frame)
        {
            const float X = CanvasMin.x + Frame * State.FrameWidth;

            const bool bMajorFrame = Frame % 5 == 0;

            // 간격이 좁으면 5의 배수만 숫자 표시
            const bool bShowFrameNumber =
                State.FrameWidth >= 24.0f || bMajorFrame;

            ImU32 LineColor = bMajorFrame
                                  ? IM_COL32(82, 82, 82, 255)
                                  : IM_COL32(45, 45, 45, 255);

            const float LineThickness = bMajorFrame ? 1.2f : 1.0f;

            DrawList->AddLine(
                ImVec2(X, CanvasMin.y),
                ImVec2(X, CanvasMax.y),
                LineColor,
                LineThickness);

            // 위쪽 작은 tick
            const float TickHeight = bMajorFrame ? 10.0f : 4.0f;

            DrawList->AddLine(
                ImVec2(X, CanvasMin.y),
                ImVec2(X, CanvasMin.y + TickHeight),
                IM_COL32(180, 180, 180, 255),
                1.0f);

            if (bShowFrameNumber)
            {
                char Buffer[32];
                snprintf(Buffer, sizeof(Buffer), "%d", Frame);

                DrawList->AddText(
                    ImVec2(X + 4.0f, CanvasMin.y + 4.0f),
                    IM_COL32(220, 220, 220, 255),
                    Buffer);
            }
        }

        const float SafeFPS = FPS > 0.0f ? FPS : 30.0f;

        struct FTimelineKeyRect
        {
            ImVec2 Min;
            ImVec2 Max;

            bool Contains(const ImVec2& Point) const
            {
                return Point.x >= Min.x && Point.x <= Max.x && Point.y >= Min.y && Point.y <= Max.y;
            }
        };

        auto DrawKey = [&](float Time, int32 Row, ImU32 Color, const char* Label) -> FTimelineKeyRect
        {
            float Frame = Time * SafeFPS;
            float X = CanvasMin.x + Frame * State.FrameWidth;
            float Y = CanvasMin.y + HeaderHeight + Row * RowHeight;
            FTimelineKeyRect KeyRect{
                ImVec2(X - 5.0f, Y + 4.0f),
                ImVec2(X + 5.0f, Y + RowHeight - 4.0f)
            };

            DrawList->AddRectFilled(
                KeyRect.Min,
                KeyRect.Max,
                Color,
                3.0f);

            if (Label && State.FrameWidth >= 16.0f)
            {
                DrawList->AddText(
                    ImVec2(X + 7.0f, Y + 4.0f),
                    IM_COL32(235, 205, 160, 255),
                    Label);
            }

            return KeyRect;
        };

        bool bNotifyMarkerClicked = false;
        if (Notifies)
        {
            for (int32 NotifyIndex = 0; NotifyIndex < static_cast<int32>(Notifies->size()); ++NotifyIndex)
            {
                const FAnimNotifyEvent& Notify = (*Notifies)[NotifyIndex];
                if (!Notify.NotifyName.IsValid())
                {
                    continue;
                }

                const bool bSelectedNotify = State.SelectedNotifyIndex == NotifyIndex;
                const FTimelineKeyRect KeyRect = DrawKey(
                    std::clamp(Notify.TriggerTime, 0.0f, Duration),
                    0,
                    bSelectedNotify ? IM_COL32(255, 195, 76, 255) : IM_COL32(220, 120, 64, 255),
                    Notify.NotifyName.ToString().c_str());

                if (bHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && KeyRect.Contains(ImGui::GetIO().MousePos))
                {
                    State.SelectedNotifyIndex = NotifyIndex;
                    bNotifyMarkerClicked = true;
                }
            }
        }

        // Scrubber / playhead
        float CurrentFrameFloat = CurrentTime * SafeFPS;
        CurrentFrameFloat = std::clamp(CurrentFrameFloat, 0.0f, static_cast<float>(TotalFrames));

        const int32 DisplayFrame = static_cast<int32>(std::floor(CurrentFrameFloat));
        const float FrameAlpha = CurrentFrameFloat - static_cast<float>(DisplayFrame);

        const float X0 = CanvasMin.x + DisplayFrame * State.FrameWidth;
        const float X1 = CanvasMin.x + (DisplayFrame + 1) * State.FrameWidth;
        const float PlayheadX = std::lerp(X0, X1, FrameAlpha);

        DrawList->AddRectFilled(
            ImVec2(PlayheadX - 6.0f, CanvasMin.y),
            ImVec2(PlayheadX + 6.0f, CanvasMin.y + HeaderHeight),
            IM_COL32(200, 90, 40, 255),
            2.0f);

        DrawList->AddLine(
            ImVec2(PlayheadX, CanvasMin.y),
            ImVec2(PlayheadX, CanvasMax.y),
            IM_COL32(230, 230, 230, 255),
            1.5f);

        char FrameText[64];
        snprintf(
            FrameText,
            sizeof(FrameText),
            "%d + %.2f",
            DisplayFrame,
            FrameAlpha);

        DrawList->AddText(
            ImVec2(PlayheadX + 8.0f, CanvasMin.y - 6.0f),
            IM_COL32(255, 255, 255, 255),
            FrameText);

        auto SetTimeFromMouseX = [&]()
        {
            const float MouseX = ImGui::GetIO().MousePos.x;

            float NewFrameFloat = (MouseX - CanvasMin.x) / State.FrameWidth;
            NewFrameFloat = std::clamp(NewFrameFloat, 0.0f, static_cast<float>(TotalFrames));

            const float NewTime = NewFrameFloat / SafeFPS;
            State.CurrentFrame = static_cast<int32>(std::round(NewFrameFloat));
            AnimationViewer->SetAnimationTime(NewTime);
        };

        if (bHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !bNotifyMarkerClicked)
        {
            SetTimeFromMouseX();
        }

        if (bActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            SetTimeFromMouseX();
        }

        // Ctrl + Wheel zoom
        if (bHovered && ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0.0f)
        {
            State.FrameWidth += ImGui::GetIO().MouseWheel * 2.0f;
            State.FrameWidth = std::clamp(State.FrameWidth, 8.0f, 80.0f);
        }
    }
    ImGui::EndChild();
}

void FEditorViewerWindowWidget::RenderBoneDetails(USkeletalMeshComponent* SkelComp)
{
    const int32 SelectedBoneIndex = Viewer ? Viewer->GetSelectedBoneIndex() : -1;
    if (!SkelComp || SelectedBoneIndex == -1) return;

    const FBoneInfo& Bone = SkelComp->GetSkeletalMesh()->GetMeshData()->Bones[SelectedBoneIndex];
    ImGui::Text("Bone: %s (Index: %d)", Bone.Name.c_str(), SelectedBoneIndex);
    ImGui::Spacing();

    FMatrix LocalTransform = SkelComp->GetBoneLocalTransform(SelectedBoneIndex);
    FVector Location, Scale;
    FMatrix RotationMatrix;
    LocalTransform.Decompose(Location, RotationMatrix, Scale);

    // 외부(기즈모 등)에서 회전이 변경되었는지 확인
    FVector CurrentEuler = RotationMatrix.GetEuler();
    FVector& CachedRotation = Viewer->GetCachedBoneRotation();

	if ((CurrentEuler - FMatrix::MakeRotationEuler(CachedRotation).GetEuler()).Size() > 0.01f)
    {
        CachedRotation = CurrentEuler;
    }

    bool bEdited = false;

    auto DrawTransformField = [&](const char* Label, FVector& Value, float Speed) {
        float Arr[3] = { Value.X, Value.Y, Value.Z };
        if (ImGui::DragFloat3(Label, Arr, Speed))
        {
            Value = FVector(Arr[0], Arr[1], Arr[2]);
            return true;
        }
        return false;
    };

    ImGui::Text("Transform (Local)");
    if (DrawTransformField("Location", Location, 0.1f)) bEdited = true;
    if (DrawTransformField("Rotation", CachedRotation, 0.1f)) bEdited = true;
    if (DrawTransformField("Scale", Scale, 0.01f)) bEdited = true;

    if (bEdited)
    {
        FMatrix NewLocal = FMatrix::MakeTRS(Location, FMatrix::MakeRotationEuler(CachedRotation), Scale);
        SkelComp->SetBoneLocalTransform(SelectedBoneIndex, NewLocal);

        // Gizmo 위치 업데이트
        FViewportClient* BaseClient = Viewer->GetViewport().GetClient();
        FEditorViewportClient* EditorClient = static_cast<FEditorViewportClient*>(BaseClient);
        if (UGizmoComponent* Gizmo = EditorClient->GetGizmo())
        {
            Gizmo->UpdateGizmoTransform();
        }
    }
}

void FEditorViewerWindowWidget::DrawBoneNode(int32 BoneIndex, const TArray<FBoneInfo>& Bones, const TArray<TArray<int32>>& Children)
{
    const FBoneInfo& Bone = Bones[BoneIndex];

    // socket까지 자식으로 그리므로 "자식 없음"은 bone-children + socket-children 모두 비어야 성립.
    const bool bHasBoneChildren   = Children[BoneIndex].size() > 0;
    const bool bHasSocketChildren = BoneIndex < static_cast<int32>(BoneToSocketIndices.size())
                                    && BoneToSocketIndices[BoneIndex].size() > 0;

    ImGuiTreeNodeFlags Flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!bHasBoneChildren && !bHasSocketChildren)
    {
        Flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (Viewer->GetSelectedBoneIndex() == BoneIndex)
    {
        Flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool bOpen = ImGui::TreeNodeEx(
        (void*)(intptr_t)BoneIndex,
        Flags,
        "%s",
        Bone.Name.c_str());

    // 클릭 → bone 선택. socket 선택은 해제 (상호 배타).
    if (ImGui::IsItemClicked())
    {
        Viewer->SelectBone(BoneIndex);
    }

    // 우클릭 컨텍스트
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Add Socket"))
        {
            AddSocketOnBone(BoneIndex);
        }
        ImGui::Separator();

        const bool bCanToggleChildren = bHasBoneChildren || bHasSocketChildren;
        if (ImGui::MenuItem("Expand Children", nullptr, false, bCanToggleChildren))
        {
            QueueBoneSubtreeOpenState(BoneIndex, true);
        }
        if (ImGui::MenuItem("Collapse Children", nullptr, false, bCanToggleChildren))
        {
            QueueBoneSubtreeOpenState(BoneIndex, false);
        }
        ImGui::EndPopup();
    }

    if (bOpen)
    {
        // (1) 자식 bone들
        for (int32 ChildIndex : Children[BoneIndex])
        {
            DrawBoneNode(ChildIndex, Bones, Children);
        }

        // (2) 이 bone에 매달린 socket들 (자식 bone 다음에 표시)
        if (bHasSocketChildren)
        {
            for (int32 SocketIdx : BoneToSocketIndices[BoneIndex])
            {
                DrawSocketNode(SocketIdx);
            }
        }

        ImGui::TreePop();
    }
}

void FEditorViewerWindowWidget::QueueBoneSubtreeOpenState(int32 BoneIdx, bool bOpen)
{
    PendingBoneTreeOpenStateRoot = BoneIdx;
    bPendingBoneTreeOpenStateValue = bOpen;
}

void FEditorViewerWindowWidget::ApplyPendingBoneTreeOpenState(const FSkeletalMesh* MeshData)
{
    if (!MeshData || PendingBoneTreeOpenStateRoot < 0)
    {
        return;
    }

    SetBoneSubtreeOpenState(PendingBoneTreeOpenStateRoot, Children, bPendingBoneTreeOpenStateValue);
    PendingBoneTreeOpenStateRoot = -1;
}

void FEditorViewerWindowWidget::SetBoneSubtreeOpenState(
    int32 BoneIdx,
    const TArray<TArray<int32>>& InChildren,
    bool bOpen)
{
    if (BoneIdx < 0 || BoneIdx >= static_cast<int32>(InChildren.size()))
    {
        return;
    }

    ImGuiStorage* Storage = ImGui::GetStateStorage();
    if (!Storage)
    {
        return;
    }

    const void* NodePtr = reinterpret_cast<void*>(static_cast<intptr_t>(BoneIdx));
    const ImGuiID NodeId = ImGui::GetID(NodePtr);

    // Expand는 부모가 먼저 열려야 화면에서 즉시 전체 subtree가 보인다.
    if (bOpen)
    {
        Storage->SetInt(NodeId, 1);
    }

    ImGui::PushID(NodePtr);
    for (int32 ChildIndex : InChildren[BoneIdx])
    {
        SetBoneSubtreeOpenState(ChildIndex, InChildren, bOpen);
    }
    ImGui::PopID();

    // Collapse는 자식부터 닫고 마지막에 부모를 닫아야,
    // 부모를 다시 열었을 때 이전에 열려 있던 하위 노드가 되살아나지 않는다.
    if (!bOpen)
    {
        Storage->SetInt(NodeId, 0);
    }
}

void FEditorViewerWindowWidget::DrawSocketNode(int32 SocketIdx)
{
    if (!CachedMesh) return;
    if (SocketIdx < 0 || SocketIdx >= static_cast<int32>(CachedMesh->Sockets.size())) return;

    const FSkeletalMeshSocket& Socket = CachedMesh->Sockets[SocketIdx];

    ImGuiTreeNodeFlags Flags =
        ImGuiTreeNodeFlags_Leaf |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_NoTreePushOnOpen;   // leaf니까 자식 push 불필요

    if (Viewer && Viewer->GetSelectedSocketIndex() == SocketIdx)
    {
        Flags |= ImGuiTreeNodeFlags_Selected;
    }

    // bone ID 공간(int32 직접)과 충돌하지 않게 high-bit 네임스페이스.
    const void* NodeId = reinterpret_cast<const void*>(
        static_cast<uintptr_t>(0x80000000u | static_cast<uint32>(SocketIdx)));

    // socket을 시각적으로 구분 — cyan-ish, "◇" prefix
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.85f, 1.0f, 1.0f));
    ImGui::TreeNodeEx(NodeId, Flags, "\xe2\x97\x87 %s", Socket.Name.ToString().c_str());   // ◇
    ImGui::PopStyleColor();

    // 클릭 → socket 선택. bone 선택은 해제.
    if (ImGui::IsItemClicked())
    {
        if (Viewer)
        {
            Viewer->SelectSocket(SocketIdx);
        }
    }

    // 우클릭 컨텍스트
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Add Preview Mesh..."))
        {
            // 모달은 popup 바깥에서 OpenPopup해야 안정적 — 여기선 트리거 idx만 기록.
            PendingPreviewPickerSocketIdx = SocketIdx;
        }

        const bool bHasPreview = HasPreview(Socket.Name);
        if (ImGui::MenuItem("Remove Preview Mesh", nullptr, false, bHasPreview))
        {
            if (EditorEngine)
            {
                Viewer->ClearSocketPreview(Socket.Name);
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Rename"))
        {
            RenameSocketIdx = SocketIdx;
            std::snprintf(RenameBuffer, sizeof(RenameBuffer), "%s",
                          Socket.Name.ToString().c_str());
        }

        if (ImGui::MenuItem("Delete Socket"))
        {
            DeleteSocket(SocketIdx);
        }

        ImGui::EndPopup();
    }
}

void FEditorViewerWindowWidget::RebuildBoneTreeCaches(const FSkeletalMesh* MeshData)
{
    Children.clear();
    BoneToSocketIndices.clear();
    if (!MeshData) return;

    const int32 BoneCount = static_cast<int32>(MeshData->Bones.size());
    Children.resize(BoneCount);

    for (int32 i = 0; i < BoneCount; ++i)
    {
        const int32 Parent = MeshData->Bones[i].ParentIndex;
        if (Parent >= 0)
        {
            Children[Parent].push_back(i);
        }
    }

    RebuildBoneToSocketIndices(MeshData);
}

void FEditorViewerWindowWidget::RebuildBoneToSocketIndices(const FSkeletalMesh* MeshData)
{
    BoneToSocketIndices.clear();
    if (!MeshData) return;

    const int32 BoneCount = static_cast<int32>(MeshData->Bones.size());
    BoneToSocketIndices.resize(BoneCount);

    for (int32 i = 0; i < static_cast<int32>(MeshData->Sockets.size()); ++i)
    {
        const int32 B = MeshData->Sockets[i].BoneIndex;
        if (B >= 0 && B < BoneCount)
        {
            BoneToSocketIndices[B].push_back(i);
        }
    }
}

void FEditorViewerWindowWidget::AddSocketOnBone(int32 BoneIdx)
{
    if (!CachedMesh) return;
    if (BoneIdx < 0 || BoneIdx >= static_cast<int32>(CachedMesh->Bones.size())) return;

    FSkeletalMeshSocket NewSocket;
    NewSocket.Name = FName(GenerateUniqueSocketName());
    NewSocket.BoneIndex = BoneIdx;
    // Loc/Rot/Scale은 기본값(0, identity, 1)

    CachedMesh->Sockets.push_back(NewSocket);
    const int32 NewIdx = static_cast<int32>(CachedMesh->Sockets.size()) - 1;

    RebuildBoneToSocketIndices(CachedMesh);

    if (Viewer)
    {
        Viewer->SelectSocket(NewIdx);
    }
    bMeshDirty = true;

    // socket-attached children의 transform이 새로 계산되도록 본 자세 dirty 전파 트리거.
    if (CachedSkComp)
    {
        CachedSkComp->MarkSkinningDirty();
    }
}

FString FEditorViewerWindowWidget::GenerateUniqueSocketName(const char* Base) const
{
    if (!CachedMesh) return FString(Base);

    auto Exists = [&](const FString& Candidate) -> bool {
        const FName CandidateName(Candidate);
        for (const FSkeletalMeshSocket& S : CachedMesh->Sockets)
        {
            if (S.Name == CandidateName) return true;
        }
        return false;
    };

    FString Candidate = Base;
    if (!Exists(Candidate)) return Candidate;

    for (int32 i = 1; i < 10000; ++i)
    {
        Candidate = FString(Base) + "_" + std::to_string(i);
        if (!Exists(Candidate)) return Candidate;
    }
    return Candidate;   // 폴백 — 거의 도달 불가
}

void FEditorViewerWindowWidget::DeleteSocket(int32 SocketIdx)
{
    if (!CachedMesh) return;
    if (SocketIdx < 0 || SocketIdx >= static_cast<int32>(CachedMesh->Sockets.size())) return;

    // (1) 해당 socket에 매달린 preview mesh 먼저 정리
    const FName SocketName = CachedMesh->Sockets[SocketIdx].Name;
    if (EditorEngine && Viewer)
    {
        Viewer->ClearSocketPreview(SocketName);
    }

    // (2) Sockets 배열에서 erase. 다른 socket들의 인덱스가 시프트됨.
    CachedMesh->Sockets.erase(CachedMesh->Sockets.begin() + SocketIdx);

    // (3) BoneToSocketIndices 통째 재빌드 (시프트된 인덱스 반영)
    RebuildBoneToSocketIndices(CachedMesh);

    // (4) 선택 상태 정리
    if (Viewer)
    {
        Viewer->NotifySocketDeleted(SocketIdx);
    }

    bMeshDirty = true;

    if (CachedSkComp)
    {
        CachedSkComp->MarkSkinningDirty();
    }
}

bool FEditorViewerWindowWidget::HasPreview(const FName& SocketName) const
{
    if (!EditorEngine || !Viewer) return false;
    return Viewer->FindPreviewMesh(SocketName) != nullptr;
}

void FEditorViewerWindowWidget::DrawSocketInspector()
{
    // Save 상태는 socket 선택 여부와 무관하게 항상 보이는 게 편함.
    auto DrawSaveButton = [&]() {
        const bool bCanSave = CanSaveMesh();
        if (!bCanSave) ImGui::BeginDisabled();
        const char* Label = IsMeshDirty() ? "Save Mesh *" : "Save Mesh";
        if (ImGui::Button(Label))
        {
            TriggerSaveMesh();
        }
        if (!bCanSave) ImGui::EndDisabled();
    };

    const int32 SelectedSocketIndex = Viewer ? Viewer->GetSelectedSocketIndex() : -1;
    if (!CachedMesh || SelectedSocketIndex < 0 ||
        SelectedSocketIndex >= static_cast<int32>(CachedMesh->Sockets.size()))
    {
        ImGui::TextDisabled("(no socket selected)");
        DrawSaveButton();
        return;
    }

    FSkeletalMeshSocket& Socket = CachedMesh->Sockets[SelectedSocketIndex];

    ImGui::Text("Socket: %s", Socket.Name.ToString().c_str());

    // Bone 콤보
    const TArray<FBoneInfo>& Bones = CachedMesh->Bones;
    const char* CurrentBoneName = (Socket.BoneIndex >= 0 && Socket.BoneIndex < (int32)Bones.size())
        ? Bones[Socket.BoneIndex].Name.c_str()
        : "<invalid>";

    if (ImGui::BeginCombo("Bone", CurrentBoneName))
    {
        for (int32 i = 0; i < static_cast<int32>(Bones.size()); ++i)
        {
            const bool bSelected = (Socket.BoneIndex == i);
            if (ImGui::Selectable(Bones[i].Name.c_str(), bSelected))
            {
                if (Socket.BoneIndex != i)
                {
                    Socket.BoneIndex = i;
                    RebuildBoneToSocketIndices(CachedMesh);   // 트리에서 새 본 밑으로 이동
                    bMeshDirty = true;
                    if (CachedSkComp) CachedSkComp->MarkSkinningDirty();
                }
            }
            if (bSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Location / Rotation / Scale
    // FVector / FRotator 모두 contiguous 3 float — &X / &Pitch로 DragFloat3에 전달.
    bool bChanged = false;
    bChanged |= ImGui::DragFloat3("Location", &Socket.RelativeLocation.X, 0.5f);
    bChanged |= ImGui::DragFloat3("Rotation (P/Y/R)", &Socket.RelativeRotation.Pitch, 0.5f);
    bChanged |= ImGui::DragFloat3("Scale", &Socket.RelativeScale.X, 0.01f, 0.001f, 100.0f);

    if (bChanged)
    {
        bMeshDirty = true;
        if (CachedSkComp) CachedSkComp->MarkSkinningDirty();
    }

    ImGui::Separator();
    DrawSaveButton();
}

void FEditorViewerWindowWidget::TriggerSaveMesh()
{
	RequestSaveMesh();
}

bool FEditorViewerWindowWidget::IsSocketNameUnique(const FString& Candidate, int32 IgnoreIdx) const
{
    if (!CachedMesh) return false;
    const FName CandidateName(Candidate);
    for (int32 i = 0; i < static_cast<int32>(CachedMesh->Sockets.size()); ++i)
    {
        if (i == IgnoreIdx) continue;
        if (CachedMesh->Sockets[i].Name == CandidateName) return false;
    }
    return true;
}

void FEditorViewerWindowWidget::DrawRenameModal()
{
    if (!ImGui::BeginPopupModal("RenameSocket", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    // 무효한 상태 — 즉시 닫기
    if (!CachedMesh || RenameSocketIdx < 0 ||
        RenameSocketIdx >= static_cast<int32>(CachedMesh->Sockets.size()))
    {
        RenameSocketIdx = -1;
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }

    ImGui::Text("Rename socket:");
    ImGui::InputText("##rename", RenameBuffer, sizeof(RenameBuffer));

    const FString Candidate(RenameBuffer);
    const bool bEmpty  = Candidate.empty();
    const bool bUnique = !bEmpty && IsSocketNameUnique(Candidate, RenameSocketIdx);
    const bool bValid  = !bEmpty && bUnique;

    if (bEmpty)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Name cannot be empty");
    }
    else if (!bUnique)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Name already in use");
    }

    if (!bValid) ImGui::BeginDisabled();
    if (ImGui::Button("OK"))
    {
        // Preview mesh가 이 socket에 attach되어 있다면 key가 socket name이므로,
        // 이름 변경 시 preview를 깔끔히 재attach해야 함.
        const FName OldName = CachedMesh->Sockets[RenameSocketIdx].Name;
        const FName NewName(Candidate);

        FString PreviewPath;
        if (EditorEngine && Viewer)
        {
            UStaticMeshComponent* Preview = Viewer->FindPreviewMesh(OldName);
            if (Preview && Preview->GetStaticMesh())
            {
                PreviewPath = Preview->GetStaticMesh()->GetAssetPathFileName();
                Viewer->ClearSocketPreview(OldName);
            }
        }

        CachedMesh->Sockets[RenameSocketIdx].Name = NewName;

        if (!PreviewPath.empty() && EditorEngine && Viewer)
        {
            Viewer->SetSocketPreviewMesh(NewName, PreviewPath);
        }

        if (Viewer && Viewer->GetSelectedSocketIndex() == RenameSocketIdx)
        {
            Viewer->SelectSocket(RenameSocketIdx);
        }

        bMeshDirty = true;
        if (CachedSkComp) CachedSkComp->MarkSkinningDirty();
        RenameSocketIdx = -1;
        ImGui::CloseCurrentPopup();
    }
    if (!bValid) ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        RenameSocketIdx = -1;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void FEditorViewerWindowWidget::DrawPreviewPickerModal()
{
    if (!ImGui::BeginPopupModal("PickStaticMesh", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    static char Filter[256] = "";
    ImGui::InputText("Filter", Filter, sizeof(Filter));
    ImGui::Separator();

    const TArray<FString>& Paths = FResourceManager::Get().GetStaticMeshPaths();

    ImGui::BeginChild("PickList", ImVec2(420.0f, 300.0f), true);
    for (const FString& Path : Paths)
    {
        if (Filter[0] != '\0' && Path.find(Filter) == FString::npos)
        {
            continue;
        }

        if (ImGui::Selectable(Path.c_str()))
        {
            if (CachedMesh && EditorEngine && Viewer &&
                PendingPreviewPickerSocketIdx >= 0 &&
                PendingPreviewPickerSocketIdx < static_cast<int32>(CachedMesh->Sockets.size()))
            {
                const FName SocketName = CachedMesh->Sockets[PendingPreviewPickerSocketIdx].Name;
                Viewer->SetSocketPreviewMesh(SocketName, Path);
            }
            PendingPreviewPickerSocketIdx = -1;
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::EndChild();

    if (ImGui::Button("Cancel"))
    {
        PendingPreviewPickerSocketIdx = -1;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
