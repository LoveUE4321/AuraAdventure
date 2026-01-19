// Copyright 2017-2019 David Romanski (Socke). All Rights Reserved.

#include "SocketClientFileFunctions.h"

UFileFunctionsSocketClient* UFileFunctionsSocketClient::fileFunctionsSocketClient;

UFileFunctionsSocketClient::UFileFunctionsSocketClient(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	fileFunctionsSocketClient = this;
}

UFileFunctionsSocketClient* UFileFunctionsSocketClient::getFileFunctionsSocketClientTarget() {
	return fileFunctionsSocketClient;
}

FString UFileFunctionsSocketClient::getCleanDirectory(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath) {
	if (directoryType == EFileFunctionsSocketClientDirectoryType::E_ad) {
		return FPaths::ConvertRelativePathToFull(filePath);
	}
	else {
		FString ProjectDir = FPaths::ProjectDir();
		return FPaths::ConvertRelativePathToFull(ProjectDir + filePath);
	}
}

void UFileFunctionsSocketClient::writeBytesToFile(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, TArray<uint8> bytes, bool& success) {
	success = FFileHelper::SaveArrayToFile(bytes, *getCleanDirectory(directoryType, filePath));
}

void UFileFunctionsSocketClient::addBytesToFileAndCloseIt(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, TArray<uint8> bytes, bool& success) {
	FArchive* writer = IFileManager::Get().CreateFileWriter(*getCleanDirectory(directoryType, filePath), EFileWrite::FILEWRITE_Append);
	if (!writer) {
		success = false;
		return;
	}
	writer->Seek(writer->TotalSize());
	writer->Serialize(bytes.GetData(), bytes.Num());
	writer->Close();
	delete writer;

	success = true;
}

//void UFileFunctionsSocketClient::splittFile(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, int32 parts, bool& success){
//	if (parts <= 0)
//		parts = 1;
//	FArchive* reader = IFileManager::Get().CreateFileReader(*getCleanDirectory(directoryType, filePath));
//	if (!reader) {
//		success = false;
//		return;
//	}
//	
//	int64 splittAfterBytes = reader->TotalSize()/ ((int64)parts);
//	TArray<uint8> bytes;
//
//	for (int32 i = 0; i < parts; i++){
//		bytes.AddUninitialized(splittAfterBytes);
//		reader->Serialize(bytes.GetData(), splittAfterBytes);
//		if (FFileHelper::SaveArrayToFile(bytes, *getCleanDirectory(directoryType, filePath)) == false) {
//			success = false;
//			return;
//		}
//		splittAfterBytes =
//		reader->Seek();
//	}
//
//}

TArray<uint8> UFileFunctionsSocketClient::readBytesFromFile(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, bool& success) {
	TArray<uint8> result;
	success = FFileHelper::LoadFileToArray(result, *getCleanDirectory(directoryType, filePath));
	return result;
}

void UFileFunctionsSocketClient::readStringFromFile(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, bool& success, FString& data) {
	data.Empty();
	success = FFileHelper::LoadFileToString(data, *getCleanDirectory(directoryType, filePath));
}

void UFileFunctionsSocketClient::writeStringToFile(EFileFunctionsSocketClientDirectoryType directoryType, FString data, FString filePath, EFileFunctionsSocketClientEncodingOptions fileEncoding, bool& success) {
	success = FFileHelper::SaveStringToFile(data, *getCleanDirectory(directoryType, filePath), (FFileHelper::EEncodingOptions)fileEncoding);
}



void UFileFunctionsSocketClient::getMD5FromFile(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, bool& success, FString& MD5) {
	getMD5FromFileAbsolutePath(getCleanDirectory(directoryType, filePath), success, MD5);
}

void UFileFunctionsSocketClient::getMD5FromFileAbsolutePath(FString filePath, bool& success, FString& MD5) {
	MD5.Empty();
	FArchive* reader = IFileManager::Get().CreateFileReader(*filePath);
	if (!reader) {
		success = false;
		return;
	}

	TArray<uint8> byteArrayTmp;
	int64 totalSize = reader->TotalSize();
	int64 loadedBytes = 0;
	int64 leftUploadBytes = 1024;


	if (totalSize < leftUploadBytes)
		leftUploadBytes = totalSize;


	uint8 Digest[16];
	FMD5 Md5Gen;

	while ((loadedBytes + leftUploadBytes) <= totalSize) {
		byteArrayTmp.Reset(leftUploadBytes);
		byteArrayTmp.AddUninitialized(leftUploadBytes);
		reader->Serialize(byteArrayTmp.GetData(), byteArrayTmp.Num());
		loadedBytes += leftUploadBytes;
		reader->Seek(loadedBytes);

		Md5Gen.Update(byteArrayTmp.GetData(), byteArrayTmp.Num());
	}

	leftUploadBytes = totalSize - loadedBytes;
	if (leftUploadBytes > 0) {
		byteArrayTmp.Reset(leftUploadBytes);
		byteArrayTmp.AddUninitialized(leftUploadBytes);
		reader->Serialize(byteArrayTmp.GetData(), byteArrayTmp.Num());
		loadedBytes += leftUploadBytes;
		Md5Gen.Update(byteArrayTmp.GetData(), byteArrayTmp.Num());
	}

	if (reader != nullptr) {
		reader->Close();
		delete reader;
	}

	if (totalSize != loadedBytes) {
		success = false;
		return;
	}

	Md5Gen.Final(Digest);
	for (int32 i = 0; i < 16; i++) {
		MD5 += FString::Printf(TEXT("%02x"), Digest[i]);
	}

	success = true;
}

void UFileFunctionsSocketClient::stringToBase64String(FString string, FString& base64String) {
	base64String.Empty();
	FTCHARToUTF8 Convert(*string);
	TArray<uint8> bytes;
	bytes.Append(((uint8*)((ANSICHAR*)Convert.Get())), Convert.Length());
	base64String = FBase64::Encode(bytes);
}

void UFileFunctionsSocketClient::base64StringToString(FString& string, FString base64String) {
	string.Empty();
	TArray<uint8> bytes;
	if (FBase64::Decode(*base64String, bytes)) {
		bytes.Add(0x00);// null-terminator
		char* Data = (char*)bytes.GetData();
		string = FString(UTF8_TO_TCHAR(Data));
	}
}


void UFileFunctionsSocketClient::bytesToBase64String(TArray<uint8> bytes, FString& base64String) {
	base64String.Empty();
	base64String = FBase64::Encode(bytes);
}

TArray<uint8> UFileFunctionsSocketClient::base64StringToBytes(FString base64String, bool& success) {
	TArray<uint8> fileData;
	if (base64String.Len() % 2 != 0 || base64String.Len() < 4) {
		success = false;
		return fileData;
	}
	success = FBase64::Decode(*base64String, fileData);
	return fileData;
}

void UFileFunctionsSocketClient::fileToBase64String(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, bool& success, FString& base64String, FString& fileName) {
	base64String.Empty();
	fileName.Empty();
	TArray<uint8> fileData;
	if (!FFileHelper::LoadFileToArray(fileData, *getCleanDirectory(directoryType, filePath))) {
		success = false;
		return;
	}
	base64String = FBase64::Encode(fileData);
	success = true;
}

bool UFileFunctionsSocketClient::fileExists(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath) {
	return FPaths::FileExists(*getCleanDirectory(directoryType, filePath));
}

bool UFileFunctionsSocketClient::fileExistsAbsolutePath(FString filePath) {
	return FPaths::FileExists(*filePath);
}

bool UFileFunctionsSocketClient::directoryExists(EFileFunctionsSocketClientDirectoryType directoryType, FString path) {
	return FPaths::DirectoryExists(*getCleanDirectory(directoryType, path));
}

int64 UFileFunctionsSocketClient::fileSize(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath) {
	return FPlatformFileManager::Get().GetPlatformFile().FileSize(*getCleanDirectory(directoryType, filePath));
}

int64 UFileFunctionsSocketClient::fileSizeAbsolutePath(FString filePath) {
	return FPlatformFileManager::Get().GetPlatformFile().FileSize(*filePath);
}

bool UFileFunctionsSocketClient::deleteFile(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath) {
	return FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*getCleanDirectory(directoryType, filePath));
}

bool UFileFunctionsSocketClient::deleteFileAbsolutePath(FString filePath) {
	return FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*filePath);
}


bool UFileFunctionsSocketClient::deleteDirectory(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath) {
	return FPlatformFileManager::Get().GetPlatformFile().DeleteDirectory(*getCleanDirectory(directoryType, filePath));
}

bool UFileFunctionsSocketClient::isReadOnly(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath) {
	return FPlatformFileManager::Get().GetPlatformFile().IsReadOnly(*getCleanDirectory(directoryType, filePath));
}

bool UFileFunctionsSocketClient::moveFile(EFileFunctionsSocketClientDirectoryType directoryTypeTo, FString filePathTo, EFileFunctionsSocketClientDirectoryType directoryTypeFrom, FString filePathFrom) {
	return FPlatformFileManager::Get().GetPlatformFile().MoveFile(*getCleanDirectory(directoryTypeTo, filePathTo), *getCleanDirectory(directoryTypeFrom, filePathFrom));
}

bool UFileFunctionsSocketClient::setReadOnly(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, bool bNewReadOnlyValue) {
	return FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*getCleanDirectory(directoryType, filePath), bNewReadOnlyValue);
}

FDateTime UFileFunctionsSocketClient::getTimeStamp(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath) {
	return FPlatformFileManager::Get().GetPlatformFile().GetTimeStamp(*getCleanDirectory(directoryType, filePath));
}

void UFileFunctionsSocketClient::setTimeStamp(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, FDateTime DateTime) {
	FPlatformFileManager::Get().GetPlatformFile().SetTimeStamp(*getCleanDirectory(directoryType, filePath), DateTime);
}

FDateTime UFileFunctionsSocketClient::getAccessTimeStamp(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath) {
	return FPlatformFileManager::Get().GetPlatformFile().GetAccessTimeStamp(*getCleanDirectory(directoryType, filePath));
}

FString UFileFunctionsSocketClient::getFilenameOnDisk(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath) {
	return FPlatformFileManager::Get().GetPlatformFile().GetFilenameOnDisk(*getCleanDirectory(directoryType, filePath));
}

bool UFileFunctionsSocketClient::createDirectory(EFileFunctionsSocketClientDirectoryType directoryType, FString path) {
	return FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*getCleanDirectory(directoryType, path));
}

void UFileFunctionsSocketClient::getAllFilesFromDirectory(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, int32& count, TArray<FString>& files, TArray<FString>& filePaths, FString fileType) {
	if (filePath.Len() < 1) {
		return;
	}
	files.Empty();
	filePaths.Empty();

	FString dir = getCleanDirectory(directoryType, filePath);
	FPaths::NormalizeDirectoryName(filePath);

	if (!FPaths::DirectoryExists(dir)) {
		return;
	}
	IFileManager& FileManager = IFileManager::Get();

	dir += "/" + fileType;

	FileManager.FindFiles(files, *dir, true, false);


	filePath += "/";
	for (int32 i = 0; i < files.Num(); i++) {
		filePaths.Add((filePath + files[i]));
		//UE_LOG(LogTemp, Display, TEXT("->%s"), *files[i]);
	}
	count = files.Num();
}

FFileFunctionsSocketClientOpenFile UFileFunctionsSocketClient::openFile(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath){

	FArchive* writer = IFileManager::Get().CreateFileWriter(*getCleanDirectory(directoryType, filePath), EFileWrite::FILEWRITE_Append);
	FFileFunctionsSocketClientOpenFile file;
	file.writer = writer;
	return file;
}

int64 UFileFunctionsSocketClient::addBytesToFile(FFileFunctionsSocketClientOpenFile openFile, TArray<uint8> bytes){
	//UE_LOG(LogTemp, Warning, TEXT("xxxxx WRITE: %i"), bytes.Num());
	if (openFile.writer != nullptr) {
		openFile.writer->Seek(openFile.writer->TotalSize());
		openFile.writer->Serialize(bytes.GetData(), bytes.Num());
		//openFile.writer->Flush();
		return openFile.writer->TotalSize();
	}
	return 0;
}

void UFileFunctionsSocketClient::closeFile(FFileFunctionsSocketClientOpenFile openFile){
	if (openFile.writer != nullptr) {
		openFile.writer->Close();
		openFile.writer = nullptr;
	}
}


bool UFileFunctionsSocketClient::encryptFileWithAES(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, FString newFileName, FString key, bool writeEncryptedFileSizeToFile){

	if (filePath.IsEmpty() || newFileName.IsEmpty() || key.Len() != 32) {
		UE_LOG(LogTemp, Error, TEXT("encryptFileWithAES: FilePath or newFileName empty or wrong key length. The key must consist of 32 ANSI compatible characters."));
		return false;
	}

	FString dir = getCleanDirectory(directoryType, filePath);
	FString fileName = FPaths::GetCleanFilename(dir);
	int64 fileSize = FPlatformFileManager::Get().GetPlatformFile().FileSize(*dir);

	TArray64<uint8> data;
	if (!FFileHelper::LoadFileToArray(data, *dir)) {
		UE_LOG(LogTemp, Error, TEXT("encryptFileWithAES: File could not be loaded: %s"),*dir);
		return false;
	}

	const int64 encryptedFileSize = Align(data.Num(), FAES::AESBlockSize);

	if (data.Num() < encryptedFileSize) {
		data.AddUninitialized(encryptedFileSize - data.Num());
	}

	FAES::EncryptData(data.GetData(), encryptedFileSize, TCHAR_TO_ANSI(*key));
	
	dir = dir.Replace(*fileName, *newFileName);
	if (FPaths::FileExists(*dir)) {
		FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*dir);
	}

	FArchive* writer = IFileManager::Get().CreateFileWriter(*dir, EFileWrite::FILEWRITE_Append);
	if (!writer) {
		UE_LOG(LogTemp, Error, TEXT("encryptFileWithAES: File could not be saved: %s"), *dir);
		data.Empty();
		return false;
	}
	
	if (writeEncryptedFileSizeToFile) {
		TArray<uint8> byteArray;
		union {
			int64 tmpVal;
			uint8 tmpArray[8];
		} u;
		u.tmpVal = fileSize;
		byteArray.AddZeroed(8);

		FMemory::Memcpy(byteArray.GetData(), u.tmpArray, 8);
		writer->Serialize(byteArray.GetData(), byteArray.Num());
		byteArray.Empty();
	}

	writer->Serialize(data.GetData(), data.Num());
	writer->Close();
	delete writer;

	data.Empty();


	return true;
}

bool UFileFunctionsSocketClient::decryptFileWithAES(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, FString newFileName, FString key,
	bool hasEncryptedFileSizeInFile, int64 originalFileLength){
	
	if (filePath.IsEmpty() || newFileName.IsEmpty() || key.Len() != 32) {
		UE_LOG(LogTemp, Error, TEXT("decryptFileWithAES: FilePath or newFileName empty or wrong key length. The key must consist of 32 ANSI compatible characters."));
		return false;
	}

	FString dir = getCleanDirectory(directoryType, filePath);
	FString fileName = FPaths::GetCleanFilename(dir);
	int64 fileSize = FPlatformFileManager::Get().GetPlatformFile().FileSize(*dir);

	FArchive* reader = IFileManager::Get().CreateFileReader(*dir);
	if (!reader) {
		UE_LOG(LogTemp, Error, TEXT("decryptFileWithAES: File could not be loaded: %s"), *dir);
		return false;
	}

	if (hasEncryptedFileSizeInFile) {
		TArray<uint8> bytes;
		bytes.AddZeroed(8);
		reader->Serialize(bytes.GetData(), bytes.Num());
		FMemory::Memcpy(&originalFileLength, bytes.GetData(), 8);
		bytes.Empty();
		fileSize  = fileSize-8;
	}

	if (fileSize < originalFileLength) {
		originalFileLength = fileSize;
		UE_LOG(LogTemp, Warning, TEXT("decryptFileWithAES: The encrypted file size is smaller than the original file size. File may be corrupted: %s"), *dir);
		return false;
	}
	
	TArray64<uint8> data;
	data.AddUninitialized(fileSize);
	reader->Serialize(data.GetData(), data.Num());
	reader->Close();
	delete reader;


	const int64 encryptedFileSize = Align(data.Num(), FAES::AESBlockSize);
	if (data.Num() < encryptedFileSize) {
		UE_LOG(LogTemp, Error, TEXT("decryptFileWithAES: Wrong data length. File could not be decrypted: %s"),*dir);
		return false;
	}

	FAES::DecryptData(data.GetData(), encryptedFileSize, TCHAR_TO_ANSI(*key));
	
	dir = dir.Replace(*fileName, *newFileName);
	if (FPaths::FileExists(*dir)) {
		FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*dir);
	}

	FArchive* writer = IFileManager::Get().CreateFileWriter(*dir, EFileWrite::FILEWRITE_Append);
	if (!writer) {
		UE_LOG(LogTemp, Error, TEXT("encryptFileWithAES: File could not be saved: %s"), *dir);
		data.Empty();
		return false;
	}
	writer->Serialize(data.GetData(), originalFileLength);
	writer->Close();
	delete writer;

	data.Empty();
	return true;
}

FString  UFileFunctionsSocketClient::encryptMessageWithAES(FString message, FString key) {

	if (message.IsEmpty() || key.Len() != 32) {
		UE_LOG(LogTemp, Error, TEXT("encryptMessageWithAES:Message empty or wrong key length. The key must consist of 32 ANSI compatible characters."));
		return FString();
	}

	TArray<uint8> data = FStringToByteArray(message);
	data.Add(0x00);// null-terminator
	const int64 encryptedFileSize = Align(data.Num(), FAES::AESBlockSize);

	if (data.Num() < encryptedFileSize){
		data.AddUninitialized(encryptedFileSize-data.Num());
	}

	FAES::EncryptData(data.GetData(), encryptedFileSize, TCHAR_TO_ANSI(*key));

	FString encryptedBase64String = FString();
	bytesToBase64String(data, encryptedBase64String);

	data.Empty();

	return encryptedBase64String;
}

FString  UFileFunctionsSocketClient::decryptMessageWithAES(FString message, FString key) {
	if (message.IsEmpty() || key.Len() != 32) {
		UE_LOG(LogTemp, Error, TEXT("decryptMessageFromAES: Message empty or wrong key length. The key must consist of 32 ANSI compatible characters."));
		return FString();
	}

	bool success = false;
	TArray<uint8> data = base64StringToBytes(message, success);


	const int64 encryptedFileSize = Align(data.Num(), FAES::AESBlockSize);
	if (data.Num() < encryptedFileSize) {
		UE_LOG(LogTemp, Error, TEXT("decryptMessageFromAES: Wrong string length. Message could not be decrypted."));
		return FString();
	}

	FAES::DecryptData(data.GetData(), encryptedFileSize, TCHAR_TO_ANSI(*key));

	FString s = FString(UTF8_TO_TCHAR((char*)data.GetData()));

	data.Empty();

	return s;
}


TArray<uint8> UFileFunctionsSocketClient::FStringToByteArray(FString s) {
	FTCHARToUTF8 Convert(*s);
	TArray<uint8> data;
	data.Append((uint8*)Convert.Get(), Convert.Length());
	return data;
}


FString UFileFunctionsSocketClient::int64ToString(int64 Num) {
	FString str = FString();
	const TCHAR* DigitToChar = TEXT("9876543210123456789");
	constexpr int64 ZeroDigitIndex = 9;
	bool bIsNumberNegative = Num < 0;
	const int64 TempBufferSize = 32; // 32 is big enough
	TCHAR TempNum[TempBufferSize];
	int64 TempAt = TempBufferSize; // fill the temp string from the top down.

	// Convert to string assuming base ten.
	do
	{
		TempNum[--TempAt] = DigitToChar[ZeroDigitIndex + (Num % 10)];
		Num /= 10;
	} while (Num);

	if (bIsNumberNegative)
	{
		TempNum[--TempAt] = TEXT('-');
	}

	const TCHAR* CharPtr = TempNum + TempAt;
	const int64 NumChars = TempBufferSize - TempAt;
	str.Append(CharPtr, NumChars);

	return str;
}




void UFileFunctionsSocketClient::readBytesFromFileInPartsAsync(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, int32 bufferSize, float delayBetweenReadsInSeconds) {
	UFileFunctionsSocketClient::getFileFunctionsSocketClientTarget()->readBytesFromFileInPartsAsyncInternal(directoryType, filePath, bufferSize, delayBetweenReadsInSeconds);
}

void UFileFunctionsSocketClient::readBytesFromFileInPartsAsyncInternal(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, int32 bufferSize, float delayBetweenReadsInSeconds) {

	FString dir = UFileFunctionsSocketClient::getCleanDirectory(directoryType, filePath);

	if (readFileInPartsThreads.Find(*dir) != nullptr) {
		UE_LOG(LogTemp, Warning, TEXT("ReadBytesFromFileInPartsAsync: %s is being read already. Operation canceled."), *dir);
		return;
	}

	FReadFileInPartsSocketClientThread* readThread = new FReadFileInPartsSocketClientThread(dir, bufferSize, delayBetweenReadsInSeconds);
	readFileInPartsThreads.Add(dir, readThread);
}

void UFileFunctionsSocketClient::cancelReadBytesFromFileInParts(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath) {
	UFileFunctionsSocketClient::getFileFunctionsSocketClientTarget()->cancelReadBytesFromFileInPartsInternal(directoryType, filePath);
}

void UFileFunctionsSocketClient::cancelReadBytesFromFileInPartsInternal(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath) {
	FString dir = UFileFunctionsSocketClient::getCleanDirectory(directoryType, filePath);

	if (readFileInPartsThreads.Find(*dir) != nullptr) {
		(*readFileInPartsThreads.Find(*dir))->stopThread();
	}
}

void UFileFunctionsSocketClient::cleanReadBytesFromFileInParts(FString cleanDir) {
	if (readFileInPartsThreads.Find(*cleanDir) != nullptr) {
		readFileInPartsThreads.Remove(*cleanDir);
	}
}

//void UFileFunctionsSocketClient::changeDelayInBytesFromFileInParts(EFileFunctionsSocketClientDirectoryType directoryType, FString filePath, float delayBetweenReadsInSeconds) {
//	FString dir = UFileFunctionsSocketClient::getCleanDirectory(directoryType, filePath);
//
//	if (readFileInPartsThreads.Find(*dir) != nullptr) {
//		(*readFileInPartsThreads.Find(*dir))->setDelayBetweenReadsInSeconds(delayBetweenReadsInSeconds);
//	}
//}
