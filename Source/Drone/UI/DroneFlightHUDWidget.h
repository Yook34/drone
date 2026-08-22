#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Telemetry/DroneTelemetryTypes.h"
#include "DroneFlightHUDWidget.generated.h"

class UDroneTelemetryComponent;
class UTextBlock;

/**
 * Minimal event-driven Flight HUD used by the Drone prototype.
 *
 * The native layout is intentionally plain so a later Widget Blueprint can
 * replace the presentation without moving telemetry calculations into UI.
 */
UCLASS()
class UDroneFlightHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Switches the event source and immediately displays its latest Snapshot. */
	UFUNCTION(BlueprintCallable, Category="Drone|HUD")
	void SetTelemetrySource(UDroneTelemetryComponent* InTelemetrySource);

	/** Releases the current telemetry event subscription and hides the HUD. */
	UFUNCTION(BlueprintCallable, Category="Drone|HUD")
	void ClearTelemetrySource();

	UFUNCTION(BlueprintPure, Category="Drone|HUD")
	UDroneTelemetryComponent* GetTelemetrySource() const { return TelemetrySource.Get(); }

	UFUNCTION(BlueprintPure, Category="Drone|HUD")
	bool HasTelemetrySource() const { return TelemetrySource.IsValid(); }

	UFUNCTION(BlueprintPure, Category="Drone|HUD")
	FDroneTelemetrySnapshot GetDisplayedSnapshot() const { return DisplayedSnapshot; }

	UFUNCTION(BlueprintPure, Category="Drone|HUD")
	FText GetSpeedDisplayText() const { return SpeedDisplayText; }

	UFUNCTION(BlueprintPure, Category="Drone|HUD")
	FText GetAltitudeDisplayText() const { return AltitudeDisplayText; }

	UFUNCTION(BlueprintPure, Category="Drone|HUD")
	FText GetVerticalSpeedDisplayText() const { return VerticalSpeedDisplayText; }

	UFUNCTION(BlueprintPure, Category="Drone|HUD")
	FText GetHeadingDisplayText() const { return HeadingDisplayText; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleTelemetryUpdated(FDroneTelemetrySnapshot Snapshot);

	void BuildDefaultLayout();
	void ApplySnapshot(const FDroneTelemetrySnapshot& Snapshot);
	void ApplyPlaceholderText();
	void PushCachedTextToWidgets();

	TWeakObjectPtr<UDroneTelemetryComponent> TelemetrySource;

	UPROPERTY(Transient)
	FDroneTelemetrySnapshot DisplayedSnapshot;

	UPROPERTY(Transient)
	FText SpeedDisplayText;

	UPROPERTY(Transient)
	FText AltitudeDisplayText;

	UPROPERTY(Transient)
	FText VerticalSpeedDisplayText;

	UPROPERTY(Transient)
	FText HeadingDisplayText;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeedValueText;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> AltitudeValueText;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> VerticalSpeedValueText;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> HeadingValueText;
};
