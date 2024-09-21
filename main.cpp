/**
* Author: QaBns
* ˼·:
* 1.Ѱ��steamʵ���Ƿ����
* 2.���steam���������HKEY_CURRENT_USER\Software\Valve\Steam�е�RunningAppID
* 3.��RunningAppID��ֵ����,��Ϊ��������Ϸ,�޸��Ƶ����Դ�ƻ���
**/

#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <TlHelp32.h>
#include <powrprof.h>
#include <iostream>
#include <memory>

#define ERROR_MESSAGE_BOX(text) MessageBox(nullptr,TEXT(text),TEXT("Error"),MB_OK | MB_ICONERROR)

#pragma comment(lib,"PowrProf.lib")

bool TryToFoundSteamInstance()
{
	bool found = false; //��ֵӦΪFalse
	HANDLE info_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (info_handle == INVALID_HANDLE_VALUE)
	{
		ERROR_MESSAGE_BOX("CreateToolhelp32Snapshot() Failed.");
		abort();
	}

	PROCESSENTRY32 program_info = {
		.dwSize = sizeof(PROCESSENTRY32)
	};
	bool bResult = Process32First(info_handle, &program_info);
	if (!bResult)
	{
		ERROR_MESSAGE_BOX("Process32First() Failed.");
		abort();
	}
	while (bResult)
	{
		int nLen = WideCharToMultiByte(CP_ACP, 0, program_info.szExeFile, -1, nullptr, 0, nullptr, nullptr);
		std::unique_ptr<char> pResult(new char[nLen]);
		WideCharToMultiByte(CP_ACP, 0, program_info.szExeFile, -1, pResult.get(), nLen, nullptr, nullptr);
		if (std::string(pResult.get()) == "steam.exe")
		{
			found = true;
			break;
		}
		bResult = Process32Next(info_handle, &program_info);
	}
	CloseHandle(info_handle);
	return found;
}

bool HasRunningApp()
{
	HKEY hKey;
	DWORD dwType = REG_DWORD;
	DWORD value;
	DWORD dwValue;
	LONG result = RegOpenKeyEx(
		HKEY_CURRENT_USER,
		TEXT("Software\\Valve\\Steam"),
		0,
		KEY_READ,
		&hKey
	);
	bool hasRunningApp = false;
	if (result == ERROR_SUCCESS)
	{
		result = RegQueryValueEx(
			hKey,
			TEXT("RunningAppID"),
			nullptr,
			&dwType,
			(LPBYTE)&value,
			&dwValue
		);
		if (result == ERROR_SUCCESS) {
			char buf[25] = { 0 };
			_ultoa(value, buf, 10);
			if (strcmp(buf,"0") != 0)
			{
				hasRunningApp = true;
			}
		}
	}
	RegCloseKey(hKey);
	return hasRunningApp;
}

GUID* GetCurrentPowerPlan()
{
	GUID* currentPowerPlan;
	if (PowerGetActiveScheme(nullptr, &currentPowerPlan) != ERROR_SUCCESS)
	{
		ERROR_MESSAGE_BOX("PowerGetActiveShceme() Failed.");
		abort();
	}
	return currentPowerPlan;
}

int GetBoostModeValue(GUID* powerPlan,bool isAc)
{
	DWORD bufSize;
	auto ReadValue = isAc ? PowerReadACValue : PowerReadDCValue;
	//��ȡҪ�����Ļ�������С
	if (ReadValue(
		nullptr,
		powerPlan,
		&GUID_PROCESSOR_SETTINGS_SUBGROUP,
		&GUID_PROCESSOR_PERF_BOOST_MODE,
		nullptr,
		nullptr,
		&bufSize
	) != ERROR_SUCCESS)
	{
		ERROR_MESSAGE_BOX("PowerReadACValue() Failed.");
		abort();
	}
	std::unique_ptr<BYTE> bytes(new BYTE[bufSize]); //����ָ���ֹ�ڴ�й¶
	// ������ȡ����
	DWORD status = ReadValue(
		nullptr,
		powerPlan,
		&GUID_PROCESSOR_SETTINGS_SUBGROUP,
		&GUID_PROCESSOR_PERF_BOOST_MODE,
		nullptr,
		bytes.get(),
		&bufSize
	);
	if (status != ERROR_SUCCESS)
	{
		ERROR_MESSAGE_BOX("PowerReadACValue() Failed.");
		abort();
	}
	return (int)*bytes;
}

void SetBoostModeValue(GUID* powerPlan,bool isAc)
{
	//Wait for implementaion
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	while (true) //�ܳ����ѭ��
	{
		bool isFound = TryToFoundSteamInstance();
		if (!isFound)
		{
			Sleep(1000);
			continue;
		}
		bool hasRunning = HasRunningApp(); //��ȡRunningAppID��ֵ
		//ToDo: �޸ĵ�ǰ��Դ�ƻ�����һ���߳������RunningAppID��ֵ�Ƿ����Ϊ0
		GUID* currentPowerPlan = GetCurrentPowerPlan();
		int acValue = GetBoostModeValue(currentPowerPlan, true); //�õ����ʱ�Ĵ������Ƶģʽ
		int dcValue = GetBoostModeValue(currentPowerPlan, true); //�õ����ʱ�Ĵ������Ƶģʽ
		if (acValue == 0 || dcValue == 0)
		{
			SetBoostModeValue(currentPowerPlan, true);
			SetBoostModeValue(currentPowerPlan, false);
		}
		LocalFree(currentPowerPlan);
		break;
	}
	return EXIT_SUCCESS;
}