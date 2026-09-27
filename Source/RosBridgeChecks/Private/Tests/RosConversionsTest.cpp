#include "RosConversions.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
	constexpr double Tolerance = 1e-9;

	// Converted values as plain numbers in Unreal's math types, to do on them what tf2 does on the ROS side
	FVector AsMath(const geometry_msgs::msg::Vector3& V) { return FVector(V.x, V.y, V.z); }
	FQuat AsMath(const geometry_msgs::msg::Quaternion& Q) { return FQuat(Q.x, Q.y, Q.z, Q.w); }
	FTransform AsMath(const geometry_msgs::msg::Transform& T) { return FTransform(AsMath(T.rotation), AsMath(T.translation)); }

	// Rotating in Unreal and then converting must match converting and then rotating in ROS
	void TestRotatesAlike(FAutomationTestBase& Test, const FRotator& Rotator)
	{
		const FQuat Rotation = Rotator.Quaternion();
		for (const FVector& Axis : { FVector(100, 0, 0), FVector(0, 100, 0), FVector(0, 0, 100) })
		{
			const FVector Expected = AsMath(ros::ToVector3(Rotation.RotateVector(Axis)));
			const FVector Actual = AsMath(ros::ToQuaternion(Rotation)).RotateVector(AsMath(ros::ToVector3(Axis)));
			Test.TestEqual(*FString::Printf(TEXT("%s on %s"), *Rotator.ToString(), *Axis.ToString()), Actual, Expected, Tolerance);
		}
	}

	void TestTime(FAutomationTestBase& Test, double Seconds, int64 Sec, int64 Nanosec)
	{
		const builtin_interfaces::msg::Time Time = ros::ToTime(Seconds);
		Test.TestEqual(*FString::Printf(TEXT("%.10f s: sec"), Seconds), static_cast<int64>(Time.sec), Sec);
		Test.TestEqual(*FString::Printf(TEXT("%.10f s: nanosec"), Seconds), static_cast<int64>(Time.nanosec), Nanosec);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RosConversionsPositionTest, "RosBridge.Conversions.Position", Flags)
bool RosConversionsPositionTest::RunTest(const FString&)
{
	// Unreal's +Y points right, ROS's +Y left
	TestEqual(TEXT("(100, 200, 50) cm"), AsMath(ros::ToVector3(FVector(100, 200, 50))), FVector(1, -2, 0.5), Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RosConversionsYawTest, "RosBridge.Conversions.Yaw", Flags)
bool RosConversionsYawTest::RunTest(const FString&)
{
	// Turning right is yaw +90 in Unreal and yaw -90 in ROS
	const FQuat Right = AsMath(ros::ToQuaternion(FRotator(0, 90, 0).Quaternion()));
	const FQuat Expected(FVector(0, 0, 1), FMath::DegreesToRadians(-90.0));
	TestTrue(*FString::Printf(TEXT("yaw -90: %s is %s"), *Right.ToString(), *Expected.ToString()), Right.Equals(Expected, Tolerance));
	TestEqual(TEXT("forward turns to ROS's right"), Right.RotateVector(FVector(1, 0, 0)), FVector(0, -1, 0), Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RosConversionsPitchTest, "RosBridge.Conversions.Pitch", Flags)
bool RosConversionsPitchTest::RunTest(const FString&)
{
	TestRotatesAlike(*this, FRotator(30, 0, 0));
	// Nose up is pitch +30 in Unreal and pitch -30 in ROS, with forward tilting toward +Z in both
	const FQuat Up = AsMath(ros::ToQuaternion(FRotator(30, 0, 0).Quaternion()));
	TestEqual(TEXT("forward tilts up"), Up.RotateVector(FVector(1, 0, 0)), FVector(FMath::Cos(FMath::DegreesToRadians(30.0)), 0, 0.5), Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RosConversionsRollTest, "RosBridge.Conversions.Roll", Flags)
bool RosConversionsRollTest::RunTest(const FString&)
{
	TestRotatesAlike(*this, FRotator(0, 0, 30));
	// A clockwise roll in Unreal drops the right side, which is ROS's -Y
	const FQuat Clockwise = AsMath(ros::ToQuaternion(FRotator(0, 0, 30).Quaternion()));
	TestEqual(TEXT("right side drops"), Clockwise.RotateVector(FVector(0, -1, 0)), FVector(0, -FMath::Cos(FMath::DegreesToRadians(30.0)), -0.5), Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RosConversionsCompositionTest, "RosBridge.Conversions.Composition", Flags)
bool RosConversionsCompositionTest::RunTest(const FString&)
{
	// A child's pose relative to its parent comes out the same whether it's computed before or after converting
	const FTransform Parent(FRotator(10, 30, -20), FVector(100, -50, 20));
	const FTransform Child(FRotator(-5, 120, 45), FVector(-30, 200, 75));
	const FTransform Before = AsMath(ros::ToTransform(Child.GetRelativeTransform(Parent)));
	const FTransform After = AsMath(ros::ToTransform(Child)).GetRelativeTransform(AsMath(ros::ToTransform(Parent)));
	TestEqual(TEXT("translation"), Before.GetTranslation(), After.GetTranslation(), Tolerance);
	TestTrue(*FString::Printf(TEXT("rotation: %s is %s"), *Before.GetRotation().ToString(), *After.GetRotation().ToString()), Before.GetRotation().Equals(After.GetRotation(), Tolerance));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RosConversionsTimeTest, "RosBridge.Conversions.Time", Flags)
bool RosConversionsTimeTest::RunTest(const FString&)
{
	TestTime(*this, 1.5, 1, 500000000);
	TestTime(*this, 1.9999999999, 2, 0);
	return true;
}

#endif
