// Copyright 2022 David Romanski (Socke). All Rights Reserved.

#pragma once

#include "SocketClient.h"

class USocketClientUDP;
class USocketClientBPLibrary;

class SOCKETCLIENT_API FSocketClientTCPSendDataThead : public FRunnable {

public:

	FSocketClientTCPSendDataThead(USocketClientBPLibrary* socketClientLibP, USocketClientTCPClient* tcpClientP, FString clientConnectionIDP);
	
	~FSocketClientTCPSendDataThead();

	virtual uint32 Run() override;
	void sendMessage(FString messageP, TArray<uint8> byteArrayP);
	void pauseThread(bool pause);


private:
	TQueue<FString> messageQueue;
	TQueue<TArray<uint8>> byteArrayQueue;
	USocketClientBPLibrary* socketClientLib;
	USocketClientTCPClient* tcpClient = nullptr;
	FString clientConnectionID;
	FRunnableThread* thread = nullptr;
	bool					run = true;
	bool					paused = false;
	bool blah = true;
};


