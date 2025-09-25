// Copyright Zwu


#include "UI/WidgetController/AuraWidgetController.h"

void UAuraWidgetController::SetWidgetControllerParams(const FAuraWidgetControllerParams& Params)
{
	PlayerController = Params.InPlayerController;
	PlayerState = Params.InPlayerState;
	AbilitySysCmp = Params.InASC;
	AttributeSet = Params.InAttributeSet;
}
