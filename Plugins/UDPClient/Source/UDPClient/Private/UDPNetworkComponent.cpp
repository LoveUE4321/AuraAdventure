// UDPNetworkComponent.cpp
#include "UDPNetworkComponent.h"
#include "Async/Async.h"
#include "HAL/RunnableThread.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Serialization/ArrayWriter.h"
#include "Serialization/ArrayReader.h"
#include "IPAddress.h"
#include "Interfaces/IPv4/IPv4Address.h"


UUDPNetworkComponent::UUDPNetworkComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UUDPNetworkComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoConnect)
    {
        Connect();
    }
}

void UUDPNetworkComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Disconnect();
    Super::EndPlay(EndPlayReason);
}

void UUDPNetworkComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsConnected)
    {
        // 控制轮询频率
        TimeSinceLastPoll += DeltaTime;
        if (TimeSinceLastPoll >= ReceivePollRate)
        {
            ReceiveData();
            ProcessReceivedData();
            TimeSinceLastPoll = 0.0f;
        }

        // Update Heartbeat 
        TimeHeartbeat += DeltaTime;
        if (TimeHeartbeat >= HeartbeatRate)
        {
            UpdateHeartbeat();
            TimeHeartbeat = 0.f;
        }

        // Update Camera
        TimeUpdateCamera += DeltaTime;
        if (TimeUpdateCamera >= TranslateRate)
        {
            UpdateCameraPostion();
            TimeUpdateCamera = 0.f;
        }
    }
}

bool UUDPNetworkComponent::Connect()
{
    if (bIsConnected)
    {
        UE_LOG(LogTemp, Warning, TEXT("UDP Network: Already connected"));
        return true;
    }

    // 初始化Socket
    InitializeSockets();

    if (ListenSocket && SendSocket)
    {
        bIsConnected = true;
        ConnectionStatus = "Connected";
        OnUDPNetEvent.Broadcast(ConnectionStatus);

        SendString(TEXT("CONNECT"));

        UE_LOG(LogTemp, Log, TEXT("UDP Network: Connected successfully"));
        UE_LOG(LogTemp, Log, TEXT("  Listening on port: %d"), LocalPort);
        UE_LOG(LogTemp, Log, TEXT("  Sending to: %s:%d"), *RemoteIP, RemotePort);

        return true;
    }

    ConnectionStatus = "Connection Failed";
    OnUDPNetEvent.Broadcast(ConnectionStatus);
    return false;
}

void UUDPNetworkComponent::Disconnect()
{
    if (!bIsConnected)
    {
        return;
    }

    SendString(TEXT("DISCONNECT"));

    CleanupSockets();

    bIsConnected = false;
    ConnectionStatus = "Disconnected";
    OnUDPNetEvent.Broadcast(ConnectionStatus);

    UE_LOG(LogTemp, Log, TEXT("UDP Network: Disconnected"));
}

void UUDPNetworkComponent::InitializeSockets()
{
    // 创建监听Socket
    if (!CreateListenSocket())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create listen socket"));
        return;
    }

    // 创建发送Socket
    if (!CreateSendSocket())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create send socket"));
        CleanupSockets();
        return;
    }
}

bool UUDPNetworkComponent::CreateListenSocket()
{
   
    /////////////////////////////////////////////////////////////////////////////////////////   
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    bool bSuccess = false;
    // Now the listen address
    TSharedPtr<class FInternetAddr> ListenAddr = SocketSubsystem->CreateInternetAddr();
    bool bIsValid = true;
    //ListenAddr->SetIp(*RemoteIP, bIsValid);
    ListenAddr->SetAnyAddress();
    ListenAddr->SetPort(LocalPort);

    // Now create and set up our sockets (no VDP)
    ListenSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("UDP_Listen"), ListenAddr->GetProtocolType());
    if (ListenSocket != NULL)
    {

        ListenSocket->SetReuseAddr(true);
        ListenSocket->SetNonBlocking(true);
        ListenSocket->SetRecvErr();
        ListenSocket->SetBroadcast(true);

        // Bind to our listen port
        if (ListenSocket->Bind(*ListenAddr))
        {
            // Set it to broadcast mode, so we can send on it
            // NOTE: You must set this to broadcast mode on Xbox 360 or the
            // secure layer will eat any packets sent
            //bSuccess = ListenSocket->SetBroadcast();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to bind listen socket to addr (%s) for LAN beacon"), *ListenAddr->ToString(true));
            return false;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create UDP listen socket on port %d"), LocalPort);
        return false;
    }
    return true;


    /////////////////////////////////////////////////////////////////////////////////////////
    // 
    // 创建IP地址
    FIPv4Address Address;
    if (!FIPv4Address::Parse(/*TEXT("0.0.0.0")*/RemoteIP, Address))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse IP address 0.0.0.0"));
        return false;
    }

    //FIPv4Endpoint Endpoint(Address, LocalPort);
    FIPv4Endpoint Endpoint(FIPv4Address::Any, LocalPort);
    ListenSocket = FUdpSocketBuilder(TEXT("UDP Listen Socket"))
        .AsReusable()           // 允许重用地址
        .WithBroadcast()        // 允许广播
        //.BoundToEndpoint(Endpoint)
        .BoundToAddress(Endpoint.Address)     // 绑定到指定IP
        .BoundToPort(Endpoint.Port)      // 绑定到指定端口
        .WithReceiveBufferSize(2 * 1024 * 1024) // 2MB接收缓冲区
        .WithSendBufferSize(1 * 1024 * 1024)    // 1MB发送缓冲区
        .Build();

    if (!ListenSocket)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create UDP listen socket on port %d"), LocalPort);
        return false;
    }

    // 设置Socket选项
    //ListenSocket->SetReuseAddr(true);
    //ListenSocket->SetRecvErr();
    ListenSocket->SetNonBlocking(true);
    // 
    //int32 SendSize = 2 * 1024 * 1024;
    //ListenSocket->SetSendBufferSize(SendSize, SendSize);
    //ListenSocket->SetReceiveBufferSize(SendSize, SendSize);
    // 
     // 获取绑定的地址信息
    TSharedRef<FInternetAddr> LocalAddr = ISocketSubsystem::Get()->CreateInternetAddr();
    ListenSocket->GetAddress(*LocalAddr);

    UE_LOG(LogTemp, Log, TEXT("UDP Socket created: %s:%d"), *LocalAddr->ToString(false), LocalAddr->GetPort());

    //UE_LOG(LogTemp, Log, TEXT("UDP listen socket created on port %d"), LocalPort);
    return true;   
     
}

bool UUDPNetworkComponent::CreateSendSocket()
{
    SendSocket = FUdpSocketBuilder(TEXT("UDP Send Socket"))
        .AsNonBlocking()
        .Build();

    if (!SendSocket)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create UDP send socket"));
        return false;
    }

    return true;
}

void UUDPNetworkComponent::CleanupSockets()
{
    if (ListenSocket)
    {
        ListenSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
        ListenSocket = nullptr;
    }

    if (SendSocket)
    {
        SendSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(SendSocket);
        SendSocket = nullptr;
    }
}

bool UUDPNetworkComponent::SendString(const FString& Message)
{
    return SendTo(Message, RemoteIP, RemotePort);
}

bool UUDPNetworkComponent::SendBytes(const TArray<uint8>& Data)
{
    return SendDataInternal(Data, RemoteIP, RemotePort);
}

bool UUDPNetworkComponent::SendTo(const FString& Message, const FString& TargetIP, int32 TargetPort)
{
    // 转换字符串到字节数组
    TArray<uint8> Data;
    FTCHARToUTF8 Converter(*Message);
    Data.Append((uint8*)Converter.Get(), Converter.Length());

    return SendDataInternal(Data, TargetIP, TargetPort);
}

bool UUDPNetworkComponent::SendDataInternal(const TArray<uint8>& Data, const FString& IP, int32 Port)
{
    if (!SendSocket || !bIsConnected)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot send data: Socket not connected"));
        return false;
    }

    // 解析目标地址
    FIPv4Address IPv4Address;
    if (!FIPv4Address::Parse(IP, IPv4Address))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid IP address: %s"), *IP);
        return false;
    }

    TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

    Addr->SetIp(IPv4Address.Value);
    Addr->SetPort(Port);

    // 发送数据
    int32 BytesSent = 0;
    bool bSuccess = SendSocket->SendTo(Data.GetData(), Data.Num(), BytesSent, *Addr);

    if (bSuccess && BytesSent > 0)
    {
        // UE_LOG(LogTemp, Verbose, TEXT("Sent %d bytes to %s:%d"), BytesSent, *IP, Port);
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to send data to %s:%d"), *IP, Port);
        return false;
    }
}

void UUDPNetworkComponent::ReceiveData()
{
    if (!ListenSocket || !bIsConnected)
    {
        return;
    }

    // 准备接收缓冲区
    TArray<uint8> ReceivedData;
    ReceivedData.SetNumUninitialized(65507); // UDP最大包大小 

    TSharedRef<FInternetAddr> SenderAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    //ListenSocket->GetAddress(*SenderAddr);
    //UE_LOG(LogTemp, Log, TEXT("UDP Socket 0 created: %s:%d"), *SenderAddr->ToString(false), SenderAddr->GetPort());

    // 尝试接收数据
    uint32 Size;
    while (SendSocket->HasPendingData(Size))
    {
        int32 BytesRead = 0;

        if (SendSocket->RecvFrom(ReceivedData.GetData(), ReceivedData.Num(), BytesRead, *SenderAddr))
        {
            if (BytesRead > 0)
            {
                // 复制有效数据到新数组
                TArray<uint8> ValidData;
                ValidData.Append(ReceivedData.GetData(), BytesRead);

                // 获取发送者地址字符串
                FString SenderIP = SenderAddr->ToString(false); 
                int32 SenderPort = SenderAddr->GetPort();
                if (SenderIP.Equals(RemoteIP) && SenderPort == RemotePort)
                {
                    FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
                    LastReceiveTime = ThreadWaitTime.GetTotalSeconds();
                }

                // 将数据放入队列
                ReceivedDataQueue.Enqueue(ValidData);

                // 也同时处理为字符串（如果可读）
                FString ReceivedString = FString(BytesRead, (ANSICHAR*)ValidData.GetData());
                if (!ReceivedString.IsEmpty())
                {
                    ReceivedStringQueue.Enqueue(ReceivedString);
                }

                // 在游戏线程中处理数据
                AsyncTask(ENamedThreads::GameThread, [this, ValidData, SenderIP]() {
                    ParseReceivedPacket(ValidData, SenderIP);
                    });
            }
        }
    }
}

void UUDPNetworkComponent::ProcessReceivedData()
{
    // 处理字符串队列
    FString ReceivedString;
    while (ReceivedStringQueue.Dequeue(ReceivedString))
    {
        OnDataReceived.Broadcast(ReceivedString);
    }

    // 处理原始数据队列
    TArray<uint8> ReceivedData;
    while (ReceivedDataQueue.Dequeue(ReceivedData))
    {
        // 原始数据广播在ParseReceivedPacket中处理
    }
}

void UUDPNetworkComponent::ParseReceivedPacket(const TArray<uint8>& Data, const FString& FromAddress)
{
    // 广播原始数据
    OnRawDataReceived.Broadcast(Data, FromAddress);

    // 这里可以添加自定义协议解析逻辑
    // 例如：JSON解析、自定义二进制格式等
}

TArray<FString> UUDPNetworkComponent::GetAvailableAdapters() const
{
    TArray<FString> Adapters;

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (SocketSubsystem)
    {
        TArray<TSharedPtr<FInternetAddr>> LocalAddresses;
        SocketSubsystem->GetLocalAdapterAddresses(LocalAddresses);

        for (const auto& Addr : LocalAddresses)
        {
            Adapters.Add(Addr->ToString(false));
        }
    }

    return Adapters;
}

FString UUDPNetworkComponent::GetLocalIP() const
{
    bool bCanBindAll;
    TSharedPtr<FInternetAddr> LocalAddr =
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(
            *GLog, bCanBindAll);

    if (LocalAddr.IsValid())
    {
        return LocalAddr->ToString(false);
    }

    return TEXT("127.0.0.1");
}

void UUDPNetworkComponent::SetCurrentCamera(UCameraComponent* Camera)
{
    CurrentCamera = Camera;
}


void UUDPNetworkComponent::UpdateHeartbeat()
{
    SendString(TEXT("PING"));    

    FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
    float CurrentTime = ThreadWaitTime.GetTotalSeconds();
    if (CurrentTime - LastReceiveTime > 30.f)
    {
        // no heartbeat update
        UE_LOG(LogTemp, Error, TEXT("UDP Unconnect ....."));

    }
}

void UUDPNetworkComponent::UpdateCameraPostion()
{
    // Translate Vector to String.
    FString PosStr = CurrentCamera->GetComponentLocation().ToString();
       
    SendString(PosStr);
}