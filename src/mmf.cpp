#include "mmf.h"

#pragma warning(push)		// 26437 26110 26439
#pragma warning(disable: 26437)	//  constexpr 
#pragma warning(disable: 26110)	// lock 잠금 해제 관련  warning
#pragma warning(disable: 26439) // throw 할 수 없습니다. 'noexcept'로 선언합니다
#pragma warning(disable: 26495) // 변수가 초기화되지 않았습니다. 항상 멤버 변수를 초기화하세요

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#include <AclAPI.h>
#undef byte
#endif

#pragma warning(pop)		// 26437 26110 26439 26495

int Mmf::Open(const char* t_name)
{
#if defined(_WIN32) || defined(_WIN64)
	auto status = Create(t_name);
	if (NO_ERROR != status)
	{
		return status;
	}

	return Open();
#else
	return ERROR_INVALID_HANDLE;
#endif
}

void Mmf::Write(const void* t_data, const size_t& t_size)
{
#if defined(_WIN32) || defined(_WIN64)
	if (nullptr == m_pointer)
		return;

	memcpy(m_pointer, t_data, t_size);
#endif
}

void Mmf::Close()
{
#if defined(_WIN32) || defined(_WIN64)
	if (nullptr != m_pointer)
	{
		UnmapViewOfFile(m_pointer);
		m_pointer = nullptr;
	}

	if (nullptr != m_handle)
	{
		CloseHandle(m_handle);
		m_handle = nullptr;
	}
#endif
}

int Mmf::Open()
{
#if defined(_WIN32) || defined(_WIN64)
	m_pointer = MapViewOfFile(m_handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (nullptr == m_pointer)
	{
		Close();
		return ERROR_INVALID_DATA;
	}

	return NO_ERROR;
#else
	return ERROR_INVALID_HANDLE;
#endif
}

int Mmf::Create(const char* t_name)
{
#if defined(_WIN32) || defined(_WIN64)
	m_handle = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 1024, t_name);

	auto status = GetLastError();
	if (NO_ERROR != status && ERROR_ALREADY_EXISTS != status)
	{
		Close();
		return status;
	}

	if (nullptr == m_handle)
		return ERROR_INVALID_HANDLE;

	SetSecurityInfo(m_handle, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, (PACL)NULL, NULL);

	return NO_ERROR;
#else
	return ERROR_INVALID_HANDLE;
#endif
}