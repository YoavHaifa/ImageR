#pragma once
#include "SharedMemFile.h"
#include "Mip.h"
#pragma warning(disable:4250) 

class CMipShared :
	public CSharedMemFile, public CMip
{
public:
	CMipShared(class CFileMapping *pMapping);
	virtual ~CMipShared(void);
    virtual CString GetWindowName(void){return CString("MipWindow");}
    virtual CString GetZoomName(void);
	virtual void OnAssignDR (void){CMip::OnAssignDR();}
};
