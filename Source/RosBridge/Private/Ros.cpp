#include "Ros.h"

void URos::Initialize(FSubsystemCollectionBase&)
{
	// 0 when unset, same as ROS
	const int32 Domain = FCString::Atoi(*FPlatformMisc::GetEnvironmentVariable(TEXT("ROS_DOMAIN_ID")));
	Participant = dds_create_participant(Domain, nullptr, nullptr);

	ReliableQos = dds_create_qos(); // rclcpp default: reliable, keep last 10
	dds_qset_reliability(ReliableQos, DDS_RELIABILITY_RELIABLE, DDS_MSECS(100));
	dds_qset_history(ReliableQos, DDS_HISTORY_KEEP_LAST, 10);

	SensorDataQos = dds_create_qos(); // rclcpp SensorDataQoS: best effort, keep last 5
	dds_qset_reliability(SensorDataQos, DDS_RELIABILITY_BEST_EFFORT, 0);
	dds_qset_history(SensorDataQos, DDS_HISTORY_KEEP_LAST, 5);
}

void URos::Deinitialize()
{
	dds_delete(Participant); // also deletes every topic, reader and writer under it
	dds_delete_qos(ReliableQos);
	dds_delete_qos(SensorDataQos);
}

dds_entity_t URos::MakeTopic(const dds_topic_descriptor_t* Type, const FString& Topic)
{
	// ponytail: one topic handle per publisher/subscription, freed with the participant; cache by name if actors respawn a lot
	const FString Name = TEXT("rt") + (Topic.StartsWith(TEXT("/")) ? Topic : TEXT("/") + Topic);
	return dds_create_topic(Participant, Type, TCHAR_TO_UTF8(*Name), nullptr, nullptr);
}

void URos::Tick(float)
{
	// Index loop and a pinned pointer, because a callback may create or drop subscriptions.
	for (int32 I = 0; I < Spinning.Num(); ++I)
		if (const TSharedPtr<FRosEntity> Entity = Spinning[I].Pin())
			Entity->Spin();
	Spinning.RemoveAll([](const TWeakPtr<FRosEntity>& Weak) { return !Weak.IsValid(); });
}
