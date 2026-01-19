// Copyright 2022 David Romanski (Socke). All Rights Reserved.

#pragma once

#include "SocketClient.h"

class USocketClientBPLibrary;

class SOCKETCLIENT_API FSocketClientUDPSendDataThead : public FRunnable {

public:

	FSocketClientUDPSendDataThead(USocketClientUDP* udpClientP, USocketClientBPLibrary* socketClientP, FString mySocketipP, int32 mySocketportP);

	~FSocketClientUDPSendDataThead();

	virtual uint32 Run() override;
	void addData(FString messageP, TArray<uint8> byteArrayP, FString domainOrIP, int32 port, ESocketClientIPType ipType);
	void pauseThread(bool pause);
	void sendBytes(FSocket*& socketP, TArray<uint8>& byteArray, int32& sent, TSharedRef<FInternetAddr>& addr);

private:
	USocketClientUDP* udpClient = nullptr;
	USocketClientBPLibrary* socketClient = nullptr;
	FString mySocketip;
	int32 mySocketport = 0;
	FString sendToip;
	int32 sendToport = 0;
	FString sendToDomainOrIP = FString();
	FRunnableThread* thread = nullptr;
	bool					paused;
	TQueue<FString> messageQueue;
	TQueue<TArray<uint8>> byteArrayQueue;
	int32 maxPacketSize = 65507;
	bool validInternetAdress = false;
	TSharedRef <FInternetAddr> internetAdress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	int64 lastErrorMessageTime = 0;
};

