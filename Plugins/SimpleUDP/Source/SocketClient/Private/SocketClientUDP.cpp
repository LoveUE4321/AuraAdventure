// Copyright 2017-2019 David Romanski (Socke). All Rights Reserved.

#include "SocketClientUDP.h"


USocketClientUDP::USocketClientUDP(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	this->AddToRoot();
}



void USocketClientUDP::socketClientUDPConnectionEventDelegate(const bool success, const FString message, const FString clientConnectionIDP){}
void USocketClientUDP::receiveUDPMessageEventDelegate(const FString message, const TArray<uint8>& byteArray, const FString IPP, const int32 portP, const FString clientConnectionIDP) {}

void USocketClientUDP::init(USocketClientBPLibrary* socketClientLibP, UUDPInitAsyncNode* udpInitAsyncNodeP, FString domainOrIPP,
	ESocketClientIPType ipType, int32 portP, EReceiveFilterClient receiveFilterP, FString connectionIDP,
	int32 maxPacketSizeP) {
	socketClientBPLibrary = socketClientLibP;
	receiveFilter = receiveFilterP;
	connectionID = connectionIDP;
	domainOrIP = domainOrIPP;
	port = portP;
	udpInitAsyncNode = udpInitAsyncNodeP;

	maxPacketSize = maxPacketSizeP;
	if (maxPacketSize < 1 || maxPacketSize > 65507)
		maxPacketSize = 65507;

	UDPThread = new FSocketClientUDPReceiveDataThread(this, socketClientLibP, domainOrIPP, portP, ipType);
}


void USocketClientUDP::sendUDPMessage(FString domainOrIPP, ESocketClientIPType ipType, int32 portP, FString message, TArray<uint8> byteArray){
	if (UDPSendThread != nullptr) {
		UDPSendThread->addData(message, byteArray, domainOrIPP, portP, ipType);
	}
}

void USocketClientUDP::closeUDPConnection() {

	if (udpSocketReceiver != nullptr) {
		udpSocketReceiver->Stop();
	}

	setRun(false);
	if (UDPSendThread != nullptr) {
		UDPSendThread->pauseThread(false);
	}

	if (socketClientBPLibrary != nullptr) {
		FSocketClientPluginSession connectionSession = FSocketClientPluginSession();
		connectionSession.udpSocketReceiver = udpSocketReceiver;
		connectionSession.udpSendDataThead = UDPSendThread;
		connectionSession.udpReceiveDataThread = UDPThread;
		connectionSession.socket = socket;
		connectionSession.clientID = connectionID;
		socketClientBPLibrary->cleanConnection(connectionSession);
	}

	//udpInitAsyncNode = nullptr;
}


void USocketClientUDP::UDPReceiver(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt) {

	if (FSocketClientModule::isShuttingDown)
		return;

	TSharedPtr<FInternetAddr> peerAddr = EndPt.ToInternetAddr();
	FString ipGlobal = peerAddr->ToString(false);
	int32 portGlobal = peerAddr->GetPort();

	TArray<uint8> byteArray;
	if (receiveFilter == EReceiveFilterClient::E_SAB || receiveFilter == EReceiveFilterClient::E_B) {
		byteArray.Append(ArrayReaderPtr->GetData(), ArrayReaderPtr->Num());
	}

	FString recvMessage;
	if (receiveFilter == EReceiveFilterClient::E_SAB || receiveFilter == EReceiveFilterClient::E_S) {
		ArrayReaderPtr->Add(0x00);// null-terminator
		char* Data = (char*)ArrayReaderPtr->GetData();
		recvMessage = FString(UTF8_TO_TCHAR(Data));
	}

	//switch to gamethread
	USocketClientBPLibrary* socketClientBPLibraryGlobal = socketClientBPLibrary;
	USocketClientUDP* udpClientGlobal = this;
	FString clientConnectionIDGlobal = connectionID;
	UUDPInitAsyncNode* udpInitAsyncNodeGlobal = udpInitAsyncNode;
	AsyncTask(ENamedThreads::GameThread, [udpInitAsyncNodeGlobal, udpClientGlobal, recvMessage, byteArray, ipGlobal, portGlobal, socketClientBPLibraryGlobal, clientConnectionIDGlobal]() {
		if (FSocketClientModule::isShuttingDown)
			return;
		socketClientBPLibraryGlobal->onreceiveUDPMessageEventDelegate.Broadcast(recvMessage, byteArray, ipGlobal, portGlobal, clientConnectionIDGlobal);
		udpClientGlobal->onreceiveUDPMessageEventDelegate.Broadcast(recvMessage, byteArray, ipGlobal, portGlobal, clientConnectionIDGlobal);
		if (udpInitAsyncNodeGlobal != nullptr) {
			udpInitAsyncNodeGlobal->triggerMessageEvent(byteArray, clientConnectionIDGlobal, recvMessage, ipGlobal, portGlobal);
		}
	});
}

bool USocketClientUDP::isRun(){
	return run;
}

void USocketClientUDP::setRun(bool runP){
	run = runP;
}

FSocket* USocketClientUDP::getSocket(){
	return socket;
}

void USocketClientUDP::setSocket(FSocket* socketP){
	socket = socketP;
}

void USocketClientUDP::setUDPSocketReceiver(FUdpSocketReceiver* udpSocketReceiverP){
	udpSocketReceiver = udpSocketReceiverP;
}

FString USocketClientUDP::getIP(){
	return domainOrIP;
}

void USocketClientUDP::setIP(FString ipP){
	domainOrIP = ipP;
}

int32 USocketClientUDP::getPort(){
	return port;
}

FString USocketClientUDP::getDomainOrIP(){
	return domainOrIP;
}

FString USocketClientUDP::getConnectionID(){
	return connectionID;
}

void USocketClientUDP::setUDPSendThread(FSocketClientUDPSendDataThead* udpSendThreadP){
	UDPSendThread = udpSendThreadP;
}

int32 USocketClientUDP::getMaxPacketSize()
{
	return maxPacketSize;
}
