// Copyright 2017-2019 David Romanski (Socke). All Rights Reserved.
#pragma once

#include "SocketClient.h"
#include "SocketClientTCPReceiveDataThread.h"
#include "SocketClientTCPSendDataThead.h"
#include "SocketClientTCPFileHandlerThread.h"
#include "SocketClientTCP.generated.h"

class USocketServerBPLibrary;

UCLASS(Blueprintable, BlueprintType)
class SOCKETCLIENT_API  USocketClientTCPClient : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	//Delegates
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FsocketClientTCPConnectionEventDelegate, bool, success, FString, message, FString, clientConnectionID);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FreceiveTCPMessageEventDelegate, FString, message, const TArray<uint8>&, byteArray, FString, clientConnectionID);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FtransferFileOverTCPProgressEventDelegate, FString, clientConnectionID, FString, filePath, float, percent, float, mbit, int64, bytesTransferred, int64, fileSize);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FfileTransferOverTCPInfoEventDelegate, FString, message, FString, clientConnectionID, FString, filePath, bool, success);

	UFUNCTION()
		void socketClientTCPConnectionEventDelegate(const bool success, const FString message, const FString clientConnectionID);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|TCP|Events|ConnectionInfo")
		FsocketClientTCPConnectionEventDelegate onsocketClientTCPConnectionEventDelegate;
	UFUNCTION()
		void receiveTCPMessageEventDelegate(const FString message, const TArray<uint8>& byteArray, const FString clientConnectionID);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|TCP|Events|ReceiveMessage")
		FreceiveTCPMessageEventDelegate onreceiveTCPMessageEventDelegate;
	UFUNCTION()
		void transferFileOverTCPProgressEventDelegate(const FString clientConnectionID, const FString filePath, const float percent, const float mbit, const int64 bytesTransferred, const int64 fileSize);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|TCP|Events|File|transferFileOverTCPProgress")
		FtransferFileOverTCPProgressEventDelegate ontransferFileOverTCPProgressEventDelegate;
	UFUNCTION()
		void fileTransferOverTCPInfoEventDelegate(const FString message, const FString clientConnectionID, const FString filePath, const bool success);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|TCP|Events|File|FileTransferOverTCPInfo")
		FfileTransferOverTCPInfoEventDelegate onfileTransferOverTCPInfoEventDelegate;

	UFUNCTION()
		void connectionEvent(bool success, FString message, FString clientConnectionID);

	void connect(USocketClientBPLibrary* mainLib, FString domainOrIP, ESocketClientIPType ipType, int32 port, EReceiveFilterClient receiveFilter,
		ESocketClientTCPSeparator messageWrapping, FString connectionID, UTCPConnectAsyncNode* tcpConnectAsyncNode,bool ignoreKeepAlivePacket,
		bool noPacketDelay = false, bool noPacketBlocking = false);
	void sendMessage(FString message, TArray<uint8> byteArray);
	void sendFile(USocketClientBPLibrary* mainLib, FString connectionID, FString domainOrIP, ESocketClientIPType ipType, int32 port, EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, FString token, FString Aes256bitKey);
	void requestFile(USocketClientBPLibrary* mainLib, FString connectionID, FString domainOrIP, ESocketClientIPType ipType, int32 port, EFileFunctionsSocketClientDirectoryType directoryType, FString filePath,bool resume, FString token, FString Aes256bitKey);
	
	void closeConnection();

	bool isRun();
	void setRun(bool runP);
	FString getConnectionID();
	FString getAesKey();
	FString getFileToken();
	FString getFilePath();

	void setSocket(FSocket* socket);
	FSocket* getSocket();
	USocketClientBPLibrary* getMainLib();

	void createSendThread();
	//void createFileSendThread(int64 startPosition);
	FString encryptMessage(FString message);
	FString decryptMessage(FString message);
	void readDataLength(TArray<uint8>& byteDataArray, int32& byteLenght);
	bool isSendFile();
	bool isReceiveFile();
	bool hasResume();
	bool isConnected();
	void deleteFile(FString filePathP);
	void getMD5FromFileAbsolutePath(FString filePath, bool& success, FString& MD5);
	int64 fileSize(FString filePath);
	FString int64ToString(int64 num);
	void getTcpSeparator(FString& stringSeparator, uint8& byteSeparator, ESocketClientTCPSeparator& messageWrapping);
	bool hasIgnoreKeepAlivePacket();

	UTCPConnectAsyncNode* tcpConnectAsyncNode = nullptr;

private:

	bool run = false;
	bool connected = false;
	bool resume = false;
	bool ignoreKeepAlivePacket = false;
	int32 sendOrReceive = -1; //0 == send, 1 == receive;
	FString connectionID = FString();
	FString aesKey = FString();
	FString fileToken = FString();
	FString filePath = FString();


	FSocket* socket = nullptr;

	FSocketClientTCPReceiveDataThread* tcpReceiveDataThread = nullptr;
	FSocketClientTCPSendDataThead* tcpSendThread = nullptr;
	FSocketClientTCPFileHandlerThread* tcpFileConnectionThread = nullptr;
	//FSocketClientTCPSendFileThread* fileSendThread = nullptr;
	USocketClientBPLibrary* mainLib = nullptr;


	ESocketClientTCPSeparator messageWrapping;
	FString tcpStringSeparator = "(~{";
	uint8 tcpByteSeparator = 0x00;

};