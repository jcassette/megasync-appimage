#include "ClassFactoryShellExtNotFound.h"

#include "ShellExtNotASync.h"
#include "MEGAinterface.h"

#include <new>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

IFACEMETHODIMP ClassFactoryShellExtNotFound::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = CLASS_E_NOAGGREGATION;

    if (ppv == NULL)
    {
        return E_POINTER;
    }

    *ppv = NULL;
    if (pUnkOuter == NULL)
    {
        hr = E_OUTOFMEMORY;

        // Create the COM component.
        ShellExt* pExt = new (std::nothrow) ShellExtNotASync(MegaInterface::FILE_NOTFOUND);

        if (pExt)
        {
            // Query the specified interface.
            hr = pExt->QueryInterface(riid, ppv);
            pExt->Release();
        }
    }

    return hr;
}
