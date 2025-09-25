// Copyright Zwu


#include "UI/WidgetController/OverlayWidgetController.h"
#include "AbilitySystem/AuraAttributeSet.h"

void UOverlayWidgetController::BroadcastInitialValues()
{
	if (AttributeSet)
	{
		const UAuraAttributeSet* AuraAtrSet = Cast<UAuraAttributeSet>(AttributeSet);
		check(AuraAtrSet);

		OnMaxHealthChanged.Broadcast(AuraAtrSet->GetMaxHealth());
		OnHealthChanged.Broadcast(AuraAtrSet->GetHealth());

		OnManaChanged.Broadcast(AuraAtrSet->GetMana());
		OnMaxManaChanged.Broadcast(AuraAtrSet->GetMaxMana());
	}
	else
	{
		OnMaxHealthChanged.Broadcast(0.f);
		OnHealthChanged.Broadcast(0.f);

		OnManaChanged.Broadcast(0.f);
		OnMaxManaChanged.Broadcast(0.f);
	}
}
