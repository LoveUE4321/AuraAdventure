// Copyright 2017-2019 David Romanski (Socke). All Rights Reserved.
#pragma once

#include "SocketClient.h"


/* asynchronous Thread*/
class SOCKETCLIENT_API FReadFileInPartsSocketClientThread : public FRunnable {

public:

	FReadFileInPartsSocketClientThread(FString cleanDirP, int32 bufferSizeP, float delayBetweenReadsInSecondsP) :
		cleanDir(cleanDirP),
		bufferSize(bufferSizeP),
		delayBetweenReadsInSeconds(delayBetweenReadsInSecondsP)
	{
		FString threadName = "FReadFileInPartsSocketClientThread" + FGuid::NewGuid().ToString();
		thread = FRunnableThread::Create(this, *threadName, 0, EThreadPriority::TPri_Normal);
	}
	~FReadFileInPartsSocketClientThread() {
		delete thread;
	}

	virtual uint32 Run() override;

	void stopThread();

	void setDelayBetweenReadsInSeconds(float d);


protected:
	bool run = true;
	FString cleanDir;
	int32 bufferSize;
	float delayBetweenReadsInSeconds;
	//USocketClientBPLibrary* mainLib = USocketClientBPLibrary::getSocketClientTarget();

	FRunnableThread* thread = nullptr;
};

