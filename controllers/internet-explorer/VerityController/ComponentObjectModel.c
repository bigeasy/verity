#include "stdafx.h"
#include "ComponentObjectModel.h"

BOOLEAN
IsIIDImplemented(REFIID riid, REFIID ariidImplemented[])
{
    DWORD i;
    for (i = 0; ariidImplemented[i]; i++)
    {
        if (IsEqualIID(riid, ariidImplemented[i]))
        {
            return TRUE;
        }
    }
    return FALSE;
}

void NO_DESTRUCTOR(LPVOID pObject) {}

void COM_Destroying(void *pClass, char* name)
{
}
