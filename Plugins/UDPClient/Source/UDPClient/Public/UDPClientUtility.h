
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketBuilder.h"
#include "Common/UdpSocketReceiver.h"
#include "Interfaces/IPv4/IPv4Address.h"

#include "UDPClientUtility.generated.h"

UCLASS()
class UDPCLIENT_API AUDPClientUtility :public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AUDPClientUtility();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:

	FSocket* udpSocket;
	//Զ�̵ĵ�ַ
	TSharedPtr<FInternetAddr> RemoteAddr;

	UFUNCTION(BlueprintCallable, Category = "UDP")
	bool CreateUdp(const FString& socketName, const FString& targetIP, const int32 targetPort, const int32 selfPort);

	UFUNCTION(BlueprintCallable, Category = "UDP")
	bool SendMsg(FString msg);

	UFUNCTION(BlueprintCallable, Category = "UDP")
	void RecvMsg(bool& result, FString& msg);


	UFUNCTION(BlueprintCallable, BlueprintCallable, Category = "SocketClient")
	bool SendUDPMessage(FString Message, const FString& TargetIP, int32 Port);
};

#pragma once