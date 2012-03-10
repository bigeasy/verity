#include "stdafx.h"
#include "Pool.h"

#define POOL_SIZE 128
#define STRING_HEAP 0
#define MEMORY_HEAP 0


typedef struct CListPool LISTPOOL;
typedef struct CListPool {
    LISTPOOL*   pPrevious;
    WORD        wNextNode; 
    LISTNODE    aln[POOL_SIZE];
} LISTPOOL;

HANDLE
PoolAllocPool()
{
    LISTPOOL* pListPool = GlobalAlloc(GMEM_FIXED, sizeof(LISTPOOL));
    if (pListPool != NULL)
    {
        pListPool->pPrevious = NULL;
        pListPool->wNextNode = 3;
    }
    pListPool->aln[0].pvValue =
        pListPool->aln[1].pvValue =
        pListPool->aln[2].pvValue = NULL;
    return pListPool;
}

LISTNODE*
PoolAllocNode(HANDLE hList)
{
    LISTNODE* pNode;
    LISTPOOL* pNextPool;
    LISTPOOL* pListPool = (LISTPOOL*) hList;
    if (pListPool->wNextNode == POOL_SIZE) {
        pNextPool = PoolAllocPool();    
        if (pNextPool == NULL)
        {
            return NULL;
        }
        pNextPool->pPrevious = pListPool;
        pListPool = pNextPool;
    }
    pNode = &pListPool->aln[pListPool->wNextNode++];
    pNode->pvValue = NULL;
    pNode->pNext = pNode->pPrevious = pNode;
    return pNode;
}

static LISTPOOL*
GetPoolHead(HANDLE hListPool)
{
    LISTPOOL* pListPool = (LISTPOOL*) hListPool;
    while (pListPool->pPrevious)
    {
        pListPool = pListPool->pPrevious;
    }
    return pListPool;
}

BSTR
PoolAllocStringBytes(HANDLE hListPool, UINT uiBytes)
{
    BSTR bstr;
    LISTNODE* pNext = PoolAllocNode(hListPool);
    if (pNext == NULL)
    {
        return NULL;
    }
    bstr = SysAllocStringByteLen(NULL, uiBytes);
    if (bstr == NULL)
    {
        return NULL;
    }
    pNext->pvValue = bstr;
    PoolLinkAfter(pNext, &GetPoolHead(hListPool)->aln[STRING_HEAP]);
    return bstr;
}

BSTR
PoolAllocString(HANDLE hListPool, TCHAR* tstr)
{
    BSTR bstr;
    LISTNODE* pNext = PoolAllocNode(hListPool);
    if (pNext == NULL)
    {
        return NULL;
    }
    bstr = SysAllocString(tstr);
    if (bstr == NULL)
    {
        return NULL;
    }
    pNext->pvValue = bstr;
    PoolLinkAfter(pNext, &GetPoolHead(hListPool)->aln[STRING_HEAP]);
    return bstr;
}

BSTR
PoolAppendString(HANDLE hListPool, BSTR string, BSTR append)
{
    UINT combinedLength, baseBytes, appendBytes;
    BSTR dest;
    baseBytes = SysStringByteLen(string);       
    appendBytes = SysStringByteLen(string);   
    combinedLength = wcslen(string) + wcslen(append) + 1;
    if (combinedLength > baseBytes) {
        dest = PoolAllocStringBytes(hListPool, combinedLength * 2);
        wcscat_s(dest, combinedLength, string);
    } 
    else
    {
        dest = string;
    }
    wcscat_s(dest, combinedLength, append);
    return dest;
}

LPVOID
PoolAllocMemory(HANDLE hListPool, SIZE_T sSize)
{
    LPVOID pMemory;
    LISTNODE* pNext = PoolAllocNode(hListPool);
    if (pNext == NULL)
    {
        return NULL;
    }
    pMemory = GlobalAlloc(GMEM_FIXED, sSize);
    if (pMemory == NULL)
    {
        return NULL;
    }
    pNext->pvValue = pMemory;
    PoolLinkAfter(pNext, &GetPoolHead(hListPool)->aln[STRING_HEAP]);
    return pNext->pvValue;
}

HRESULT
PoolCoCreateInstance(
    HANDLE hPool, REFCLSID rclsid, REFIID riid, LPVOID* ppv
) {
    HRESULT err = S_FALSE;
    LISTNODE* pNext = PoolAllocNode(hPool);
    if (pNext)
    {
        err = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC, riid, ppv);
        if (err == S_OK)
        {
            pNext->pvValue = *ppv;
            PoolLinkAfter(pNext, &GetPoolHead(hPool)->aln[2]);
        }
    }
    return err;
}

HRESULT
PoolQueryInterface(
    HANDLE hPool, LPVOID pInstance, REFIID guidVtbl, void **ppv
) {
    HRESULT err     = S_FALSE;
    IUnknown *pSelf = (IUnknown*) pInstance;
    LISTNODE* pNext = PoolAllocNode(hPool);
    if (pNext)
    {
        err = pSelf->lpVtbl->QueryInterface(pSelf, guidVtbl, ppv);
        if (err == S_OK)
        {
            pNext->pvValue = *ppv;
            PoolLinkAfter(pNext, &GetPoolHead(hPool)->aln[2]);
        }
    }
    return err;
}

LISTNODE*
PoolLinkAfter(LISTNODE* pNode, LISTNODE* pAfter)
{
    pNode->pNext = pAfter->pNext;
    pNode->pPrevious = pAfter;
    pNode->pNext->pPrevious = pNode;
    pNode->pPrevious->pNext = pNode;
    return pNode;
}

LISTNODE*
PoolLinkBefore(LISTNODE* pNode, LISTNODE* pBefore)
{
    pNode->pPrevious = pBefore->pPrevious;
    pNode->pNext = pBefore;
    pNode->pNext->pPrevious = pNode;
    pNode->pPrevious->pNext = pNode;
    return pNode;
}

LISTNODE*
PoolUnlink(LISTNODE* pNode)
{
    pNode->pPrevious->pNext = pNode->pNext;
    pNode->pNext->pPrevious = pNode->pPrevious;
    return pNode;
}

void
PoolFreePool(HANDLE hListPool)
{
    LISTNODE* pIterator;
    LISTPOOL* pListPool, *pNextPool;
    if (hListPool)
    {
        pIterator = &GetPoolHead(hListPool)->aln[STRING_HEAP];
        while (pIterator->pNext->pvValue)
        {
            pIterator = pIterator->pNext;
            SysFreeString((BSTR) pIterator->pvValue);
        }
        pIterator = &GetPoolHead(hListPool)->aln[MEMORY_HEAP];
        while (pIterator->pNext->pvValue)
        {
            pIterator = pIterator->pNext;
            GlobalFree(pIterator->pvValue);
        }
        pListPool = (LISTPOOL *) hListPool;
        do
        {
            pNextPool = pListPool->pPrevious;
            GlobalFree((HGLOBAL) pNextPool);
            pListPool = pNextPool;
        }
        while (pListPool != NULL);
    }
}
