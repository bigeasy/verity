/* TOOD Outgoing. */
// {0308456E-B8AA-4267-9A3E-09010E0A6754}
DEFINE_GUID(CLSID_IVerityController, 
0x308456e, 0xb8aa, 0x4267, 0x9a, 0x3e, 0x9, 0x1, 0xe, 0xa, 0x67, 0x54);

#define CLSID_VERITY _T("{0308456E-B8AA-4267-9A3E-09010E0A6754}")

typedef struct SVerityControllerFactory {
    const IClassFactoryVtbl *lpVtbl;
    DWORD dwLockCount;
} IVerityControllerFactory;

extern IVerityControllerFactory factory;

const IObjectWithSiteVtbl IVerityControllerVtbl;

void ConstructVerityControllerFactory();
