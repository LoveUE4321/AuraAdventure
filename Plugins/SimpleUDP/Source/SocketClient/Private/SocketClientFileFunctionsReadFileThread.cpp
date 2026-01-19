// Copyright 2017-2019 David Romanski (Socke). All Rights Reserved.

#include "SocketClientFileFunctionsReadFileThread.h"

uint32 FReadFileInPartsSocketClientThread::FReadFileInPartsSocketClientThread::Run()
{

	FArchive* reader = IFileManager::Get().CreateFileReader(*cleanDir);
	if (reader == nullptr || reader->TotalSize() == 0) {
		AsyncTask(ENamedThreads::GameThread, []() {
			USocketClientBPLibrary::getSocketClientTarget()->onreadBytesFromFileInPartsEventDelegate.Broadcast(0, 0, true, TArray<uint8>());
			});
		if (reader != nullptr) {
			reader->Close();
		}
		delete reader;
		return 0;
	}

	if (delayBetweenReadsInSeconds <= 0) {
		delayBetweenReadsInSeconds = 0.0001f;
	}

	int64 fileSize = reader->TotalSize();
	int64 readSize = 0;
	int64 lastPosition = 0;
	TArray<uint8> buffer;

	if (bufferSize > fileSize) {
		bufferSize = fileSize;
	}

	while (run && lastPosition < fileSize) {
		if ((lastPosition + bufferSize) > fileSize) {
			bufferSize = fileSize - lastPosition;
		}

		//buffer.Reset(bufferSize);
		buffer.Empty();
		buffer.AddUninitialized(bufferSize);

		reader->Serialize(buffer.GetData(), buffer.Num());
		lastPosition += buffer.Num();

		//UE_LOG(LogTemp, Warning, TEXT("xxxxx READ: %i"), buffer.Num());


		AsyncTask(ENamedThreads::GameThread, [fileSize, lastPosition, buffer]() {
			USocketClientBPLibrary::getSocketClientTarget()->onreadBytesFromFileInPartsEventDelegate.Broadcast(fileSize, lastPosition, false, buffer);
			});

		FPlatformProcess::Sleep(delayBetweenReadsInSeconds);

	}

	AsyncTask(ENamedThreads::GameThread, [fileSize, lastPosition]() {
		USocketClientBPLibrary::getSocketClientTarget()->onreadBytesFromFileInPartsEventDelegate.Broadcast(fileSize, lastPosition, true, TArray<uint8>());
		});

	UFileFunctionsSocketClient::getFileFunctionsSocketClientTarget()->cleanReadBytesFromFileInParts(cleanDir);
	//buffer.Empty();
	if (reader != nullptr) {
		reader->Close();
	}
	delete reader;
	thread = nullptr;
	return 0;
}

void FReadFileInPartsSocketClientThread::stopThread(){
	run = false;
}

void FReadFileInPartsSocketClientThread::setDelayBetweenReadsInSeconds(float d) {
	delayBetweenReadsInSeconds = d;
	if (delayBetweenReadsInSeconds <= 0) {
		delayBetweenReadsInSeconds = 0.001f;
	}
}
