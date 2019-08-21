#include "StdAfx.h"

#ifdef __VR__
#include "VolumeRenderingIF.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\DynamicTimer.h"

#define M_PI       3.14159265358979323846

CVolumeRenderingIF::CVolumeRenderingIF(CMetaData *pData)
: CMetaData("VolumeRenderingIF")
, mpSrcData(pData)
, mpDataInRam(NULL)
, mapRasters(NULL)
, mbFailed(false)
, mSiceDeltaMm(1.0f)
, mS2C("S2C_RotMat")
, mC2S("C2S_RotMat")
, mC2SUnitRotMat("Unit_RotMat")
, mRotationMatrix("RotMat")
, mRotationMatrix4("RotMat4")
, mShiftSource("ShiftSource")
, mShiftCamera("ShiftCamera")
, mRotationMatrixInv("RotMatInv")
, mRotationMatrix4Inv("RotMat4Inv")
, mShiftSourceInv("ShiftSourceInv")
, mShiftCameraInv("ShiftCameraInv")
, mnThreads(0)
{
	//CRotationMatrix::umsPrintFormat = " %18.6f";
	CRotationMatrix::umbPrintDeterminant = true;

	CString sLogDir(CMyWindows::GetLogDir());
	CMyWindows::DeleteDirFiles(sLogDir += "\\RotMat");

	SetTypedDataHandler(DATA_TYPE_USHORT);

	mnLinesInPage = mpSrcData->GetNLinesInPage();
	mnCols = mpSrcData->GetNCols();
	mnInputSlices = mpSrcData->GetNPages();

	int nToAllocate = mnLinesInPage * mnCols * mnInputSlices;
	mpDataInRam = new unsigned short [nToAllocate];
	if (!mpDataInRam)
	{
		CMyWindows::MessBox("Failed to allocate memory to load all images", "VR Error");
	}
	
	mapRasters = new unsigned short *[mnInputSlices];

	LoadAllImages();
	InitParams();
	SetMmPerPixel(*pData);
        
	AddCheck(&mRotationMatrix);
}
CVolumeRenderingIF::~CVolumeRenderingIF(void)
{
	if (mpDataInRam)
		delete mpDataInRam;

	if (mapRasters)
		delete mapRasters;
}
bool CVolumeRenderingIF::LoadAllImages(void)
{
	static CEventTracer EvTracer ("CVRIF_LoadAllImages");
	CDynamicTimer DynTimer (&EvTracer);

	unsigned short *pLoad = mpDataInRam;
	int nPixelsInSlice = mnLinesInPage * mnCols;

	float firstBedPos = 0;
	bool bLookForBedInc = true;

	for (int i = mpSrcData->StartLoop(); i >= 0; i = mpSrcData->LoopNext())
	{
		static CEventTracer EvTracer ("CVRIF_LoadImage");
		CDynamicTimer DynTimer (&EvTracer);

		if (!mpSrcData->UpdatePageData())
		{
			char zBuf[256];
			sprintf_s(zBuf,sizeof(zBuf),"Load Images Failed for image %d/%d",
				i+1, mnInputSlices);
			CMyWindows::MessBox(zBuf, "VR Error");
			return false;
		}
		unsigned short *pData = (unsigned short *)mpSrcData->GetPageData();
		memcpy(pLoad, pData, nPixelsInSlice * sizeof(pData[0]));

		mapRasters[i] = pLoad;
		pLoad += nPixelsInSlice;

		if (bLookForBedInc)
		{
			CMyDicom *pDicom = mpSrcData->GetDicom();
			if (pDicom)
			{
				if (i == 0)
				{
					if (!pDicom->GetBedPosition(firstBedPos))
						bLookForBedInc = false;
				}
				else if (i == 1)
				{
					float seconBedPos = firstBedPos;
					if (pDicom->GetBedPosition(seconBedPos))
						mSiceDeltaMm = seconBedPos - firstBedPos;
					
					bLookForBedInc = false;
				}
			}
			else
				bLookForBedInc = false;
		}
	}

	mbValid = true;
	return true;
}
void CVolumeRenderingIF::InitParams(void)
{
	memset(&mParams, 0, sizeof(mParams));

	mParams.RP_version = RENDER_PARAMS_VERSION;
	mParams.pp_slices = mapRasters;
	mParams.bits = 16;
	mParams.Sz2xy_mmscale = mSiceDeltaMm / mpSrcData->MmPerPixelWidth();
	
	if (mSiceDeltaMm == 0)
		mParams.Sz_spacing_mm = 1.0;
	else
		mParams.Sz_spacing_mm = mSiceDeltaMm;

	// Set S Bounds
	maSBounds[0][0] = 0;
	maSBounds[0][1] = maSBounds[0][0] + mnCols - 1;
	maSBounds[1][0] = 0;
	maSBounds[1][1] = maSBounds[1][0] + mnLinesInPage - 1;
	maSBounds[2][0] = 0;
	maSBounds[2][1] = maSBounds[2][0] + mnInputSlices - 1;
	mParams.Sbounds = maSBounds[0];

	mShiftSource.SetTransform(- maSBounds[0][1] / 2.0, - maSBounds[1][1] / 2.0, - maSBounds[2][1] / 2.0);
	mShiftSourceInv.SetInverseTransform(mShiftSource);

	// Set C Bounds
	maCBounds[0][0] = maSBounds[0][0];
	maCBounds[0][1] = maSBounds[0][1];
	maCBounds[1][0] = maSBounds[1][0];
	maCBounds[1][1] = maSBounds[1][1];
	maCBounds[2][0] = 0;
	maCBounds[2][1] = max(maSBounds[2][1],maSBounds[1][1]) * 2;
	mParams.Cbounds = maCBounds[0];

	mShiftCamera.SetTransform(maCBounds[0][1] / 2.0, maCBounds[1][1] / 2.0, maCBounds[2][1] / 2.0);
	mShiftCameraInv.SetInverseTransform(mShiftCamera);

	// Set Conversion Matrices
	mParams.S2C = mS2C.GetStart();
	mParams.C2S = mC2S.GetStart();
	mParams.C2S_unit_rotmat = mC2SUnitRotMat.GetStart();

	mParams.camera_ID = 1;
	mParams.slicestack_ID = 2;

	mParams.render_op = rmMIP;
	mParams.interpolation = imTL;

	mParams.rendered_image_addr = GetPageData();

	mParams.force_raycasting = 1;
	mParams.interactive = 0;
	mParams.out_algorithm_used = ray_casting; //???? It should be output????
	mParams.RP_version_end = RENDER_PARAMS_VERSION;
}
bool CVolumeRenderingIF::DoUpdatePageData(void)
{
	if (mbFailed)
		return false;

	static CEventTracer EvTracer ("CVRIF_DoUpdatePageData");
	CDynamicTimer DynTimer (&EvTracer);

	mParams.rendered_image_addr = GetPageData();

	mRotationMatrix4 = mRotationMatrix;
	mS2C.Multiply(mShiftSource, mRotationMatrix4);
	mS2C.Multiply(mShiftCamera);

	mRotationMatrixInv.Inverse(mRotationMatrix);
	mRotationMatrix4Inv = mRotationMatrixInv;
	mC2S.Multiply(mShiftCameraInv, mRotationMatrix4Inv);
	mC2S.Multiply(mShiftSourceInv);

	LogCommand();
	VR_return_code rc = VR_render(&mParams, mnThreads, 0);

	if (rc == VR_OK)
	{
		return true;
	}

	mbFailed = true;
	char zBuf[128];
	sprintf_s(zBuf,sizeof(zBuf),"VR_render returned error %d", (int)rc);
	CMyWindows::MessBox(zBuf, "SW Error");
	return false;
}
bool CVolumeRenderingIF::SpecialMouseScroll(CPoint deltaPoint, bool & obChanged)
{
	static int count = 0;
	char zBuf[128];
	sprintf_s(zBuf,sizeof(zBuf),"SpecialMouseScroll_%03d", ++count);
	CLogFile log("RotMat", zBuf);
	if (log)
		fprintf(log.mpf,"delta deltaPoint x %d y %d\n\n", deltaPoint.x, deltaPoint.y);

	if (deltaPoint.y != 0)
	{
		int dx = (int)deltaPoint.y;
		CMyMath::Clip(-10, dx, 10);
		double radX = dx * M_PI / 180.0;
		double sinX = sin(radX);
		double cosX = cos(radX);
		if (log)
			fprintf(log.mpf,"dx %d, rad %f, sin %f cos %f\n",
				dx, radX, sinX, cosX);
		CRotationMatrix3 rotx("rotx");
		rotx.maMatrix[1][1] = cosX;
		rotx.maMatrix[1][2] = sinX;
		rotx.maMatrix[2][1] = -sinX;
		rotx.maMatrix[2][2] = cosX;
		mRotationMatrix.Print(log.mpf);
		rotx.Print(log.mpf);
		mRotationMatrix.Multiply(rotx);
		obChanged = true;
		mRotationMatrix.Print(log.mpf);
		fprintf(log.mpf,"Determinant rotx %f rm %f\n", rotx.Determinant(), mRotationMatrix.Determinant());
	}
	if (deltaPoint.x != 0)
	{
		int dy = (int)deltaPoint.x;
		CMyMath::Clip(-10, dy, 10);
		double radY = dy * M_PI / 180.0;
		double sinY = sin(radY);
		double cosY = cos(radY);
		if (log)
			fprintf(log.mpf,"dy %d, rad %f, sin %f cos %f\n",
				deltaPoint.x, radY, sinY, cosY);
		CRotationMatrix3 roty("roty");
		roty.maMatrix[0][0] = cosY;
		roty.maMatrix[0][2] = -sinY;
		roty.maMatrix[2][0] = sinY;
		roty.maMatrix[2][2] = cosY;
		mRotationMatrix.Print(log.mpf);
		roty.Print(log.mpf);
		mRotationMatrix.Multiply(roty);
		obChanged = true;
		mRotationMatrix.Print(log.mpf);
		fprintf(log.mpf,"Determinant roty %f rm %f\n", roty.Determinant(), mRotationMatrix.Determinant());
	}

	if (obChanged)
	{
		double det = mRotationMatrix.Determinant();
		fprintf(log.mpf,"Determinant %f\n", det);
		//while (det < 0.95)
		//{
		//	mRotationMatrix.Multiply(1.1f);
		//	det = mRotationMatrix.Determinant();
		//}
		//fprintf(log.mpf,"Corrected Determinant %f\n", det);
		mRotationMatrix.OnChange();
	}
	return true;
}
void CVolumeRenderingIF::LogCommand(void)
{
	static int count = 0;
	char zBuf[128];
	sprintf_s(zBuf,sizeof(zBuf),"VR_Command_%03d", count++);
	CLogFile log("VolumeRenderingIF", zBuf);
	if (!log)
		return;

	mRotationMatrix.Print(log.mpf);

	fprintf(log.mpf,"RP_version       %d\n", mParams.RP_version);
	fprintf(log.mpf,"bits             %d\n", mParams.bits);
	fprintf(log.mpf,"zero_background  %d\n", mParams.zero_background);
	fprintf(log.mpf,"Sz2xy_mmscale    %f\n", mParams.Sz2xy_mmscale);
	fprintf(log.mpf,"Sz_spacing_mm    %f\n", mParams.Sz_spacing_mm);
	fprintf(log.mpf,"\n");

	for (int i = 0; i < 3; i++)
		fprintf(log.mpf,"Sbounds[%d]     %d - %d\n", i, mParams.Sbounds[2*i],mParams.Sbounds[2*i+1]);
	fprintf(log.mpf,"\n");

	for (int i = 0; i < 4; i++)
		fprintf(log.mpf,"C2S[%d]     %8.3f - %8.3f - %8.3f - %8.3f\n", i, mParams.C2S[4*i],
			mParams.C2S[4*i+1], mParams.C2S[4*i+2], mParams.C2S[4*i+3]);
	fprintf(log.mpf,"\n");

	for (int i = 0; i < 4; i++)
		fprintf(log.mpf,"S2C[%d]     %8.3f - %8.3f - %8.3f - %8.3f\n", i, mParams.S2C[4*i],
			mParams.S2C[4*i+1], mParams.S2C[4*i+2], mParams.S2C[4*i+3]);
	fprintf(log.mpf,"\n");

	for (int i = 0; i < 4; i++)
		fprintf(log.mpf,"C2S_unit_rotmat[%d]     %8.3f - %8.3f - %8.3f - %8.3f\n", i, mParams.C2S_unit_rotmat[4*i],
			mParams.C2S_unit_rotmat[4*i+1], mParams.C2S_unit_rotmat[4*i+2], mParams.C2S_unit_rotmat[4*i+3]);
	fprintf(log.mpf,"\n");

	for (int i = 0; i < 3; i++)
		fprintf(log.mpf,"Cbounds[%d]     %d - %d\n", i, mParams.Cbounds[2*i],mParams.Cbounds[2*i+1]);
	fprintf(log.mpf,"\n");


}

#endif // __VR__