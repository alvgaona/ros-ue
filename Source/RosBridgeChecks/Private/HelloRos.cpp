#include "HelloRos.h"
#include "TimerManager.h"

void AHelloRos::BeginPlay()
{
	Super::BeginPlay();
	Publisher = ros::CreatePublisher<std_msgs::msg::String>(this, TEXT("/chatter"));
	Subscription = ros::CreateSubscription<std_msgs::msg::String>(this, TEXT("/chatter"), [](const std_msgs::msg::String& Msg) {
		UE_LOG(LogRos, Log, TEXT("I heard: [%s]"), UTF8_TO_TCHAR(Msg.data));
	});
	GetWorldTimerManager().SetTimer(Timer, this, &AHelloRos::Talk, 1.0f, true);
}

void AHelloRos::Talk()
{
	const FString Text = FString::Printf(TEXT("Hello World: %d"), ++Count);
	Publisher.Publish(std_msgs::msg::String{ TCHAR_TO_UTF8(*Text) });
	UE_LOG(LogRos, Log, TEXT("Publishing: '%s'"), *Text);
}

void AHelloRos::EndPlay(const EEndPlayReason::Type Reason)
{
	Publisher = {}; // ROS sees the writer and reader leave
	Subscription = {};
	Super::EndPlay(Reason);
}
