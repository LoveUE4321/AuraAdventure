#include "UDPNetworkLibrary.h"

UUDPNetworkLibrary* UUDPNetworkLibrary::m_UdpNetworkLibrary;

UUDPNetworkLibrary::UUDPNetworkLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) 
{
	m_UdpNetworkLibrary = this;
    UDPClients.Empty();
}

UUDPNetworkLibrary::~UUDPNetworkLibrary()
{

}

UUDPNetworkLibrary* UUDPNetworkLibrary::GetUdpNetworkLibrary()
{
    return m_UdpNetworkLibrary;
}

// FTickableGameObject 接口
void UUDPNetworkLibrary::Tick(float DeltaTime)
{
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

bool UUDPNetworkLibrary::IsTickable() const
{
    // 控制是否启用Tick
    return  true;//!IsTemplate(RF_ClassDefaultObject);  // 排除CDO
}

bool UUDPNetworkLibrary::IsTickableInEditor() const
{
    return true;
}

TStatId UUDPNetworkLibrary::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UUDPNetworkLibrary, STATGROUP_Tickables);
}

void UUDPNetworkLibrary::BeginDestroy()
{
    // 重要：取消注册，避免野指针
    // FTickableGameObject 会自动处理，但显式清理更安全
    Super::BeginDestroy();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

bool UUDPNetworkLibrary::ConnectServer(FString Ip, int32 RomtePort, int32 LocalPort, int32 DevNum, FString DevSN, FString Progress, EUDPGameState ClientGS)
{
    if (UUDPNetworkLibrary::GetUdpNetworkLibrary()->bIsConnected)
    {
        UE_LOG(LogTemp, Warning, TEXT("UDP Network: Already connected"));
        return true;
    }

    UUDPNetworkLibrary::GetUdpNetworkLibrary()->RemoteIP = Ip;
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->RemotePort = RomtePort;
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->LocalPort = LocalPort;

    UUDPNetworkLibrary::GetUdpNetworkLibrary()->UDPClients.Empty();

    // 初始化Socket
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->InitializeSockets();

    if (UUDPNetworkLibrary::GetUdpNetworkLibrary()->ListenSocket && UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendSocket)
    {
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->bIsConnected = true;
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->ConnectionStatus = "Connected";
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->OnUDPNetEvent.Broadcast(UUDPNetworkLibrary::GetUdpNetworkLibrary()->ConnectionStatus);

        TSharedPtr<FJsonObject> InfoObj = MakeShareable(new FJsonObject);
        InfoObj->SetNumberField("Num", DevNum);
        InfoObj->SetNumberField("State", ClientGS);
        InfoObj->SetStringField("SN", DevSN);
        InfoObj->SetStringField("Progress", Progress);

        auto JsonObj = UUDPNetworkLibrary::GetUdpNetworkLibrary()->CreateJsonObject(EUDPMsgType::Connect, InfoObj);
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendJson(JsonObj);

        UE_LOG(LogTemp, Log, TEXT("UDP Network: Connected successfully"));
        UE_LOG(LogTemp, Log, TEXT("  Listening on port: %d"), UUDPNetworkLibrary::GetUdpNetworkLibrary()->LocalPort);
        UE_LOG(LogTemp, Log, TEXT("  Sending to: %s:%d"), *UUDPNetworkLibrary::GetUdpNetworkLibrary()->RemoteIP, UUDPNetworkLibrary::GetUdpNetworkLibrary()->RemotePort);

        return true;
    }

    UUDPNetworkLibrary::GetUdpNetworkLibrary()->ConnectionStatus = "Connection Failed";
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->OnUDPNetEvent.Broadcast(UUDPNetworkLibrary::GetUdpNetworkLibrary()->ConnectionStatus);

    return true;
}

void UUDPNetworkLibrary::Disconnect()
{
    if (!UUDPNetworkLibrary::GetUdpNetworkLibrary()->bIsConnected)
    {
        return;
    }

    TSharedPtr<FJsonObject> JsonDis = MakeShareable(new FJsonObject);
    JsonDis->SetNumberField("Num", UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_DevNum);
    JsonDis->SetNumberField("State", (EUDPGameState)EUDPGameState::GS_Logout);
    JsonDis->SetStringField("SN", UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_DevSN);
    JsonDis->SetStringField("Progress", UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_ClientPro);
    JsonDis->SetStringField("Room", UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_RoomName);

    auto JsonObj = UUDPNetworkLibrary::GetUdpNetworkLibrary()->CreateJsonObject(EUDPMsgType::DisConnect, JsonDis);
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendJson(JsonObj);

    UUDPNetworkLibrary::GetUdpNetworkLibrary()->CleanupSockets();

    UUDPNetworkLibrary::GetUdpNetworkLibrary()->bIsConnected = false;
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->ConnectionStatus = "Disconnected";
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->OnUDPNetEvent.Broadcast(UUDPNetworkLibrary::GetUdpNetworkLibrary()->ConnectionStatus);

    UUDPNetworkLibrary::GetUdpNetworkLibrary()->UDPClients.Empty();

    UE_LOG(LogTemp, Log, TEXT("UDP Network: Disconnected"));
}

bool UUDPNetworkLibrary::SendString(EUDPMsgType MsgType, const FString& Message)
{
    return UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendTo(Message, 
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->RemoteIP,
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->RemotePort);
}

bool UUDPNetworkLibrary::SendBytes(const TArray<uint8>& Data)
{
    return UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendDataInternal(Data, 
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->RemoteIP, 
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->RemotePort);
}

bool UUDPNetworkLibrary::SendTo(const FString& Message, const FString& TargetIP, int32 TargetPort)
{
    // 转换字符串到字节数组
    TArray<uint8> Data;
    FTCHARToUTF8 Converter(*Message);
    Data.Append((uint8*)Converter.Get(), Converter.Length());

    return UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendDataInternal(Data, TargetIP, TargetPort);
}

//TArray<FString> UUDPNetworkLibrary::GetAvailableAdapters() const
//{
//}

FString UUDPNetworkLibrary::GetLocalIP() 
{
    bool bCanBindAll;
    TSharedPtr<FInternetAddr> LocalAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);

    if (LocalAddr.IsValid())
    {
        return LocalAddr->ToString(false);
    }

    return TEXT("127.0.0.1");
}

void UUDPNetworkLibrary::SetCurrentCamera(UCameraComponent* Camera)
{
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->CurrentCamera = Camera;
}

TArray<FClientInfo> UUDPNetworkLibrary::GetUDPClient() 
{
	return UUDPNetworkLibrary::GetUdpNetworkLibrary()->UDPClients;
}

void UUDPNetworkLibrary::CreateRoomDone(FString RoomName, FString Progress)
{
    TSharedPtr<FJsonObject> JsonCreate = MakeShareable(new FJsonObject);
    JsonCreate->SetNumberField("Num", UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_DevNum);
    JsonCreate->SetNumberField("State", (EUDPGameState)EUDPGameState::GS_Create);
    JsonCreate->SetStringField("SN", UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_DevSN);
    JsonCreate->SetStringField("Progress", Progress);
    JsonCreate->SetStringField("Room", RoomName);

    auto JsonObj = UUDPNetworkLibrary::GetUdpNetworkLibrary()->CreateJsonObject(EUDPMsgType::StatusUpdate, JsonCreate);
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendJson(JsonObj);
}

void UUDPNetworkLibrary::JoinRoomDone(FString RoomName, FString Progress)
{
    TSharedPtr<FJsonObject> JsonJoin = MakeShareable(new FJsonObject);
    JsonJoin->SetNumberField("Num", UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_DevNum);
    JsonJoin->SetNumberField("State", (EUDPGameState)EUDPGameState::GS_Join);
    JsonJoin->SetStringField("SN", UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_DevSN);
    JsonJoin->SetStringField("Progress", Progress);
    JsonJoin->SetStringField("Room", RoomName);

    auto JsonObj = UUDPNetworkLibrary::GetUdpNetworkLibrary()->CreateJsonObject(EUDPMsgType::StatusUpdate, JsonJoin);
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendJson(JsonObj);
}


//////////////////////////////////////////////////////////////////////////////////////////
void UUDPNetworkLibrary::InitializeSockets()
{
    // 创建监听Socket
    if (!UUDPNetworkLibrary::GetUdpNetworkLibrary()->CreateListenSocket())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create listen socket"));
        return;
    }

    // 创建发送Socket
    if (!UUDPNetworkLibrary::GetUdpNetworkLibrary()->CreateSendSocket())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create send socket"));
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->CleanupSockets();
        return;
    }
}

void UUDPNetworkLibrary::CleanupSockets()
{
    if (UUDPNetworkLibrary::GetUdpNetworkLibrary()->ListenSocket)
    {
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->ListenSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(UUDPNetworkLibrary::GetUdpNetworkLibrary()->ListenSocket);
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->ListenSocket = nullptr;
    }

    if (UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendSocket)
    {
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendSocket);
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendSocket = nullptr;
    }
}

void UUDPNetworkLibrary::ReceiveData()
{
    if (!UUDPNetworkLibrary::GetUdpNetworkLibrary()->ListenSocket || !UUDPNetworkLibrary::GetUdpNetworkLibrary()->bIsConnected)
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
    while (UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendSocket->HasPendingData(Size))
    {
        int32 BytesRead = 0;

        if (UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendSocket->RecvFrom(ReceivedData.GetData(), ReceivedData.Num(), BytesRead, *SenderAddr))
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
                    UUDPNetworkLibrary::GetUdpNetworkLibrary()->LastReceiveTime = ThreadWaitTime.GetTotalSeconds();
                }

                // 将数据放入队列
                UUDPNetworkLibrary::GetUdpNetworkLibrary()->ReceivedDataQueue.Enqueue(ValidData);

                // 也同时处理为字符串（如果可读）
                FString ReceivedString = FString(BytesRead, (ANSICHAR*)ValidData.GetData());
                if (!ReceivedString.IsEmpty())
                {
                    UUDPNetworkLibrary::GetUdpNetworkLibrary()->ReceivedStringQueue.Enqueue(ReceivedString);
                }

                // 在游戏线程中处理数据
                AsyncTask(ENamedThreads::GameThread, [this, ValidData, ReceivedString, SenderIP]() {
                    ParseReceivedPacket(ValidData, ReceivedString, SenderIP);
                    });
            }
        }
    }
}

void UUDPNetworkLibrary::ProcessReceivedData()
{

}

bool UUDPNetworkLibrary::CreateListenSocket()
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
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->ListenSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("UDP_Listen"), ListenAddr->GetProtocolType());
    if (UUDPNetworkLibrary::GetUdpNetworkLibrary()->ListenSocket != NULL)
    {

       UUDPNetworkLibrary::GetUdpNetworkLibrary()->ListenSocket->SetReuseAddr(true);
       UUDPNetworkLibrary::GetUdpNetworkLibrary()->ListenSocket->SetNonBlocking(true);
       UUDPNetworkLibrary::GetUdpNetworkLibrary()->ListenSocket->SetRecvErr();
       UUDPNetworkLibrary::GetUdpNetworkLibrary()->ListenSocket->SetBroadcast(true);

        // Bind to our listen port
        if (UUDPNetworkLibrary::GetUdpNetworkLibrary()->ListenSocket->Bind(*ListenAddr))
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
        UE_LOG(LogTemp, Error, TEXT("Failed to create UDP listen socket on port %d"), UUDPNetworkLibrary::GetUdpNetworkLibrary()->LocalPort);
        return false;
    }
    return true;
}

bool UUDPNetworkLibrary::CreateSendSocket()
{
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendSocket = FUdpSocketBuilder(TEXT("UDP Send Socket"))
        .AsNonBlocking()
        .Build();

    if (!UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendSocket)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create UDP send socket"));
        return false;
    }

    return true;
}

void UUDPNetworkLibrary::SetClientInfo(TMap<int32, FString> DevInfo, FString Progress, EUDPGameState ClientGS)
{
    for (auto info : DevInfo)
    {
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_DevNum = info.Key;
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_DevSN = info.Value;
    }

    UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_ClientPro = Progress;
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_ClientGS = ClientGS;
}

// 线程安全发送
bool UUDPNetworkLibrary::SendDataInternal(const TArray<uint8>& Data, const FString& IP, int32 Port)
{
    if (!UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendSocket || !UUDPNetworkLibrary::GetUdpNetworkLibrary()->bIsConnected)
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
    bool bSuccess = UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendSocket->SendTo(Data.GetData(), Data.Num(), BytesSent, *Addr);

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

bool UUDPNetworkLibrary::SendTo(TSharedPtr<FJsonObject> JsonObj, const FString& TargetIP, int32 TargetPort)
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

    return UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendDataInternal(Data, TargetIP, TargetPort);
}

bool UUDPNetworkLibrary::SendJson(TSharedPtr<FJsonObject> JsonObj)
{
    return UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendTo(JsonObj, RemoteIP, RemotePort);
}

// 解析接收到的数据
void UUDPNetworkLibrary::ParseReceivedPacket(const TArray<uint8>& Data, const FString& MsgStr, const FString& FromAddress)
{
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->HandleRecvMsg(MsgStr);
}

void UUDPNetworkLibrary::UpdateHeartbeat()
{
    auto JsonObj = UUDPNetworkLibrary::GetUdpNetworkLibrary()->CreateJsonObject(EUDPMsgType::Heartbeat);
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendJson(JsonObj);

    FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
    float CurrentTime = ThreadWaitTime.GetTotalSeconds();
    if (CurrentTime - LastReceiveTime > 30.f)
    {
        // no heartbeat update
        UE_LOG(LogTemp, Error, TEXT("UDP Unconnect ....."));

        // ???
        auto JsonDis = UUDPNetworkLibrary::GetUdpNetworkLibrary()->CreateJsonObject(EUDPMsgType::DisConnect);
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendJson(JsonDis);
    }
}

void UUDPNetworkLibrary::UpdateCameraPostion()
{
    if (CurrentCamera == nullptr)
    {
        //UE_LOG(LogTemp, Error, TEXT("Current Camera is Null ....."));
        return;
    }

    auto JsonObj = UUDPNetworkLibrary::GetUdpNetworkLibrary()->CreateJsonObject(EUDPMsgType::Location, CurrentCamera->GetComponentLocation());
    UUDPNetworkLibrary::GetUdpNetworkLibrary()->SendTo(JsonObj, RemoteIP, RemotePort);
}

void UUDPNetworkLibrary::HandleRecvMsg(const FString& Msg)
{
    auto JsonMsg = UUDPNetworkLibrary::GetUdpNetworkLibrary()->ParseJsonString(Msg);
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
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->UDPClients.Add(Info);

        UUDPNetworkLibrary::GetUdpNetworkLibrary()->OnClientStateUpdate.Broadcast(Info, true);
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

        for (auto& Value : UUDPNetworkLibrary::GetUdpNetworkLibrary()->UDPClients)
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
        for (auto& Value : UUDPNetworkLibrary::GetUdpNetworkLibrary()->UDPClients)
        {
            Index++;
            if (Value.ClientName.Equals(StrName) && Index != -1)
            {
                UUDPNetworkLibrary::GetUdpNetworkLibrary()->OnClientStateUpdate.Broadcast(Value, false);

                UUDPNetworkLibrary::GetUdpNetworkLibrary()->UDPClients.RemoveAt(Index);
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
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_DevNum = DataObj->GetNumberField(TEXT("Num"));
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_ClientGS = (EUDPGameState)DataObj->GetNumberField(TEXT("State"));
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_DevSN = DataObj->GetStringField(TEXT("SN"));
        //m_ClientPro = DataObj->GetStringField(TEXT("Progress"));
        FString RoomStr = DataObj->GetStringField(TEXT("Room"));
        UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_RoomName = RoomStr;

        switch (UUDPNetworkLibrary::GetUdpNetworkLibrary()->m_ClientGS)
        {
        case EUDPGameState::GS_Create:
            UUDPNetworkLibrary::GetUdpNetworkLibrary()->OnHostCreateRoom.Broadcast(RoomStr);
            break;
        case EUDPGameState::GS_Join:
            UUDPNetworkLibrary::GetUdpNetworkLibrary()->OnClientFindRoom.Broadcast(RoomStr);
            break;
        }
    }
    break;
    default:
        break;
    }
}

TSharedPtr<FJsonObject> UUDPNetworkLibrary::CreateJsonObject(EUDPMsgType Type)
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

TSharedPtr<FJsonObject> UUDPNetworkLibrary::CreateJsonObject(EUDPMsgType Type, FVector Location)
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

TSharedPtr<FJsonObject> UUDPNetworkLibrary::CreateJsonObject(EUDPMsgType Type, TSharedPtr<FJsonObject>  JsonObj)
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
FString UUDPNetworkLibrary::JsonObjectToString(const TSharedPtr<FJsonObject>& JsonObject, bool bPrettyPrint)
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

TSharedPtr<FJsonObject> UUDPNetworkLibrary::ParseJsonString(const FString& JsonString)
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