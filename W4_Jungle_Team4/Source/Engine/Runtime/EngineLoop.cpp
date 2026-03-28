#include "Engine/Runtime/EngineLoop.h"

#include "Engine/Asset/StaticMesh/StaticMeshManager.h"
#include "Engine/Core/Paths.h"
#include "Editor/EditorEngine.h"
#include "Viewer/ViewerEngine.h"

void FEngineLoop::CreateEngine()
{
#if	WITH_EDITOR
	GEngine = UObjectManager::Get().CreateObject<UEditorEngine>();
#elif IS_OBJ_VIEWER
	GEngine = UObjectManager::Get().CreateObject<UViewerEngine>();
#else
	GEngine = UObjectManager::Get().CreateObject<UEngine>();
#endif
}

bool FEngineLoop::Init(HINSTANCE hInstance, int nShowCmd)
{
	(void)nShowCmd;

	if (!Application.Init(hInstance))
	{
		return false;
	}

	Application.SetOnSizingCallback([this]()
		{
			Timer.Tick();
			GEngine->Tick(Timer.GetDeltaTime());
		});

	Application.SetOnResizedCallback([](unsigned int Width, unsigned int Height)
		{
			if (GEngine)
			{
				GEngine->OnWindowResized(Width, Height);
			}
		});

	CreateEngine();
	GEngine->Init(&Application.GetWindow());
	GEngine->SetTimer(&Timer);
	GEngine->BeginPlay();

	// opt) test
	/*[StaticMeshManager] request load : C:\Users\jungle\HB\source\repos\W4\W4_Jungle_Team4\Data / cube - tex.obj
	[StaticMeshManager] normalized path : C: / Users / jungle / HB / source / repos / W4 / W4_Jungle_Team4 / Data / cube - tex.obj
	[StaticMeshManager] bake path : C:\Users\jungle\HB\source\repos\W4\W4_Jungle_Team4\Data\cube - tex.smbin
	[ObjImporter] parsed raw data positions = 8, texcoords = 14, normals = 6, faces = 12, mtllib = cube - tex.mtl
	[ObjImporter] section firstIndex = 0, indexCount = 36, material = texture, texture = C: / Users / jungle / HB / source / repos / W4 / W4_Jungle_Team4 / Data / cube - tex.png
	[ObjImporter] cooked asset vertices = 36, indices = 36, sections = 1
	[StaticMeshManager] imported from obj and saved bake
	[StaticMeshManager] done : vertices = 36, indices = 36, sections = 1*/
	const FString TestObjPath = FPaths::ToUtf8(FPaths::RootDir()) + "Data/cube-tex.obj";
	FStaticMeshAsset* TestMesh = FStaticMeshManager::Get().LoadStaticMeshAsset(TestObjPath);
	(void)TestMesh;
	// opt) test 재로드
	/*[StaticMeshManager] request load : C:\Users\jungle\HB\source\repos\W4\W4_Jungle_Team4\Data / cube - tex.obj
	[StaticMeshManager] already loaded, reuse cached asset : C: / Users / jungle / HB / source / repos / W4 / W4_Jungle_Team4 / Data / cube - tex.obj(vertices = 36, indices = 36, sections = 1)*/
	FStaticMeshAsset* TestMeshAgain = FStaticMeshManager::Get().LoadStaticMeshAsset(TestObjPath);
	(void)TestMeshAgain;

	Timer.Initialize();

	return true;
}

int FEngineLoop::Run()
{
	while (!Application.IsExitRequested())
	{
		Application.PumpMessages();

		if (Application.IsExitRequested())
		{
			break;
		}

		Timer.Tick();
		GEngine->Tick(Timer.GetDeltaTime());
	}

	return 0;
}

void FEngineLoop::Shutdown()
{
	if (GEngine)
	{
		GEngine->Shutdown();
		UObjectManager::Get().DestroyObject(GEngine);
		GEngine = nullptr;
	}
}
