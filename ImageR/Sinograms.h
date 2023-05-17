#pragma once
#include "metadata.h"

class CSinograms :
    public CMetaData
{
public:
    typedef enum ESinoType
    {
        SINO_UNDEF,
        SINO_SINOGRAMS,
        SINO_ATTENUATION,
        SINO_CORRECTED_IMAGE,
        SINO_LAST
    } ESinoType;

    class CBeamHardeningConfig
    {
    public:
        CBeamHardeningConfig(void);

        int mnSinograms;
        int mCorrectionThreshold;
        float mCorrectionFactor;

        bool mbUseSpecialParamsForL1;
        int mCorrectionThresholdL1;
        float mCorrectionFactorL1;

        bool mbAddedToParamsControl;
        int mXmlDumpId;

        void AddToParamsControl(int maxNSinograms);
        void SaveState(class CXMLDump & xmlDump);
        void ResumeState(class CXMLParseNode *pXmlNode);
    };

    static CBeamHardeningConfig umConfig;

    CSinograms(CMetaData *pImage); // Sinograms
    CSinograms(CSinograms *pSinograms); // Attenuation image
    CSinograms(CMetaData *pImage, CSinograms *pSinograms); // Attenuation image
    CSinograms(class CXMLParseNode *pXmlNode);
    ~CSinograms(void);

    virtual bool DoUpdatePageData(void);
    virtual CString GetWindowName(void);
    virtual CString GetZoomName(void);
    virtual float GetBias (void) {return mpImage ? mpImage->GetBias() : 0.0f;}
    virtual bool ShouldUpdate(void);
    virtual void SaveState(class CXMLDump & xmlDump);

    CSinograms *mpSinograms;
    CMetaData *mpImage;
    ESinoType meType;

private:
    void ComputeSinogramFromLines(float * pSinogram, float theta);
    void ComputeSinogramFromCols(float * pSinogram, float theta);

    bool UpdateAttenuation(void);
    bool UpdateCorrection(void);

    float mDeltaTheta;

    int mCorrectionThreshold;
    float mCorrectionFactor;

public:
    int mnImageLines;
    float mImageLineCenter;
    int mnImageCols;
    int mCount;
    bool mbPrint;
    float mXStart;
    float mXEnd;
    float mMidSino;
    int mnOut;
    float mImageColCenter;
    int mnSinogramsComputed;
    int miFirstSinogramLine;
    int mCorrectThresholdDone;
    float mCorrectionFactorDone;
    void ComputeCorrectionParameters(void);
    void InitOnConstruction(void);
};
