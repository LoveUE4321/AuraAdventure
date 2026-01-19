// Copyright 2022 David Romanski (Socke). All Rights Reserved.

#pragma once

#include "SocketClient.h"


class SOCKETCLIENT_API FSocketClientUDPReceiveDataThread : public FRunnable {

public:

	FSocketClientUDPReceiveDataThread(USocketClientUDP* udpClientP, USocketClientBPLibrary* socketClientP, FString ipP, int32 portP, ESocketClientIPType ipTypeP);

	~FSocketClientUDPReceiveDataThread();

	void triggerInitEvent(bool success, USocketClientUDP* udpClientP, USocketClientBPLibrary* socketClientP, FString serverMessage, FString connectionID);

	virtual uint32 Run() override;


private :
	USocketClientUDP* udpClient = nullptr;
	USocketClientBPLibrary* socketClient = nullptr;
	FRunnableThread* thread = nullptr;
	FString					ipGlobal;
	int32					portGlobal;
	ESocketClientIPType ipType = ESocketClientIPType::E_ipv4;
	FSocket* socket = nullptr;
	bool reuseSocket = false;
};
