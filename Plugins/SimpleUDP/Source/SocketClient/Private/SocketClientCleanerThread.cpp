// Copyright 2022 David Romanski (Socke). All Rights Reserved.

#include "SocketClientCleanerThread.h"

FSocketClientCleanerThread::FSocketClientCleanerThread() {
	FString threadName = "FSocketClientPluginCleanerThread_" + FGuid::NewGuid().ToString();
	thread = FRunnableThread::Create(this, *threadName, 0, EThreadPriority::TPri_Lowest);
}


void FSocketClientCleanerThread::addSession(FSocketClientPluginSession& session) {
	session.addToCleanerTime = FDateTime::Now().GetTicks();
	sessionQueue.Enqueue(session);
}

void FSocketClientCleanerThread::changeSettings(bool showLogsP, int32 minLiveTimeInSecondsP) {
	showLogs = showLogsP;
	minLiveTimeInSeconds = minLiveTimeInSecondsP;
}

uint32 FSocketClientCleanerThread::Run() {
	while (true) {
		TArray<FSocketClientPluginSession> tryItAgain;

		if (showLogs && sessionQueue.IsEmpty()) {
			UE_LOG(LogTemp, Display, TEXT("SocketClient: Cleaner: No connections available to clean up."));
		}

		while (sessionQueue.IsEmpty() == false) {
			FSocketClientPluginSession session;
			sessionQueue.Dequeue(session);


			//one second = 10000000 ticks
			if ((FDateTime::Now().GetTicks() - session.addToCleanerTime) < (10000000 * minLiveTimeInSeconds)) {
				tryItAgain.Add(session);
				continue;
			}

			if (showLogs) {
				UE_LOG(LogTemp, Display, TEXT("SocketClient: Clean connection: %s"), *session.clientID);
			}


			delete session.tcpSendThread;
			delete session.tcpRecieverThread;

			delete session.tcpFileHandlerThread;

			delete session.udpSocketReceiver;
			delete session.udpSendDataThead;
			delete session.udpReceiveDataThread;

			delete session.socket;

		}

		for (int32 i = 0; i < tryItAgain.Num(); i++) {
			sessionQueue.Enqueue(tryItAgain[i]);
		}
		tryItAgain.Empty();


		FPlatformProcess::Sleep(minLiveTimeInSeconds);
	}
	return 0;
};