#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include "dds/dds.h"
THIRD_PARTY_INCLUDES_END

namespace ros
{
	// The DDS reader and callback behind a Subscription. URos spins it while a subscription holds it.
	class ROSBRIDGE_API Reader
	{
	public:
		Reader(dds_entity_t InEntity, TFunction<void(const void*)> InCallback);
		~Reader();
		Reader(const Reader&) = delete;
		Reader& operator=(const Reader&) = delete;

		void Spin();

	private:
		dds_entity_t Entity;
		TFunction<void(const void*)> Callback;
	};

	// Receives messages until it is destroyed or cleared with `= {}`. Move-only.
	class Subscription
	{
	public:
		Subscription() = default;
		explicit Subscription(TSharedRef<Reader> InReader) : Impl(MoveTemp(InReader)) {}
		Subscription(Subscription&&) = default;
		Subscription& operator=(Subscription&&) = default;

	private:
		TSharedPtr<Reader> Impl;
	};
}
