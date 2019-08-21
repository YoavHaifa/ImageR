#pragma once
#include "..\..\ImageRLib\GraphicElement.h"
#include "..\..\ImageRLib\TImage.h"
#include "afxtempl.h"

class CGContourEdge
{
public:
	CGContourEdge(int line1, int pix1, int line2, int pix2)
		: dc1((float)pix1, (float)line1)
		, dc2((float)pix2, (float)line2)
	{
	}

	CDataCoordinates dc1;
	CDataCoordinates dc2;
};

class CGContour :
    public CGraphicElement
{
public:
    CGContour(class CDisplayRectangle *pDispRect, const char *zName, COLORREF color = 0x80ffff);
    CGContour(class CDisplayRectangle *pDispRect, class CDataVolume *pVolume, COLORREF color = 0x80ffff);
    CGContour(class CDisplayRectangle *pDispRect, class CXMLParseNode *pGRoiXml);
    CGContour(class CDisplayRectangle *pDispRect, class CSharedMemFile *pSharedMemFile);
    virtual ~CGContour(void);

	virtual bool DrawPointless(class CDisplayRectangle *pDispRect, CDC &dc);
    virtual void SaveState(class CXMLDump & xmlDump);
    //virtual bool Drag(CRect rect, CSize size, CGPoint ** /*ppPointsOnDR*/);
    virtual bool NewDrawOnDrag(class CDisplayRectangle *pDisplayRectangle);

private:
	CDataCoordinates mSeed;
	class CRebm *mpRebm;
    CTImage<bool> *mpFull;
    CTImage<bool> *mpEdges;

	class CSharedMemFile *mpSharedMemFile;

	bool ComputeEdges(void);

    CList<CGContourEdge*,CGContourEdge*> mEdgesToDraw;

	void InitEdgesRaster(class CDisplayRectangle *pDispRect);
	void DrawEdges(class CDisplayRectangle *pDispRect, CDC &dc);
	bool ComputeEdgesToDraw(void);
	void AddGraphPoints(class CDisplayRectangle *pDispRect);
	void AddEdgeToDraw(int line1, int pix1, int line2, int pix2);
public:
	void DeleteEdges(void);
	bool ComputeRebmFromSeed(class CDisplayRectangle *pDisplayRectangle);
	CDataVolume *mpDataVolume;
	bool UpdateRebmFromShared(void);
};
