#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include "dds/dds.h"
THIRD_PARTY_INCLUDES_END

class ROSBRIDGE_API RosPublisherBase
{
public:
	RosPublisherBase() = default;
	explicit RosPublisherBase(dds_entity_t InWriter);
	RosPublisherBase(RosPublisherBase&& Other);
	RosPublisherBase& operator=(RosPublisherBase&& Other);
	~RosPublisherBase();

protected:
	void Write(const void* Sample) const;

private:
	dds_entity_t Writer = 0;
};

// Owns a DDS writer until it is destroyed or cleared with `= {}`. Move-only.
template<class T>
class RosPublisher : public RosPublisherBase
{
public:
	using RosPublisherBase::RosPublisherBase;
	void Publish(const T& Msg) const { Write(&Msg); }
};
