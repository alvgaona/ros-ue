#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "RosClock.h"
#include "RosPublisher.h"
#include "RosQos.h"
#include "RosSubscription.h"
#include "RosTypeSupport.h"
#include "Ros.generated.h"

ROSBRIDGE_API DECLARE_LOG_CATEGORY_EXTERN(LogRos, Log, All);

// One per game instance: the DDS participant that serves every publisher and subscription in it.
// Not a ROS node. Callbacks run on the game thread.
UCLASS()
class ROSBRIDGE_API URos : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	// The URos of the game instance WorldContext plays in, or null outside one.
	static URos* Get(const UObject* WorldContext);

	ros::PublisherBase CreatePublisher(const FString& Topic, const ros::TypeSupport& Type, ros::Qos Profile);
	ros::Subscription CreateSubscription(const FString& Topic, const ros::TypeSupport& Type, ros::Qos Profile, TFunction<void(const void*)> Callback);
	builtin_interfaces::msg::Time Now() const;

	virtual void Initialize(FSubsystemCollectionBase&) override;
	virtual void Deinitialize() override;
	virtual void Tick(float) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(URos, STATGROUP_Tickables); }
	virtual ETickableTickType GetTickableTickType() const override
	{
		return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Always;
	}

private:
	dds_entity_t MakeTopic(const ros::TypeSupport& Type, const FString& Topic);

	dds_entity_t Participant = 0;
	TUniquePtr<ros::Clock> Clock;
	TArray<TWeakPtr<ros::Reader>> Spinning;
};

namespace ros
{
	template<class T>
	Publisher<T> CreatePublisher(const UObject* WorldContext, const FString& Topic, Qos Profile = Qos::Reliable)
	{
		URos* Ros = URos::Get(WorldContext);
		return Ros ? Publisher<T>(Ros->CreatePublisher(Topic, TypeSupportOf<T>(), Profile)) : Publisher<T>();
	}

	template<class T>
	Subscription CreateSubscription(const UObject* WorldContext, const FString& Topic, TFunction<void(const T&)> Callback, Qos Profile = Qos::Reliable)
	{
		URos* Ros = URos::Get(WorldContext);
		return Ros ? Ros->CreateSubscription(Topic, TypeSupportOf<T>(), Profile,
			[Callback = MoveTemp(Callback)](const void* Sample) { Callback(*static_cast<const T*>(Sample)); }) : Subscription();
	}
}
