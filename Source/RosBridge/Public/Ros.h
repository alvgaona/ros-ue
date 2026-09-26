#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"

THIRD_PARTY_INCLUDES_START
#include "dds/dds.h"
THIRD_PARTY_INCLUDES_END

#include "Ros.generated.h"

// Maps an idlc-generated message struct to its type descriptor.
template<class T> const dds_topic_descriptor_t* TypeOf();
#define ROS_MESSAGE(T) template<> inline const dds_topic_descriptor_t* TypeOf<T>() { return &T##_desc; }

enum class ERosQos { Reliable, SensorData };

struct FRosEntity
{
	explicit FRosEntity(dds_entity_t InEntity) : Entity(InEntity)
	{
		ensureMsgf(Entity > 0, TEXT("DDS create failed: %s"), UTF8_TO_TCHAR(dds_strretcode(Entity)));
	}
	virtual ~FRosEntity() { dds_delete(Entity); }
	virtual void Spin() {}
	const dds_entity_t Entity;
};

template<class T>
struct TPublisher : FRosEntity
{
	using FRosEntity::FRosEntity;
	void Publish(const T& Msg) const { dds_write(Entity, &Msg); }
};

template<class T>
struct TSubscription : FRosEntity
{
	TSubscription(dds_entity_t InEntity, TFunction<void(const T&)> InCallback)
		: FRosEntity(InEntity), Callback(MoveTemp(InCallback)) {}

	virtual void Spin() override
	{
		for (;;)
		{
			void* Samples[16] = {}; // null first entry: Cyclone loans the buffers
			dds_sample_info_t Infos[16];
			const int32 N = dds_take(Entity, Samples, Infos, 16, 16);
			if (N <= 0) return;
			for (int32 I = 0; I < N; ++I)
				if (Infos[I].valid_data) Callback(*static_cast<const T*>(Samples[I]));
			dds_return_loan(Entity, Samples, N);
		}
	}

	TFunction<void(const T&)> Callback;
};

// Talks to ROS 2 as a plain DDS participant, not a ROS node. Callbacks run on the game thread.
UCLASS()
class ROSBRIDGE_API URos : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	template<class T>
	TSharedRef<TPublisher<T>> CreatePublisher(const FString& Topic, ERosQos Qos = ERosQos::Reliable)
	{
		return MakeShared<TPublisher<T>>(dds_create_writer(Participant, MakeTopic(TypeOf<T>(), Topic), QosOf(Qos), nullptr));
	}

	template<class T>
	TSharedRef<TSubscription<T>> CreateSubscription(const FString& Topic, TFunction<void(const T&)> Callback, ERosQos Qos = ERosQos::Reliable)
	{
		auto Sub = MakeShared<TSubscription<T>>(
			dds_create_reader(Participant, MakeTopic(TypeOf<T>(), Topic), QosOf(Qos), nullptr), MoveTemp(Callback));
		Spinning.Add(Sub);
		return Sub;
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
	dds_entity_t MakeTopic(const dds_topic_descriptor_t* Type, const FString& Topic);
	const dds_qos_t* QosOf(ERosQos Qos) const { return Qos == ERosQos::SensorData ? SensorDataQos : ReliableQos; }

	dds_entity_t Participant = 0;
	dds_qos_t* ReliableQos = nullptr;
	dds_qos_t* SensorDataQos = nullptr;
	TArray<TWeakPtr<FRosEntity>> Spinning;
};
