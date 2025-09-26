// Copyright Zwu

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
//#include "Components/SphereComponent.h"
#include "GameplayEffect.h"
#include "AuraEffectActor.generated.h"

UCLASS()
class AURAADVENTURE_API AAuraEffectActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AAuraEffectActor();

	//UFUNCTION()
	//virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
	//	bool bFromSweep, const FHitResult& SweepResult);

	//UFUNCTION()
	//virtual void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> GameEffectClass);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AuraEffect")
	TSubclassOf<UGameplayEffect> InstantGameplayEffectCalss;

	/*UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Sphere;*/

};
