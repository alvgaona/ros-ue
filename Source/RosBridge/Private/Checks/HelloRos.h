#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ros.h"
#include "RosMessages.h"
#include "HelloRos.generated.h"

// Talker and listener on /chatter, like ROS's demo_nodes_cpp. Drop one in a level and press Play.
UCLASS()
class AHelloRos : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void Talk();

	ros::Publisher<std_msgs::msg::String> Publisher;
	ros::Subscription Subscription;
	FTimerHandle Timer;
	int32 Count = 0;
};
