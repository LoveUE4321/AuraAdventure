#include "UDPClientUtility.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "IPAddress.h"

// Sets default values
AUDPClientUtility::AUDPClientUtility()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	udpSocket = NULL;
}

// Called when the game starts or when spawned
void AUDPClientUtility::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AUDPClientUtility::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AUDPClientUtility::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (udpSocket)
	{
		udpSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(udpSocket);
	}
}

bool AUDPClientUtility::CreateUdp(const FString& socketName, const FString& targetIP, const int32 targetPort, const int32 selfPort)
{
	bool bIsValid;
	RemoteAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	RemoteAddr->SetIp(*targetIP, bIsValid);
	RemoteAddr->SetPort(targetPort);
	if (!bIsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateUdp>> IP address was not valid! "), *targetIP);
		return false;
	}
	int32 BufferSize = 2 * 1024 * 1024;
	FIPv4Endpoint Endpoint(FIPv4Address::Any, selfPort);  //所有ip地址本地
	udpSocket = FUdpSocketBuilder(*socketName)
		.AsReusable()
		.WithBroadcast() // 广播
		.WithSendBufferSize(BufferSize)
		.AsNonBlocking()
		.BoundToEndpoint(Endpoint)
		.WithReceiveBufferSize(BufferSize);
	if (udpSocket == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateUdp Socket was not valid! "), *targetIP);
		return false;
	}

	udpSocket->SetSendBufferSize(BufferSize, BufferSize);
	udpSocket->SetReceiveBufferSize(BufferSize, BufferSize);
	return bIsValid;
}

bool AUDPClientUtility::SendMsg(FString msg)//发送消息
{
	if (!udpSocket)
	{
		UE_LOG(LogTemp, Warning, TEXT("No udpSocket"));
		return false;
	}
	int32 BytesSent = 0;
	FString serialized = msg;
	TCHAR* serializedChar = serialized.GetCharArray().GetData();
	int32 size = FCString::Strlen(serializedChar);
	int32 sent = 0;
	udpSocket->SendTo((uint8*)TCHAR_TO_UTF8(serializedChar), size, BytesSent, *RemoteAddr);
	if (BytesSent < 0)
	{
		const FString Str = "Socket is valid but the receiver received 0 bytes, make sure it is listening properly!";
		UE_LOG(LogTemp, Error, TEXT("%s"), *Str);
		return false;
	}
	UE_LOG(LogTemp, Warning, TEXT("SendMsg Succcess! INFO msg = %s "), *msg);
	return true;
}

void AUDPClientUtility::RecvMsg(bool& result, FString& msg)//接收消息
{
	if (!udpSocket)
	{
		UE_LOG(LogTemp, Warning, TEXT("No udpSocket"));
		result = false;
		return;
	}
	TSharedRef<FInternetAddr> targetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	TArray<uint8> ReceivedData;//定义一个接收器
	uint32 Size;
	if (udpSocket->HasPendingData(Size))
	{
		result = true;
		msg = "";
		uint8* Recv = new uint8[Size];
		int32 BytesRead = 0;

		ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));
		udpSocket->RecvFrom(ReceivedData.GetData(), ReceivedData.Num(), BytesRead, *targetAddr);//创建远程接收地址
		char ansiiData[1024];
		memcpy(ansiiData, ReceivedData.GetData(), BytesRead);//拷贝数据到接收器
		ansiiData[BytesRead] = 0;                            //判断数据结束
		FString debugData = UTF8_TO_TCHAR(ansiiData);         //字符串转换
		msg = debugData;
	}
	else
	{
		result = false;
	}
}


bool AUDPClientUtility::SendUDPMessage(FString Message, const FString& TargetIP, int32 Port)
{

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem) return false;

	// 创建UDP套接字
	TUniquePtr<FSocket> UDPSocket(SocketSubsystem->CreateSocket(NAME_DGram, TEXT("UDP_Client")));
	if (!UDPSocket || !UDPSocket.IsValid()) return false;

	// 设置非阻塞模式
	UDPSocket->SetNonBlocking(true);

	// 转换字符串为字节流
	TArray<uint8> Data;
	Data.AddZeroed(Message.Len());
	FCStringAnsi::Strncpy((ANSICHAR*)Data.GetData(), TCHAR_TO_UTF8(*Message), Message.Len());

	// 配置目标地址
	//FInternetAddr Recipient;
	TSharedRef<FInternetAddr> Recipient = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

	const TCHAR* InAddr = *TargetIP;
	bool bIsValid = true;
	Recipient->SetIp(*TargetIP, bIsValid);
	Recipient->SetPort(Port);

	// 发送数据
	int32 BytesSent = 0;
	bool bSuccess = UDPSocket->SendTo(Data.GetData(), Data.Num(), BytesSent, *Recipient);
	return bSuccess && (BytesSent == Data.Num());
}