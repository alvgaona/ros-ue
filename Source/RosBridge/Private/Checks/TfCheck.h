#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ros.h"
#include "TfCheck.generated.h"

class UBoxComponent;

// Sends frames for Checks/tf.py and tf_static.py: three posed from game time, a box moved by physics, read before and
// after, and two static children sent a second apart.
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
	ros::StaticTransformBroadcaster Static;
	FDelegateHandle BeforePhysics;
	double Start = 0;
	bool ArmSent = false;
};
