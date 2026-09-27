#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ros.h"
#include "RosMessages.h"
#include "ImageCheck.generated.h"

// Publishes a test pattern on /image_raw at 30 Hz of sim time for Checks/image.py, and how long each write held the game
// thread on /image_check/write_seconds. IMAGE_SIZE (640x480 by default) and IMAGE_QOS (default, keep1 or sensor) set the run.
UCLASS()
class AImageCheck : public AActor
{
	GENERATED_BODY()

public:
	AImageCheck();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	ros::Publisher<sensor_msgs::msg::Image> Images;
	ros::Publisher<std_msgs::msg::Float64> WriteSeconds;
	TArray<uint8> Pixels;
	int32 Width = 640;
	int32 Height = 480;
	double NextSend = 0;
};
