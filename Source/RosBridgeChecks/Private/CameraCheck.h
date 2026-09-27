#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraCheck.generated.h"

class URosCameraComponent;
class UStaticMeshComponent;

// A camera on /camera/image_raw for Checks/camera.py, facing a red cube that stands still up and to the left and a green
// cube that swings side to side with game time. Needs a run that renders.
UCLASS()
class ACameraCheck : public AActor
{
	GENERATED_BODY()

public:
	ACameraCheck();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void Swing();

	UPROPERTY()
	TObjectPtr<URosCameraComponent> Camera;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Still;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Swinging;
};
