#pragma once
#include "Interfaces/IPv4/IPv4Address.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketBuilder.h"
#include "Camera/CameraComponent.h"
#include "UDPNetworkLibrary.generated.h"

USTRUCT(BlueprintType)
struct FClientInfo
{
    GENERATED_BODY()
    FClientInfo() {}

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ClientName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Location;
};

UENUM(BlueprintType)
enum EUDPMsgType
{
    Connect = 0,
    Ack,
    Join,
    Heartbeat,
    Location,
    StatusUpdate,
    DisConnect,
    Custom = 100
};

UENUM(BlueprintType)
enum EUDPGameState
{
    GS_Idle = 0,
    GS_Create,
    GS_Join,
    GS_Playing,
    GS_Logout,
};


UCLASS()
class UDPCLIENT_API UUDPNetworkLibrary : public UObject, public FTickableGameObject
{
	GENERATED_UCLASS_BODY()

public:
    // FTickableGameObject 接口
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableInEditor() const override;
    virtual TStatId GetStatId() const override;

    virtual void BeginDestroy() override;

public:
	~UUDPNetworkLibrary();

    // 网络数据接收委托
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDataReceived, const FString&, Data);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRawDataReceived, const TArray<uint8>&, Data, const FString&, FromAddress);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUDPNetEvent, const FString&, Data);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnClientStateUpdate, const FClientInfo&, Client, bool, IsAdd);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHostCreateRoom, const FString&, RmName);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClientFindRoom, const FString&, RmName);

    UPROPERTY(BlueprintAssignable, Category = "UDP Events")
    FOnDataReceived OnDataReceived;

    UPROPERTY(BlueprintAssignable, Category = "UDP Events")
    FOnRawDataReceived OnRawDataReceived;

    UPROPERTY(BlueprintAssignable, Category = "UDP Events")
    FOnUDPNetEvent OnUDPNetEvent;

    UPROPERTY(BlueprintAssignable, Category = "UDP Events")
    FOnClientStateUpdate OnClientStateUpdate;

    UPROPERTY(BlueprintAssignable, Category = "UDP Events")
    FOnHostCreateRoom OnHostCreateRoom;

    UPROPERTY(BlueprintAssignable, Category = "UDP Events")
    FOnClientFindRoom OnClientFindRoom;

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient")
    static UUDPNetworkLibrary* GetUdpNetworkLibrary();

	UFUNCTION(BlueprintCallable, Category = "UdpNetwork")
	static bool ConnectServer(FString Ip="127.0.0.1", int32 RomtePort =8899, int32 LocalPort=8890, int32 DevNum = -1, FString DevSN = "", FString Progress = "", EUDPGameState ClientGS = EUDPGameState::GS_Idle);

    UFUNCTION(BlueprintCallable, Category = "UdpNetwork")
    static void Disconnect();

    UFUNCTION(BlueprintCallable, Category = "UdpNetwork")
    static bool SendString(EUDPMsgType MsgType, const FString& Message = "");

    UFUNCTION(BlueprintCallable, Category = "UdpNetwork")
    static bool SendBytes(const TArray<uint8>& Data);

    //UFUNCTION(BlueprintCallable, Category = "UdpNetwork")
    //static TArray<FString> GetAvailableAdapters() const;

    UFUNCTION(BlueprintCallable, Category = "UdpNetwork")
    static FString GetLocalIP() ;

    UFUNCTION(BlueprintCallable, Category = "UdpNetwork")
    static void SetCurrentCamera(UCameraComponent* Camera);

    UFUNCTION(BlueprintPure, Category = "UdpNetwork")
    static TArray<FClientInfo> GetUDPClient() ;

    UFUNCTION(BlueprintCallable, Category = "UdpNetwork")
    static void CreateRoomDone(FString RoomName, FString Progress);

    UFUNCTION(BlueprintCallable, Category = "UdpNetwork")
    static void JoinRoomDone(FString RoomName, FString Progress);

private:
    // 内部方法
    void InitializeSockets();
    void CleanupSockets();
    void ReceiveData();
    void ProcessReceivedData();
    bool CreateListenSocket();
    bool CreateSendSocket();

    void SetClientInfo(TMap<int32, FString> DevInfo, FString Progress, EUDPGameState ClientGS);

    // 线程安全发送
    bool SendDataInternal(const TArray<uint8>& Data, const FString& IP, int32 Port);
    bool SendTo(TSharedPtr<FJsonObject> JsonObj, const FString& TargetIP, int32 TargetPort);
    bool SendTo(const FString& Message, const FString& TargetIP, int32 TargetPort);
    bool SendJson(TSharedPtr<FJsonObject> JsonObj);

    // 解析接收到的数据
    void ParseReceivedPacket(const TArray<uint8>& Data, const FString& MsgStr, const FString& FromAddress);

    void UpdateHeartbeat();
    void UpdateCameraPostion();
    void HandleRecvMsg(const FString& Msg);

    TSharedPtr<FJsonObject> CreateJsonObject(EUDPMsgType Type);
    TSharedPtr<FJsonObject> CreateJsonObject(EUDPMsgType Type, FVector Location);
    TSharedPtr<FJsonObject> CreateJsonObject(EUDPMsgType Type, TSharedPtr<FJsonObject>  JsonObj);
    FString JsonObjectToString(const TSharedPtr<FJsonObject>& JsonObject, bool bPrettyPrint = false);
    TSharedPtr<FJsonObject> ParseJsonString(const FString& JsonString);

private:
    static UUDPNetworkLibrary* m_UdpNetworkLibrary;

private:

    // 配置参数
    FString RemoteIP = "127.0.0.1";
    int32 RemotePort = 8000;
    int32 LocalPort = 8001;
    bool bAutoConnect = true;
    float ReceivePollRate = 0.01f; // 接收轮询频率(秒)
    float HeartbeatRate = 5.0f; // 接收心跳频率(秒)
    float TranslateRate = 1.0f; // update translate(秒)
    FString ClientName = "Player";
    bool bIsConnected = false;
    FString ConnectionStatus = "Disconnected";


    // Socket相关
    FSocket* ListenSocket = nullptr;
    FSocket* SendSocket = nullptr;

    // 线程安全队列
    TQueue<TArray<uint8>, EQueueMode::Mpsc> ReceivedDataQueue;
    TQueue<FString, EQueueMode::Mpsc> ReceivedStringQueue;

    // 时间控制
    float TimeSinceLastPoll = 0.0f;
    float TimeHeartbeat = 0.0f;
    float LastReceiveTime = 0.0f;
    float TimeUpdateCamera = 0.f;

    UCameraComponent* CurrentCamera;
    TArray<FClientInfo> UDPClients;
    const FString MsgKey;

    // client upd info
    int32 m_DevNum;
    EUDPGameState m_ClientGS;
    FString m_DevSN;
    FString m_ClientPro;
    FString m_RoomName;
};