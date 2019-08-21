#pragma once
#include "MetaData.h"
#include "RotationMatrix.h"
#include "D:\Views\GRAD\ViewingInfra\Src\VR2Solution\VR2.h"

class CVolumeRenderingIF :
    public virtual CMetaData
{
public:
	CVolumeRenderingIF(CMetaData *pData);
	~CVolumeRenderingIF(void);

    virtual bool DoUpdatePageData(void);
	virtual bool SpecialMouseScroll(CPoint point, bool & obChanged);

private:
	bool mbFailed;
	CMetaData *mpSrcData;
	render_params mParams;
	unsigned short *mpDataInRam;
	int mnInputSlices;
	unsigned short **mapRasters;
	float mSiceDeltaMm;

	int maSBounds [3][2];
	int maCBounds [3][2];

	// Working Set of three matrices
	CRotationMatrix3 mRotationMatrix;
	CRotationMatrix4 mRotationMatrix4;
	CRotationMatrix4 mShiftSource;
	CRotationMatrix4 mShiftCamera;

	CRotationMatrix3 mRotationMatrixInv;
	CRotationMatrix4 mRotationMatrix4Inv;
	CRotationMatrix4 mShiftSourceInv;
	CRotationMatrix4 mShiftCameraInv;

	CRotationMatrix4 mS2C;
	CRotationMatrix4 mC2S;
	CRotationMatrix4 mC2SUnitRotMat;

	bool LoadAllImages(void);
	void InitParams(void);
public:
	void LogCommand(void);
	int mnThreads;
};
