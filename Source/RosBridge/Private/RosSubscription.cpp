#include "RosSubscription.h"

namespace ros
{
	Reader::Reader(dds_entity_t InEntity, TFunction<void(const void*)> InCallback)
		: Entity(InEntity)
		, Callback(MoveTemp(InCallback))
	{
		ensureMsgf(Entity > 0, TEXT("DDS reader create failed: %s"), UTF8_TO_TCHAR(dds_strretcode(Entity)));
	}

	Reader::~Reader()
	{
		if (Entity > 0) dds_delete(Entity);
	}

	void Reader::Spin()
	{
		for (;;)
		{
			void* Samples[16] = {}; // null first entry: Cyclone loans the buffers
			dds_sample_info_t Infos[16];
			const int32 N = dds_take(Entity, Samples, Infos, 16, 16);
			if (N <= 0) return;
			for (int32 I = 0; I < N; ++I)
				if (Infos[I].valid_data) Callback(Samples[I]);
			dds_return_loan(Entity, Samples, N);
		}
	}
}
