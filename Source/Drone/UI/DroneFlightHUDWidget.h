#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Telemetry/DroneTelemetryTypes.h"
#include "DroneFlightHUDWidget.generated.h"

class UDroneTelemetryComponent;
class UTextBlock;

/**
 * Drone Prototype의 비행 정보를 표시하는 공용 HUD 기반 클래스.
 *
 * 책임 분리:
 * - UDroneTelemetryComponent: 속도·고도·수직 속도·Heading 계산
 * - UDroneFlightHUDWidget: Snapshot을 표시용 Text로 바꾸고 화면에 반영
 * - Widget Blueprint: 위치·색·폰트 같은 최종 외형만 편집
 *
 * WBP를 만들 때 아래 BindWidget 멤버와 같은 이름의 TextBlock 4개를 배치하면
 * C++가 자동으로 찾아 갱신한다. 이름/타입이 틀리면 WBP 컴파일에서 바로 잡는다.
 * Designer Tree가 없는 native Class를 직접 실행할 때는 학습·테스트용 fallback을 생성한다.
 */
UCLASS(Blueprintable)
class UDroneFlightHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 기존 Event 연결을 해제하고 새 Telemetry Source의 최신 Snapshot을 즉시 표시한다. */
	UFUNCTION(BlueprintCallable, Category="Drone|HUD")
	void SetTelemetrySource(UDroneTelemetryComponent* InTelemetrySource);

	/** 현재 Telemetry Event 연결을 해제하고 데이터가 없는 HUD를 숨긴다. */
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

	/** true면 WBP Designer 대신 C++ 기본 레이아웃을 사용하고 있다는 뜻이다. */
	UFUNCTION(BlueprintPure, Category="Drone|HUD")
	bool IsUsingNativeFallbackLayout() const { return bUsingNativeFallbackLayout; }

protected:
	/** Widget 인스턴스당 한 번 호출되며 WBP 또는 native fallback 레이아웃을 준비한다. */
	virtual void NativeOnInitialized() override;

	/** Widget 제거 시 Telemetry Delegate가 남지 않도록 명시적으로 정리한다. */
	virtual void NativeDestruct() override;

private:
	/** 주기적 또는 즉시 갱신 Telemetry Event가 도착했을 때 호출되는 C++ 수신 함수다. */
	UFUNCTION()
	void HandleTelemetryUpdated(FDroneTelemetrySnapshot Snapshot);

	/** WBP TextBlock 계약을 먼저 확인하고, 없으면 실행 가능한 C++ 기본 UI를 만든다. */
	void BuildDefaultLayout();

	/** WBP Designer의 정확한 이름을 가진 TextBlock 4개를 찾는다. */
	bool TryBindBlueprintLayout();

	/** Snapshot 값 자체를 다시 계산하지 않고 표시 문자열만 만든다. */
	void ApplySnapshot(const FDroneTelemetrySnapshot& Snapshot);

	/** Telemetry Source가 없을 때 사용할 자리표시자 Text를 준비한다. */
	void ApplyPlaceholderText();

	/** 캐시된 Text를 현재 WBP 또는 native TextBlock에 한 번씩 밀어 넣는다. */
	void PushCachedTextToWidgets();

	/** Pawn이 파괴돼도 강한 참조로 수명을 늘리지 않도록 Weak Pointer를 사용한다. */
	TWeakObjectPtr<UDroneTelemetryComponent> TelemetrySource;

	/** 테스트와 Blueprint 디버깅에서 마지막으로 받은 원본 Snapshot을 확인한다. */
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

	/**
	 * 아래 네 이름은 C++ ↔ WBP Designer 계약이다.
	 * BindWidget은 위젯을 생성하지 않는다. WBP Designer에 같은 이름·타입이 있어야
	 * 컴파일에 성공하고, 생성된 Widget 인스턴스의 이 포인터에 연결된다.
	 */
	UPROPERTY(Transient, meta=(BindWidget))
	TObjectPtr<UTextBlock> SpeedValueText;

	UPROPERTY(Transient, meta=(BindWidget))
	TObjectPtr<UTextBlock> AltitudeValueText;

	UPROPERTY(Transient, meta=(BindWidget))
	TObjectPtr<UTextBlock> VerticalSpeedValueText;

	UPROPERTY(Transient, meta=(BindWidget))
	TObjectPtr<UTextBlock> HeadingValueText;

	UPROPERTY(Transient)
	bool bUsingNativeFallbackLayout = false;
};
