// Copyright Zwu


#include "UI/WidgetController/OverlayWidgetController.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"

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

void UOverlayWidgetController::BindCallbacksToDependencies()
{
	if (AbilitySysCmp && AttributeSet)
	{
		const UAuraAttributeSet* AuraAtrSet = Cast<UAuraAttributeSet>(AttributeSet);
		check(AuraAtrSet);

		AbilitySysCmp->GetGameplayAttributeValueChangeDelegate(AuraAtrSet->GetHealthAttribute()).AddUObject(this, &UOverlayWidgetController::OnHealthChangedData);
		AbilitySysCmp->GetGameplayAttributeValueChangeDelegate(AuraAtrSet->GetMaxHealthAttribute()).AddUObject(this, &UOverlayWidgetController::OnMaxHealthChangedData);
		AbilitySysCmp->GetGameplayAttributeValueChangeDelegate(AuraAtrSet->GetManaAttribute()).AddUObject(this, &UOverlayWidgetController::OnManaChangedData);
		AbilitySysCmp->GetGameplayAttributeValueChangeDelegate(AuraAtrSet->GetMaxManaAttribute()).AddUObject(this, &UOverlayWidgetController::OnMaxManaChangedData);

		// Bind ASC OnEffectAssetTags
		Cast<UAuraAbilitySystemComponent>(AbilitySysCmp)->OnEffectAssetTags.AddLambda(
			[this](const FGameplayTagContainer& AssetTags) 
			{
				for (auto Tag : AssetTags)
				{
					FString Msg = FString::Printf(TEXT("OverlayWidgetController Effect Tag: %s"), *Tag.ToString());
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Msg);

					GetDataTableRowByTag<FUIWidgetRow>(UIWidgetDataTable, Tag);
				}
			}
		);
	}
}

void UOverlayWidgetController::OnHealthChangedData(const FOnAttributeChangeData& Data) const
{
	OnHealthChanged.Broadcast(Data.NewValue);

}

void UOverlayWidgetController::OnMaxHealthChangedData(const FOnAttributeChangeData& Data) const
{
	OnMaxHealthChanged.Broadcast(Data.NewValue);
}

void UOverlayWidgetController::OnManaChangedData(const FOnAttributeChangeData& Data) const
{
	OnManaChanged.Broadcast(Data.NewValue);
}

void UOverlayWidgetController::OnMaxManaChangedData(const FOnAttributeChangeData& Data) const
{
	OnMaxManaChanged.Broadcast(Data.NewValue);
}
