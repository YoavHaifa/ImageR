#pragma once

#include "..\..\ImageRLib\NamedObject.h"

class CRotationMatrix :
    public CNamedObject
{
public:
	CRotationMatrix(const char *zName);
	virtual ~CRotationMatrix(void);

	static CString umsPrintFormat;
	static bool umbPrintDeterminant;
};

class CRotationMatrix3 :
    public CRotationMatrix
{
public:
	CRotationMatrix3(const char *zName);
	CRotationMatrix3(const CRotationMatrix3 &other);
	virtual ~CRotationMatrix3(void);

	double *GetStart(void);
	double Determinant(void)const;
	bool Inverse(const CRotationMatrix3 &other);
	void Multiply(const CRotationMatrix3 &a, const CRotationMatrix3 &b);
	void Multiply(const CRotationMatrix3 &other);
	void Multiply(float factor);

	// Debug functionality
	void SetRandom(double dMin, double dMax);
	void SetIdent(void);
	void Print(FILE *pf)const;
	static void TestInverse(int n);
	static double TestInverse(const CRotationMatrix3 &a, FILE *pf);
	double MaxDiff(const CRotationMatrix3 &other);

	static CRotationMatrix3 umIdent;

private:
	friend class CVolumeRenderingIF;
	friend class CRotationMatrix4;
	double maMatrix[3][3];
};
//------------------------------------------------------------------------
class CRotationMatrix4 :
    public CRotationMatrix
{
public:
	CRotationMatrix4(const char *zName);
	CRotationMatrix4(const CRotationMatrix4 &other);
	virtual ~CRotationMatrix4(void);

	void SetIdent(void);
	void SetTransform(double x, double y, double z);
	void SetInverseTransform(const CRotationMatrix4 &other);

	double *GetStart(void);
	CRotationMatrix4 & operator = (CRotationMatrix3 &other3x3);
	CRotationMatrix4 & operator = (CRotationMatrix4 &other);

	void Multiply(const CRotationMatrix4 &a, const CRotationMatrix4 &b);
	void Multiply(const CRotationMatrix4 &other);

	void Print(FILE *pf)const;
	static double TestInverse(const CRotationMatrix4 &a, const CRotationMatrix4 &b, FILE *pf);
	static void TestInverse(int n);
	double MaxDiff(const CRotationMatrix4 &other);

	static CRotationMatrix4 umIdent;

private:
	double maMatrix[4][4];
};

