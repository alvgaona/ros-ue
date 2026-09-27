#include "Ros.h"

void URos::Initialize(FSubsystemCollectionBase&)
{
	// 0 when unset, same as ROS
	const int32 Domain = FCString::Atoi(*FPlatformMisc::GetEnvironmentVariable(TEXT("ROS_DOMAIN_ID")));
	Participant = dds_create_participant(Domain, nullptr, nullptr);
}

void URos::Deinitialize()
{
	dds_delete(Participant); // also deletes every topic, reader and writer under it
}

dds_entity_t URos::CreateWriter(const FString& Topic, const RosTypeSupport& Type, RosQos Qos)
{
	return dds_create_writer(Participant, MakeTopic(Type, Topic), MakeQos(Qos, Type.Hash).Get(), nullptr);
}

RosSubscription URos::AddSubscription(const FString& Topic, const RosTypeSupport& Type, RosQos Qos, TFunction<void(const void*)> Callback)
{
	const dds_entity_t Entity = dds_create_reader(Participant, MakeTopic(Type, Topic), MakeQos(Qos, Type.Hash).Get(), nullptr);
	const TSharedRef<RosReader> Reader = MakeShared<RosReader>(Entity, MoveTemp(Callback));
	Spinning.Add(Reader);
	return RosSubscription(Reader);
}

dds_entity_t URos::MakeTopic(const RosTypeSupport& Type, const FString& Topic)
{
	// ponytail: one topic handle per publisher/subscription, freed with the participant; cache by name if actors respawn a lot
	const FString Name = TEXT("rt") + (Topic.StartsWith(TEXT("/")) ? Topic : TEXT("/") + Topic);
	return dds_create_topic(Participant, Type.Descriptor, TCHAR_TO_UTF8(*Name), nullptr, nullptr);
}

void URos::Tick(float)
{
	// Index loop and a pinned pointer, because a callback may create or drop subscriptions.
	for (int32 I = 0; I < Spinning.Num(); ++I)
		if (const TSharedPtr<RosReader> Reader = Spinning[I].Pin())
			Reader->Spin();
	Spinning.RemoveAll([](const TWeakPtr<RosReader>& Weak) { return !Weak.IsValid(); });
}
