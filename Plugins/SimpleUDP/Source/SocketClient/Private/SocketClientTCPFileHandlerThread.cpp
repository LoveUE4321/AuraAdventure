// Copyright 2022 David Romanski (Socke). All Rights Reserved.

#include "SocketClientTCPFileHandlerThread.h"


FSocketClientTCPFileHandlerThread::FSocketClientTCPFileHandlerThread(USocketClientBPLibrary* socketClientP, FString clientConnectionIDP, FString ipOrDomainP, ESocketClientIPType ipTypeP, int32 portP, USocketClientTCPClient* tcpClientP) :
	socketClient(socketClientP),
	clientConnectionID(clientConnectionIDP),
	ipOrDomain(ipOrDomainP),
	ipType(ipTypeP),
	port(portP),
	tcpClient(tcpClientP) {
	FString threadName = "FServerFileConnectionThread" + FGuid::NewGuid().ToString();
	thread = FRunnableThread::Create(this, *threadName, 0, EThreadPriority::TPri_Normal);
}

FSocketClientTCPFileHandlerThread::~FSocketClientTCPFileHandlerThread() {
	delete thread;
}

uint32 FSocketClientTCPFileHandlerThread::Run() {
	//UE_LOG(LogTemp, Display, TEXT("DoWork:%s"),*(FDateTime::Now()).ToString());
	FString ip = socketClient->resolveDomain(ipOrDomain, ipType);
	int32 portGlobal = port;
	FString clientConnectionIDGlobal = clientConnectionID;
	USocketClientBPLibrary* socketClientGlobal = socketClient;
	USocketClientTCPClient* tcpClientGlobal = tcpClient;
	


	//UE_LOG(LogTemp, Warning, TEXT("Tread:%s:%i"),*ip, port);
	ISocketSubsystem* sSS = USocketClientBPLibrary::getSocketSubSystem();
	if (sSS == nullptr) {
		AsyncTask(ENamedThreads::GameThread, [ip, portGlobal, clientConnectionIDGlobal, socketClientGlobal, tcpClientGlobal]() {
			if (socketClientGlobal != nullptr)
			socketClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection failed(1). SocketSubSystem does not exist." + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
		if (tcpClientGlobal != nullptr)
			tcpClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection failed(1). SocketSubSystem does not exist." + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
			});
		return 0;
	}
	TSharedRef<FInternetAddr> addr = sSS->CreateInternetAddr();
	bool bIsValid;
	addr->SetIp(*ip, bIsValid);
	addr->SetPort(port);

	if (bIsValid) {
		// create the socket
		FSocket* socket = sSS->CreateSocket(NAME_Stream, TEXT("socketClient"), addr->GetProtocolType());
		tcpClient->setSocket(socket);


		// try to connect to the server
		if (socket == nullptr || socket->Connect(*addr) == false) {
			const TCHAR* socketErr = sSS->GetSocketError(SE_GET_LAST_ERROR_CODE);
			AsyncTask(ENamedThreads::GameThread, [ip, portGlobal, clientConnectionIDGlobal, socketClientGlobal, tcpClientGlobal, socketErr]() {
				if (socketClientGlobal != nullptr)
					socketClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection failed(2):" + FString(socketErr) + "|" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
				if (tcpClientGlobal != nullptr)
					tcpClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection failed(2):" + FString(socketErr) + "|" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
				});
		}
		else {
			//connected
			AsyncTask(ENamedThreads::GameThread, [ip, portGlobal, clientConnectionIDGlobal, tcpClientGlobal, socketClientGlobal]() {
				if (socketClientGlobal != nullptr)
					socketClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(true, "Connection successful:" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
				if (tcpClientGlobal != nullptr)
					tcpClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(true, "Connection successful:" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
				});


			tcpClient->setRun(true);

			if (tcpClient->isReceiveFile()) {
				doRequestFileFromServer(socket);
			}
			else {
				if (tcpClient->isSendFile()) {
					doSendFileToServer(socket);
				}
			}
		}


		AsyncTask(ENamedThreads::GameThread, [ip, portGlobal, clientConnectionIDGlobal, tcpClientGlobal, socketClientGlobal]() {
			if (socketClientGlobal != nullptr) {
				socketClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection close:" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
				if (tcpClientGlobal != nullptr)
					tcpClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection close:" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
			}
		});

		USocketClientBPLibrary::getSocketClientTarget()->closeSocketClientConnectionTCPNonStatic(clientConnectionID);



		tcpClient->setRun(false);
		if (socket != nullptr) {
			socket->Close();
		}
	}
	else {
		AsyncTask(ENamedThreads::GameThread, [ip, portGlobal, clientConnectionIDGlobal, tcpClientGlobal, socketClientGlobal]() {
			if (socketClientGlobal != nullptr)
			socketClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection failed(3). IP not valid:" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
		if (tcpClientGlobal != nullptr)
			tcpClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection failed(3). IP not valid:" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
			});
	}

	return 0;
}

void FSocketClientTCPFileHandlerThread::doRequestFileFromServer(FSocket* socket){
	FString filePath = tcpClient->getFilePath();

	if (!FPaths::DirectoryExists(filePath)) {
		triggerFileTransferOverTCPInfoEvent("Data transmission aborted. Directory not found.", clientConnectionID, filePath, false, socketClient, tcpClient);
		tcpClient->setRun(false);
		return;
	}


	FString message = "REQUEST_FILE_FROM_SERVER_|_" + tcpClient->getFileToken();
	sendMessageToServer(message, socket);

	message = readMessageFromServer(socket);

	if (message.StartsWith("REQUEST_FILE_FROM_SERVER_ACCEPTED_|_" + tcpClient->getFileToken())) {
		TArray<FString> lines;
		message.ParseIntoArray(lines, TEXT("_|_"), true);


		if (lines.Num() == 5) {
			if (lines[0].Equals("REQUEST_FILE_FROM_SERVER_ACCEPTED") && lines[1].Len() > 0 && lines[2].Len() > 0 && lines[3].Len() > 0 && lines[4].Len() > 0) {
				FArchive* writer = nullptr;
				FString md5Server = lines[2];
				int64 fileSize = FCString::Atoi64(*lines[3]);
				if (filePath.EndsWith("/")) {
					filePath += lines[4];
				}
				else {
					filePath += "/" + lines[4];
				}


				int64 fileSizeOnClient = 0;

				if (FPaths::FileExists(filePath)) {
					fileSizeOnClient = tcpClient->fileSize(filePath);
					if (tcpClient->hasResume()) {
						writer = IFileManager::Get().CreateFileWriter(*filePath, EFileWrite::FILEWRITE_Append);
					}
					else {
						writer = IFileManager::Get().CreateFileWriter(*filePath);
					}
				}
				else {
					writer = IFileManager::Get().CreateFileWriter(*filePath);
				}

				if (writer == nullptr) {
					triggerFileTransferOverTCPInfoEvent("File could not be created. ", clientConnectionID, filePath, false, socketClient, tcpClient);
					tcpClient->setRun(false);
					return;
				}



				if (writer->TotalSize() >= fileSize) {
					writer->Close();
					delete writer;
					triggerFileTransferOverTCPInfoEvent("File on the client has the same size or is larger than the file on the server.", clientConnectionID, filePath, false, socketClient, tcpClient);
					tcpClient->setRun(false);
					return;
				}


				//downloadFile = true;
				int64 bytesDownloaded = writer->TotalSize();

				message = "REQUEST_FILE_FROM_SERVER_ACCEPTED_|_" + tcpClient->getFileToken() + "_|_" + tcpClient->int64ToString(bytesDownloaded);
				sendMessageToServer(message, socket);

				int64 lastTimeWithData = 0;
				int64 ticks1 = 0;
				int64 ticks2 = 0;
				int64 ticksDownload = 0;
				int64 lastByte = 0;
				uint32 dataSize = 0;
				TArray<uint8> buffer;
				while (socket != nullptr && tcpClient->isRun()) {

					ticks1 = FDateTime::Now().GetTicks();
					socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(waitForRead));
					ticks2 = FDateTime::Now().GetTicks();

					bool hasData = socket->HasPendingData(dataSize);
					if (!hasData && ticks1 == ticks2) {
						triggerFileTransferOverTCPInfoEvent("Connection broken.", clientConnectionID, filePath, false, socketClient, tcpClient);
						tcpClient->setRun(false);
						break;
					}

				//	UE_LOG(LogTemp, Display, TEXT("download %i"),dataSize);

					if (hasData) {
						lastTimeWithData = FDateTime::Now().GetTicks();

						//downlnoad
						int32 bytesRead = 0;
						buffer.Empty();
						buffer.SetNumUninitialized(dataSize);
						if (socket->Recv(buffer.GetData(), buffer.Num(), bytesRead)) {
							writer->Serialize(buffer.GetData(), buffer.Num());

							//show progress each second
							if ((ticksDownload + 10000000) <= FDateTime::Now().GetTicks()) {
								writer->Flush();
								int64 bytesTransferredLastSecond = bytesDownloaded - lastByte;
								//float speed = ((float)bytesTransferredLastSecond) / 125000;
								float mbit = ((float)bytesTransferredLastSecond) / 1024 / 1024 * 8;
								float sent = ((float)bytesDownloaded) / 1048576;
								float left = 0;
								float percent = 0;
								if (fileSize > 0) {
									left = ((float)(fileSize - bytesDownloaded)) / 1048576;
									percent = ((float)bytesDownloaded / (float)fileSize * 100);
								}


								triggerTransferFileEvent(clientConnectionID, filePath, socketClient, tcpClient, percent, mbit, bytesDownloaded, fileSize);

								ticksDownload = FDateTime::Now().GetTicks();
								lastByte = bytesDownloaded;
							}
							bytesDownloaded += bytesRead;
						}
						if (bytesDownloaded >= fileSize) {

							if (writer != nullptr) {
								writer->Close();
								delete writer;
								writer = nullptr;
							}
											
							sendEndMessage(filePath, tcpClient->getFileToken(), md5Server, clientConnectionID, socket, socketClient, tcpClient);

							//transfer file finish
							triggerTransferFileEvent(clientConnectionID, filePath, socketClient, tcpClient, 100, 0, bytesDownloaded, fileSize);
							tcpClient->setRun(false);
						}
						
					}
					//no data time out
					else {
						//one second = 10000000 ticks
						//5 seconds timeout
					//	UE_LOG(LogTemp, Display, TEXT("timeout %i"),(FDateTime::Now().GetTicks() - lastTimeWithData));
						if ((FDateTime::Now().GetTicks() - lastTimeWithData) >= 50000000) {
							triggerFileTransferOverTCPInfoEvent("Connection timeout", clientConnectionID, filePath, false, socketClient, tcpClient);
							tcpClient->setRun(false);
							break;
						}
					}
				}
				buffer.Empty();
				if (writer != nullptr) {
					writer->Close();
					delete writer;
					writer = nullptr;
				}
			}
		}
	}
	
}

void FSocketClientTCPFileHandlerThread::doSendFileToServer(FSocket* socket){
	FString filePath = tcpClient->getFilePath();

	if (!FPaths::FileExists(filePath)) {
		triggerFileTransferOverTCPInfoEvent("Data transmission aborted. File not found.", clientConnectionID, filePath, false, socketClient, tcpClient);
		tcpClient->setRun(false);
		return;
	}
	
	
	int64 fileSize = tcpClient->fileSize(filePath);
	bool md5okay = false;
	FString md5 = FString();
	tcpClient->getMD5FromFileAbsolutePath(filePath, md5okay, md5);
	FString fileAuthMessage = "SEND_FILE_TO_SERVER_|_" + tcpClient->getFileToken() + "_|_" + md5 + "_|_" + FPaths::GetCleanFilename(filePath) + "_|_" + tcpClient->int64ToString(fileSize);
	sendMessageToServer(fileAuthMessage, socket);

	FString message = readMessageFromServer(socket);


	if (message.StartsWith("SEND_FILE_TO_SERVER_ACCEPTED_|_" + tcpClient->getFileToken())) {
		if (message.RemoveFromStart("SEND_FILE_TO_SERVER_ACCEPTED_|_" + tcpClient->getFileToken() + "_|_")) {
			int64 startPosition = FCString::Atoi64(*message);
			
			// upload file
	
			FArchive* reader = IFileManager::Get().CreateFileReader(*filePath);
			if (reader == nullptr || reader->TotalSize() == 0) {
				if (reader != nullptr) {
					reader->Close();
				}
				tcpClient->setRun(false);
				return;
			}


			fileSize = reader->TotalSize();
			/*int64 ticks1 = 0;
			int64 ticks2 = 0;*/
			int64 readSize = 0;
			int64 lastPosition = startPosition;
			int64 bytesSentSinceLastTick = 0;

			int32 bufferSize = 1024 * 64;
			int32 dataSendBySocket = 0;

			float percent = 0.f;
			float mbit = 0.f;

			TArray<uint8> buffer;

			int64 lastTimeTicks = FDateTime::Now().GetTicks();

			if (bufferSize > fileSize) {
				bufferSize = fileSize;
			}

			if (lastPosition > 0) {
				reader->Seek(lastPosition);
			}

			while (tcpClient->isRun() && lastPosition < fileSize) {
				if ((lastPosition + bufferSize) > fileSize) {
					bufferSize = fileSize - lastPosition;
				}

				//buffer.Reset(bufferSize);
				buffer.Empty();
				buffer.AddUninitialized(bufferSize);

				reader->Serialize(buffer.GetData(), buffer.Num());
				lastPosition += buffer.Num();

				//UE_LOG(LogTemp, Warning, TEXT("Send: %i"),buffer.Num());

				//ticks1 = FDateTime::Now().GetTicks();
				socket->Send(buffer.GetData(), buffer.Num(), dataSendBySocket);
				//socket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::FromSeconds(0.001));
				//ticks2 = FDateTime::Now().GetTicks();

				if (dataSendBySocket == 0 || dataSendBySocket != buffer.Num()) {
					triggerFileTransferOverTCPInfoEvent("Data transmission aborted. Connection possibly broken.", clientConnectionID, filePath, false, socketClient, tcpClient);
					tcpClient->setRun(false);
					return;
				}

				//slowdown for tests
				//FPlatformProcess::Sleep(0.01f);

				//fire event every second
				//one second = 10000000 ticks
				if (((FDateTime::Now().GetTicks()) - lastTimeTicks) >= 10000000) {
					mbit = ((float)lastPosition - (float)bytesSentSinceLastTick) / 1024 / 1024 * 8;
					lastTimeTicks = FDateTime::Now().GetTicks();
					bytesSentSinceLastTick = lastPosition;

					percent = ((float)lastPosition / fileSize) * 100;
					triggerTransferFileEvent(clientConnectionID, filePath, socketClient, tcpClient, percent, mbit, lastPosition, fileSize);
				}

			}

			mbit = ((float)lastPosition - (float)bytesSentSinceLastTick) / 1024 / 1024 * 8;
			percent = ((float)lastPosition / fileSize) * 100;

			triggerTransferFileEvent(clientConnectionID, filePath, socketClient, tcpClient, percent, mbit, lastPosition, fileSize);

			buffer.Empty();
			if (reader != nullptr) {
				reader->Close();
				reader = nullptr;
			}
					

			message = readMessageFromServer(socket);

			if (message.StartsWith("SEND_FILE_TO_SERVER_END_|_" + tcpClient->getFileToken())) {
				if (message.RemoveFromStart("SEND_FILE_TO_SERVER_END_|_" + tcpClient->getFileToken() + "_|_")) {
					if (message.Equals("OKAY")) {
						triggerFileTransferOverTCPInfoEvent("Data transmission successful. ", clientConnectionID, filePath, true, socketClient, tcpClient);
						tcpClient->setRun(false);
					}
					else {
						triggerFileTransferOverTCPInfoEvent("There was an error during data transmission. File may be corrupted or incomplete. ", clientConnectionID, filePath, false, socketClient, tcpClient);
						tcpClient->setRun(false);
					}
				}
				else {
					triggerFileTransferOverTCPInfoEvent("Data transmission aborted. Wrong answer from the server (3). ", clientConnectionID, filePath, false, socketClient, tcpClient);
					tcpClient->setRun(false);
				}
			}
		}
		else {
			triggerFileTransferOverTCPInfoEvent("Data transmission aborted. Wrong answer from the server (2). ", clientConnectionID, filePath, false, socketClient, tcpClient);
			tcpClient->setRun(false);
			return;
		}
	}
	else {
		if (message.StartsWith("SEND_FILE_TO_SERVER_END_|_" + tcpClient->getFileToken())) {
			if (message.RemoveFromStart("SEND_FILE_TO_SERVER_END_|_" + tcpClient->getFileToken() + "_|_")) {
				if (message.Equals("OKAY")) {
					triggerFileTransferOverTCPInfoEvent("Data transmission successful. ", clientConnectionID, filePath, true, socketClient, tcpClient);
					tcpClient->setRun(false);
				}
				else {
					triggerFileTransferOverTCPInfoEvent("There was an error during data transmission. File may be corrupted or incomplete. ", clientConnectionID, filePath, false, socketClient, tcpClient);
					tcpClient->setRun(false);
				}
			}
			else {
				triggerFileTransferOverTCPInfoEvent("Data transmission aborted. Wrong answer from the server (3). ", clientConnectionID, filePath, false, socketClient, tcpClient);
				tcpClient->setRun(false);
			}
		}
		else {
			triggerFileTransferOverTCPInfoEvent("Data transmission aborted. Wrong answer from the server (1). ", clientConnectionID, filePath, false, socketClient, tcpClient);
			tcpClient->setRun(false);
		}
	}
	
}

void FSocketClientTCPFileHandlerThread::sendMessageToServer(FString message, FSocket* socket){
	message = tcpClient->encryptMessage(message);

	TArray<uint8> byteCache;
	FTCHARToUTF8 Convert(*message);
	int32 sent = 0;
	if (FGenericPlatformProperties::IsLittleEndian()) {
		byteCache.Add(0x00);
	}
	else {
		byteCache.Add(0x01);
	}
	byteCache.SetNum(5);
	int32 dataLength = Convert.Length();
	FMemory::Memcpy(byteCache.GetData() + 1, &dataLength, 4);
	byteCache.Append((uint8*)Convert.Get(), Convert.Length());

	socket->Send(byteCache.GetData(), byteCache.Num(), sent);
	byteCache.Empty();
}

void FSocketClientTCPFileHandlerThread::triggerFileTransferOverTCPInfoEvent(FString messageP, FString clientConnectionIDP, FString filePathP, bool successP, USocketClientBPLibrary* socketClientP, USocketClientTCPClient* tcpClientP) {
	AsyncTask(ENamedThreads::GameThread, [messageP, clientConnectionIDP, filePathP, successP, socketClientP, tcpClientP]() {
		if (socketClientP != nullptr)
		socketClientP->onfileTransferOverTCPInfoEventDelegate.Broadcast(messageP, clientConnectionIDP, filePathP, successP);
	if (tcpClientP != nullptr)
		tcpClientP->onfileTransferOverTCPInfoEventDelegate.Broadcast(messageP, clientConnectionIDP, filePathP, successP);
		});
}

void FSocketClientTCPFileHandlerThread::triggerTransferFileEvent(FString clientConnectionIDP, FString filePathP, USocketClientBPLibrary* socketClientP, USocketClientTCPClient* tcpClientP, float percentP, float mbitP, int64 transferredP, int64 fileSizeP) {
	AsyncTask(ENamedThreads::GameThread, [clientConnectionIDP, socketClientP, filePathP, tcpClientP, percentP, mbitP, transferredP, fileSizeP]() {
		if (socketClientP != nullptr)
		socketClientP->ontransferFileOverTCPProgressEventDelegate.Broadcast(clientConnectionIDP, filePathP, percentP, mbitP, transferredP, fileSizeP);
	if (tcpClientP != nullptr)
		tcpClientP->ontransferFileOverTCPProgressEventDelegate.Broadcast(clientConnectionIDP, filePathP, percentP, mbitP, transferredP, fileSizeP);
		});
}

void FSocketClientTCPFileHandlerThread::sendEndMessage(FString fullFilePathP, FString tokenP, FString md5ServerP, FString clientConnectionIDP, FSocket* clientSocketP, USocketClientBPLibrary* socketClientP, USocketClientTCPClient* tcpClientP) {
	bool md5okay = false;
	FString md5Client = FString();
	tcpClientP->getMD5FromFileAbsolutePath(fullFilePathP, md5okay, md5Client);

	FString response = "REQUEST_FILE_FROM_SERVER_END_|_" + tokenP + "_|_";

	if (md5okay && md5ServerP.Equals(md5Client)) {
		triggerFileTransferOverTCPInfoEvent("File successfully received.", clientConnectionIDP, fullFilePathP, true, socketClientP, tcpClientP);

		response += "OKAY";
	}
	else {
		triggerFileTransferOverTCPInfoEvent("File received but MD5 does not match. Corrupted file will be deleted if resume is not disabled.", clientConnectionIDP, fullFilePathP, false, socketClientP, tcpClientP);
		response += "MD5ERROR";
		if (tcpClientP->hasResume()) {
			tcpClientP->deleteFile(fullFilePathP);
		}
	}

	sendMessageToServer(response, clientSocketP);
}


FString FSocketClientTCPFileHandlerThread::readMessageFromServer(FSocket* socket){
	int64 ticks1 = 0;
	int64 ticks2 = 0;
	int32 lastDataLengthFromHeader = 0;
	FString message = FString();
	TArray<uint8> byteDataArrayCache;
	TArray<uint8> byteDataArray;

	uint32 dataSize = 0;
	TArray<FString> lines;
	while (tcpClient->isRun() && socket != nullptr) {

		ticks1 = FDateTime::Now().GetTicks();
		socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(waitForRead));
		ticks2 = FDateTime::Now().GetTicks();

		bool hasData = socket->HasPendingData(dataSize);

		if (!hasData && ticks1 == ticks2) {
			//if (showFTPLogs) {
			//	UE_LOG(LogTemp, Warning, TEXT("FTP Client: TCP connection broken."));
			//}
			//fireConnectionEvent(false, 0, "TCP connection broken.");
			return "TCP connection broken.";
		}

		if (!hasData) {
			return message;
		}


		if (hasData) {
			TArray<uint8> dataFromSocket;
			dataFromSocket.SetNumUninitialized(dataSize);
			int32 BytesRead = 0;
			if (socket->Recv(dataFromSocket.GetData(), dataFromSocket.Num(), BytesRead)) {
				if (lastDataLengthFromHeader == 0 && dataFromSocket.Num() >= 5) {

					tcpClient->readDataLength(dataFromSocket, lastDataLengthFromHeader);

					if (dataFromSocket.Num() == 5) {
						dataFromSocket.Empty();
						continue;
					}

					byteDataArrayCache.Append(dataFromSocket.GetData() + 5, dataFromSocket.Num() - 5);
					dataFromSocket.Empty();
				}
				else {
					byteDataArrayCache.Append(dataFromSocket.GetData(), dataFromSocket.Num());
				}

				int32 maxLoops = 1000;//to prevent endless loop

				while (byteDataArrayCache.Num() > 0 && byteDataArrayCache.Num() >= lastDataLengthFromHeader && maxLoops > 0) {
					maxLoops--;

					byteDataArray.Append(byteDataArrayCache.GetData(), lastDataLengthFromHeader);
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 5
					byteDataArrayCache.RemoveAt(0, lastDataLengthFromHeader, EAllowShrinking::Yes);
#else
					byteDataArrayCache.RemoveAt(0, lastDataLengthFromHeader, true);
#endif

					if (byteDataArrayCache.Num() == 0) {
						lastDataLengthFromHeader = 0;
						break;
					}

					if (byteDataArrayCache.Num() > 5) {
						tcpClient->readDataLength(byteDataArrayCache, lastDataLengthFromHeader);
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 5
						byteDataArrayCache.RemoveAt(0, 5, EAllowShrinking::Yes);
#else
						byteDataArrayCache.RemoveAt(0, 5, true);
#endif
					}
				}

				byteDataArray.Add(0x00);// null-terminator
				message = FString(UTF8_TO_TCHAR((char*)byteDataArray.GetData()));
				byteDataArray.Empty();
				if (message.IsEmpty() == false) {
					message = tcpClient->decryptMessage(message);
				}
				return message;
			}
		}
	}
	if (message.IsEmpty() == false) {
		message = tcpClient->decryptMessage(message);
	}
	return message;
}