#include "Ros.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "RosClock.h"
#include "RosQosProfile.h"

URos* URos::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	URos* Ros = World ? UGameInstance::GetSubsystem<URos>(World->GetGameInstance()) : nullptr;
	ensureMsgf(Ros, TEXT("%s isn't in a game instance, so it can't reach ROS"), *GetNameSafe(WorldContext));
	return Ros;
}

void URos::Initialize(FSubsystemCollectionBase&)
{
	// 0 when unset, same as ROS
	const int32 Domain = FCString::Atoi(*FPlatformMisc::GetEnvironmentVariable(TEXT("ROS_DOMAIN_ID")));
	Participant = dds_create_participant(Domain, nullptr, nullptr);
	Clock = MakeShared<ros::Clock>(*GetGameInstance(), ros::Publisher<rosgraph_msgs::msg::Clock>(CreatePublisher(TEXT("/clock"), ros::TypeSupportOf<rosgraph_msgs::msg::Clock>(), 10)));
}

void URos::Deinitialize()
{
	Clock.Reset();
	dds_delete(Participant); // also deletes every topic, reader and writer under it
}

ros::PublisherBase URos::CreatePublisher(const FString& Topic, const ros::TypeSupport& Type, ros::Qos Profile)
{
	return ros::PublisherBase(dds_create_writer(Participant, MakeTopic(Type, Topic), ros::MakeQos(Profile, Type.Hash).Get(), nullptr));
}

ros::Subscription URos::CreateSubscription(const FString& Topic, const ros::TypeSupport& Type, ros::Qos Profile, TFunction<void(const void*)> Callback)
{
	const dds_entity_t Entity = dds_create_reader(Participant, MakeTopic(Type, Topic), ros::MakeQos(Profile, Type.Hash).Get(), nullptr);
	const TSharedRef<ros::Reader> Reader = MakeShared<ros::Reader>(Entity, MoveTemp(Callback));
	Spinning.Add(Reader);
	return ros::Subscription(Reader);
}

builtin_interfaces::msg::Time URos::Now() const
{
	return Clock ? Clock->Now() : builtin_interfaces::msg::Time{};
}

dds_entity_t URos::MakeTopic(const ros::TypeSupport& Type, const FString& Topic)
{
	// ponytail: one topic handle per publisher/subscription, freed with the participant; cache by name if actors respawn a lot
	const FString Name = TEXT("rt") + (Topic.StartsWith(TEXT("/")) ? Topic : TEXT("/") + Topic);
	return dds_create_topic(Participant, Type.Descriptor, TCHAR_TO_UTF8(*Name), nullptr, nullptr);
}

void URos::Tick(float)
{
	// Index loop and a pinned pointer, because a callback may create or drop subscriptions.
	for (int32 I = 0; I < Spinning.Num(); ++I)
		if (const TSharedPtr<ros::Reader> Reader = Spinning[I].Pin())
			Reader->Spin();
	Spinning.RemoveAll([](const TWeakPtr<ros::Reader>& Weak) { return !Weak.IsValid(); });
}

namespace ros
{
	builtin_interfaces::msg::Time Now(const UObject* WorldContext)
	{
		const URos* Ros = URos::Get(WorldContext);
		return Ros ? Ros->Now() : builtin_interfaces::msg::Time{};
	}
}
