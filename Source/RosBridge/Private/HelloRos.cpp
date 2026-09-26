#include "HelloRos.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"

void AHelloRos::BeginPlay()
{
	Super::BeginPlay();
	URos* Ros = UGameInstance::GetSubsystem<URos>(GetGameInstance());
	Publisher = Ros->CreatePublisher<FStringMsg>(TEXT("/chatter"));
	Subscription = Ros->CreateSubscription<FStringMsg>(TEXT("/chatter"), [](const FStringMsg& Msg)
	{
		UE_LOG(LogTemp, Log, TEXT("I heard: [%s]"), UTF8_TO_TCHAR(Msg.data));
	});
	GetWorldTimerManager().SetTimer(Timer, this, &AHelloRos::Talk, 1.0f, true);
}

void AHelloRos::Talk()
{
	const FString Text = FString::Printf(TEXT("Hello World: %d"), ++Count);
	Publisher->Publish(FStringMsg{ TCHAR_TO_UTF8(*Text) });
	UE_LOG(LogTemp, Log, TEXT("Publishing: '%s'"), *Text);
}

void AHelloRos::EndPlay(const EEndPlayReason::Type Reason)
{
	Publisher.Reset(); // ROS sees the writer and reader leave
	Subscription.Reset();
	Super::EndPlay(Reason);
}
