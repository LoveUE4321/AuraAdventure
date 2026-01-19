// Copyright 2022 David Romanski (Socke). All Rights Reserved.

#include "SocketClientUDPSendDataThead.h"

FSocketClientUDPSendDataThead::FSocketClientUDPSendDataThead(USocketClientUDP* udpClientP, USocketClientBPLibrary* socketClientP, FString mySocketipP, int32 mySocketportP) :
	udpClient(udpClientP),
	socketClient(socketClientP),
	mySocketip(mySocketipP),
	mySocketport(mySocketportP) {
	FString threadName = "FServerUDPSendMessageThread_" + FGuid::NewGuid().ToString();
	thread = FRunnableThread::Create(this, *threadName, 0, EThreadPriority::TPri_Normal);
}

FSocketClientUDPSendDataThead::~FSocketClientUDPSendDataThead() {
	delete thread;
}

uint32 FSocketClientUDPSendDataThead::Run() {

	FSocket* socket = udpClient->getSocket();
	FString connectionID = udpClient->getConnectionID();
	maxPacketSize = udpClient->getMaxPacketSize();

	while (udpClient->isRun() && socket != nullptr) {


		if (udpClient->isRun() && (messageQueue.IsEmpty() == false || byteArrayQueue.IsEmpty() == false)) {

			int32 sent = 0;
			TArray<uint8> byteArray;
			if (validInternetAdress) {
				while (messageQueue.IsEmpty() == false) {
					FString m;
					messageQueue.Dequeue(m);
					FTCHARToUTF8 Convert(*m);
					byteArray.Append((uint8*)Convert.Get(), Convert.Length());
					sendBytes(socket, byteArray, sent, internetAdress);
				}

				while (byteArrayQueue.IsEmpty() == false) {
					byteArrayQueue.Dequeue(byteArray);
					sendBytes(socket, byteArray, sent, internetAdress);
				}

			}
			else {
				UE_LOG(LogTemp, Error, TEXT("Can't send to %s:%i . Adress not valid."), *sendToip, sendToport);
			}

		}

		if (udpClient->isRun()) {
			pauseThread(true);
			//workaround. suspend do not work on all platforms. lets sleep
			while (paused && udpClient->isRun()) {
				FPlatformProcess::Sleep(0.01);
			}
		}
	}

	if (socket != nullptr) {
		socket->Close();
		socket = nullptr;
		udpClient->setSocket(nullptr);
	}


	USocketClientBPLibrary* socketClientTMP = socketClient;
	USocketClientUDP* udpClientGlobal = udpClient;
	FString ipGlobal = mySocketip;
	int32 portGlobal = mySocketport;
	AsyncTask(ENamedThreads::GameThread, [socketClientTMP, udpClientGlobal, ipGlobal, portGlobal, connectionID]() {
		if (socketClientTMP != nullptr)
		socketClientTMP->onsocketClientUDPConnectionEventDelegate.Broadcast(false, "UDP connection closed. " + ipGlobal + ":" + FString::FromInt(portGlobal), connectionID);
	if (udpClientGlobal != nullptr)
		udpClientGlobal->onsocketClientUDPConnectionEventDelegate.Broadcast(false, "UDP connection closed. " + ipGlobal + ":" + FString::FromInt(portGlobal), connectionID);
		});


	thread = nullptr;
	return 0;
}


void FSocketClientUDPSendDataThead::addData(FString messageP, TArray<uint8> byteArrayP, FString domainOrIP, int32 port, ESocketClientIPType ipType) {

	//new adress?
	if (sendToport != port || sendToDomainOrIP.Equals(domainOrIP) == false) {

		sendToDomainOrIP = domainOrIP;
		sendToip = socketClient->resolveDomain(domainOrIP, ipType);
		sendToport = port;

		internetAdress->SetIp(*sendToip, validInternetAdress);
		internetAdress->SetPort(sendToport);
		if (!validInternetAdress) {
			//don't send to many error messages. one second = 10000000 ticks
			if (((FDateTime::Now().GetTicks()) - lastErrorMessageTime) >= 10000000) {
				UE_LOG(LogTemp, Error, TEXT("Can't create Adress %s:%i"), *sendToip, sendToport);
				lastErrorMessageTime = FDateTime::Now().GetTicks();
			}
			return;
		}
	}
	if (!validInternetAdress) {
		return;
	}

	if (messageP.Len() > 0)
		messageQueue.Enqueue(messageP);
	if (byteArrayP.Num() > 0)
		byteArrayQueue.Enqueue(byteArrayP);
	pauseThread(false);

}

void FSocketClientUDPSendDataThead::pauseThread(bool pause) {
	paused = pause;
	if (thread != nullptr)
		thread->Suspend(pause);
}

void FSocketClientUDPSendDataThead::sendBytes(FSocket*& socketP, TArray<uint8>& byteArray, int32& sent, TSharedRef<FInternetAddr>& addr) {
	if (byteArray.Num() > maxPacketSize) {
		TArray<uint8> byteArrayTemp;
		for (int32 i = 0; i < byteArray.Num(); i++) {
			byteArrayTemp.Add(byteArray[i]);
			if (byteArrayTemp.Num() == maxPacketSize) {
				sent = 0;
				socketP->SendTo(byteArrayTemp.GetData(), byteArrayTemp.Num(), sent, *addr);
				byteArrayTemp.Empty();
			}
		}
		if (byteArrayTemp.Num() > 0) {
			sent = 0;
			socketP->SendTo(byteArrayTemp.GetData(), byteArrayTemp.Num(), sent, *addr);
			byteArrayTemp.Empty();
		}
	}
	else {
		sent = 0;
		socketP->SendTo(byteArray.GetData(), byteArray.Num(), sent, *addr);
	}

	byteArray.Empty();
}