// UDPNetworkComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketBuilder.h"
#include "Camera/CameraComponent.h"
//#include "UDPNetworkComponent.generated.h"


// 网络数据接收委托
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDataReceived, const FString&, Data);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRawDataReceived, const TArray<uint8>&, Data, const FString&, FromAddress);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUDPNetEvent, const FString&, Data);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnClientStateUpdate, const FClientInfo&, Client, bool, IsAdd);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHostCreateRoom, const FString&, RmName);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClientFindRoom, const FString&, RmName);

//UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
//class UDPCLIENT_API UUDPNetworkComponent : public UActorComponent
//{
//    GENERATED_BODY()
//
//public:
//    UUDPNetworkComponent();
//
//    // 网络事件委托
//    UPROPERTY(BlueprintAssignable, Category = "UDP Events")
//    FOnDataReceived OnDataReceived;
//
//    UPROPERTY(BlueprintAssignable, Category = "UDP Events")
//    FOnRawDataReceived OnRawDataReceived;
//
//    UPROPERTY(BlueprintAssignable, Category = "UDP Events")
//    FOnUDPNetEvent OnUDPNetEvent;
//
//    UPROPERTY(BlueprintAssignable, Category = "UDP Events")
//    FOnClientStateUpdate OnClientStateUpdate;
//
//    UPROPERTY(BlueprintAssignable, Category = "UDP Events")
//    FOnHostCreateRoom OnHostCreateRoom;
//
//    UPROPERTY(BlueprintAssignable, Category = "UDP Events")
//    FOnClientFindRoom OnClientFindRoom;
//
//protected:
//    virtual void BeginPlay() override;
//    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
//    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
//        FActorComponentTickFunction* ThisTickFunction) override;
//
//public:
//    // 配置参数
//    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP Settings")
//    FString RemoteIP = "127.0.0.1";
//
//    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP Settings")
//    int32 RemotePort = 8000;
//
//    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP Settings")
//    int32 LocalPort = 8001;
//
//    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP Settings")
//    bool bAutoConnect = true;
//
//    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP Settings")
//    float ReceivePollRate = 0.01f; // 接收轮询频率(秒)
//
//    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP Settings")
//    float HeartbeatRate = 5.0f; // 接收心跳频率(秒)
//
//    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP Settings")
//    float TranslateRate = 1.0f; // update translate(秒)
//
//    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP Settings")
//    FString ClientName = "Player";
//
//    // 状态
//    UPROPERTY(BlueprintReadOnly, Category = "UDP Status")
//    bool bIsConnected = false;
//
//    UPROPERTY(BlueprintReadOnly, Category = "UDP Status")
//    FString ConnectionStatus = "Disconnected";
//
//    // 核心网络功能
//    UFUNCTION(BlueprintCallable, Category = "UDP Network")
//    bool Connect(int32 DevNum=-1, FString DevSN="", FString Progress = "", EUDPGameState ClientGS = EUDPGameState::GS_Idle);
//
//    UFUNCTION(BlueprintCallable, Category = "UDP Network")
//    void Disconnect();
//
//    UFUNCTION(BlueprintCallable, Category = "UDP Network")
//    bool SendString(EUDPMsgType MsgType, const FString& Message="");
//
//    UFUNCTION(BlueprintCallable, Category = "UDP Network")
//    bool SendBytes(const TArray<uint8>& Data);
//
//    UFUNCTION(BlueprintCallable, Category = "UDP Network")
//    bool SendTo(const FString& Message, const FString& TargetIP, int32 TargetPort);
//
//    UFUNCTION(BlueprintCallable, Category = "UDP Network")
//    TArray<FString> GetAvailableAdapters() const;
//
//    UFUNCTION(BlueprintCallable, Category = "UDP Network")
//    FString GetLocalIP() const;
//
//    UFUNCTION(BlueprintCallable, Category = "UDP Network")
//    void SetCurrentCamera(UCameraComponent* Camera);
//
//    UFUNCTION(BlueprintPure, Category = "UDP Network")
//    TArray<FClientInfo> GetUDPClient() const;
//
//    UFUNCTION(BlueprintCallable, Category = "UDP Network")
//    void CreateRoomDone(FString RoomName);
//
//private:
//
//    // 内部方法
//    void InitializeSockets();
//    void CleanupSockets();
//    void ReceiveData();
//    void ProcessReceivedData();
//    bool CreateListenSocket();
//    bool CreateSendSocket();
//
//    void SetClientInfo(TMap<int32, FString> DevInfo, FString Progress, EUDPGameState ClientGS);
//
//    // 线程安全发送
//    bool SendDataInternal(const TArray<uint8>& Data, const FString& IP, int32 Port);
//    bool SendTo(TSharedPtr<FJsonObject> JsonObj, const FString& TargetIP, int32 TargetPort);
//    bool SendJson(TSharedPtr<FJsonObject> JsonObj);
//
//    // 解析接收到的数据
//    void ParseReceivedPacket(const TArray<uint8>& Data, const FString& MsgStr, const FString& FromAddress);
//
//    void UpdateHeartbeat();
//    void UpdateCameraPostion();
//    void HandleRecvMsg(const FString& Msg);
//
//    TSharedPtr<FJsonObject> CreateJsonObject(EUDPMsgType Type);
//    TSharedPtr<FJsonObject> CreateJsonObject(EUDPMsgType Type, FVector Location);
//    TSharedPtr<FJsonObject> CreateJsonObject(EUDPMsgType Type, TSharedPtr<FJsonObject>  JsonObj);
//    FString JsonObjectToString(const TSharedPtr<FJsonObject>& JsonObject, bool bPrettyPrint = false);
//    TSharedPtr<FJsonObject> ParseJsonString(const FString& JsonString);
//
//private:
//    // Socket相关
//    FSocket* ListenSocket = nullptr;
//    FSocket* SendSocket = nullptr;
//
//    // 线程安全队列
//    TQueue<TArray<uint8>, EQueueMode::Mpsc> ReceivedDataQueue;
//    TQueue<FString, EQueueMode::Mpsc> ReceivedStringQueue;
//
//    // 时间控制
//    float TimeSinceLastPoll = 0.0f;
//    float TimeHeartbeat = 0.0f;
//    float LastReceiveTime = 0.0f;
//    float TimeUpdateCamera = 0.f;
//
//    UCameraComponent* CurrentCamera;
//    TArray<FClientInfo> UDPClients;
//    const FString MsgKey;
//
//    // client upd info
//    int32 m_DevNum;
//    EUDPGameState m_ClientGS;
//    FString m_DevSN;
//    FString m_ClientPro;
//};