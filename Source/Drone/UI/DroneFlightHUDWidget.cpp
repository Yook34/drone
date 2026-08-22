#include "UI/DroneFlightHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "Telemetry/DroneTelemetryComponent.h"

namespace DroneFlightHUD
{
const FName RootCanvasName(TEXT("FlightHUDRoot"));
const FName SpeedTextName(TEXT("SpeedValueText"));
const FName AltitudeTextName(TEXT("AltitudeValueText"));
const FName VerticalSpeedTextName(TEXT("VerticalSpeedValueText"));
const FName HeadingTextName(TEXT("HeadingValueText"));
}

void UDroneFlightHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildDefaultLayout();
	if (UDroneTelemetryComponent* CurrentSource = TelemetrySource.Get())
	{
		ApplySnapshot(CurrentSource->GetLatestSnapshot());
	}
	else
	{
		ApplyPlaceholderText();
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UDroneFlightHUDWidget::NativeDestruct()
{
	ClearTelemetrySource();
	Super::NativeDestruct();
}

void UDroneFlightHUDWidget::SetTelemetrySource(UDroneTelemetryComponent* InTelemetrySource)
{
	UDroneTelemetryComponent* CurrentSource = TelemetrySource.Get();
	if (CurrentSource != InTelemetrySource)
	{
		if (CurrentSource)
		{
			CurrentSource->OnTelemetryUpdated.RemoveDynamic(
				this,
				&UDroneFlightHUDWidget::HandleTelemetryUpdated);
		}

		TelemetrySource = InTelemetrySource;
	}

	if (InTelemetrySource)
	{
		InTelemetrySource->OnTelemetryUpdated.AddUniqueDynamic(
			this,
			&UDroneFlightHUDWidget::HandleTelemetryUpdated);
		ApplySnapshot(InTelemetrySource->GetLatestSnapshot());
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		TelemetrySource.Reset();
		ApplyPlaceholderText();
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UDroneFlightHUDWidget::ClearTelemetrySource()
{
	SetTelemetrySource(nullptr);
}

void UDroneFlightHUDWidget::HandleTelemetryUpdated(const FDroneTelemetrySnapshot Snapshot)
{
	ApplySnapshot(Snapshot);
}

void UDroneFlightHUDWidget::BuildDefaultLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	if (WidgetTree->RootWidget)
	{
		SpeedValueText = Cast<UTextBlock>(WidgetTree->FindWidget(DroneFlightHUD::SpeedTextName));
		AltitudeValueText = Cast<UTextBlock>(WidgetTree->FindWidget(DroneFlightHUD::AltitudeTextName));
		VerticalSpeedValueText = Cast<UTextBlock>(WidgetTree->FindWidget(DroneFlightHUD::VerticalSpeedTextName));
		HeadingValueText = Cast<UTextBlock>(WidgetTree->FindWidget(DroneFlightHUD::HeadingTextName));
		PushCachedTextToWidgets();
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		DroneFlightHUD::RootCanvasName);
	WidgetTree->RootWidget = RootCanvas;
	RootCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);

	UBorder* ReadoutPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("FlightReadoutPanel"));
	ReadoutPanel->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.035f, 0.78f));
	ReadoutPanel->SetPadding(FMargin(14.0f, 10.0f));
	ReadoutPanel->SetVisibility(ESlateVisibility::HitTestInvisible);

	UCanvasPanelSlot* ReadoutSlot = RootCanvas->AddChildToCanvas(ReadoutPanel);
	ReadoutSlot->SetAnchors(FAnchors(0.0f, 0.0f));
	ReadoutSlot->SetAlignment(FVector2D::ZeroVector);
	ReadoutSlot->SetPosition(FVector2D(24.0f, 24.0f));
	ReadoutSlot->SetAutoSize(true);

	UVerticalBox* ReadoutColumn = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("FlightReadoutColumn"));
	ReadoutPanel->SetContent(ReadoutColumn);

	UTextBlock* HeaderText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("FlightReadoutHeader"));
	HeaderText->SetText(FText::FromString(TEXT("FLIGHT DATA")));
	HeaderText->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 0.85f, 1.0f, 1.0f)));
	HeaderText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 16.0f));
	UVerticalBoxSlot* HeaderSlot = ReadoutColumn->AddChildToVerticalBox(HeaderText);
	HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	auto AddValueText = [this, ReadoutColumn](const FName WidgetName) -> UTextBlock*
	{
		UTextBlock* ValueText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			WidgetName);
		ValueText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		ValueText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 18.0f));
		ValueText->SetJustification(ETextJustify::Left);
		UVerticalBoxSlot* ValueSlot = ReadoutColumn->AddChildToVerticalBox(ValueText);
		ValueSlot->SetPadding(FMargin(0.0f, 1.0f));
		return ValueText;
	};

	SpeedValueText = AddValueText(DroneFlightHUD::SpeedTextName);
	AltitudeValueText = AddValueText(DroneFlightHUD::AltitudeTextName);
	VerticalSpeedValueText = AddValueText(DroneFlightHUD::VerticalSpeedTextName);
	HeadingValueText = AddValueText(DroneFlightHUD::HeadingTextName);
	PushCachedTextToWidgets();
}

void UDroneFlightHUDWidget::ApplySnapshot(const FDroneTelemetrySnapshot& Snapshot)
{
	DisplayedSnapshot = Snapshot;
	SpeedDisplayText = FText::FromString(FString::Printf(
		TEXT("SPD  %.1f km/h"),
		Snapshot.SpeedKilometersPerHour));
	AltitudeDisplayText = FText::FromString(FString::Printf(
		TEXT("ALT  %.1f m"),
		Snapshot.AltitudeMeters));
	VerticalSpeedDisplayText = FText::FromString(FString::Printf(
		TEXT("V/S  %+.1f m/s"),
		Snapshot.VerticalSpeedMetersPerSecond));

	const int32 RoundedHeading = FMath::RoundToInt(FRotator::ClampAxis(Snapshot.HeadingDegrees)) % 360;
	HeadingDisplayText = FText::FromString(FString::Printf(
		TEXT("HDG  %03d\u00B0"),
		RoundedHeading));
	PushCachedTextToWidgets();
}

void UDroneFlightHUDWidget::ApplyPlaceholderText()
{
	DisplayedSnapshot = FDroneTelemetrySnapshot();
	SpeedDisplayText = FText::FromString(TEXT("SPD  --.- km/h"));
	AltitudeDisplayText = FText::FromString(TEXT("ALT  --.- m"));
	VerticalSpeedDisplayText = FText::FromString(TEXT("V/S  --.- m/s"));
	HeadingDisplayText = FText::FromString(TEXT("HDG  ---\u00B0"));
	PushCachedTextToWidgets();
}

void UDroneFlightHUDWidget::PushCachedTextToWidgets()
{
	if (SpeedValueText)
	{
		SpeedValueText->SetText(SpeedDisplayText);
	}
	if (AltitudeValueText)
	{
		AltitudeValueText->SetText(AltitudeDisplayText);
	}
	if (VerticalSpeedValueText)
	{
		VerticalSpeedValueText->SetText(VerticalSpeedDisplayText);
	}
	if (HeadingValueText)
	{
		HeadingValueText->SetText(HeadingDisplayText);
	}
}
