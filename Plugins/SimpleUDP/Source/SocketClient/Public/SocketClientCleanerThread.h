// Copyright 2022 David Romanski (Socke). All Rights Reserved.

#pragma once

#include "SocketClient.h"


class SOCKETCLIENT_API FSocketClientCleanerThread : public FRunnable {

public:

	FSocketClientCleanerThread();


	virtual uint32 Run() override;

	void addSession(FSocketClientPluginSession& session);
	void changeSettings(bool showLogs, int32 minLiveTimeInSeconds);

private:
	bool showLogs = false;
	int32 minLiveTimeInSeconds = 10;
	FRunnableThread* thread = nullptr;
	TQueue<FSocketClientPluginSession> sessionQueue;
};

