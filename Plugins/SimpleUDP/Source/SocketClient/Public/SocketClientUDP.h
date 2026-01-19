// Copyright 2017-2019 David Romanski (Socke). All Rights Reserved.
#pragma once

#include "SocketClient.h"
#include "SocketClientUDPSendDataThead.h"
#include "SocketClientUDPReceiveDataThread.h"
#include "SocketClientUDP.generated.h"


class USocketClientBPLibrary;

UCLASS(Blueprintable, BlueprintType)
class SOCKETCLIENT_API USocketClientUDP : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	//Delegates
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FsocketClientUDPConnectionEventDelegate, bool, success, FString, message, FString, clientConnectionID);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FreceiveUDPMessageEventDelegate, FString, message, const TArray<uint8>&, byteArray, FString, IP_FromSender, int32, portFromSender, FString, clientConnectionID);

	UFUNCTION()
		void socketClientUDPConnectionEventDelegate(const bool success, const FString message, const FString clientConnectionID);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|UDP|Events|ConnectionInfo")
		FsocketClientUDPConnectionEventDelegate onsocketClientUDPConnectionEventDelegate;
	UFUNCTION()
		void receiveUDPMessageEventDelegate(const FString message, const TArray<uint8>& byteArray, const  FString IP, const int32 port, const FString clientConnectionID);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|UDP|Events|ReceiveMessage")
		FreceiveUDPMessageEventDelegate onreceiveUDPMessageEventDelegate;

	void init(USocketClientBPLibrary* socketClientLibP, UUDPInitAsyncNode* udpInitAsyncNode, FString domain, ESocketClientIPType ipType, int32 port, EReceiveFilterClient receiveFilter, FString clientConnectionID, int32 maxPacketSize = 65507);
	void sendUDPMessage(FString domainOrIP, ESocketClientIPType ipType, int32 port, FString message, TArray<uint8> byteArray);
	void closeUDPConnection();
	void UDPReceiver(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);


	bool isRun();
	void setRun(bool runP);

	FSocket* getSocket();
	void setSocket(FSocket* socketP);

	void setUDPSocketReceiver(FUdpSocketReceiver* udpSocketReceiver);

	FString getIP();
	void setIP(FString ipP);
	int32 getPort();
	FString getDomainOrIP();

	FString getConnectionID();
	void setUDPSendThread(FSocketClientUDPSendDataThead* udpSendThreadP);

	int32 getMaxPacketSize();

	UUDPInitAsyncNode* udpInitAsyncNode = nullptr;

private:
	bool run = false;
	EReceiveFilterClient receiveFilter;
	FString connectionID;
	FString domainOrIP;
	int32 port = 0;
	int32 maxPacketSize = 65507;

	USocketClientBPLibrary* socketClientBPLibrary = nullptr;

	FUdpSocketReceiver* udpSocketReceiver = nullptr;
	FSocket* socket = nullptr;
	FSocketClientUDPReceiveDataThread* UDPThread = nullptr;
	FSocketClientUDPSendDataThead* UDPSendThread = nullptr;
};

