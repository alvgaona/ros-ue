#include "RosCameraComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GlobalShader.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "RenderGraphUtils.h"
#include "RenderingThread.h"
#include "RHIGPUReadback.h"
#include "Ros.h"
#include "ShaderParameterStruct.h"
#include "Tasks/Task.h"
#include "TextureResource.h"

namespace
{
	// Outside namespace ros because the shader macros name the type by its spelling
	class PackBgr8Shader : public FGlobalShader
	{
		DECLARE_GLOBAL_SHADER(PackBgr8Shader);
		SHADER_USE_PARAMETER_STRUCT(PackBgr8Shader, FGlobalShader);
		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, Source)
			SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, Packed)
			SHADER_PARAMETER(uint32, Width)
			SHADER_PARAMETER(uint32, Pixels)
		END_SHADER_PARAMETER_STRUCT()
	};
}

IMPLEMENT_GLOBAL_SHADER(PackBgr8Shader, "/Plugin/RosBridge/Private/RosCamera.usf", "MainCS", SF_Compute);

namespace ros
{
	// The camera's render thread side, which reads images back once the GPU has them, and the worker that publishes them.
	class CameraStream : public TSharedFromThis<CameraStream, ESPMode::ThreadSafe>
	{
	public:
		CameraStream(const URosCameraComponent& Camera, Publisher<sensor_msgs::msg::Image> InImages, Publisher<sensor_msgs::msg::CompressedImage> InJpegs)
			: Images(MoveTemp(InImages))
			, Jpegs(MoveTemp(InJpegs))
			, FrameId(Camera.FrameId)
			, Width(Camera.Width)
			, Height(Camera.Height)
			, JpegQuality(Camera.JpegQuality)
			, ImageWrappers(FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper")))
		{
		}

		void Read(FRHICommandListImmediate& RHICmdList, FRHITexture* Texture, const builtin_interfaces::msg::Time& Stamp)
		{
			// A buffer, not a texture: Metal maps a readback texture only once the GPU is idle, and a buffer once the copy is done
			TUniquePtr<FRHIGPUBufferReadback> Readback = Free.IsEmpty() ? MakeUnique<FRHIGPUBufferReadback>(TEXT("RosCamera")) : Free.Pop();
			const int32 Threads = FMath::DivideAndRoundUp(Width * Height, 4);
			FRDGBuilder GraphBuilder(RHICmdList);
			FRDGBufferRef Packed = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), Threads * 3), TEXT("RosCamera"));
			PackBgr8Shader::FParameters* Parameters = GraphBuilder.AllocParameters<PackBgr8Shader::FParameters>();
			Parameters->Source = RegisterExternalTexture(GraphBuilder, Texture, TEXT("RosCamera"));
			Parameters->Packed = GraphBuilder.CreateUAV(Packed, PF_R32_UINT);
			Parameters->Width = Width;
			Parameters->Pixels = Width * Height;
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("RosCamera"), TShaderMapRef<PackBgr8Shader>(GetGlobalShaderMap(GMaxRHIFeatureLevel)),
				Parameters, FComputeShaderUtils::GetGroupCount(Threads, 256));
			AddEnqueueCopyPass(GraphBuilder, Readback.Get(), Packed, Width * Height * 3);
			GraphBuilder.Execute();
			Reading.Add({ MoveTemp(Readback), Stamp });
		}

		void Publish()
		{
			// Oldest first, one at a time. While the worker sends one, only the newest image the GPU has ready waits for it.
			while (Reading.Num() > 1 && Reading[1].Readback->IsReady() && !Publishing.IsCompleted())
			{
				Free.Add(MoveTemp(Reading[0].Readback));
				Reading.RemoveAt(0);
			}
			if (Reading.IsEmpty() || !Reading[0].Readback->IsReady() || !Publishing.IsCompleted())
				return;
			Capture Done = MoveTemp(Reading[0]);
			Reading.RemoveAt(0);
			Pixels.SetNumUninitialized(Width * Height * 3);
			FMemory::Memcpy(Pixels.GetData(), Done.Readback->Lock(Pixels.Num()), Pixels.Num());
			Done.Readback->Unlock();
			Free.Add(MoveTemp(Done.Readback));
			Publishing = UE::Tasks::Launch(TEXT("RosCamera"), [Self = AsShared(), Stamp = Done.Stamp] { Self->Send(Stamp); });
		}

	private:
		struct Capture
		{
			TUniquePtr<FRHIGPUBufferReadback> Readback;
			builtin_interfaces::msg::Time Stamp;
		};

		void Send(const builtin_interfaces::msg::Time& Stamp)
		{
			const FTCHARToUTF8 Frame(*FrameId);
			sensor_msgs::msg::Image Message{};
			Message.header.stamp = Stamp;
			Message.header.frame_id = const_cast<char*>(Frame.Get());
			Message.height = Height;
			Message.width = Width;
			Message.encoding = const_cast<char*>("bgr8");
			Message.step = Width * 3;
			Message.data._buffer = Pixels.GetData();
			Message.data._length = Message.data._maximum = Pixels.Num();
			if (Images.SubscriptionCount() > 0)
				Images.Publish(Message);
			if (Jpegs.SubscriptionCount() == 0)
				return;

			// Unreal's JPEG encoder takes four bytes a pixel
			Bgra.SetNumUninitialized(Width * Height * 4);
			for (int32 I = 0; I < Width * Height; ++I)
			{
				Bgra[I * 4] = Pixels[I * 3];
				Bgra[I * 4 + 1] = Pixels[I * 3 + 1];
				Bgra[I * 4 + 2] = Pixels[I * 3 + 2];
				Bgra[I * 4 + 3] = 255;
			}
			const TSharedPtr<IImageWrapper> Encoder = ImageWrappers.CreateImageWrapper(EImageFormat::JPEG);
			Encoder->SetRaw(Bgra.GetData(), Bgra.Num(), Width, Height, ERGBFormat::BGRA, 8);
			TArray64<uint8> Jpeg = Encoder->GetCompressed(JpegQuality);
			sensor_msgs::msg::CompressedImage Compressed{};
			Compressed.header = Message.header;
			Compressed.format = const_cast<char*>("bgr8; jpeg compressed bgr8"); // what image_transport's decoder reads
			Compressed.data._buffer = Jpeg.GetData();
			Compressed.data._length = Compressed.data._maximum = Jpeg.Num();
			Jpegs.Publish(Compressed);
		}

		const Publisher<sensor_msgs::msg::Image> Images;
		const Publisher<sensor_msgs::msg::CompressedImage> Jpegs;
		const FString FrameId;
		const int32 Width;
		const int32 Height;
		const int32 JpegQuality;
		IImageWrapperModule& ImageWrappers;
		// The render thread's, except Pixels, which the worker has until Publishing completes
		TArray<Capture> Reading;
		TArray<TUniquePtr<FRHIGPUBufferReadback>> Free;
		TArray<uint8> Pixels;
		TArray<uint8> Bgra; // the worker's
		UE::Tasks::FTask Publishing;
	};
}

URosCameraComponent::URosCameraComponent()
{
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork; // everything has moved by then, physics included
	bTickInEditor = false;
	bCaptureEveryFrame = false;
	bCaptureOnMovement = false;
	bAlwaysPersistRenderingState = true; // temporal effects and exposure carry over from one capture to the next
	CaptureSource = SCS_FinalColorLDR;
}

void URosCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	TextureTarget = NewObject<UTextureRenderTarget2D>(this);
	TextureTarget->RenderTargetFormat = RTF_RGBA8; // 8 bits a channel, as bgr8 has
	TextureTarget->InitAutoFormat(Width, Height);
	Stream = MakeShared<ros::CameraStream, ESPMode::ThreadSafe>(*this,
		Raw ? ros::CreatePublisher<sensor_msgs::msg::Image>(this, Topic, Qos) : ros::Publisher<sensor_msgs::msg::Image>(),
		Compressed ? ros::CreatePublisher<sensor_msgs::msg::CompressedImage>(this, Topic + TEXT("/compressed"), Qos) : ros::Publisher<sensor_msgs::msg::CompressedImage>());
	NextCapture = GetWorld()->GetTimeSeconds();
}

void URosCameraComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	Stream = nullptr;
	Super::EndPlay(Reason);
}

void URosCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Stream)
		return;
	FTextureRenderTargetResource* Target = nullptr;
	const double Time = GetWorld()->GetTimeSeconds();
	if (Time + 1e-9 >= NextCapture)
	{
		NextCapture = FMath::Max(NextCapture + 1.0 / FrameRate, Time);
		CaptureScene();
		Target = TextureTarget->GameThread_GetRenderTargetResource();
	}
	// Every frame, so an image goes out as soon as the GPU has it
	ENQUEUE_RENDER_COMMAND(RosCamera)([Stream = Stream, Target, Stamp = ros::Now(this)](FRHICommandListImmediate& RHICmdList) {
		if (Target)
			Stream->Read(RHICmdList, Target->GetRenderTargetTexture(), Stamp);
		Stream->Publish();
	});
}
