#include "RosPublisher.h"
#include "Ros.h"

namespace ros
{
	PublisherBase::PublisherBase(dds_entity_t InWriter)
		: Writer(InWriter)
	{
		ensureMsgf(Writer > 0, TEXT("DDS writer create failed: %s"), UTF8_TO_TCHAR(dds_strretcode(Writer)));
	}

	PublisherBase::PublisherBase(PublisherBase&& Other)
		: Writer(Other.Writer)
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

	int32 PublisherBase::SubscriptionCount() const
	{
		dds_publication_matched_status_t Status{};
		return Writer > 0 && dds_get_publication_matched_status(Writer, &Status) == DDS_RETCODE_OK ? Status.current_count : 0;
	}

	void PublisherBase::Write(const void* Sample) const
	{
		if (!ensureMsgf(Writer > 0, TEXT("Publish on an empty ros::Publisher")))
			return;
		const dds_return_t Result = dds_write(Writer, Sample); // serializes before returning, so the sample may point at temporaries
		if (Result < 0)
		{
			char Topic[256] = "";
			dds_get_name(dds_get_topic(Writer), Topic, sizeof(Topic));
			UE_LOG(LogRos, Warning, TEXT("DDS write on %s failed: %s"), UTF8_TO_TCHAR(Topic), UTF8_TO_TCHAR(dds_strretcode(Result)));
		}
	}
}
