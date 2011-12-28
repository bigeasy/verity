// Really struggled to find arbitrary names with opaque meaning and
// inconsistant notations so that Windows programmers would feel comfortable.
typedef struct CListNode LISTNODE;
typedef struct CListNode {
    LISTNODE*   pNext;
    LISTNODE*   pPrevious;
    LPVOID      pvValue;      
} LISTNODE;

HANDLE      PoolAllocPool           ();
LISTNODE*   PoolAllocNode           (HANDLE hList);
BSTR        PoolAllocStringBytes    (HANDLE hListPool, UINT uiBytes);
BSTR        PoolAllocString         (HANDLE hListPool, TCHAR* tstr);
HRESULT     PoolQueryInterface      (HANDLE hPool, LPVOID pSelf, REFIID guidVtbl, LPVOID* ppv);
HRESULT     PoolCoCreateInstance    (REFCLSID rclsid, REFIID riid, LPVOID* ppv);

BSTR        PoolAppendString        (HANDLE hListPool, BSTR string, BSTR append);
LPVOID      PoolAllocMemory         (HANDLE hListPool, SIZE_T sSize);
LISTNODE*   PoolLinkAfter           (LISTNODE* pNode, LISTNODE* pAfter);
LISTNODE*   PoolLinkBefore          (LISTNODE* pNode, LISTNODE* pBefore);
LISTNODE*   PoolUnlink              (LISTNODE* pNode);
void        PoolFreePool            (HANDLE hListPool);
