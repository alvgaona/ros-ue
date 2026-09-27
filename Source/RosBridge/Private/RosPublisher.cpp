#include "RosPublisher.h"

RosPublisherBase::RosPublisherBase(dds_entity_t InWriter) : Writer(InWriter)
{
	ensureMsgf(Writer > 0, TEXT("DDS writer create failed: %s"), UTF8_TO_TCHAR(dds_strretcode(Writer)));
}

RosPublisherBase::RosPublisherBase(RosPublisherBase&& Other) : Writer(Other.Writer)
{
	Other.Writer = 0;
}

RosPublisherBase& RosPublisherBase::operator=(RosPublisherBase&& Other)
{
	if (this != &Other)
	{
		if (Writer > 0) dds_delete(Writer);
		Writer = Other.Writer;
		Other.Writer = 0;
	}
	return *this;
}

RosPublisherBase::~RosPublisherBase()
{
	if (Writer > 0) dds_delete(Writer);
}

void RosPublisherBase::Write(const void* Sample) const
{
	if (ensureMsgf(Writer > 0, TEXT("Publish on an empty RosPublisher")))
		dds_write(Writer, Sample); // serializes before returning, so the sample may point at temporaries
}
