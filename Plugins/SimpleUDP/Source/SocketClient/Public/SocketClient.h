// Copyright 2017-2019 David Romanski (Socke). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Common/UdpSocketReceiver.h"
#include "Common/UdpSocketBuilder.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Async/Async.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Containers/Queue.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Base64.h"
#include "Misc/SecureHash.h"
#include "Misc/AES.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 & ENGINE_MINOR_VERSION >= 2
#include "IPAddressAsyncResolve.h"
#endif
#include "SocketClient.generated.h"


class FSocketClientTCPReceiveDataThread;
class FSocketClientTCPSendDataThead;

class FSocketClientTCPFileHandlerThread;

class FUdpSocketReceiver;
class FSocketClientUDPReceiveDataThread;
class FSocketClientUDPSendDataThead;

class UTCPConnectAsyncNode;
class UUDPInitAsyncNode;

USTRUCT(BlueprintType)
struct FFileFunctionsSocketClientOpenFile
{
	GENERATED_USTRUCT_BODY()

		FArchive* writer = nullptr;

};

USTRUCT()
struct FSocketClientPluginSession
{
	GENERATED_USTRUCT_BODY()

	int64 addToCleanerTime = 0;
	FString clientID = FString();
	FSocket* socket = nullptr;

	FSocketClientTCPSendDataThead* tcpSendThread = nullptr;
	FSocketClientTCPReceiveDataThread* tcpRecieverThread = nullptr;

	FSocketClientTCPFileHandlerThread* tcpFileHandlerThread = nullptr;

	FUdpSocketReceiver* udpSocketReceiver = nullptr;
	FSocketClientUDPSendDataThead* udpSendDataThead = nullptr;
	FSocketClientUDPReceiveDataThread* udpReceiveDataThread = nullptr;

};


UENUM(BlueprintType)
enum class EFileFunctionsSocketClientDirectoryType : uint8
{
	E_gd	UMETA(DisplayName = "Game directory"),
	E_ad 	UMETA(DisplayName = "Absolute directory")
};


UENUM(BlueprintType)
enum class EFileFunctionsSocketClientEncodingOptions : uint8
{
	E_AutoDetect	UMETA(DisplayName = "AutoDetect"),
	E_ForceAnsi 	UMETA(DisplayName = "ForceAnsi"),
	E_ForceUnicode	UMETA(DisplayName = "ForceUnicode"),
	E_ForceUTF8 	UMETA(DisplayName = "ForceUTF8"),
	E_ForceUTF8WithoutBOM	UMETA(DisplayName = "ForceUTF8WithoutBOM")
};

UENUM(BlueprintType)
enum class EReceiveFilterClient : uint8
{
	E_SAB 	UMETA(DisplayName = "Message And Bytes"),
	E_S		UMETA(DisplayName = "Message"),
	E_B		UMETA(DisplayName = "Bytes")

};


UENUM(BlueprintType)
enum class ESocketPlatformClient : uint8
{
	E_SSC_SYSTEM		UMETA(DisplayName = "System"),
	E_SSC_DEFAULT 		UMETA(DisplayName = "Auto"),
	E_SSC_WINDOWS		UMETA(DisplayName = "WINDOWS"),
	E_SSC_MAC			UMETA(DisplayName = "MAC"),
	E_SSC_IOS			UMETA(DisplayName = "IOS"),
	E_SSC_UNIX			UMETA(DisplayName = "UNIX"),
	E_SSC_ANDROID		UMETA(DisplayName = "ANDROID"),
	E_SSC_PS4			UMETA(DisplayName = "PS4"),
	E_SSC_XBOXONE		UMETA(DisplayName = "XBOXONE"),
	E_SSC_HTML5			UMETA(DisplayName = "HTML5"),
	E_SSC_SWITCH		UMETA(DisplayName = "SWITCH")

};

UENUM(BlueprintType)
enum class ESocketClientIPType : uint8
{
	E_ipv4	UMETA(DisplayName = "IPv4"),
	E_ipv6 	UMETA(DisplayName = "IPv6")
};


UENUM(BlueprintType)
enum class ESocketClientTCPSeparator : uint8
{
	E_None 				UMETA(DisplayName = "None"),
	E_ByteSeparator		UMETA(DisplayName = "Separate via one Byte"),
	E_StringSeparator	UMETA(DisplayName = "Separate via String"),
	E_LengthSeparator	UMETA(DisplayName = "Separate by Length")

};




#ifndef __SocketClientFileFunctions
#define __SocketClientFileFunctions
#include "SocketClientFileFunctions.h"
#endif

#ifndef __SocketClientCleanerThread
#define __SocketClientCleanerThread
#include "SocketClientCleanerThread.h"
#endif

#ifndef __SocketClientAsyncNodes
#define __SocketClientAsyncNodes
#include "SocketClientAsyncNodes.h"
#endif

#ifndef __SocketClientBPLibrary
#define __SocketClientBPLibrary
#include "SocketClientBPLibrary.h"
#endif

#ifndef __SocketClientTCP
#define __SocketClientTCP
#include "SocketClientTCP.h"
#endif

#ifndef __SocketClientUDP
#define __SocketClientUDP
#include "SocketClientUDP.h"
#endif

class FSocketClientModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static bool isShuttingDown;
};