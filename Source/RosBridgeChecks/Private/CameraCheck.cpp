#include "CameraCheck.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RosCameraComponent.h"
#include "UObject/ConstructorHelpers.h"

ACameraCheck::ACameraCheck()
{
	PrimaryActorTick.bCanEverTick = true;
	Camera = CreateDefaultSubobject<URosCameraComponent>(TEXT("Camera"));
	RootComponent = Camera;
	// Nothing that smears the swinging cube over time
	Camera->ShowFlags.SetMotionBlur(false);
	Camera->ShowFlags.SetAntiAliasing(false);

	// Half-meter cubes 10 m ahead, positioned in the camera's frame
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Colored(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Still = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Still"));
	Swinging = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Swinging"));
	for (UStaticMeshComponent* Each : { Still.Get(), Swinging.Get() })
	{
		Each->SetupAttachment(Camera);
		Each->SetStaticMesh(Cube.Object);
		Each->SetMaterial(0, Colored.Object); // the cube's own is a grid without a Color parameter
		Each->SetRelativeScale3D(FVector(0.5));
	}
	Still->SetRelativeLocation(FVector(1000, -300, 200));
}

void ACameraCheck::BeginPlay()
{
	Camera->Raw = FPlatformMisc::GetEnvironmentVariable(TEXT("CAMERA_RAW")) != TEXT("off"); // before the camera's BeginPlay reads it
	Super::BeginPlay();
	// The empty map the check runs in has no light, and a second sun in a real level would light all of it
	if (!TActorIterator<ADirectionalLight>(GetWorld()))
	{
		// Lights only the faces the camera sees head on
		UDirectionalLightComponent* Light = NewObject<UDirectionalLightComponent>(this);
		Light->SetupAttachment(Camera);
		Light->SetMobility(EComponentMobility::Movable);
		Light->RegisterComponent();
	}
	Camera->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList; // not the rest of the map
	Camera->ShowOnlyActors.Add(this);
	Still->CreateDynamicMaterialInstance(0)->SetVectorParameterValue(TEXT("Color"), FLinearColor::Red);
	Swinging->CreateDynamicMaterialInstance(0)->SetVectorParameterValue(TEXT("Color"), FLinearColor::Green);
	Swing(); // the camera may capture before this actor first ticks
}

void ACameraCheck::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Swing();
}

void ACameraCheck::Swing()
{
	Swinging->SetRelativeLocation(FVector(1000, 600 * FMath::Sin(PI * GetWorld()->GetTimeSeconds()), -200));
}
