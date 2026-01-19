// Copyright 2022 David Romanski (Socke). All Rights Reserved.

#include "SocketClientUDPReceiveDataThread.h"


FSocketClientUDPReceiveDataThread::FSocketClientUDPReceiveDataThread(USocketClientUDP* udpClientP, USocketClientBPLibrary* socketClientP, FString ipP, int32 portP, ESocketClientIPType ipTypeP) :
	udpClient(udpClientP),
	socketClient(socketClientP),
	ipGlobal(ipP),
	portGlobal(portP),
	ipType(ipTypeP) {
	FString threadName = "FServerUDPConnectionThread_" + FGuid::NewGuid().ToString();
	thread = FRunnableThread::Create(this, *threadName, 0, EThreadPriority::TPri_Normal);
}

FSocketClientUDPReceiveDataThread::~FSocketClientUDPReceiveDataThread() {
	if (udpClient != nullptr && udpClient->udpInitAsyncNode != nullptr) {
		udpClient->udpInitAsyncNode = nullptr;
	}
	delete thread;
}

void FSocketClientUDPReceiveDataThread::triggerInitEvent(bool success, USocketClientUDP* udpClientP, USocketClientBPLibrary* socketClientP,
	FString serverMessage, FString connectionID){
	AsyncTask(ENamedThreads::GameThread, [success, udpClientP, socketClientP, serverMessage, connectionID]() {
	if (socketClientP != nullptr)
		socketClientP->onsocketClientUDPConnectionEventDelegate.Broadcast(success, serverMessage, connectionID);
	if (udpClientP != nullptr) {
		udpClientP->onsocketClientUDPConnectionEventDelegate.Broadcast(success, serverMessage, connectionID);
		if (udpClientP->udpInitAsyncNode != nullptr) {
			udpClientP->udpInitAsyncNode->triggerInitEvent(success, connectionID, serverMessage);
		}
	}
	});
}

uint32 FSocketClientUDPReceiveDataThread::Run() {

	USocketClientUDP* udpClientGlobal = udpClient;
	FString ip = socketClient->resolveDomain(ipGlobal, ipType);
	udpClient->setIP(ip);
	int32 port = portGlobal;
	FString connectionID = udpClient->getConnectionID();


	if (socket == nullptr || socket == NULL) {

		FString endpointAdress = ip + ":" + FString::FromInt(port);
		FIPv4Endpoint Endpoint;

		// create the socket
		FString socketName;
		ISocketSubsystem* socketSubsystem = USocketClientBPLibrary::getSocketSubSystem();

		TSharedRef<FInternetAddr> addr = socketSubsystem->CreateInternetAddr();
		bool validIP = true;
		addr->SetPort(port);
		addr->SetIp(*ip, validIP);


		if (!validIP) {
			UE_LOG(LogTemp, Error, TEXT("SocketClient UDP. Can't set ip"));
			triggerInitEvent(false, udpClient, socketClient, "SocketClient UDP. Can't set ip", connectionID);
			return 0;
		}

		socket = socketSubsystem->CreateSocket(NAME_DGram, *socketName, addr->GetProtocolType());


		if (socket == nullptr || socket == NULL) {
			const TCHAR* SocketErr = socketSubsystem->GetSocketError(SE_GET_LAST_ERROR_CODE);
			UE_LOG(LogTemp, Error, TEXT("UE could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router. %s:%i. Error: %s"), *ip, port, SocketErr);
			triggerInitEvent(false, udpClient, socketClient, "(Error 0) UE could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router." + addr->ToString(true) + " Error:" + SocketErr, connectionID);
			return 0;
		}

		if (!socket->SetRecvErr()) {
			UE_LOG(LogTemp, Error, TEXT("SocketClient UDP. Can't set recverr"));
		}


		if (socket == nullptr || socket == NULL || !validIP) {
			const TCHAR* SocketErr = socketSubsystem->GetSocketError(SE_GET_LAST_ERROR_CODE);
			UE_LOG(LogTemp, Error, TEXT("UE could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router. %s:%i. Error: %s"), *ip, port, SocketErr);
			triggerInitEvent(false, udpClient, socketClient, "(Error 1) UE could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router." + addr->ToString(true) + " Error:" + SocketErr, connectionID);
			return 0;
		}
		socket->SetReuseAddr(true);
		socket->SetNonBlocking(true);
		socket->SetBroadcast(true);

		if (!socket->Bind(*addr)) {
			const TCHAR* SocketErr = socketSubsystem->GetSocketError(SE_GET_LAST_ERROR_CODE);
			UE_LOG(LogTemp, Error, TEXT("UE could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router. %s:%i. Error: %s"), *ip, port, SocketErr);
			triggerInitEvent(false, udpClient, socketClient, "(Error 2) UE could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router." + addr->ToString(true) + " Error:" + SocketErr, connectionID);
			return 0;
		}



		FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
		FUdpSocketReceiver* udpSocketReceiver = new FUdpSocketReceiver(socket, ThreadWaitTime, TEXT("SocketClientBPLibUDPReceiverThread"));
		udpSocketReceiver->OnDataReceived().BindUObject(udpClient, &USocketClientUDP::UDPReceiver);
		udpSocketReceiver->Start();
		udpClient->setUDPSocketReceiver(udpSocketReceiver);

		udpClient->setSocket(socket);
		udpClient->setRun(true);
		udpClient->setUDPSendThread(new FSocketClientUDPSendDataThead(udpClient, socketClient, ip, port));


		triggerInitEvent(true, udpClient, socketClient, "Init UDP Connection OK. " + addr->ToString(true), connectionID);

	}
	thread = nullptr;
	return 0;
}