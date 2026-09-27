#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "RosPublisher.h"
#include "RosQos.h"
#include "RosSubscription.h"
#include "RosTypeSupport.h"
#include "Ros.generated.h"

ROSBRIDGE_API DECLARE_LOG_CATEGORY_EXTERN(LogRos, Log, All);

// Talks to ROS 2 as a plain DDS participant, not a ROS node. Callbacks run on the game thread.
UCLASS()
class ROSBRIDGE_API URos : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	template<class T>
	RosPublisher<T> CreatePublisher(const FString& Topic, RosQos Qos = RosQos::Reliable)
	{
		return RosPublisher<T>(CreateWriter(Topic, RosTypeSupportOf<T>(), Qos));
	}

	template<class T>
	RosSubscription CreateSubscription(const FString& Topic, TFunction<void(const T&)> Callback, RosQos Qos = RosQos::Reliable)
	{
		return AddSubscription(Topic, RosTypeSupportOf<T>(), Qos,
			[Callback = MoveTemp(Callback)](const void* Sample) { Callback(*static_cast<const T*>(Sample)); });
	}

	virtual void Initialize(FSubsystemCollectionBase&) override;
	virtual void Deinitialize() override;
	virtual void Tick(float) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(URos, STATGROUP_Tickables); }
	virtual ETickableTickType GetTickableTickType() const override
	{
		return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Always;
	}

private:
	dds_entity_t CreateWriter(const FString& Topic, const RosTypeSupport& Type, RosQos Qos);
	RosSubscription AddSubscription(const FString& Topic, const RosTypeSupport& Type, RosQos Qos, TFunction<void(const void*)> Callback);
	dds_entity_t MakeTopic(const RosTypeSupport& Type, const FString& Topic);

	dds_entity_t Participant = 0;
	TArray<TWeakPtr<RosReader>> Spinning;
};
