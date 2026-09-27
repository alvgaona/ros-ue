#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include "dds/dds.h"
THIRD_PARTY_INCLUDES_END

namespace ros
{
	class ROSBRIDGE_API PublisherBase
	{
	public:
		PublisherBase() = default;
		explicit PublisherBase(dds_entity_t InWriter);
		PublisherBase(PublisherBase&& Other);
		PublisherBase& operator=(PublisherBase&& Other);
		~PublisherBase();

	protected:
		void Write(const void* Sample) const;

	private:
		dds_entity_t Writer = 0;
	};

	// Owns a DDS writer until it is destroyed or cleared with `= {}`. Move-only.
	template<class T>
	class Publisher : public PublisherBase
	{
	public:
		Publisher() = default;
		explicit Publisher(PublisherBase&& Base) : PublisherBase(MoveTemp(Base)) {}
		void Publish(const T& Msg) const { Write(&Msg); }
	};
}
