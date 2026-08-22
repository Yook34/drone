#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Telemetry/DroneTelemetryComponent.h"
#include "UI/DroneFlightHUDWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDroneFlightHUDTelemetryBindingTest,
	"Drone.UI.FlightHUDTelemetryBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDroneFlightHUDTelemetryBindingTest::RunTest(const FString& Parameters)
{
	// 이 테스트는 화면 디자인이 아니라 Event 연결·표시 포맷·정리 계약을 검증한다.
	UDroneFlightHUDWidget* Widget = NewObject<UDroneFlightHUDWidget>();
	UDroneTelemetryComponent* FirstSource = NewObject<UDroneTelemetryComponent>();
	UDroneTelemetryComponent* SecondSource = NewObject<UDroneTelemetryComponent>();
	TestNotNull(TEXT("Native Flight HUD Widget can be created"), Widget);
	TestNotNull(TEXT("First telemetry source can be created"), FirstSource);
	TestNotNull(TEXT("Second telemetry source can be created"), SecondSource);
	if (!Widget || !FirstSource || !SecondSource)
	{
		return false;
	}

	// 같은 Source를 두 번 지정해도 Dynamic Delegate는 한 번만 연결되어야 한다.
	Widget->SetTelemetrySource(FirstSource);
	Widget->SetTelemetrySource(FirstSource);
	TestTrue(TEXT("Flight HUD retains the first source"), Widget->GetTelemetrySource() == FirstSource);
	TestTrue(
		TEXT("Flight HUD binds to its first telemetry source"),
		FirstSource->OnTelemetryUpdated.Contains(Widget, FName(TEXT("HandleTelemetryUpdated"))));
	TestEqual(TEXT("Flight HUD is visible while telemetry is available"), Widget->GetVisibility(), ESlateVisibility::HitTestInvisible);

	// 첫 Event가 네 표시 문자열에 올바른 단위·자릿수·부호로 반영되는지 확인한다.
	FDroneTelemetrySnapshot FirstSnapshot;
	FirstSnapshot.SpeedKilometersPerHour = 42.5f;
	FirstSnapshot.AltitudeMeters = 18.2f;
	FirstSnapshot.VerticalSpeedMetersPerSecond = 1.4f;
	FirstSnapshot.HeadingDegrees = 315.0f;
	FirstSource->OnTelemetryUpdated.Broadcast(FirstSnapshot);

	TestEqual(TEXT("Speed text uses the HUD-02 candidate format"), Widget->GetSpeedDisplayText().ToString(), FString(TEXT("SPD  42.5 km/h")));
	TestEqual(TEXT("Altitude text uses the HUD-02 candidate format"), Widget->GetAltitudeDisplayText().ToString(), FString(TEXT("ALT  18.2 m")));
	TestEqual(TEXT("Vertical speed preserves its sign"), Widget->GetVerticalSpeedDisplayText().ToString(), FString(TEXT("V/S  +1.4 m/s")));
	TestEqual(TEXT("Heading text uses a normalized three-digit bearing"), Widget->GetHeadingDisplayText().ToString(), FString(TEXT("HDG  315\u00B0")));

	// Pawn 교체를 흉내 내어 이전 Source 해제 후 새 Source 연결을 검증한다.
	Widget->SetTelemetrySource(SecondSource);
	TestFalse(
		TEXT("Changing source removes the previous telemetry binding"),
		FirstSource->OnTelemetryUpdated.Contains(Widget, FName(TEXT("HandleTelemetryUpdated"))));
	TestTrue(
		TEXT("Changing source installs the new telemetry binding"),
		SecondSource->OnTelemetryUpdated.Contains(Widget, FName(TEXT("HandleTelemetryUpdated"))));

	FDroneTelemetrySnapshot SecondSnapshot;
	SecondSnapshot.SpeedKilometersPerHour = 10.0f;
	SecondSnapshot.AltitudeMeters = -2.0f;
	SecondSnapshot.VerticalSpeedMetersPerSecond = -0.5f;
	SecondSnapshot.HeadingDegrees = 359.6f;
	SecondSource->OnTelemetryUpdated.Broadcast(SecondSnapshot);
	TestEqual(TEXT("Descending vertical speed keeps its negative sign"), Widget->GetVerticalSpeedDisplayText().ToString(), FString(TEXT("V/S  -0.5 m/s")));
	TestEqual(TEXT("Rounded 360-degree heading wraps to north"), Widget->GetHeadingDisplayText().ToString(), FString(TEXT("HDG  000\u00B0")));

	// 이미 해제한 Source의 늦은 Event가 현재 HUD를 덮어쓰면 안 된다.
	FirstSnapshot.SpeedKilometersPerHour = 999.0f;
	FirstSource->OnTelemetryUpdated.Broadcast(FirstSnapshot);
	TestTrue(
		TEXT("An old telemetry source can no longer change the HUD"),
		FMath::IsNearlyEqual(Widget->GetDisplayedSnapshot().SpeedKilometersPerHour, 10.0f));

	// UnPossess/종료 경로처럼 Source를 비우면 구독 해제와 숨김이 함께 일어나야 한다.
	Widget->ClearTelemetrySource();
	TestFalse(TEXT("Flight HUD clears its telemetry source"), Widget->HasTelemetrySource());
	TestFalse(
		TEXT("Clearing the source removes the final telemetry binding"),
		SecondSource->OnTelemetryUpdated.Contains(Widget, FName(TEXT("HandleTelemetryUpdated"))));
	TestEqual(TEXT("Flight HUD is collapsed without telemetry"), Widget->GetVisibility(), ESlateVisibility::Collapsed);

	return !HasAnyErrors();
}

#endif
