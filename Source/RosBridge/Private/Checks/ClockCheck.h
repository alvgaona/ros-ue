#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ros.h"
#include "RosMessages.h"
#include "ClockCheck.generated.h"

// Runs, pauses, resumes, then slows the game to half speed, announcing each phase on /clock_check/phase for Checks/clock.py.
UCLASS()
class AClockCheck : public AActor
{
	GENERATED_BODY()

public:
	AClockCheck();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	ros::Publisher<std_msgs::msg::String> Phases;
	double Start = 0;
	int32 Step = 0;
};
