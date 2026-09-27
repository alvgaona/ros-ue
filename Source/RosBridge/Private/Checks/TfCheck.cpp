#include "TfCheck.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

ATfCheck::ATfCheck()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics; // physics has moved Body by then
	Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
	RootComponent = Body;
	Body->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void ATfCheck::BeginPlay()
{
	Super::BeginPlay();
	Broadcaster = ros::TransformBroadcaster(this);
	Static = ros::StaticTransformBroadcaster(this);
	Static.SendTransform({ TEXT("slide"), TEXT("mount"), FTransform(FVector(0, 0, 50)) });
	Start = GetWorld()->GetTimeSeconds();
	Body->SetSimulatePhysics(true);
	Body->SetEnableGravity(false);
	Body->SetLinearDamping(0.f);
	Body->SetPhysicsLinearVelocity(FVector(0, 100, 0)); // right at 1 m/s
	BeforePhysics = FWorldDelegates::OnWorldPreActorTick.AddUObject(this, &ATfCheck::SendBeforePhysics);
}

void ATfCheck::EndPlay(const EEndPlayReason::Type Reason)
{
	FWorldDelegates::OnWorldPreActorTick.Remove(BeforePhysics);
	Broadcaster = {};
	Static = {};
	Super::EndPlay(Reason);
}

void ATfCheck::SendBeforePhysics(UWorld* World, ELevelTick, float)
{
	// Where a TG_PrePhysics tick would read the box: this frame's time, before physics moves it
	if (World == GetWorld())
		Broadcaster.SendTransform({ TEXT("world"), TEXT("body_pre"), Body->GetComponentTransform() });
}

void ATfCheck::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Time = GetWorld()->GetTimeSeconds();
	Broadcaster.SendTransform({
		{ TEXT("/world"), TEXT("fixed"), FTransform(FRotator(0, 90, 0), FVector(100, 200, 50)) },
		{ TEXT("world"), TEXT("slide"), FTransform(FVector(0, 100 * Time, 0)) },
		{ TEXT("world"), TEXT("turn"), FTransform(FQuat(FVector::UpVector, FMath::DegreesToRadians(90 * Time))) },
		{ TEXT("world"), TEXT("body"), Body->GetComponentTransform() },
	});
	if (!ArmSent && Time - Start >= 1)
	{
		Static.SendTransform({ TEXT("turn"), TEXT("arm"), FTransform(FVector(100, 0, 0)) });
		ArmSent = true;
	}
}
