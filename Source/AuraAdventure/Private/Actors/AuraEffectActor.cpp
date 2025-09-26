// Copyright Zwu

#include "Actors/AuraEffectActor.h"
#include "AbilitySystemInterface.h"
#include"Character/AuraCharacterBase.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"


// Sets default values
AAuraEffectActor::AAuraEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));

	/*Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	SetRootComponent(Sphere);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	Mesh->SetupAttachment(GetRootComponent());*/
}

//void AAuraEffectActor::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
//	bool bFromSweep, const FHitResult& SweepResult)
//{
//	//TODO: Change this to apply a GameplayEffect.
//	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OtherActor))
//	{
//		AAuraCharacterBase* ActorBase = Cast<AAuraCharacterBase>(OtherActor);
//		check(ActorBase);
//
//		// Mine Test
//		//const UAuraAttributeSet* AuraAtrSet = Cast<UAuraAttributeSet>(ActorBase->GetAttributeSet());
//		//check(AuraAtrSet);
//
//		const UAuraAttributeSet* AuraAtrSet = Cast<UAuraAttributeSet>(ASI->GetAbilitySystemComponent()->GetAttributeSet(UAuraAttributeSet::StaticClass()));
//		check(AuraAtrSet);
//
//		UAuraAttributeSet* MutableAtrSet = const_cast<UAuraAttributeSet*>(AuraAtrSet);
//		check(MutableAtrSet);
//
//		MutableAtrSet->SetHealth(AuraAtrSet->GetHealth() - 10.f);
//		MutableAtrSet->SetMana(AuraAtrSet->GetMana() - 10.f);
//
//		 Destroy();
//	}
//}
//
//void AAuraEffectActor::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
//{
//
//}

void AAuraEffectActor::BeginPlay()
{
	Super::BeginPlay();

	//Sphere->OnComponentBeginOverlap.AddDynamic(this, &AAuraEffectActor::OnBeginOverlap);
	//Sphere->OnComponentEndOverlap.AddDynamic(this, &AAuraEffectActor::OnEndOverlap);
}

void AAuraEffectActor::ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> GameEffectClass)
{
	/*if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target))
	{
		ASI->GetAbilitySystemComponent()->ApplyGameplayEffectToSelf(
			GameEffectClass->GetDefaultObject<UGameplayEffect>(), 1.f, ASI->GetAbilitySystemComponent()->MakeEffectContext()
		);
	}*/

	// UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent Equals to up there.
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (TargetASC == nullptr) return;

	check(GameEffectClass);
	FGameplayEffectContextHandle ECHandle = TargetASC->MakeEffectContext();
	ECHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle GESpecHandle = TargetASC->MakeOutgoingSpec(GameEffectClass, 1.f, ECHandle);
	TargetASC->ApplyGameplayEffectSpecToSelf(*GESpecHandle.Data.Get());
}