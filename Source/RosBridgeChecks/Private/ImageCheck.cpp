#include "ImageCheck.h"
#include "Engine/World.h"

AImageCheck::AImageCheck()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AImageCheck::BeginPlay()
{
	Super::BeginPlay();
	FString W, H;
	if (FPlatformMisc::GetEnvironmentVariable(TEXT("IMAGE_SIZE")).Split(TEXT("x"), &W, &H))
	{
		Width = FCString::Atoi(*W);
		Height = FCString::Atoi(*H);
	}
	const FString Qos = FPlatformMisc::GetEnvironmentVariable(TEXT("IMAGE_QOS"));
	Images = ros::CreatePublisher<sensor_msgs::msg::Image>(this, TEXT("/image_raw"),
		Qos == TEXT("keep1") ? ros::Qos(1) : Qos == TEXT("sensor") ? ros::SensorDataQos() : ros::Qos(10));
	WriteSeconds = ros::CreatePublisher<std_msgs::msg::Float64>(this, TEXT("/image_check/write_seconds"), 1000);

	// Blue and green carry x and y, red their high bits, so every byte depends on where it is
	Pixels.SetNumUninitialized(Width * Height * 4);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			uint8* Pixel = &Pixels[(Y * Width + X) * 4];
			Pixel[0] = X & 0xFF;
			Pixel[1] = Y & 0xFF;
			Pixel[2] = (X >> 8) | ((Y >> 8) << 4);
			Pixel[3] = 0xFF;
		}
	}
	NextSend = GetWorld()->GetTimeSeconds();
}

void AImageCheck::EndPlay(const EEndPlayReason::Type Reason)
{
	Images = {};
	WriteSeconds = {};
	Super::EndPlay(Reason);
}

void AImageCheck::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Time = GetWorld()->GetTimeSeconds();
	if (Time + 1e-9 < NextSend)
		return;
	NextSend = FMath::Max(NextSend + 1.0 / 30, Time);

	sensor_msgs::msg::Image Image{};
	Image.header.stamp = ros::Now(this);
	Image.header.frame_id = const_cast<char*>("camera");
	Image.height = Height;
	Image.width = Width;
	Image.encoding = const_cast<char*>("bgra8");
	Image.step = Width * 4;
	Image.data._buffer = Pixels.GetData();
	Image.data._length = Image.data._maximum = Pixels.Num();
	const double Start = FPlatformTime::Seconds();
	Images.Publish(Image);
	WriteSeconds.Publish({ FPlatformTime::Seconds() - Start });
}
