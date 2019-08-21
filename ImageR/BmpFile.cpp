#include "StdAfx.h"
#include ".\bmpfile.h"
#include "..\..\yUtils\MyWindows.h"

CBmpFile::CBmpFile(const char *zName)
: CMetaFile (zName)
{
    typedef struct {
        //unsigned short int type;                 /* Magic identifier            */
        unsigned int size;                       /* File size in bytes          */
        unsigned short int reserved1, reserved2;
        unsigned int offset;                     /* Offset to image data, bytes */
    } BMP_HEADER;
    typedef struct {
        unsigned int size;               /* Header size in bytes      */
        int width,height;                /* Width and height of image */
        unsigned short int planes;       /* Number of colour planes   */
        unsigned short int bits;         /* Bits per pixel            */
        unsigned int compression;        /* Compression type          */
        unsigned int imagesize;          /* Image size in bytes       */
        int xresolution,yresolution;     /* Pixels per meter          */
        unsigned int ncolours;           /* Number of colours         */
        unsigned int importantcolours;   /* Important colours         */
    } BMP_INFOHEADER;

    short magic;
    BMP_HEADER hdr;
    BMP_INFOHEADER info_hdr;

    FILE *pf = MyFOpen (msName, "rb");
    if (!pf)
        return;

    size_t nRead = fread (&magic, sizeof(magic), 1, pf);
    nRead = fread (&hdr, sizeof(hdr), 1, pf);
    if (nRead)
    {
        nRead = fread (&info_hdr, sizeof(info_hdr), 1, pf);
        if (nRead)
        {
            mOffsetBytes = hdr.offset;
            mnCols = info_hdr.width;
            mnLinesInPage = info_hdr.height;
        }
    }
    fclose (pf);
}

CBmpFile::~CBmpFile(void)
{
}
