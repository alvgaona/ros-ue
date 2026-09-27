#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "RosQos.h"
#include "RosCameraComponent.generated.h"

namespace ros { class CameraStream; }

// Publishes what it sees as sensor_msgs/Image in bgr8, FrameRate times per second of sim time, each image stamped with the
// sim time it was captured at. The GPU copies each image back while the game runs on, and a worker thread publishes it.
UCLASS(ClassGroup = Ros, meta = (BlueprintSpawnableComponent))
class ROSBRIDGE_API URosCameraComponent : public USceneCaptureComponent2D
{
	GENERATED_BODY()

public:
	URosCameraComponent();

	UPROPERTY(EditAnywhere, Category = Ros)
	FString Topic = TEXT("/camera/image_raw");

	UPROPERTY(EditAnywhere, Category = Ros)
	FString FrameId = TEXT("camera_optical_frame");

	UPROPERTY(EditAnywhere, Category = Ros, meta = (ClampMin = 1))
	int32 Width = 640;

	UPROPERTY(EditAnywhere, Category = Ros, meta = (ClampMin = 1))
	int32 Height = 480;

	UPROPERTY(EditAnywhere, Category = Ros, meta = (ClampMin = 1))
	float FrameRate = 30;

	// Read at BeginPlay
	ros::Qos Qos = 10;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	TSharedPtr<ros::CameraStream, ESPMode::ThreadSafe> Stream; // shared with the render thread and the worker, which may outlive EndPlay
	double NextCapture = 0;
};
