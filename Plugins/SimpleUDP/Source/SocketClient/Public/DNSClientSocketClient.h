// Copyright 2017-2019 David Romanski (Socke). All Rights Reserved.

#pragma once

#include "SocketClient.h"
#include "DNSClientSocketClient.generated.h"

UCLASS()
class SOCKETCLIENT_API UDNSClientSocketClient : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	void resolveDomain(ISocketSubsystem* socketSubSystem, FString domain, bool useDNSCache = true, FString dnsIP = FString("8.8.8.8"));

	void UDPReceiver(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);

	FSocket* socket = nullptr;

	bool isResloving();
	FString getIP();

private:
	bool resolving;
	FString ip;
	FString domain;
	TMap<FString, FString> dnsCache;
};