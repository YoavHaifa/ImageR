#pragma once
#include "MetaFile.h"

typedef unsigned char    uint8_t;
typedef unsigned short   uint16_t;
typedef unsigned int     uint32_t;
typedef unsigned __int64 uint64_t;
typedef signed   short   int16_t;

typedef struct S7200RHeader
{
    uint8_t    NumOfSlices; // 0
    uint8_t    CtType; // 1
    uint16_t   DmsStatus; // 2
    uint16_t   DmsError; // 4
    uint16_t   IntegrationTime; // 6
    uint16_t   ReferenceDetector; // 8
    uint16_t   DetectorTemp1; // 10
    uint16_t   DetectorTemp2; // 12
    uint16_t   DetectorTemp3; // 14
    uint32_t   ReadingNumber; // 16
    uint16_t   ASICmodelow; // 18
    uint16_t   ASICmodeHigh; // 20
    uint16_t   DAS_cnt1; // 22
    uint16_t   DAS_cnt2; // 24
    float      TableExtrapPos; // 26
    // Fix me, int16_t or uint16_t
    int16_t    TablePosition; // 30
    uint16_t   GantryTilt; // 32
    uint16_t   RotationAngle; //34
    uint16_t   ReconAngle; // 36
    uint16_t   HWlines;
    uint16_t   Res5;
    uint32_t   RcomTimestamp;
    uint16_t   UIDuniqueId;
    // Added ShotNumber -- X.H.
    // uint16_t   Res6[31];
    uint16_t   ShotNumber;
    uint32_t   CardiacTimestamp;
    uint32_t   Timestamp;
    uint16_t   DetSamplesPerSlice;
    uint16_t   Res6[25];
    // Since we only use 24 bits of this field, changed to uint32_t to match SGL -- X.H.
    // int32_t    ReadingNumber;
    uint32_t   ReadingNumber2;  // Reading Number of the Header Footer - See GRIF for details
    uint16_t   SliceNum;
    uint16_t   Res7;
    uint16_t   Res8;
    uint16_t   Res9;
    uint16_t   Res10;
    uint16_t   Stamp;

    void PrintExcelHeader(FILE *pf)
    {
        fprintf (pf, "NumOfSlices");
        fprintf (pf, "\tCtType");

        fprintf (pf, "\tDmsStatus");
        fprintf (pf, "\tDmsError");
        fprintf (pf, "\tIntegrationTime");
        fprintf (pf, "\tReferenceDetector");
        fprintf (pf, "\tDetectorTemp1");
        fprintf (pf, "\tDetectorTemp2");
        fprintf (pf, "\tDetectorTemp3");
        fprintf (pf, "\tReadingNumber");

        fprintf (pf, "\tASICmodelow");
        fprintf (pf, "\tASICmodeHigh");
        fprintf (pf, "\tDAS_cnt1");
        fprintf (pf, "\tDAS_cnt2");
        fprintf (pf, "\tTableExtrapPos");
        fprintf (pf, "\tTablePosition");
        fprintf (pf, "\tGantryTilt");
        fprintf (pf, "\tRotationAngle");
        fprintf (pf, "\tReconAngle");

        fprintf (pf, "\tHWlines");
        fprintf (pf, "\tRcomTimestamp");
        fprintf (pf, "\tUIDuniqueId");
        fprintf (pf, "\tShotNumber");
        fprintf (pf, "\tCardiacTimestamp");
        fprintf (pf, "\tTimestamp");
        fprintf (pf, "\tDetSamplesPerSlice");
        fprintf (pf, "\tReadingNumber2");
        fprintf (pf, "\tSliceNum");
        fprintf (pf, "\tStamp");
        fprintf (pf, "\n");
    }
    void PrintExcel(FILE *pf)
    {
        fprintf (pf, "%u", NumOfSlices);
        fprintf (pf, "\t%u", CtType);

        fprintf (pf, "\t%u", DmsStatus);
        fprintf (pf, "\t%u", DmsError);
        fprintf (pf, "\t%u", IntegrationTime);
        fprintf (pf, "\t%u", ReferenceDetector);
        fprintf (pf, "\t%u", DetectorTemp1);
        fprintf (pf, "\t%u", DetectorTemp2);
        fprintf (pf, "\t%u", DetectorTemp3);
        fprintf (pf, "\t%u", ReadingNumber);

        fprintf (pf, "\t%u", ASICmodelow);
        fprintf (pf, "\t%u", ASICmodeHigh);
        fprintf (pf, "\t%u", DAS_cnt1);
        fprintf (pf, "\t%u", DAS_cnt2);
        fprintf (pf, "\t%f", TableExtrapPos);
        fprintf (pf, "\t%u", TablePosition);
        fprintf (pf, "\t%u", GantryTilt);
        fprintf (pf, "\t%u", RotationAngle);
        fprintf (pf, "\t%u", ReconAngle);

        fprintf (pf, "\t0x%x", HWlines);
        fprintf (pf, "\t%u", RcomTimestamp);
        fprintf (pf, "\t%u", UIDuniqueId);
        fprintf (pf, "\t%u", ShotNumber);
        fprintf (pf, "\t%u", CardiacTimestamp);
        fprintf (pf, "\t%u", Timestamp);
        fprintf (pf, "\t%u", DetSamplesPerSlice);
        fprintf (pf, "\t%u", ReadingNumber2);
        fprintf (pf, "\t%u", SliceNum);
        fprintf (pf, "\t0x%x", Stamp);
        fprintf (pf, "\n");
    }
    void Print(FILE *pf)
    {
        fprintf (pf, "NumOfSlices\t%u\n", NumOfSlices);
        fprintf (pf, "CtType\t%u\n", CtType);

        fprintf (pf, "DmsStatus\t%u\n", DmsStatus);
        fprintf (pf, "DmsError\t%u\n", DmsError);
        fprintf (pf, "IntegrationTime\t%u\n", IntegrationTime);
        fprintf (pf, "ReferenceDetector\t%u\n", ReferenceDetector);
        fprintf (pf, "DetectorTemp1\t%u\n", DetectorTemp1);
        fprintf (pf, "DetectorTemp2\t%u\n", DetectorTemp2);
        fprintf (pf, "DetectorTemp3\t%u\n", DetectorTemp3);
        fprintf (pf, "ReadingNumber\t%u\n", ReadingNumber);

        fprintf (pf, "ASICmodelow\t%u\n", ASICmodelow);
        fprintf (pf, "ASICmodeHigh\t%u\n", ASICmodeHigh);
        fprintf (pf, "DAS_cnt1\t%u\n", DAS_cnt1);
        fprintf (pf, "DAS_cnt2\t%u\n", DAS_cnt2);
        fprintf (pf, "TableExtrapPos\t%f\n", TableExtrapPos);
        fprintf (pf, "TablePosition\t%u\n", TablePosition);
        fprintf (pf, "GantryTilt\t%u\n", GantryTilt);
        fprintf (pf, "RotationAngle\t%u\n", RotationAngle);
        fprintf (pf, "ReconAngle\t%u\n", ReconAngle);

        fprintf (pf, "HWlines\t0x%x\n", HWlines);
        fprintf (pf, "RcomTimestamp\t%u\n", RcomTimestamp);
        fprintf (pf, "UIDuniqueId\t%u\n", UIDuniqueId);
        fprintf (pf, "ShotNumber\t%u\n", ShotNumber);
        fprintf (pf, "CardiacTimestamp\t%u\n", CardiacTimestamp);
        fprintf (pf, "Timestamp\t%u\n", Timestamp);
        fprintf (pf, "DetSamplesPerSlice\t%u\n", DetSamplesPerSlice);
        fprintf (pf, "ReadingNumber2\t%u\n", ReadingNumber2);
        fprintf (pf, "SliceNum\t%u\n", SliceNum);
        fprintf (pf, "Stamp\t0x%x\n", Stamp);
        fprintf (pf, "\n");
    }
}S7200RHeader;

class CReadingsFile :
    public CMetaFile
{
    friend class CReadingPosition;

public:
    CReadingsFile(const char *zfName);
    ~CReadingsFile(void);

    virtual int GetFanHeaderLen(void) {return mlFanHeaderBytes;}
    virtual int GetNFFS(void) {return mnFFS;}
    virtual int GetSliceLen(void) {return mSliceLen;}
    virtual int GetNSlices(void) {return mnSlices;}
    virtual int GetSliceLenWithoutFooter(void) {return mSliceLenWithoutFooter;}
    virtual CString GetPageText (void) {return CString("Slice");}
    virtual bool DisableZoom(void){return true;}
    virtual CSize DefaultImageSize(void);
    virtual int GetSpecTextSize(void){return 150;}
    virtual int GetDataTextSize(void){return 40;}
    virtual void ParsePosition(class CXMLParseNode *pPosXmlNode);
    virtual void PrintReadingHeaders(void);
    virtual bool IsReadingsFile() {return true;}
	int GetNFrames(void){return mnFrames;}
	unsigned int GetNDetectors(void){return mnDetectors;}
	void DisplaySinus(float initialAngle, float maxDetectorDistanceFromCenter, float deltaAngle);
    void EditRaw(void);

protected:
    unsigned long mSliceLenWithoutFooter;
    unsigned long mlFanHeaderBytes;
    unsigned long mnSlices;
    unsigned long mnFFS;
    unsigned long mSliceLen;
    unsigned long mReadingSize;
    unsigned long mnFrames;
    unsigned long mFrameSize;

    unsigned int mDetectorSize;
	unsigned int mnDetectors;

public:
    ULONGLONG GetOffsetToReading(int iFrame);
    virtual bool DoUpdatePageData(void);
    void ComputeReadingSize(void);
    virtual void InitPosition(void);
    virtual void SaveState(class CXMLDump & xmlDump);
	int miFirstReadingInCurrentPage;
	static bool umbPrintBedPosition;
	static bool umbEditConstantPitch;
	static bool umbEditSkipStart;
};
