#include "StdAfx.h"
#include ".\ggraph.h"
#include "DisplayRectangle.h"
#include "MetaData.h"
#include "..\..\yUtils\XMLParse.h"
#include "DisplayRectangle.h"
#include "GraphicElementOnDR.h"

CGGraph::CGGraph(class CDisplayRectangle *pDispRect, class CXMLParseNode *pXml)
: CGraphicElement(pDispRect->GetCurrentData()->PageData(), pXml)
{
    CXMLParseNode *pLineXml = pXml->GetDFS("graph_line");
    while (pLineXml)
    {
        SLineInfo info (pLineXml);
        mLines.AddTail (info);

        pLineXml = pXml->GetNextDFS("graph_line", &pLineXml);
    }
}

CGGraph::~CGGraph(void)
{
}

CGGraph::SLineInfo::SLineInfo(class CXMLParseNode *pXml)
{
    pXml->GetValue("name", msName);
    pXml->GetValue("n_points", mnPoints);
    pXml->GetColor(mColor);
}
bool CGGraph::SpecialDraw (CDisplayRectangle &displayRect, CDC &dc, CGPoint *apPointsOnDR[])
{
    int iFirstPerLine = 0;
    POSITION pos = mLines.GetHeadPosition();
    while (pos)
    {
        SLineInfo lineInfo = mLines.GetNext(pos);
        int iLastperLine = iFirstPerLine + lineInfo.mnPoints - 1;

        for (int i = iFirstPerLine; i < iLastperLine; i++)
            displayRect.DrawLine (dc, *apPointsOnDR[i], *apPointsOnDR[i+1], lineInfo.mColor);

        iFirstPerLine += lineInfo.mnPoints;
    }
    return true;
}