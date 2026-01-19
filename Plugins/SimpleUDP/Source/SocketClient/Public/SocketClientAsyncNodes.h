// Copyright 2022 David Romanski(Socke). All Rights Reserved.

#pragma once

#include "SocketClient.h"

#ifndef __SocketClientBPLibrary
#define __SocketClientBPLibrary
#include "SocketClientBPLibrary.h"
#endif

#include "SocketClientAsyncNodes.generated.h"


/*--- TCP -------------------------------------------------------------------------------------------------------------*/


UCLASS()
class SOCKETCLIENT_API UTCPConnectAsyncNode : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()


public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FTCPConnectDelegate, const FString, connenctionInfo, const FString, clientConnectionID,
		const FString, messageFromServer, const TArray<uint8>&, byteArrayFromServer);

	UPROPERTY(BlueprintAssignable)
		FTCPConnectDelegate OnConnect;

	UPROPERTY(BlueprintAssignable)
		FTCPConnectDelegate OnDisconnect;

	UPROPERTY(BlueprintAssignable)
		FTCPConnectDelegate OnServerMessage;

	/**
	* Connect to a TCP Server
	* @param domainOrIP IP or Domain of your server
	* @param ipType
	* @param port
	* @param receiveFilter This allows you to decide which data type you want to receive. If you receive files it makes no sense to convert them into a string.
	* @param messageSeparator It may be that data packets are split or merged when transmitted over TCP in order to optimize the transmission. For example, if you send "Hello" two times in a row very quickly, it can happen that "HelloHa" and "llo" arrive. To counteract this circumstance there are options to separate the data packets.
	* @param optionalCustomConnectionID Instead of an automatically generated ConnectionID you can use your own ID with this parameter.
	* @param ignoreTheKeepAlivePacket Activate to ignore the (0x00) keep alive byte of the server that can optionally be sent by the server plugin.
	* @param disableNaglesAlgorithm Don't change it if you don't know what this option is for! With this you can disable the TCP Nagle's algorithm to send (in LAN) very small data packets faster.
	**/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP", meta = (BlueprintInternalUseOnly = "true", AdvancedDisplay = 7))
		static UTCPConnectAsyncNode* socketClientTCPConnectionAsyncNode(FString domainOrIP, ESocketClientIPType ipType, int32 port, EReceiveFilterClient receiveFilters,
			ESocketClientTCPSeparator messageSeparator, FString optionalCustomConnectionID,bool ignoreTheKeepAlivePacket = false, bool disableNaglesAlgorithm = false);


	virtual void Activate() override;
	virtual void BeginDestroy() override;

	void triggerConnectionEvent(bool success, FString clientConnectionID, FString serverMessage);
	void triggerMessageEvent(TArray<uint8> byteDataArray, FString clientConnectionID, FString serverMessage);

private:
	UTCPConnectAsyncNode* instance = nullptr;
	FString domainOrIP = "0.0.0.0";
	ESocketClientIPType ipType = ESocketClientIPType::E_ipv4;
	int32 port = 9999;
	EReceiveFilterClient receiveFilters = EReceiveFilterClient::E_SAB;
	ESocketClientTCPSeparator messageWrapping = ESocketClientTCPSeparator::E_None;
	FString optionalCustomConnectionID = FString();
	FString connectionID = FString();
	bool ignoreKeepAlivePacket = false;
	bool disableNaglesAlgorithm = false;
};


/*--- UDP -------------------------------------------------------------------------------------------------------------*/


UCLASS()
class SOCKETCLIENT_API UUDPInitAsyncNode : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()


public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FUDPInitDelegate, const FString, initializationInfo, const FString, clientConnectionID,
		const FString, peerIP,const int32, peerPort,
		const FString, messageFromServer, const TArray<uint8>&, byteArrayFromServer);

	UPROPERTY(BlueprintAssignable)
		FUDPInitDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable)
		FUDPInitDelegate OnFail;

	UPROPERTY(BlueprintAssignable)
		FUDPInitDelegate OnServerMessage;

	/**
	*Opens a connection on specific ip and port and listen on it.
	*@param DomainOrIP IP or Domain to listen on. 0.0.0.0 means that data can be received on all local IPs.
	*@param port port to listen on
	*@param receiveFilter This allows you to decide which data type you want to receive. If you receive files it makes no sense to convert them into a string.
	*@param maxPacketSize sets the maximum UDP packet size. More than 65507 is not possible.
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|UDP", meta = (BlueprintInternalUseOnly = "true", AdvancedDisplay = 5))
		static UUDPInitAsyncNode* socketClientInitUDPReceiverAsyncNode(FString domainOrIP = "0.0.0.0",
			ESocketClientIPType ipType = ESocketClientIPType::E_ipv4, int32 port = 8888,
			EReceiveFilterClient receiveFilter = EReceiveFilterClient::E_SAB, int32 maxPacketSize = 65507);


	virtual void Activate() override;

	void triggerInitEvent(bool success, FString clientConnectionID, FString serverMessage);
	void triggerMessageEvent(TArray<uint8> byteDataArray, FString clientConnectionID, FString serverMessage, FString peerIP, int32 peerPort);

private:
	UUDPInitAsyncNode* instance = nullptr;
	FString domainOrIP = "0.0.0.0";
	ESocketClientIPType ipType = ESocketClientIPType::E_ipv4;
	int32 port = 8888;
	EReceiveFilterClient receiveFilter = EReceiveFilterClient::E_SAB;
	int32 maxPacketSize = 65507;
	FString connectionID = FString();
};
