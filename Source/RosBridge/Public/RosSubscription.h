#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include "dds/dds.h"
THIRD_PARTY_INCLUDES_END

// The DDS reader and callback behind a RosSubscription. URos spins it while a subscription holds it.
class ROSBRIDGE_API RosReader
{
public:
	RosReader(dds_entity_t InReader, TFunction<void(const void*)> InCallback);
	~RosReader();
	RosReader(const RosReader&) = delete;
	RosReader& operator=(const RosReader&) = delete;

	void Spin();

private:
	dds_entity_t Reader;
	TFunction<void(const void*)> Callback;
};

// Receives messages until it is destroyed or cleared with `= {}`. Move-only.
class RosSubscription
{
public:
	RosSubscription() = default;
	explicit RosSubscription(TSharedRef<RosReader> InReader) : Reader(MoveTemp(InReader)) {}
	RosSubscription(RosSubscription&&) = default;
	RosSubscription& operator=(RosSubscription&&) = default;

private:
	TSharedPtr<RosReader> Reader;
};
