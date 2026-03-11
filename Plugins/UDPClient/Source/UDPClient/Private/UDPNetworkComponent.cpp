// UDPNetworkComponent.cpp
#include "UDPNetworkComponent.h"
#include "Async/Async.h"
#include "HAL/RunnableThread.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Serialization/ArrayWriter.h"
#include "Serialization/ArrayReader.h"
#include "IPAddress.h"
#include "Interfaces/IPv4/IPv4Address.h"

#ifdef UDPNETCMP


UUDPNetworkComponent::UUDPNetworkComponent()
    :MsgKey(TEXT("LOC:"))
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    UDPClients.Empty();
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
    if (bAutoConnect)
    {
        Disconnect();
    }
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

bool UUDPNetworkComponent::Connect(int32 DevNum, FString DevSN, FString Progress, EUDPGameState ClientGS)
{
    if (bIsConnected)
    {
        UE_LOG(LogTemp, Warning, TEXT("UDP Network: Already connected"));
        return true;
    }
    UDPClients.Empty();

    // 初始化Socket
    InitializeSockets();

    if (ListenSocket && SendSocket)
    {
        bIsConnected = true;
        ConnectionStatus = "Connected";
        OnUDPNetEvent.Broadcast(ConnectionStatus);
               
        TSharedPtr<FJsonObject> InfoObj = MakeShareable(new FJsonObject);
        InfoObj->SetNumberField("Num", DevNum);
        InfoObj->SetNumberField("State", ClientGS);
        InfoObj->SetStringField("SN", DevSN);
        InfoObj->SetStringField("Progress", Progress);

        auto JsonObj = CreateJsonObject(EUDPMsgType::Connect, InfoObj);
        SendJson(JsonObj);

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

    auto JsonObj = CreateJsonObject(EUDPMsgType::DisConnect);
    SendJson(JsonObj);

    CleanupSockets();

    bIsConnected = false;
    ConnectionStatus = "Disconnected";
    OnUDPNetEvent.Broadcast(ConnectionStatus);

    UDPClients.Empty();

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

bool UUDPNetworkComponent::SendString(EUDPMsgType MsgType, const FString& Message)
{
    return SendTo(Message, RemoteIP, RemotePort);
}

bool UUDPNetworkComponent::SendBytes(const TArray<uint8>& Data)
{
    return SendDataInternal(Data, RemoteIP, RemotePort);
}

bool UUDPNetworkComponent::SendJson(TSharedPtr<FJsonObject> JsonObj)
{
    return SendTo(JsonObj, RemoteIP, RemotePort);
}

bool UUDPNetworkComponent::SendTo(const FString& Message, const FString& TargetIP, int32 TargetPort)
{
    // 转换字符串到字节数组
    TArray<uint8> Data;
    FTCHARToUTF8 Converter(*Message);
    Data.Append((uint8*)Converter.Get(), Converter.Length());

    return SendDataInternal(Data, TargetIP, TargetPort);
}

bool UUDPNetworkComponent::SendTo(TSharedPtr<FJsonObject> JsonObj, const FString& TargetIP, int32 TargetPort)
{
    if (JsonObj == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot send Json Object: Obj is null."));
        return false;
    }

    FString StrMsg = JsonObjectToString(JsonObj);

    // 转换字符串到字节数组
    TArray<uint8> Data;
    FTCHARToUTF8 Converter(*StrMsg);
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
                AsyncTask(ENamedThreads::GameThread, [this, ValidData, ReceivedString,SenderIP]() {
                    ParseReceivedPacket(ValidData, ReceivedString, SenderIP);
                    });
            }
        }
    }
}

void UUDPNetworkComponent::ProcessReceivedData()
{
    //// 处理字符串队列
    //FString ReceivedString;
    //while (ReceivedStringQueue.Dequeue(ReceivedString))
    //{
    //    OnDataReceived.Broadcast(ReceivedString);
    //}

    //// 处理原始数据队列
    //TArray<uint8> ReceivedData;
    //while (ReceivedDataQueue.Dequeue(ReceivedData))
    //{
    //    // 原始数据广播在ParseReceivedPacket中处理
    //}
}

void UUDPNetworkComponent::ParseReceivedPacket(const TArray<uint8>& Data, const FString& MsgStr, const FString& FromAddress)
{
    // 广播原始数据
    //OnRawDataReceived.Broadcast(Data, FromAddress);

    // 这里可以添加自定义协议解析逻辑
    // 例如：JSON解析、自定义二进制格式等
    HandleRecvMsg(MsgStr);
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

TArray<FClientInfo> UUDPNetworkComponent::GetUDPClient() const
{
    return UDPClients;
}

void UUDPNetworkComponent::CreateRoomDone(FString RoomName)
{
    TSharedPtr<FJsonObject> JsonCreate = MakeShareable(new FJsonObject);
    JsonCreate->SetNumberField("Num", m_DevNum);  
    JsonCreate->SetNumberField("State", (EUDPGameState)EUDPGameState::GS_Create);
    JsonCreate->SetStringField("SN", m_DevSN);
    JsonCreate->SetStringField("Room", RoomName);

    auto JsonObj = CreateJsonObject(EUDPMsgType::StatusUpdate, JsonCreate);
    SendJson(JsonObj);   
}

void UUDPNetworkComponent::SetClientInfo(TMap<int32, FString> DevInfo, FString Progress, EUDPGameState ClientGS)
{
    for (auto info : DevInfo)
    {
        m_DevNum = info.Key;
        m_DevSN = info.Value;
    }

    m_ClientPro = Progress;
    m_ClientGS = ClientGS;
}

//
void UUDPNetworkComponent::UpdateHeartbeat()
{
    auto JsonObj = CreateJsonObject(EUDPMsgType::Heartbeat);
    SendJson(JsonObj);

    FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
    float CurrentTime = ThreadWaitTime.GetTotalSeconds();
    if (CurrentTime - LastReceiveTime > 30.f)
    {
        // no heartbeat update
        UE_LOG(LogTemp, Error, TEXT("UDP Unconnect ....."));

        // ???
        auto JsonDis = CreateJsonObject(EUDPMsgType::DisConnect);
        SendJson(JsonDis);
    }
}

void UUDPNetworkComponent::UpdateCameraPostion()
{
    if (CurrentCamera == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Current Camera is Null ....."));
        return;
    }

    auto JsonObj = CreateJsonObject(EUDPMsgType::Location, CurrentCamera->GetComponentLocation());       
    SendTo(JsonObj, RemoteIP, RemotePort);
}

void UUDPNetworkComponent::HandleRecvMsg(const FString& Msg)
{
    auto JsonMsg = ParseJsonString(Msg);
    if (JsonMsg == nullptr)
    {
        return;
    }

    EUDPMsgType Type = (EUDPMsgType)JsonMsg->GetIntegerField(TEXT("type"));
    switch (Type)
    {
    case EUDPMsgType::Ack:
        break;
    case EUDPMsgType::Join:
    {
        FString StrName = JsonMsg->GetStringField(TEXT("sender"));

        /*for (auto Value : UDPClients)
        {
            if (Value.ClientName.Equals(StrName))
                return;
        }*/

        FClientInfo Info;
        Info.ClientName = StrName;
        Info.Location = FVector::Zero();
        UDPClients.Add(Info);

        OnClientStateUpdate.Broadcast(Info, true);
    }        
        break;
    case EUDPMsgType::Location:
    {
        TSharedPtr<FJsonObject> DataObj = JsonMsg->GetObjectField(TEXT("data"));
        if (DataObj == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to get Json Data."));
            return;
        }

        FVector TempLoc(DataObj->GetNumberField(TEXT("x")),
            DataObj->GetNumberField(TEXT("y")),
            DataObj->GetNumberField(TEXT("z")));
        FString StrName = JsonMsg->GetStringField(TEXT("sender"));

        //UE_LOG(LogTemp, Error, TEXT("Json Str: %s"), *TempLoc.ToString());

        for (auto &Value : UDPClients)
        {
            if (Value.ClientName.Equals(StrName))
            {
                Value.Location = TempLoc;
                break;
            }
        }

        //OnClientStateUpdate.Broadcast(UDPClients);
    }        
        break;
    case EUDPMsgType::DisConnect:
    {
        FString StrName = JsonMsg->GetStringField(TEXT("sender"));

        int32 Index = -1;
        for (auto& Value : UDPClients)
        {
            Index++;
            if (Value.ClientName.Equals(StrName) && Index != -1)
            {
                OnClientStateUpdate.Broadcast(Value, false);

                UDPClients.RemoveAt(Index);
                break;
            }
        }
    }
        break;
    case EUDPMsgType::StatusUpdate:
    {
        TSharedPtr<FJsonObject> DataObj = JsonMsg->GetObjectField(TEXT("data"));
        if (DataObj == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to get Json Data."));
            return;
        }
        m_DevNum = DataObj->GetNumberField(TEXT("Num"));
        m_ClientGS = (EUDPGameState)DataObj->GetNumberField(TEXT("State"));
        m_DevSN = DataObj->GetStringField(TEXT("SN"));
        //m_ClientPro = DataObj->GetStringField(TEXT("Progress"));
        FString RoomStr = DataObj->GetStringField(TEXT("Room"));

        switch (m_ClientGS)
        {
        case EUDPGameState::GS_Create:
            OnHostCreateRoom.Broadcast(RoomStr);
            break;
        case EUDPGameState::GS_Join:
            OnClientFindRoom.Broadcast(RoomStr);
            break;
        }
    }
        break;
    default:
        break;
    }
}

TSharedPtr<FJsonObject> UUDPNetworkComponent::CreateJsonObject(EUDPMsgType Type)
{
    // 创建根对象
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);

    // 添加基本类型
    RootObject->SetNumberField("type", Type);
    RootObject->SetStringField("sender", ClientName);
    RootObject->SetStringField("receiver", RemoteIP);
    RootObject->SetStringField("id", "27315");

    int64 Timestamp = FDateTime::UtcNow().ToUnixTimestamp();
    RootObject->SetNumberField("timestamp", Timestamp);

    // 添加嵌套对象
    //TSharedPtr<FJsonObject> PositionObject = MakeShareable(new FJsonObject);
    //RootObject->SetObjectField("data", PositionObject);

    return RootObject;
}

TSharedPtr<FJsonObject> UUDPNetworkComponent::CreateJsonObject(EUDPMsgType Type, FVector Location)
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);

    RootObject->SetNumberField("type", Type);
    RootObject->SetStringField("sender", ClientName);
    RootObject->SetStringField("receiver", RemoteIP);
    RootObject->SetStringField("id", "23715");

    int64 Timestamp = FDateTime::UtcNow().ToUnixTimestamp();
    RootObject->SetNumberField("timestamp", Timestamp);

    // 添加嵌套对象
    TSharedPtr<FJsonObject> PositionObject = MakeShareable(new FJsonObject);
    PositionObject->SetNumberField("x", Location.X);
    PositionObject->SetNumberField("y", Location.Y);
    PositionObject->SetNumberField("z", Location.Z);
    RootObject->SetObjectField("data", PositionObject);

    return RootObject;
}

TSharedPtr<FJsonObject> UUDPNetworkComponent::CreateJsonObject(EUDPMsgType Type, TSharedPtr<FJsonObject>  JsonObj)
{
    // 创建根对象
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);

    // 添加基本类型
    RootObject->SetNumberField("type", Type);
    RootObject->SetStringField("sender", ClientName);
    RootObject->SetStringField("receiver", RemoteIP);
    RootObject->SetStringField("id", "27315");

    int64 Timestamp = FDateTime::UtcNow().ToUnixTimestamp();
    RootObject->SetNumberField("timestamp", Timestamp);

    // 添加嵌套对象
    RootObject->SetObjectField("data", JsonObj);

    return RootObject;
}

FString UUDPNetworkComponent::JsonObjectToString(const TSharedPtr<FJsonObject>& JsonObject, bool bPrettyPrint)
{
    FString OutputString;

    // 使用 JsonWriter 序列化
    TSharedRef<TJsonWriter<TCHAR>> JsonWriter = bPrettyPrint ?
        TJsonWriterFactory<TCHAR>::Create(&OutputString, 0) :  // 紧凑格式
        TJsonWriterFactory<TCHAR>::Create(&OutputString);      // 格式化（缩进）

    // 写入对象
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    return OutputString;
}

TSharedPtr<FJsonObject> UUDPNetworkComponent::ParseJsonString(const FString& JsonString)
{
    TSharedPtr<FJsonObject> JsonObject;

    // 创建 JsonReader
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonString);

    // 反序列化
    if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
    {
        return JsonObject;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON: %s"), *JsonReader->GetErrorMessage());
        return nullptr;
    }
}

#endif