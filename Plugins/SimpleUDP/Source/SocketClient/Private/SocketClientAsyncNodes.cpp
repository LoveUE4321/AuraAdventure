// Copyright 2022 David Romanski(Socke). All Rights Reserved.

#include "SocketClientAsyncNodes.h"


/*--- TCP -------------------------------------------------------------------------------------------------------------*/

UTCPConnectAsyncNode* UTCPConnectAsyncNode::socketClientTCPConnectionAsyncNode(FString domainOrI, ESocketClientIPType ipType, int32 port,
	EReceiveFilterClient receiveFilters, ESocketClientTCPSeparator messageWrapping, FString optionalCustomConnectionID,bool ignoreKeepAlivePacket,
	bool disableNaglesAlgorithm) {

	UTCPConnectAsyncNode* instance = NewObject<UTCPConnectAsyncNode>();

	instance->domainOrIP = domainOrI;
	instance->ipType = ipType;
	instance->port = port;
	instance->receiveFilters = receiveFilters;
	instance->messageWrapping = messageWrapping;
	instance->optionalCustomConnectionID = optionalCustomConnectionID;
	instance->ignoreKeepAlivePacket =  ignoreKeepAlivePacket;
	instance->disableNaglesAlgorithm = disableNaglesAlgorithm;

	instance->AddToRoot();

	//The node is only visible in Blueprints if the function is declared in a UBlueprintAsyncActionBase class. 
	//Therefore an instance of itself is created here.
	return instance;
}

void UTCPConnectAsyncNode::Activate() {
	USocketClientBPLibrary::getSocketClientTarget()->connectSocketClientTCPNonStatic(domainOrIP, ipType, port, receiveFilters,
		messageWrapping,optionalCustomConnectionID, connectionID, this,ignoreKeepAlivePacket, disableNaglesAlgorithm);
}

void UTCPConnectAsyncNode::BeginDestroy(){
	RemoveFromRoot();
	Super::BeginDestroy();
}

void UTCPConnectAsyncNode::triggerConnectionEvent(bool success, FString clientConnectionID, FString serverMessage){
	if (success) {
		OnConnect.Broadcast(serverMessage, clientConnectionID, "", TArray<uint8>());
	}
	else {
		OnDisconnect.Broadcast(serverMessage, clientConnectionID, "", TArray<uint8>());
	}
}

void UTCPConnectAsyncNode::triggerMessageEvent(TArray<uint8> byteDataArray, FString clientConnectionID, FString serverMessage){
	OnServerMessage.Broadcast("",clientConnectionID,serverMessage,byteDataArray);
}


/*--- UDP -------------------------------------------------------------------------------------------------------------*/

UUDPInitAsyncNode* UUDPInitAsyncNode::socketClientInitUDPReceiverAsyncNode(FString domainOrIP,ESocketClientIPType ipType, int32 port,
	EReceiveFilterClient receiveFilter, int32 maxPacketSize) {

	UUDPInitAsyncNode* instance = NewObject<UUDPInitAsyncNode>();

	instance->domainOrIP = domainOrIP;
	instance->ipType = ipType;
	instance->port = port;
	instance->receiveFilter = receiveFilter;
	instance->maxPacketSize = maxPacketSize;
	instance->AddToRoot();

	//The node is only visible in Blueprints if the function is declared in a UBlueprintAsyncActionBase class. 
	//Therefore an instance of itself is created here.
	return instance;
}

void UUDPInitAsyncNode::Activate() {
	USocketClientBPLibrary::getSocketClientTarget()->socketClientInitUDPReceiverNonStatic(connectionID, this,domainOrIP, ipType, port,
		receiveFilter, maxPacketSize);
}

void UUDPInitAsyncNode::triggerInitEvent(bool success, FString clientConnectionID, FString serverMessage) {
	if (success) {
		OnSuccess.Broadcast(serverMessage, clientConnectionID,"",0, "", TArray<uint8>());
	}
	else {
		OnFail.Broadcast(serverMessage, clientConnectionID, "",0,"", TArray<uint8>());
	}
}

void UUDPInitAsyncNode::triggerMessageEvent(TArray<uint8> byteDataArray, FString clientConnectionID, FString serverMessage, FString peerIP, int32 peerPort) {
	OnServerMessage.Broadcast("", clientConnectionID,peerIP, peerPort, serverMessage, byteDataArray);
}