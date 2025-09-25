// Copyright Zwu


#include "UI/HUD/AuraHUD.h"
#include "UI/Widget/AuraUserWidget.h"
#include "UI/WidgetController/OverlayWidgetController.h"

void AAuraHUD::InitOverlayWidget(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	checkf(AuraWidgetClass, TEXT("Please assign AuraWidgetClass in AuraHUD"));
	
	UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), AuraWidgetClass);
	CurrentWidget = Cast<UAuraUserWidget>(Widget);
	check(CurrentWidget);

	const FAuraWidgetControllerParams Params(PC, PS, ASC, AS);

	UOverlayWidgetController* WidgetController = GetOverlayWidgetController(Params);
	CurrentWidget->SetWidgetController(WidgetController); 
	WidgetController->BroadcastInitialValues();

	Widget->AddToViewport();
}

UOverlayWidgetController* AAuraHUD::GetOverlayWidgetController(const FAuraWidgetControllerParams& Params)
{
	checkf(OverlayWidgetControllerClass, TEXT("Please assign OverlayWidgetControllerClass in AuraHUD"));

	if (OverlayWidgetController == nullptr)
	{
		OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
		check(OverlayWidgetController);

		OverlayWidgetController->SetWidgetControllerParams(Params);
		return OverlayWidgetController;
	}

	return OverlayWidgetController;
}
