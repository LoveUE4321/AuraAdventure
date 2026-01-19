// Copyright 2022 David Romanski (Socke). All Rights Reserved.

#pragma once

#include "SocketClient.h"


class SOCKETCLIENT_API FSocketClientTCPFileHandlerThread : public FRunnable {

public:

	FSocketClientTCPFileHandlerThread(USocketClientBPLibrary* socketClientP, FString clientConnectionIDP, FString ipOrDomainP, ESocketClientIPType ipTypeP,
		int32 portP, USocketClientTCPClient* tcpClientP);

	~FSocketClientTCPFileHandlerThread();

	virtual uint32 Run() override;

	void doRequestFileFromServer(FSocket* socket);
	void doSendFileToServer(FSocket* socket);

	void sendMessageToServer(FString message, FSocket* socket);

	void triggerFileTransferOverTCPInfoEvent(FString messageP, FString clientConnectionIDP, FString filePathP, bool successP,
		USocketClientBPLibrary* socketClientP, USocketClientTCPClient* tcpClientP);

	void triggerTransferFileEvent(FString clientConnectionIDP, FString filePathP, USocketClientBPLibrary* socketClientP,
		USocketClientTCPClient* tcpClientP, float percentP, float mbitP, int64 transferredP, int64 fileSizeP);

	void sendEndMessage(FString fullFilePathP, FString tokenP, FString md5ServerP, FString clientConnectionIDP, FSocket* clientSocketP,
		USocketClientBPLibrary* socketClientP, USocketClientTCPClient* tcpClientP);

	FString readMessageFromServer(FSocket* socket);

private:
	USocketClientBPLibrary* socketClient = nullptr;
	//USocketClientBPLibrary*		oldClient;
	FString						clientConnectionID;
	FString						originalIP;
	FString ipOrDomain;
	ESocketClientIPType ipType;
	int32 port;
	USocketClientTCPClient* tcpClient = nullptr;
	FRunnableThread* thread = nullptr;
	double waitForRead = 30;
};
