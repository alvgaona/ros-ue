#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ros.h"
#include "TfCheck.generated.h"

class UBoxComponent;

// Sends frames on /tf for Checks/tf.py: three posed from game time, and a box moved by physics, read before and after.
UCLASS()
class ATfCheck : public AActor
{
	GENERATED_BODY()

public:
	ATfCheck();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void SendBeforePhysics(UWorld* World, ELevelTick, float);

	UPROPERTY()
	TObjectPtr<UBoxComponent> Body;

	ros::TransformBroadcaster Broadcaster;
	FDelegateHandle BeforePhysics;
};
