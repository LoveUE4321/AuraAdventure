// Copyright 2022 David Romanski (Socke). All Rights Reserved.

#pragma once

#include "SocketClient.h"

class USocketClientBPLibrary;
class USocketClientTCPClient;

class SOCKETCLIENT_API FSocketClientTCPReceiveDataThread : public FRunnable {

public:

	FSocketClientTCPReceiveDataThread(USocketClientBPLibrary* socketClientBPLibraryP, FString clientConnectionIDP, EReceiveFilterClient receiveFilterP,
		FString ipOrDomainP, ESocketClientIPType ipTypeP,int32 portP, USocketClientTCPClient* tcpClientP, bool noPacketDelayP, bool noPacketBlockingP);
	
	~FSocketClientTCPReceiveDataThread();

	virtual uint32 Run() override;


	void triggerConnectionEvent(bool succsess, FString clientConnectionIDGlobal, FString serverMessage, USocketClientTCPClient* tcpClientGlobal,
		USocketClientBPLibrary* socketClientGlobal);
	void triggerMessageEvent(TArray<uint8>& byteDataArray, FString& clientConnectionIDGlobal, USocketClientTCPClient* tcpClientGlobal,
		USocketClientBPLibrary* socketClientGlobal, bool addNullTerminator = true);


private:
	USocketClientBPLibrary* socketClientBPLibrary = nullptr;
	//USocketClientBPLibrary*		oldClient;
	FString						clientConnectionID;
	FString						originalIP;
	EReceiveFilterClient		receiveFilter;
	FString ipOrDomain;
	ESocketClientIPType ipType;
	int32 port;
	USocketClientTCPClient* tcpClient = nullptr;
	bool noPacketDelay = false;
	bool noPacketBlocking = false;


	FRunnableThread* thread = nullptr;
};
