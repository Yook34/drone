#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "Misc/PackageName.h"
#include "StructUtils/UserDefinedStruct.h"
#include "UObject/FieldIterator.h"
#include "UObject/UnrealType.h"

namespace DroneNavigationArrowsAssets
{
constexpr const TCHAR* WidgetClassPath = TEXT(
	"/Game/Drone/ThirdParty/NavigationArrows/Blueprints/NavigationArrow."
	"NavigationArrow_C");

constexpr const TCHAR* TexturePaths[] = {
	TEXT("/Game/Drone/ThirdParty/NavigationArrows/Icons/NewTransparentArrow.NewTransparentArrow"),
	TEXT("/Game/Drone/ThirdParty/NavigationArrows/Icons/TransparentArrow.TransparentArrow")
};

constexpr const TCHAR* StructPaths[] = {
	TEXT("/Game/Drone/ThirdParty/NavigationArrows/InfoStructs/ImageInfo.ImageInfo"),
	TEXT("/Game/Drone/ThirdParty/NavigationArrows/InfoStructs/MovementInfo.MovementInfo"),
	TEXT("/Game/Drone/ThirdParty/NavigationArrows/InfoStructs/TextInfo.TextInfo")
};

// 데모와 예제는 기능 Widget의 런타임 의존성이 아니므로 프로젝트에 넣지 않는다.
constexpr const TCHAR* ExcludedPackagePaths[] = {
	TEXT("/Game/Drone/ThirdParty/NavigationArrows/Blueprints/NavigationArrowExampleActor"),
	TEXT("/Game/Drone/ThirdParty/NavigationArrows/Demo/Demo"),
	TEXT("/Game/Drone/ThirdParty/NavigationArrows/Demo/Demo_BuiltData"),
	TEXT("/Game/Drone/ThirdParty/NavigationArrows/Icons/TransparentCircle"),
	TEXT("/Game/Drone/ThirdParty/NavigationArrows/Meshes/ExampleMesh")
};

const FProperty* FindPropertyByPrefix(const UClass* Class, const FString& Prefix)
{
	if (!Class)
	{
		return nullptr;
	}

	// User Defined Struct를 사용하는 Blueprint 변수는 저장 과정에서 GUID 접미사가 붙을 수 있다.
	for (TFieldIterator<FProperty> PropertyIt(Class, EFieldIterationFlags::IncludeSuper);
		PropertyIt;
		++PropertyIt)
	{
		if (PropertyIt->GetName().StartsWith(Prefix))
		{
			return *PropertyIt;
		}
	}
	return nullptr;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDroneNavigationArrowsAssetTest,
	"Drone.Integration.NavigationArrowsAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDroneNavigationArrowsAssetTest::RunTest(const FString& Parameters)
{
	// 공급사 Widget은 표현만 담당한다. 현재 목표 계산은 Drone 프로젝트 코드가 계속 소유한다.
	UClass* WidgetClass = LoadClass<UUserWidget>(
		nullptr,
		DroneNavigationArrowsAssets::WidgetClassPath);
	TestNotNull(TEXT("NavigationArrow generated Class loads"), WidgetClass);
	if (WidgetClass)
	{
		TestTrue(
			TEXT("NavigationArrow derives from UUserWidget"),
			WidgetClass->IsChildOf(UUserWidget::StaticClass()));
		TestNotNull(TEXT("NavigationArrow CDO exists"), WidgetClass->GetDefaultObject<UUserWidget>());

		// 향후 프로젝트 소유 Host가 목표를 전달할 때 사용할 최소 공개 계약이다.
		TestNotNull(
			TEXT("NavigationArrow exposes TargetComponent"),
			DroneNavigationArrowsAssets::FindPropertyByPrefix(
				WidgetClass,
				TEXT("TargetComponent")));
		TestNotNull(
			TEXT("NavigationArrow exposes TargetWorldLocation"),
			DroneNavigationArrowsAssets::FindPropertyByPrefix(
				WidgetClass,
				TEXT("TargetWorldLocation")));
	}

	for (const TCHAR* TexturePath : DroneNavigationArrowsAssets::TexturePaths)
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TexturePath);
		TestNotNull(*FString::Printf(TEXT("Selected Texture loads: %s"), TexturePath), Texture);
	}

	for (const TCHAR* StructPath : DroneNavigationArrowsAssets::StructPaths)
	{
		UUserDefinedStruct* Struct = LoadObject<UUserDefinedStruct>(nullptr, StructPath);
		TestNotNull(*FString::Printf(TEXT("Selected Struct loads: %s"), StructPath), Struct);
	}

	for (const TCHAR* PackagePath : DroneNavigationArrowsAssets::ExcludedPackagePaths)
	{
		TestFalse(
			*FString::Printf(TEXT("Demo or example Package stays excluded: %s"), PackagePath),
			FPackageName::DoesPackageExist(PackagePath));
	}

	// 원래 공급사 루트가 남으면 이동되지 않은 참조나 잘못 복사한 파일이 있다는 뜻이다.
	TestFalse(
		TEXT("Original NavigationArrows root is not present in the Drone project"),
		FPackageName::DoesPackageExist(
			TEXT("/Game/NavigationArrows/Blueprints/NavigationArrow")));

	return !HasAnyErrors();
}

#endif
