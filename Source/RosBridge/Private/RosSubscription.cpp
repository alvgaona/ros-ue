#include "RosSubscription.h"

RosReader::RosReader(dds_entity_t InReader, TFunction<void(const void*)> InCallback)
	: Reader(InReader), Callback(MoveTemp(InCallback))
{
	ensureMsgf(Reader > 0, TEXT("DDS reader create failed: %s"), UTF8_TO_TCHAR(dds_strretcode(Reader)));
}

RosReader::~RosReader()
{
	if (Reader > 0) dds_delete(Reader);
}

void RosReader::Spin()
{
	for (;;)
	{
		void* Samples[16] = {}; // null first entry: Cyclone loans the buffers
		dds_sample_info_t Infos[16];
		const int32 N = dds_take(Reader, Samples, Infos, 16, 16);
		if (N <= 0) return;
		for (int32 I = 0; I < N; ++I)
			if (Infos[I].valid_data) Callback(Samples[I]);
		dds_return_loan(Reader, Samples, N);
	}
}
