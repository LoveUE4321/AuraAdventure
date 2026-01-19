// Copyright 2017-2019 David Romanski (Socke). All Rights Reserved.

#pragma once

#include "SocketClient.h"
#include "DNSClientSocketClient.h"
#include "SocketClientBPLibrary.generated.h"

class FSocketClientCleanerThread;

UENUM(BlueprintType)
enum class ESocketClientSystem : uint8
{
	Android,
	IOS,
	Windows,
	Linux,
	Mac
};

UENUM(BlueprintType)
enum class ESocketClientDirectoryType : uint8
{
	E_gd	UMETA(DisplayName = "Game directory"),
	E_ad 	UMETA(DisplayName = "Absolute directory")
};


class USocketClientTCPClient;
class USocketClientUDP;
class FSocketClientUDPReceiveDataThread;
//class FReadFileInPartsThread;

UCLASS()
class SOCKETCLIENT_API USocketClientBPLibrary : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	~USocketClientBPLibrary();


	//Delegates
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FsocketClientTCPConnectionEventDelegate, bool, success, FString, message, FString, clientConnectionID);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FreceiveTCPMessageEventDelegate, FString, message, const TArray<uint8>&, byteArray, FString, clientConnectionID);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FsocketClientUDPConnectionEventDelegate, bool, success, FString, message, FString, clientConnectionID);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FreceiveUDPMessageEventDelegate, FString, message, const TArray<uint8>&, byteArray, FString, IP_FromSender, int32, portFromSender, FString, clientConnectionID);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FreadBytesFromFileInPartsEventDelegate, int64, fileSize, int64, position,bool, end, const TArray<uint8>&, byteArray);
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
		void socketClientUDPConnectionEventDelegate(const bool success, const FString message, const FString clientConnectionID);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|UDP|Events|ConnectionInfo")
		FsocketClientUDPConnectionEventDelegate onsocketClientUDPConnectionEventDelegate;
	UFUNCTION()
		void receiveUDPMessageEventDelegate(const FString message, const TArray<uint8>& byteArray, const  FString IP, const int32 port,const FString clientConnectionID);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|UDP|Events|ReceiveMessage")
		FreceiveUDPMessageEventDelegate onreceiveUDPMessageEventDelegate;
	UFUNCTION()
		void readBytesFromFileInPartsEventDelegate(const int64 fileSize, const int64 position,const bool end, const TArray<uint8>& byteArray);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|SpecialFunctions|File|Events|ReadBytesFromFileInPartsAsync")
		FreadBytesFromFileInPartsEventDelegate onreadBytesFromFileInPartsEventDelegate;
	UFUNCTION()
		void transferFileOverTCPProgressEventDelegate(const FString clientConnectionID,const FString filePath, const float percent, const float mbit, const int64 bytesTransferred, const int64 fileSize);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|TCP|Events|File|transferFileOverTCPProgress")
		FtransferFileOverTCPProgressEventDelegate ontransferFileOverTCPProgressEventDelegate;
	UFUNCTION()
		void fileTransferOverTCPInfoEventDelegate(const FString message, const FString clientConnectionID, const FString filePath, const bool success);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|TCP|Events|File|FileTransferOverTCPInfo")
		FfileTransferOverTCPInfoEventDelegate onfileTransferOverTCPInfoEventDelegate;


	/**
	*  Get an instance of this library. This allows non-static functions to be called. 
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient")
		static USocketClientBPLibrary* getSocketClientTarget();
	static USocketClientBPLibrary* socketClientBPLibrary;



	/**
	* Connect to a TCP Server
	* @param domainOrIP IP or Domain of your server
	* @param ipType
	* @param port
	* @param receiveFilter This allows you to decide which data type you want to receive. If you receive files it makes no sense to convert them into a string.
	* @param messageSeparator It may be that data packets are split or merged when transmitted over TCP in order to optimize the transmission. For example, if you send "Hello" two times in a row very quickly, it can happen that "HelloHa" and "llo" arrive. To counteract this circumstance there are options to separate the data packets.
	* @param optionalCustomConnectionID Instead of an automatically generated ConnectionID you can use your own ID with this parameter.
	* @param ignoreTheKeepAlivePacket Activate to ignore the (0x00) keep alive byte of the server that can optionally be sent by the server plugin.
	* @param disableNaglesAlgorithm Don't change it if you don't know what this option is for! With this you can disable the TCP Nagle's algorithm to send (in LAN) very small data packets faster.
	**/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP", meta = (AdvancedDisplay = 8))
		static void connectSocketClientTCP(FString domainOrIP, ESocketClientIPType ipType, int32 port, EReceiveFilterClient receiveFilters,
			ESocketClientTCPSeparator messageSeparator, FString optionalCustomConnectionID, FString& connectionID, bool ignoreTheKeepAlivePacket = false, bool disableNaglesAlgorithm = false);
		void connectSocketClientTCPNonStatic(FString domainOrIP, ESocketClientIPType ipType, int32 port, EReceiveFilterClient receiveFilters,
			ESocketClientTCPSeparator messageSeparator,FString optionalCustomConnectionID, FString &connectionID, UTCPConnectAsyncNode* tcpConnectAsyncNode,bool ignoreKeepAlivePacket, bool disableNaglesAlgorithm = false);


	/**
	* Sends a string or byte array to the server. 
	*@param connectionID The ID to an existing connection.
	*@param message String to send
	*@param byteArray bytes to send
	*@param addLineBreak add a line break at the end
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP", meta = (AutoCreateRefTerm = "byteArray"))
		static void socketClientSendTCP(FString connectionID, FString message, TArray<uint8> byteArray, bool addLineBreak = true);
		void socketClientSendTCPNonStatic(FString connectionID, FString message, TArray<uint8> byteArray, bool addLineBreak = true);

	/**
	* Sends files as a kind of stream. This allows extremely large files to be sent since they do not have to be loaded into RAM beforehand. At the end of the transfer a MD5 checksum is created and compared between client and server to exclude errors during the transfer. 
	*@param connectionID The ID to an existing connection.
	*@param domainOrIP IP or Domain of your server
	*@param directoryType Absolute or relative directory. Absolute directory starts at the disk (e.g. Windows C:\).  A relative directory starts one level higher than the Content directory in the project or game.
	*@param filePath Directory including the file to be sent. 
	*@param token The token is a unique ID that the client and server must know. The server knows from the token in which directory the file should be saved. 
	*@param Aes256bitKey The AES key must consist of 32 ASCII characters. The communication between client and server is encrypted via AES in 256bit. Therefore a key must be entered.
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP")
		static void socketClientSendFileOverTCP(FString& connectionID, FString domainOrIP, ESocketClientIPType ipType, int32 port, EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, FString token, FString Aes256bitKey);
		void socketClientSendFileOverTCPNonStatic(FString& connectionID, FString domainOrIP, ESocketClientIPType ipType, int32 port, EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, FString token, FString Aes256bitKey);

	/**
	* Lets the server send a file to this client as a kind of stream. This allows extremely large files to be sent since they do not have to be loaded into RAM beforehand. At the end of the transfer a MD5 checksum is created and compared between client and server to exclude errors during the transfer.
	*@param connectionID The ID to an existing connection.
	*@param domainOrIP IP or Domain of your server
	*@param directoryType Absolute or relative directory. Absolute directory starts at the disk (e.g. Windows C:\).  A relative directory starts one level higher than the Content directory in the project or game.
	*@param filePath Directory including the file to be sent.
	*@param token The token is a unique ID that the client and server must know. The server recognizes by the toke what kind of file should be sent.
	*@param Aes256bitKey The AES key must consist of 32 ASCII characters. The communication between client and server is encrypted via AES in 256bit. Therefore a key must be entered.
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP")
		static void socketClientRequestFileOverTCP(FString& connectionID, FString domainOrIP, ESocketClientIPType ipType, int32 port, EFileFunctionsSocketClientDirectoryType directoryType, FString downloadDirectory, bool resume, FString token, FString Aes256bitKey);
	void socketClientRequestFileOverTCPNonStatic(FString& connectionID, FString domainOrIP, ESocketClientIPType ipType, int32 port, EFileFunctionsSocketClientDirectoryType directoryType, FString downloadDirectory, bool resume, FString token, FString Aes256bitKey);

	/**
	* Terminates an existing connection.
	*@param connectionID The ID to an existing connection.
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP")
		static void closeSocketClientConnectionTCP(FString connectionID);
	void closeSocketClientConnectionTCPNonStatic(FString connectionID);

	/**
	* Terminates all connections.
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP")
		static void closeAllSocketClientConnectionsTCP();
	void closeAllSocketClientConnectionsTCPNonStatic();


	/**
	* Useful if you want to attach events to a certain connection.
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP")
		static void getTCPConnectionByConnectionID(FString connectionID, bool &found, USocketClientTCPClient* &connection);
	void getTCPConnectionByConnectionIDNonStatic(FString connectionID, bool& found, USocketClientTCPClient*& connection);


	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP")
		static bool isTCPConnected(FString connectionID);
	bool isTCPConnectedNonStatic(FString connectionID);


	/**
	*Opens a connection on specific ip and port and listen on it.
	*@param DomainOrIP IP or Domain to listen on. 0.0.0.0 means that data can be received on all local IPs.
	*@param port port to listen on
	*@param receiveFilter This allows you to decide which data type you want to receive. If you receive files it makes no sense to convert them into a string.
	*@param maxPacketSize sets the maximum UDP packet size. More than 65507 is not possible.
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|UDP")
		static void socketClientInitUDPReceiver(FString& connectionID, FString domainOrIP = "0.0.0.0", ESocketClientIPType ipType = ESocketClientIPType::E_ipv4, int32 port = 8888, EReceiveFilterClient receiveFilter = EReceiveFilterClient::E_SAB, int32 maxPacketSize = 65507);
	void socketClientInitUDPReceiverNonStatic(FString& connectionID, UUDPInitAsyncNode* udpInitAsyncNode, FString domainOrIP = "0.0.0.0", ESocketClientIPType ipType = ESocketClientIPType::E_ipv4, int32 port = 8888, EReceiveFilterClient receiveFilter = EReceiveFilterClient::E_SAB, int32 maxPacketSize = 65507);

	/**
	* A ConnectionID must be created first with "socketClientInitUDPReceiver". Messages and bytes can be sent to different hosts with the same ConnectionID.
	*@param DomainOrIP target IP or Domain
	*@param port target port
	*@param message String to send
	*@param addLineBreak add a line break at the end
	*@param uniqueID is optional and required when multiple connections to the same server (same ip and port) shall be established. You can use getUniquePlayerID
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|UDP", meta = (AutoCreateRefTerm = "byteArray"))
		static void socketClientSendUDP(FString domainOrIP, ESocketClientIPType ipType, int32 port, FString message, TArray<uint8> byteArray, bool addLineBreak = true, FString connectionID = "");
	void socketClientSendUDPNonStatic(FString domainOrIP, ESocketClientIPType ipType, int32 port, FString message, TArray<uint8> byteArray, bool addLineBreak = true, FString connectionID = "");

	/**
	* Terminates an existing connection.
	*@param connectionID The ID to an existing connection.
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|UDP")
		static void closeSocketClientConnectionUDP(FString connectionID);
	void closeSocketClientConnectionUDPNonStatic(FString connectionID);

	/**
	* Useful if you want to attach events to a certain connection.
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|UDP")
		static void getUDPInitializationByConnectionID(FString connectionID, bool& found, USocketClientUDP*& connection);
	void getUDPInitializationByConnectionIDNonStatic(FString connectionID, bool& found, USocketClientUDP*& connection);


	UFUNCTION(BlueprintCallable, Category = "SocketClient|UDP")
		static bool isUDPInitialized(FString connectionID);
	bool isUDPInitializedNonStatic(FString connectionID);

	/**
	*Trying to determine the local IP. It uses a function in the engine that does not work on all devices. On Windows and Linux it seems to work very well. Very bad on Android. 0.0.0.0 will be returned if it doesn't work.
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions")
		static FString getLocalIP();


	/**
	*UE4 uses different socket connections. When Steam is active, Steam Sockets are used for all connections. This leads to problems if you want to use Steam but not Steam Sockets. Therefore you can change the sockets to "System".
	*@param ESocketPlatformServer System = Windows on Windows, Mac = Mac on Mac ect.
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions")
		static void changeSocketPlatform(ESocketPlatformClient platform);


	/**
	*The cleaner thread is a thread that runs endlessly and deletes data remnants from closed/broken connections from RAM. 
	*@param showLogs Writes to the logs when data remnants are deleted.
	*@param minLiveTimeInSeconds When a connection is closed it is passed to the cleaner thread. The thread ignores the connection for "minLiveTimeInSeconds" until it clears the data. This is necessary because sometimes connections need some time to be closed completely.
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions")
		static void changeCleanerThreadSettingsOnClient(bool showLogs, int32 minLiveTimeInSeconds = 10);
	
	/**
	* Returns which system you are currently use. (Windows, OSX, IOS ...)
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions", Meta = (ExpandEnumAsExecs = "system"))
		static void getSystemType(ESocketClientSystem& system);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions")
		static int32 getUniquePlayerID(APlayerController* playerController = nullptr);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions")
		static FString getRandomID();

	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions|Hex")
		static TArray<uint8> parseHexToBytes(FString hex);

	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions|Hex")
		static FString parseHexToString(FString hex);

	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions|Hex")
		static FString parseBytesToHex(TArray<uint8> bytes);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Hex")
		static TArray<uint8> parseHexToBytesPure(FString hex);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Hex")
		static FString parseHexToStringPure(FString hex);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Hex")
		static FString parseBytesToHexPure(TArray<uint8> bytes);


	//number stuff
	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions|Number")
		static void parseBytesToFloat(TArray<uint8> bytes, float& value);

	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions|Number")
		static void parseBytesToInteger(TArray<uint8> bytes, int32& value);

	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions|Number")
		static void parseBytesToInteger64(TArray<uint8> bytes, int64& value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Number")
		static void parseBytesToFloatPure(TArray<uint8> bytes, float& value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Number")
		static void parseBytesToIntegerPure(TArray<uint8> bytes, int32& value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Number")
		static void parseBytesToInteger64Pure(TArray<uint8> bytes, int64& value);

	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions|Number")
		static void parseBytesToFloatEndian(TArray<uint8> bytes, float& littleEndian, float& bigEndian);

	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions|Number")
		static void parseBytesToIntegerEndian(TArray<uint8> bytes, int32& littleEndian, int32& bigEndian);

	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions|Number")
		static void parseBytesToInteger64Endian(TArray<uint8> bytes, int64& littleEndian, int64& bigEndian);

	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions|Number", meta = (AutoCreateRefTerm = "byteArray"))
		static void parseFloatToBytes(TArray<uint8>& byteArray, float value, bool switchByteOrder = false);

	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions|Number", meta = (AutoCreateRefTerm = "byteArray"))
		static void parseIntegerToBytes(TArray<uint8>& byteArray, int32 value, bool switchByteOrder = false);

	UFUNCTION(BlueprintCallable, Category = "SocketClient|SpecialFunctions|Number", meta = (AutoCreateRefTerm = "byteArray"))
		static void parseInteger64ToBytes(TArray<uint8>& byteArray, int64 value, bool switchByteOrder = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Number", meta = (AutoCreateRefTerm = "byteArray"))
		static void parseFloatToBytesPure(TArray<uint8>& byteArray, float value, bool switchByteOrder = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Number", meta = (AutoCreateRefTerm = "byteArray"))
		static void parseIntegerToBytesPure(TArray<uint8>& byteArray, int32 value, bool switchByteOrder = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Number", meta = (AutoCreateRefTerm = "byteArray"))
		static void parseInteger64ToBytesPure(TArray<uint8>& byteArray, int64 value, bool switchByteOrder = false);


	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Number", meta = (AutoCreateRefTerm = "value"))
		static void parseBytesToFloatArrayPure(TArray<float>& value, TArray<uint8> bytes);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Number", meta = (AutoCreateRefTerm = "value"))
		static void parseBytesToIntegerArrayPure(TArray<int32>& value, TArray<uint8> bytes);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Number", meta = (AutoCreateRefTerm = "value"))
		static void parseBytesToInteger64ArrayPure(TArray<int64>& value, TArray<uint8> bytes);


	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Number", meta = (AutoCreateRefTerm = "byteArray"))
		static void parseFloatArrayToBytesPure(TArray<uint8>& byteArray, TArray<float> value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Number", meta = (AutoCreateRefTerm = "byteArray"))
		static void parseIntegerArrayToBytesPure(TArray<uint8>& byteArray, TArray<int32> value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient|SpecialFunctions|Number", meta = (AutoCreateRefTerm = "byteArray"))
		static void parseInteger64ArrayToBytesPure(TArray<uint8>& byteArray, TArray<int64> value);



	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP")
		static void changeTCPSeparatorStringOnClient(FString separator = "(~{");
	void changeTCPSeparatorStringOnClientNonStatic(FString separator);

	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP")
		static void changeTCPSeparatorByteOnClient(uint8 separator = 0x00);
	void changeTCPSeparatorByteOnClientNonStatic(uint8 separator);


	void getTcpSeparator(uint8& byteSeparator, FString& stringSeparator);

	static ISocketSubsystem* getSocketSubSystem();

	FString resolveDomain(FString domain, ESocketClientIPType ipType);
	//ue4 domain resolve does not work with steam. this is my own dns client
	class UDNSClientSocketClient* dnsClient = nullptr;
	TMap<FString, FString> domainCache;
	

	void cleanConnection(FSocketClientPluginSession& session);

private:
	ESocketPlatformClient systemSocketPlatform;

	TMap<FString, USocketClientTCPClient*> tcpClients;
	TMap<FString, USocketClientUDP*> udpClients;
	//TMap<FString, FReadFileInPartsThread*> readFileInPartsThreads;

	int64 lastErrorMessageTime = 0;

	FString tcpStringSeparator = "(~{";
	uint8 tcpByteSeparator = 0x00;
	
	FSocketClientCleanerThread* socketClientCleanerThread = nullptr;
};
