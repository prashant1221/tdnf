/*
      * Copyright (C) 2014-2015 VMware, Inc. All rights reserved.
      *
      * Module   : utils.c
      *
      * Abstract :
      *
      *            tdnfclientlib
      *
      *            client library
      *
      * Authors  : Priyesh Padmavilasom (ppadmavilasom@vmware.com)
      *
*/
#include "includes.h"

uint32_t
TDNFUtilsFormatSize(
    uint32_t unSize,
    char** ppszFormattedSize
    )
{
    uint32_t dwError = 0;
    char* pszFormattedSize = NULL;
    char* pszSizes = "bkMG";
    double dSize = unSize;

    int nIndex = 0;
    int nLimit = strlen(pszSizes);
    double dKiloBytes = 1024.0;
    int nMaxSize = 25;

    if(!ppszFormattedSize)
    {
      dwError = ERROR_TDNF_INVALID_PARAMETER;
      BAIL_ON_TDNF_ERROR(dwError);
    }

    while(nIndex < nLimit && dSize > dKiloBytes)
    {
        dSize /= dKiloBytes;
        nIndex++;
    }

    dwError = TDNFAllocateMemory(nMaxSize, (void**)&pszFormattedSize);
    BAIL_ON_TDNF_ERROR(dwError);

    if(sprintf(pszFormattedSize, "%.2f %c", dSize, pszSizes[nIndex]) < 0)
    {
        dwError = ERROR_TDNF_OUT_OF_MEMORY;
        BAIL_ON_TDNF_ERROR(dwError);
    }

    *ppszFormattedSize = pszFormattedSize;
  
cleanup:
    return dwError;

error:
    if(ppszFormattedSize)
    {
        *ppszFormattedSize = NULL;
    }
    TDNF_SAFE_FREE_MEMORY(pszFormattedSize);
    goto cleanup;
}

uint32_t
TDNFGetErrorString(
    uint32_t dwErrorCode,
    char** ppszError
    )
{
    uint32_t dwError = 0;
    char* pszError = NULL;
    char* pszSystemError = NULL;
    int i = 0;
    int nCount = 0;
    uint32_t dwActualError = 0;
    

    //Allow mapped error strings to override
    TDNF_ERROR_DESC arErrorDesc[] = TDNF_ERROR_TABLE;

    nCount = sizeof(arErrorDesc)/sizeof(arErrorDesc[0]);

    for(i = 0; i < nCount; i++)
    {
        if (dwErrorCode == arErrorDesc[i].nCode)
        {
            dwError = TDNFAllocateString(arErrorDesc[i].pszDesc, &pszError);
            BAIL_ON_TDNF_ERROR(dwError);
            break;
        }
    }


    //Get system error 
    if(!pszError && TDNFIsSystemError(dwErrorCode))
    {
        dwActualError = TDNFGetSystemError(dwErrorCode);
        pszSystemError = strerror(dwActualError);
        if(pszSystemError)
        {
            dwError = TDNFAllocateString(pszSystemError, &pszError);
            BAIL_ON_TDNF_ERROR(dwError);
        }
    }

    //If the above attempts did not yield an error string,
    //do default unknown error.
    if(!pszError)
    {
        dwError = TDNFAllocateString(TDNF_UNKNOWN_ERROR_STRING, &pszError);
        BAIL_ON_TDNF_ERROR(dwError);
    }
 
    *ppszError = pszError;
cleanup:
    return dwError;

error:
    if(ppszError)
    {
        *ppszError = NULL;
    }
    TDNF_SAFE_FREE_MEMORY(pszError);
    goto cleanup;
}

uint32_t
TDNFIsSystemError(
    uint32_t dwError
    )
{
    return dwError > ERROR_TDNF_SYSTEM_BASE;
}

uint32_t
TDNFGetSystemError(
    uint32_t dwError
    )
{
    uint32_t dwSystemError = 0;
    if(TDNFIsSystemError(dwError))
    {
        dwSystemError = dwError - ERROR_TDNF_SYSTEM_BASE;
    }
    return dwSystemError;
}

int
TDNFIsGlob(
    const char* pszString
    )
{
    int nResult = 0;
    while(*pszString)
    {
        char ch = *pszString;
        
        if(ch == '*' || ch == '?' || ch == '[')
        {
            nResult = 1;
            break;
        }
        
        pszString++;
    }
    return nResult;
}

uint32_t
TDNFUtilsMakeDir(
    const char* pszDir
    )
{
    uint32_t dwError = 0;

    if(IsNullOrEmptyString(pszDir))
    {
        dwError = ERROR_TDNF_INVALID_PARAMETER;
        BAIL_ON_TDNF_ERROR(dwError);
    }

    if(access(pszDir, F_OK))
    {
        if(errno != ENOENT)
        {
            dwError = errno;
        }
        BAIL_ON_TDNF_SYSTEM_ERROR(dwError);

        if(mkdir(pszDir, 755))
        {
            dwError = errno;
            BAIL_ON_TDNF_SYSTEM_ERROR(dwError);
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

uint32_t
TDNFUtilsMakeDirs(
    const char* pszPath
    )
{
    uint32_t dwError = 0;
    char* pszTempPath = NULL;
    char* pszTemp = NULL;
    int nLength = 0;

    if(IsNullOrEmptyString(pszPath))
    {
        dwError = ERROR_TDNF_INVALID_PARAMETER;
        BAIL_ON_TDNF_ERROR(dwError);
    }

    if(access(pszPath, F_OK))
    {
        if(errno != ENOENT)
        {
            dwError = errno;
        }
        BAIL_ON_TDNF_SYSTEM_ERROR(dwError);
    }
    else
    {
        dwError = EEXIST;
        BAIL_ON_TDNF_SYSTEM_ERROR(dwError);
    }

    dwError = TDNFAllocateString(pszPath, &pszTempPath);
    BAIL_ON_TDNF_ERROR(dwError);

    nLength = strlen(pszTempPath);
    if(pszTempPath[nLength - 1] == '/')
    {
        pszTempPath[nLength - 1] = '\0';
    }
    for(pszTemp = pszTempPath + 1; *pszTemp; pszTemp++)
    {
        if(*pszTemp == '/')
        {
            *pszTemp = '\0';
            dwError = TDNFUtilsMakeDir(pszTempPath);
            BAIL_ON_TDNF_ERROR(dwError);
            *pszTemp = '/';
        }
    }
    dwError = TDNFUtilsMakeDir(pszTempPath);
    BAIL_ON_TDNF_ERROR(dwError);
    
cleanup:
    TDNF_SAFE_FREE_MEMORY(pszTempPath);
    return dwError;

error:
    goto cleanup;
}

uint32_t
TDNFIsDir(
    const char* pszPath,
    int* pnPathIsDir
    )
{
    uint32_t dwError = 0;
    int nPathIsDir = 0;
    struct stat stStat = {0};

    if(!pnPathIsDir || IsNullOrEmptyString(pszPath))
    {
        dwError = ERROR_TDNF_INVALID_PARAMETER;
        BAIL_ON_TDNF_ERROR(dwError);
    }

    if(stat(pszPath, &stStat))
    {
        dwError = errno;
        BAIL_ON_TDNF_SYSTEM_ERROR(dwError);
    }

    nPathIsDir = S_ISDIR(stStat.st_mode);

    *pnPathIsDir = nPathIsDir;
cleanup:
    return dwError;

error:
    if(pnPathIsDir)
    {
        *pnPathIsDir = 0;
    }
    goto cleanup;
}
