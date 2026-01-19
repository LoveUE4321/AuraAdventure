// Copyright 2022 David Romanski (Socke). All Rights Reserved.

#include "SocketClientTCPSendDataThead.h"


FSocketClientTCPSendDataThead::FSocketClientTCPSendDataThead(USocketClientBPLibrary* socketClientLibP, USocketClientTCPClient* tcpClientP, FString clientConnectionIDP) :
	socketClientLib(socketClientLibP),
	tcpClient(tcpClientP),
	clientConnectionID(clientConnectionIDP) {
	FString threadName = "FSendDataToServerThread" + FGuid::NewGuid().ToString();
	thread = FRunnableThread::Create(this, *threadName, 0, EThreadPriority::TPri_Normal);
}

FSocketClientTCPSendDataThead::~FSocketClientTCPSendDataThead() {
	delete thread;
}

uint32 FSocketClientTCPSendDataThead::Run(){

	if (tcpClient == nullptr) {
		UE_LOG(LogTemp, Error, TEXT("Class is not initialized."));
		return 0;
	}

	/*if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Green, TEXT("tcp socket 1"));*/

	FString clientConnectionIDGlobal = clientConnectionID;
	USocketClientBPLibrary* socketClientLibGlobal = socketClientLib;

	//message wrapping
	FString stringSeparator = FString();
	uint8 byteSeparator = 0x00;
	ESocketClientTCPSeparator messageWrapping = ESocketClientTCPSeparator::E_None;
	tcpClient->getTcpSeparator(stringSeparator, byteSeparator, messageWrapping);

	TArray<TCHAR> stringSeparatorArray = stringSeparator.GetCharArray();
	if (stringSeparatorArray.Num() > 0 && stringSeparatorArray.Last() == 0x00) {
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 5
		stringSeparatorArray.RemoveAt(stringSeparatorArray.Num() - 1, 1, EAllowShrinking::Yes);
#else
		stringSeparatorArray.RemoveAt(stringSeparatorArray.Num() - 1, 1, true);
#endif
	}

	if (messageWrapping == ESocketClientTCPSeparator::E_LengthSeparator && stringSeparatorArray.Num() == 0) {
		messageWrapping = ESocketClientTCPSeparator::E_None;
		UE_LOG(LogTemp, Warning, TEXT("Socket Client Plugin: Separator mode is set to String but there is no String Separator. Mode changed to none."));
	}

	// get the socket
	FSocket* socket = tcpClient->getSocket();
	int32 sent = 0;

	while (tcpClient->isRun()) {

		// try to connect to the server
		if (socket == NULL || socket == nullptr) {
			UE_LOG(LogTemp, Error, TEXT("Connection not exist."));
			//AsyncTask(ENamedThreads::GameThread, [clientConnectionIDGlobal, socketClientLibGlobal]() {
			//	if (socketClientLibGlobal != nullptr) {
			//		socketClientLibGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection not exist:" + clientConnectionIDGlobal, clientConnectionIDGlobal);
			//		socketClientLibGlobal->closeSocketClientConnection();
			//	}
			//});
			break;
		}

		if (socket != nullptr && tcpClient->isRun()) {

			while (messageQueue.IsEmpty() == false) {
				FString m;
				messageQueue.Dequeue(m);
				FTCHARToUTF8 Convert(*m);
				sent = 0;

				TArray<uint8> byteCache;

				switch (messageWrapping)
				{
				case ESocketClientTCPSeparator::E_None:
					byteCache.Append((uint8*)Convert.Get(), Convert.Length());
					break;
				case ESocketClientTCPSeparator::E_ByteSeparator:
					byteCache.Append((uint8*)Convert.Get(), Convert.Length());
					byteCache.Add(byteSeparator);
					break;
				case ESocketClientTCPSeparator::E_StringSeparator:
				{
					m += stringSeparator;
					FTCHARToUTF8 ConvertWithSeparator(*m);
					byteCache.Append((uint8*)ConvertWithSeparator.Get(), ConvertWithSeparator.Length());
				}
				break;
				case ESocketClientTCPSeparator::E_LengthSeparator:
					if (FGenericPlatformProperties::IsLittleEndian()) {
						byteCache.Add(0x00);
					}
					else {
						byteCache.Add(0x01);
					}
					byteCache.SetNum(5);
					int32 dataLength = Convert.Length();
					FMemory::Memcpy(byteCache.GetData() + 1, &dataLength, 4);
					byteCache.Append((uint8*)Convert.Get(), Convert.Length());
					break;
				}

				socket->Send(byteCache.GetData(), byteCache.Num(), sent);
			}

			while (byteArrayQueue.IsEmpty() == false) {

				TArray<uint8> byteCache;
				byteArrayQueue.Dequeue(byteCache);
				sent = 0;

				switch (messageWrapping)
				{
				case ESocketClientTCPSeparator::E_ByteSeparator:
					byteCache.Add(byteSeparator);
					break;
				case ESocketClientTCPSeparator::E_StringSeparator:
				{
					FTCHARToUTF8 ConvertWithSeparator(*stringSeparator);
					byteCache.Append((uint8*)ConvertWithSeparator.Get(), ConvertWithSeparator.Length());
				}
				break;
				case ESocketClientTCPSeparator::E_LengthSeparator:
					byteCache.InsertZeroed(0, 5);
					if (FGenericPlatformProperties::IsLittleEndian() == false) {
						uint8 a = 0x01;
						FMemory::Memcpy(byteCache.GetData(), &a, 1);
					}
					int32 dataLength = byteCache.Num() - 5;
					FMemory::Memcpy(byteCache.GetData() + 1, &dataLength, 4);
					break;
				}

				socket->Send(byteCache.GetData(), byteCache.Num(), sent);

			}

		}

		if (tcpClient->isRun()) {
			pauseThread(true);
			//workaround. suspend do not work on all platforms. lets sleep
			while (paused && tcpClient->isRun()) {
				FPlatformProcess::Sleep(0.01);
			}
		}
	}
	return 0;
}


void FSocketClientTCPSendDataThead::sendMessage(FString messageP, TArray<uint8> byteArrayP) {
	if (messageP.Len() > 0)
		messageQueue.Enqueue(messageP);
	if (byteArrayP.Num() > 0)
		byteArrayQueue.Enqueue(byteArrayP);
	pauseThread(false);
}


void FSocketClientTCPSendDataThead::pauseThread(bool pause) {
	paused = pause;
	if (thread != nullptr)
		thread->Suspend(pause);
}