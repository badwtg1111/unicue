/************************************************************************/
/*                                                                      */
/* A Simple File Obejct using win 32 api.                               */
/* It is simplified from CIOFile of win32-standard-expansion.           */
/* See http://code.google.com/p/win32-standard-expansion/ for detail.   */
/*                                                                      */
/* win32-standard-expansion                                             */
/* Publishedunder New BSD License.                                      */
/* Copyright memleak@darkc.at                                           */
/*                                                                      */
/* Author:  kuyur (kuyur@kuyur.info)                                    */
/* Published under Apache License 2.0                                   */
/* http://www.apache.org/licenses/LICENSE-2.0                           */
/*                                                                      */
/* Project URL: http://github.com/kuyur/unicue                          */
/*                                                                      */
/************************************************************************/

#include "stdafx.h"
#include "winfile.h"

CWinFile::CWinFile(const std::wstring &filename, UINT openFlags)
    : m_hFile(INVALID_HANDLE_VALUE), m_nFlags(openFlags)
{
    m_filename = NULL;
    UINT len = filename.length();
    if (len > 0)
    {
        m_filename = new wchar_t[len+1];
        wmemcpy(m_filename, filename.c_str(), len);
        m_filename[len] = L'\0';
    }
}

CWinFile::CWinFile(LPCWSTR filename, UINT openFlags)
    : m_hFile(INVALID_HANDLE_VALUE), m_nFlags(openFlags)
{
    m_filename = NULL;
    if (filename)
    {
        UINT len = wcslen(filename);
        if (len > 0)
        {
            m_filename = new wchar_t[len+1];
            wmemcpy(m_filename, filename, len);
            m_filename[len] = L'\0';
        }
    }
}

CWinFile::~CWinFile()
{
    close();
    if (m_filename)
        delete []m_filename;
}

BOOL CWinFile::open()
{
    if (!m_filename) return FALSE;
    if (wcslen(m_filename) > MAX_PATH) return FALSE;
    tryClose();

    DWORD dwAccess = 0;
    switch (m_nFlags & (modeRead | modeWrite | modeReadWrite))
    {
    case modeRead:
        dwAccess = GENERIC_READ;
        break;
    case modeWrite:
        dwAccess = GENERIC_WRITE;
        break;
    case modeReadWrite:
        dwAccess = GENERIC_READ | GENERIC_WRITE;
        break;
    default:
        return FALSE;
    }

    DWORD dwShareMode = 0;
    switch (m_nFlags & (shareDenyWrite | shareDenyRead | shareDenyNone | shareExclusive))
    {
    case shareExclusive:
        dwShareMode = 0;
        break;
    case shareDenyWrite:
        dwShareMode = FILE_SHARE_READ;
        break;
    case shareDenyRead:
        dwShareMode = FILE_SHARE_WRITE;
        break;
    case shareDenyNone:
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
        break;
    default:
        return FALSE;
    }

    DWORD dwCreateFlag;
    if (m_nFlags & modeCreate)
    {
        if (m_nFlags & modeNoTruncate)
            dwCreateFlag = OPEN_ALWAYS;
        else
            dwCreateFlag = CREATE_ALWAYS;
    }
    else
        dwCreateFlag = OPEN_EXISTING;

    m_hFile = ::CreateFileW(m_filename, dwAccess, dwShareMode, NULL, dwCreateFlag, FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_hFile == INVALID_HANDLE_VALUE)
        return FALSE;
    return TRUE;
}

BOOL CWinFile::close()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
    return TRUE;
}

void CWinFile::tryClose()
{
    close();
}

UINT64 CWinFile::length()
{
    if (m_hFile == INVALID_HANDLE_VALUE) return -1;
    ULARGE_INTEGER liSize;
    liSize.LowPart = ::GetFileSize(m_hFile, &liSize.HighPart);
    if (liSize.LowPart == INVALID_FILE_SIZE)
        return -1;
    return liSize.QuadPart;
}

UINT64 CWinFile::getPosition()
{
    if (m_hFile == INVALID_HANDLE_VALUE) return -1;
    LARGE_INTEGER liPos;
    liPos.QuadPart = 0;
    liPos.LowPart = ::SetFilePointer(m_hFile, liPos.LowPart, &liPos.HighPart, FILE_CURRENT);
    if (liPos.LowPart == (DWORD)-1)
        return -1;
    return liPos.QuadPart;
}

BOOL CWinFile::seek(INT64 offset, int position)
{
    if (m_hFile == INVALID_HANDLE_VALUE) return FALSE;
    if (position != begin && position != end && position != current) return FALSE;
    LARGE_INTEGER liOff;
    liOff.QuadPart = offset;
    liOff.LowPart = ::SetFilePointer(m_hFile, liOff.LowPart, &liOff.HighPart, (DWORD)position);
    return (liOff.LowPart != (DWORD)-1);
}

DWORD CWinFile::read(char *dst, UINT length)
{
    if (!dst || length == 0) return -1;
    if (isEOF()) return -1;
    DWORD dwBytesRead = 0;
    if(::ReadFile(m_hFile, dst, length, &dwBytesRead, NULL))
        return dwBytesRead;
    else
        return -1;
}

DWORD CWinFile::write(const char *src, UINT length)
{
    if (!src || length == 0) return -1;
    if (m_hFile == INVALID_HANDLE_VALUE) return -1;
    DWORD dwBytesWritten = 0;
    if(::WriteFile(m_hFile, src, length, &dwBytesWritten, NULL))
        return dwBytesWritten;
    else
        return -1;
}

BOOL CWinFile::flush()
{
    if (m_hFile == INVALID_HANDLE_VALUE) return FALSE;
    return ::FlushFileBuffers(m_hFile);
}

BOOL CWinFile::isEOF()
{
    if (m_hFile == INVALID_HANDLE_VALUE) return TRUE;
    UINT64 size = length();
    if (size == (UINT64)-1) return TRUE;
    UINT64 pos = getPosition();
    if (pos == (UINT64)-1) return TRUE;

    return (pos >= size);
}
