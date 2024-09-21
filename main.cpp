/**
* Author: QaBns
* 思路:
* 1.寻找steam实例是否存在
* 2.如果steam开启，检测HKEY_CURRENT_USER\Software\Valve\Steam中的RunningAppID
* 3.若RunningAppID的值存在,视为开启了游戏,修改睿频（电源计划）
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
	bool found = false; //初值应为False
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
	//获取要创建的缓冲区大小
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
	std::unique_ptr<BYTE> bytes(new BYTE[bufSize]); //智能指针防止内存泄露
	// 真正获取数据
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
	while (true) //总程序的循环
	{
		bool isFound = TryToFoundSteamInstance();
		if (!isFound)
		{
			Sleep(1000);
			continue;
		}
		bool hasRunning = HasRunningApp(); //获取RunningAppID键值
		//ToDo: 修改当前电源计划并留一个线程来检测RunningAppID键值是否更改为0
		GUID* currentPowerPlan = GetCurrentPowerPlan();
		int acValue = GetBoostModeValue(currentPowerPlan, true); //得到插电时的处理器睿频模式
		int dcValue = GetBoostModeValue(currentPowerPlan, true); //得到离电时的处理器睿频模式
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