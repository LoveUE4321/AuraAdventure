// Copyright 2017-2019 David Romanski (Socke). All Rights Reserved.

#include "SocketClientTCP.h"


USocketClientTCPClient::USocketClientTCPClient(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	//This prevents the garbage collector from killingand deleting the class from RAM.
	this->AddToRoot();
	onsocketClientTCPConnectionEventDelegate.AddDynamic(this, &USocketClientTCPClient::connectionEvent);
}


void USocketClientTCPClient::socketClientTCPConnectionEventDelegate(const bool success, const FString message, const FString clientConnectionID) {}
void USocketClientTCPClient::receiveTCPMessageEventDelegate(const FString message, const TArray<uint8>& byteArray, const FString clientConnectionID){}
void USocketClientTCPClient::transferFileOverTCPProgressEventDelegate(const FString clientConnectionID, const FString filePathP, const float percent, const float mbit, const int64 bytesTransferred, const int64 fileSize) {}
void USocketClientTCPClient::fileTransferOverTCPInfoEventDelegate(const FString message, const FString clientConnectionID, const FString filePathP, const bool success) {}

void USocketClientTCPClient::connectionEvent(bool success, FString message, FString clientConnectionID){
	connected = success;
}

void USocketClientTCPClient::connect(USocketClientBPLibrary* mainLibP, FString domainOrIP, ESocketClientIPType ipType, int32 port, EReceiveFilterClient receiveFilter,
	ESocketClientTCPSeparator messageWrappingP, FString connectionIDP, UTCPConnectAsyncNode* tcpConnectAsyncNodeP,bool ignoreKeepAlivePacketP, bool noPacketDelay, bool noPacketBlocking){
	mainLib = mainLibP;
	connectionID = connectionIDP;
	messageWrapping = messageWrappingP;
	tcpConnectAsyncNode = tcpConnectAsyncNodeP;
	ignoreKeepAlivePacket = ignoreKeepAlivePacketP;
	USocketClientBPLibrary::socketClientBPLibrary->getTcpSeparator(tcpByteSeparator, tcpStringSeparator);
	tcpReceiveDataThread = new FSocketClientTCPReceiveDataThread(mainLib, connectionID, receiveFilter, domainOrIP, ipType, port,this,noPacketDelay,noPacketBlocking);
}

void USocketClientTCPClient::sendMessage(FString message, TArray<uint8> byteArray){
	if (run && tcpSendThread != nullptr) {
		tcpSendThread->sendMessage(message, byteArray);
	}
}

void USocketClientTCPClient::sendFile(USocketClientBPLibrary* mainLibP, FString connectionIDP, FString domainOrIP, ESocketClientIPType ipType, int32 port, EFileFunctionsSocketClientDirectoryType directoryType, FString filePathP, FString token, FString Aes256bitKey){
	mainLib = mainLibP;
	connectionID = connectionIDP;
	aesKey = Aes256bitKey;
	fileToken = token;
	sendOrReceive = 0;
	filePath = UFileFunctionsSocketClient::getCleanDirectory(directoryType, filePathP);
	tcpFileConnectionThread = new FSocketClientTCPFileHandlerThread(mainLib, connectionID, domainOrIP, ipType, port, this);
}

void USocketClientTCPClient::requestFile(USocketClientBPLibrary* mainLibP, FString connectionIDP, FString domainOrIP, ESocketClientIPType ipType, int32 port, EFileFunctionsSocketClientDirectoryType directoryType, FString filePathP, bool resumeP, FString token, FString Aes256bitKey) {
	mainLib = mainLibP;
	connectionID = connectionIDP;
	aesKey = Aes256bitKey;
	fileToken = token;
	sendOrReceive = 1;
	resume = resumeP;
	filePath = UFileFunctionsSocketClient::getCleanDirectory(directoryType, filePathP);
	tcpFileConnectionThread = new FSocketClientTCPFileHandlerThread(mainLib, connectionID, domainOrIP, ipType, port, this);
}

void USocketClientTCPClient::closeConnection(){

	setRun(false);

	if (tcpFileConnectionThread != nullptr) {
		tcpFileConnectionThread->triggerFileTransferOverTCPInfoEvent("Data transfer aborted.", connectionID, filePath, false, mainLib, this);
	}

	if (tcpSendThread != nullptr) {
		tcpSendThread->pauseThread(false);
	}


	if (mainLib != nullptr) {
		FSocketClientPluginSession connectionSession = FSocketClientPluginSession();
		connectionSession.tcpRecieverThread = tcpReceiveDataThread;
		connectionSession.tcpSendThread = tcpSendThread;
		//connectionSession.tcpSendFileThread = fileSendThread;
		connectionSession.tcpFileHandlerThread = tcpFileConnectionThread;
		connectionSession.socket = socket;
		connectionSession.clientID = connectionID;
		mainLib->cleanConnection(connectionSession);
	}
}

bool USocketClientTCPClient::isRun(){
	return run;
}

void USocketClientTCPClient::setRun(bool runP) {
	run = runP;
}

FString USocketClientTCPClient::getConnectionID(){
	return connectionID;
}

FString USocketClientTCPClient::getAesKey(){
	return aesKey;
}

FString USocketClientTCPClient::getFileToken(){
	return fileToken;
}

FString USocketClientTCPClient::getFilePath() {
	return filePath;
}

void USocketClientTCPClient::setSocket(FSocket* socketP){
	socket = socketP;
}

FSocket* USocketClientTCPClient::getSocket(){
	return socket;
}

void USocketClientTCPClient::createSendThread(){
	tcpSendThread = new FSocketClientTCPSendDataThead(mainLib, this, connectionID);
}

void USocketClientTCPClient::getTcpSeparator(FString& stringSeparator, uint8& byteSeparator, ESocketClientTCPSeparator& messageWrappingP) {
	messageWrappingP = messageWrapping;
	stringSeparator = tcpStringSeparator;
	tcpByteSeparator = byteSeparator;
}

bool USocketClientTCPClient::hasIgnoreKeepAlivePacket(){
	return ignoreKeepAlivePacket;
}

FString USocketClientTCPClient::encryptMessage(FString message) {
	return UFileFunctionsSocketClient::encryptMessageWithAES(message, aesKey);
}

FString USocketClientTCPClient::decryptMessage(FString message) {
	return UFileFunctionsSocketClient::decryptMessageWithAES(message, aesKey);
}

void USocketClientTCPClient::readDataLength(TArray<uint8>& byteDataArray, int32& byteLenght) {
	if (FGenericPlatformProperties::IsLittleEndian() && byteDataArray[0] == 0x00) {
		FMemory::Memcpy(&byteLenght, byteDataArray.GetData() + 1, 4);
	}
	else {
		//endian fits not. swap bytes that contains the length
		byteDataArray.SwapMemory(1, 4);
		byteDataArray.SwapMemory(2, 3);
		FMemory::Memcpy(&byteLenght, byteDataArray.GetData() + 1, 4);
	}
}

bool USocketClientTCPClient::isSendFile(){
	return (sendOrReceive == 0);
}

bool USocketClientTCPClient::isReceiveFile(){
	return (sendOrReceive == 1);
}

bool USocketClientTCPClient::hasResume(){
	return resume;
}

bool USocketClientTCPClient::isConnected()
{
	return connected;
}

void USocketClientTCPClient::deleteFile(FString filePathP){
	UFileFunctionsSocketClient::deleteFileAbsolutePath(filePathP);
}

void USocketClientTCPClient::getMD5FromFileAbsolutePath(FString filePathP, bool& success, FString& MD5) {
	UFileFunctionsSocketClient::getMD5FromFileAbsolutePath(filePathP, success, MD5);
}

int64 USocketClientTCPClient::fileSize(FString filePathP) {
	return UFileFunctionsSocketClient::fileSizeAbsolutePath(filePathP);
}

USocketClientBPLibrary* USocketClientTCPClient::getMainLib() {
	return mainLib;
}

FString USocketClientTCPClient::int64ToString(int64 num) {
	return UFileFunctionsSocketClient::int64ToString(num);
}
