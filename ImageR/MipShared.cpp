#include "StdAfx.h"
#include "MipShared.h"
#include "..\..\ImageRLib\FileMapping.h"
#include "..\..\ImageRLib\Position.h"

CMipShared::CMipShared(class CFileMapping *pMapping)
: CMetaData (pMapping->msName)
, CSharedMemFile(pMapping)
{
	CMetaData *pData = NULL;
	CMetaDataBase *pDataBase = NULL; // GetPosition()->GetMultiFileData();
	if (pDataBase)
		pData = pDataBase->GetMetaData();


	if (mpHeader && pData)
	{
		if (mpHeader->mzGraphicName[0])
		{
			CNamedObject *pNamed = GetNamed(mpHeader->mzGraphicName);
			if (pNamed->GetClass() == "CGEllipse")
			{
				mpVoi = (CGEllipse *)pNamed;
				CDataRoi *pRoi = (CDataRoi *)pNamed;
				if (!mpVoiBounds)
				{
					mpVoiBounds = new CDataRoiLineBounds;
					mpVoiBounds->Init(*pData->PageData(), *pRoi);
				}
			}
		}
	}
}
CMipShared::~CMipShared()
{
}
CString CMipShared::GetZoomName(void)
{
	return CMip::GetZoomName();
}
