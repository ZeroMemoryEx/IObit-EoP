#pragma once
#include <Windows.h>
#include <Shlwapi.h>
#include <Msi.h>
#include <PathCch.h>
#include <shellapi.h>
#include <iostream>
#include "resource.h"
#include <vector>
#include <string>
#pragma comment(lib, "Msi.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "PathCch.lib")


bool stage2FilesystemChangeDetected;

class Resources
{
public:
	static Resources& instance()
	{
		static Resources singleton;
		return singleton;
	};
	const std::vector<BYTE>& msi() { return m_msi; };
	const std::vector<BYTE>& fakeRbs() { return m_fakeRbs; };
	const std::vector<BYTE>& fakeRbf() { return m_fakeRbf; };
private:
	Resources()
	{
		m_hModule = GetModuleHandle(NULL);
		initFromResource(m_msi, MAKEINTRESOURCE(IDR_MSI1), L"msi");
		initFromResource(m_fakeRbs, MAKEINTRESOURCE(IDR_RBS1), L"rbs");
		initFromResource(m_fakeRbf, MAKEINTRESOURCE(IDR_RBF1), L"rbf");
	};
	void initFromResource(std::vector<BYTE>& vec, LPCWSTR lpResourceName, LPCWSTR lpResourceType)
	{
		HRSRC hRsrc = FindResource(m_hModule, lpResourceName, lpResourceType);
		DWORD resSize = SizeofResource(m_hModule, hRsrc);
		vec.reserve(resSize);
		HGLOBAL hRes = LoadResource(m_hModule, hRsrc);
		BYTE* resData = (BYTE*)LockResource(hRes);
		vec.assign(resData, resData + resSize);

	};
	HMODULE m_hModule;
	std::vector<BYTE> m_msi;
	std::vector<BYTE> m_fakeRbs;
	std::vector<BYTE> m_fakeRbf;
};
