#include "ClockCheck.h"
#include "Kismet/GameplayStatics.h"

AClockCheck::AClockCheck()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true; // the schedule has to go on to unpause
}

void AClockCheck::BeginPlay()
{
	Super::BeginPlay();
	Phases = ros::CreatePublisher<std_msgs::msg::String>(this, TEXT("/clock_check/phase"));
	Start = FPlatformTime::Seconds();
}

void AClockCheck::EndPlay(const EEndPlayReason::Type Reason)
{
	Phases = {};
	Super::EndPlay(Reason);
}

void AClockCheck::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Real seconds after BeginPlay, since game time stops while paused. The first wait lets the checker discover the writer.
	static constexpr double At[] = { 3, 7, 11, 13, 17 };
	static const TCHAR* const Names[] = { TEXT("run"), TEXT("pause"), TEXT("resume"), TEXT("slomo"), TEXT("done") };
	if (Step == UE_ARRAY_COUNT(At) || FPlatformTime::Seconds() - Start < At[Step])
		return;
	bool bApplied = true;
	switch (Step)
	{
	case 1: bApplied = UGameplayStatics::SetGamePaused(this, true); break;
	case 2: bApplied = UGameplayStatics::SetGamePaused(this, false); break;
	case 3: UGameplayStatics::SetGlobalTimeDilation(this, 0.5f); break;
	case 4: UGameplayStatics::SetGlobalTimeDilation(this, 1.0f); break;
	}
	UE_CLOG(!bApplied, LogRos, Warning, TEXT("Clock check couldn't %s"), Names[Step]);
	UE_LOG(LogRos, Log, TEXT("Clock check: %s"), Names[Step]);
	Phases.Publish(std_msgs::msg::String{ TCHAR_TO_UTF8(Names[Step]) });
	++Step;
}
