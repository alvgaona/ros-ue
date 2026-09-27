#include "RosPublisher.h"

namespace ros
{
	PublisherBase::PublisherBase(dds_entity_t InWriter) : Writer(InWriter)
	{
		ensureMsgf(Writer > 0, TEXT("DDS writer create failed: %s"), UTF8_TO_TCHAR(dds_strretcode(Writer)));
	}

	PublisherBase::PublisherBase(PublisherBase&& Other) : Writer(Other.Writer)
	{
		Other.Writer = 0;
	}

	PublisherBase& PublisherBase::operator=(PublisherBase&& Other)
	{
		if (this != &Other)
		{
			if (Writer > 0) dds_delete(Writer);
			Writer = Other.Writer;
			Other.Writer = 0;
		}
		return *this;
	}

	PublisherBase::~PublisherBase()
	{
		if (Writer > 0) dds_delete(Writer);
	}

	void PublisherBase::Write(const void* Sample) const
	{
		if (ensureMsgf(Writer > 0, TEXT("Publish on an empty ros::Publisher")))
			dds_write(Writer, Sample); // serializes before returning, so the sample may point at temporaries
	}
}
