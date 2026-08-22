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

	Widget->SetTelemetrySource(FirstSource);
	Widget->SetTelemetrySource(FirstSource);
	TestTrue(TEXT("Flight HUD retains the first source"), Widget->GetTelemetrySource() == FirstSource);
	TestTrue(
		TEXT("Flight HUD binds to its first telemetry source"),
		FirstSource->OnTelemetryUpdated.Contains(Widget, FName(TEXT("HandleTelemetryUpdated"))));
	TestEqual(TEXT("Flight HUD is visible while telemetry is available"), Widget->GetVisibility(), ESlateVisibility::HitTestInvisible);

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

	FirstSnapshot.SpeedKilometersPerHour = 999.0f;
	FirstSource->OnTelemetryUpdated.Broadcast(FirstSnapshot);
	TestTrue(
		TEXT("An old telemetry source can no longer change the HUD"),
		FMath::IsNearlyEqual(Widget->GetDisplayedSnapshot().SpeedKilometersPerHour, 10.0f));

	Widget->ClearTelemetrySource();
	TestFalse(TEXT("Flight HUD clears its telemetry source"), Widget->HasTelemetrySource());
	TestFalse(
		TEXT("Clearing the source removes the final telemetry binding"),
		SecondSource->OnTelemetryUpdated.Contains(Widget, FName(TEXT("HandleTelemetryUpdated"))));
	TestEqual(TEXT("Flight HUD is collapsed without telemetry"), Widget->GetVisibility(), ESlateVisibility::Collapsed);

	return !HasAnyErrors();
}

#endif
