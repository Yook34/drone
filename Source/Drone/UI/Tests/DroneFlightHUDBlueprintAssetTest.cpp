#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Prototype/DronePrototypeGameMode.h"
#include "Prototype/DronePrototypePlayerController.h"
#include "UI/DroneFlightHUDWidget.h"

namespace DroneFlightHUDBlueprintAsset
{
// 실제 실행에 사용하는 Blueprint Generated Class 경로다. Asset 이름 변경 시 테스트도 함께 고친다.
constexpr const TCHAR* WidgetClassPath =
	TEXT("/Game/Drone/Prototype/UI/WBP_DroneFlightHUD.WBP_DroneFlightHUD_C");
constexpr const TCHAR* ControllerClassPath =
	TEXT("/Game/Drone/Prototype/Blueprints/BP_DronePrototypePlayerController.BP_DronePrototypePlayerController_C");
constexpr const TCHAR* GameModeClassPath =
	TEXT("/Game/Drone/Prototype/Blueprints/BP_DronePrototypeGameMode.BP_DronePrototypeGameMode_C");

const FName RequiredTextNames[] = {
	TEXT("SpeedValueText"),
	TEXT("AltitudeValueText"),
	TEXT("VerticalSpeedValueText"),
	TEXT("HeadingValueText")
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDroneFlightHUDBlueprintAssetTest,
	"Drone.UI.FlightHUDBlueprintAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDroneFlightHUDBlueprintAssetTest::RunTest(const FString& Parameters)
{
	// 1) WBP가 native 데이터/수명주기 클래스를 올바르게 상속하는지 확인한다.
	UClass* WidgetClass = LoadClass<UDroneFlightHUDWidget>(
		nullptr,
		DroneFlightHUDBlueprintAsset::WidgetClassPath);
	TestNotNull(TEXT("WBP_DroneFlightHUD generated Class loads"), WidgetClass);
	if (!WidgetClass)
	{
		return false;
	}
	TestTrue(
		TEXT("WBP_DroneFlightHUD derives from the native Flight HUD base"),
		WidgetClass->IsChildOf(UDroneFlightHUDWidget::StaticClass()));

	// 2) Designer에 C++ BindWidget 계약인 TextBlock 4개가 실제로 저장됐는지 확인한다.
	const UWidgetBlueprintGeneratedClass* WidgetBlueprintClass =
		Cast<UWidgetBlueprintGeneratedClass>(WidgetClass);
	TestNotNull(TEXT("Flight HUD uses a Widget Blueprint generated Class"), WidgetBlueprintClass);
	const UWidgetTree* WidgetTree = WidgetBlueprintClass
		? WidgetBlueprintClass->GetWidgetTreeArchetype()
		: nullptr;
	TestNotNull(TEXT("WBP_DroneFlightHUD has a Designer WidgetTree"), WidgetTree);
	if (WidgetTree)
	{
		for (const FName RequiredName : DroneFlightHUDBlueprintAsset::RequiredTextNames)
		{
			const UTextBlock* RequiredWidget = Cast<UTextBlock>(WidgetTree->FindWidget(RequiredName));
			TestTrue(
				*FString::Printf(TEXT("%s exists as a Designer TextBlock"), *RequiredName.ToString()),
				RequiredWidget != nullptr);
			if (RequiredWidget)
			{
				// FontObject가 비어 있으면 Standalone에서 글자가 깨지거나 대체 글리프로 보일 수 있다.
				TestTrue(
					*FString::Printf(TEXT("%s stores a valid Designer font"), *RequiredName.ToString()),
					RequiredWidget->GetFont().HasValidFont());
			}
		}

		const UTextBlock* HeaderText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("FlightReadoutHeader")));
		TestNotNull(TEXT("FlightReadoutHeader exists as a Designer TextBlock"), HeaderText);
		if (HeaderText)
		{
			TestTrue(TEXT("FlightReadoutHeader stores a valid Designer font"), HeaderText->GetFont().HasValidFont());
		}
	}

	// 3) BP Controller가 WBP Class를 선택하고 있는지 확인한다.
	UClass* ControllerClass = LoadClass<ADronePrototypePlayerController>(
		nullptr,
		DroneFlightHUDBlueprintAsset::ControllerClassPath);
	TestNotNull(TEXT("BP_DronePrototypePlayerController generated Class loads"), ControllerClass);
	const ADronePrototypePlayerController* ControllerDefaults = ControllerClass
		? Cast<ADronePrototypePlayerController>(ControllerClass->GetDefaultObject())
		: nullptr;
	TestNotNull(TEXT("Blueprint PlayerController CDO exists"), ControllerDefaults);
	if (ControllerDefaults)
	{
		TestTrue(
			TEXT("Blueprint PlayerController selects WBP_DroneFlightHUD"),
			ControllerDefaults->GetFlightHUDWidgetClass() == WidgetClass);
	}

	// 4) Map이 사용하는 BP GameMode까지 BP Controller 연결이 이어지는지 확인한다.
	UClass* GameModeClass = LoadClass<ADronePrototypeGameMode>(
		nullptr,
		DroneFlightHUDBlueprintAsset::GameModeClassPath);
	TestNotNull(TEXT("BP_DronePrototypeGameMode generated Class loads"), GameModeClass);
	const ADronePrototypeGameMode* GameModeDefaults = GameModeClass
		? Cast<ADronePrototypeGameMode>(GameModeClass->GetDefaultObject())
		: nullptr;
	TestNotNull(TEXT("Blueprint GameMode CDO exists"), GameModeDefaults);
	if (GameModeDefaults)
	{
		TestTrue(
			TEXT("Blueprint GameMode selects BP_DronePrototypePlayerController"),
			GameModeDefaults->PlayerControllerClass == ControllerClass);
	}

	return !HasAnyErrors();
}

#endif
