#include "StdAfx.h"
#include "RotationMatrix.h"
#include "..\..\yUtils\LogFile.h"
#include "..\..\yUtils\MyMath.h"

#define M_PI       3.14159265358979323846

//-------------------------------------------------------------
// CRotationMatrix
//-------------------------------------------------------------

CString CRotationMatrix::umsPrintFormat(_T(" %12.3f"));
bool CRotationMatrix::umbPrintDeterminant = false;

CRotationMatrix::CRotationMatrix(const char *zName)
: CNamedObject(zName)
{
}
CRotationMatrix::~CRotationMatrix(void)
{
}

//-------------------------------------------------------------
// CRotationMatrix3
//-------------------------------------------------------------

CRotationMatrix3 CRotationMatrix3::umIdent("Ident3x3");
CRotationMatrix4 CRotationMatrix4::umIdent("Ident4x4");

CRotationMatrix3::CRotationMatrix3(const char *zName)
: CRotationMatrix(zName)
{
	memset(maMatrix[0],0,sizeof(maMatrix));
	maMatrix[0][0] = 1;
	maMatrix[1][1] = 1;
	maMatrix[2][2] = 1;
}
CRotationMatrix3::CRotationMatrix3(const CRotationMatrix3 &other)
: CRotationMatrix(other.msName)
{
	memcpy(maMatrix[0],other.maMatrix[0],sizeof(maMatrix));
}
CRotationMatrix3::~CRotationMatrix3(void)
{
}
double *CRotationMatrix3::GetStart(void)
{
	return maMatrix[0];
}
double CRotationMatrix3::Determinant(void)const
{
	double d1 = maMatrix[0][0]*maMatrix[1][1]*maMatrix[2][2];
	double d2 = maMatrix[0][1]*maMatrix[1][2]*maMatrix[2][0];
	double d3 = maMatrix[0][2]*maMatrix[1][0]*maMatrix[2][1];
	double e1 = maMatrix[0][2]*maMatrix[1][1]*maMatrix[2][0];
	double e2 = maMatrix[0][1]*maMatrix[1][0]*maMatrix[2][2];
	double e3 = maMatrix[0][0]*maMatrix[1][2]*maMatrix[2][1];
	return d1 + d2 + d3 - e1 - e2 - e3;
}
bool CRotationMatrix3::Inverse(const CRotationMatrix3 &other)
{
	double det = other.Determinant();
	double epsilon = 0.000001;
	if (det < epsilon && det > -epsilon)
		return false;

	double a = other.maMatrix[0][0];
	double b = other.maMatrix[0][1];
	double c = other.maMatrix[0][2];

	double d = other.maMatrix[1][0];
	double e = other.maMatrix[1][1];
	double f = other.maMatrix[1][2];

	double g = other.maMatrix[2][0];
	double h = other.maMatrix[2][1];
	double k = other.maMatrix[2][2];

	maMatrix[0][0] = (e*k - f*h)/det;
	maMatrix[0][1] = (c*h - b*k)/det;
	maMatrix[0][2] = (b*f - c*e)/det;

	maMatrix[1][0] = (f*g - d*k)/det;
	maMatrix[1][1] = (a*k - c*g)/det;
	maMatrix[1][2] = (c*d - a*f)/det;

	maMatrix[2][0] = (d*h - e*g)/det;
	maMatrix[2][1] = (g*b - a*h)/det;
	maMatrix[2][2] = (a*e - b*d)/det;
	return true;
}
void CRotationMatrix3::Multiply(const CRotationMatrix3 &a, const CRotationMatrix3 &b)
{
	for (int iRow = 0; iRow < 3; iRow++)
	{
		for (int iCol = 0; iCol < 3; iCol++)
		{
			double sum = 0;
			for (int i = 0; i < 3; i++)
				sum += a.maMatrix[iRow][i] * b.maMatrix[i][iCol];

			maMatrix[iRow][iCol] = sum;
		}
	}
}
void CRotationMatrix3::Multiply(const CRotationMatrix3 &other)
{
	CRotationMatrix3 tmp(*this);
	Multiply(tmp,other);
}
void CRotationMatrix3::Multiply(float factor)
{
	for (int iRow = 0; iRow < 3; iRow++)
	{
		for (int iCol = 0; iCol < 3; iCol++)
		{
			maMatrix[iRow][iCol] *= factor;
		}
	}
}
double CRotationMatrix3::MaxDiff(const CRotationMatrix3 &other)
{
	double maxDiff = 0;
	for (int iRow = 0; iRow < 3; iRow++)
	{
		for (int iCol = 0; iCol < 3; iCol++)
		{
			double diff = CMyMath::AbsDiff(maMatrix[iRow][iCol], other.maMatrix[iRow][iCol]);
			if (diff > maxDiff)
				maxDiff = diff;
		}
	}
	return maxDiff;
}
void CRotationMatrix3::SetIdent(void)
{
	for (int iRow = 0; iRow < 3; iRow++)
	{
		for (int iCol = 0; iCol < 3; iCol++)
		{
			if (iRow == iCol)
				maMatrix[iRow][iCol] = 1;
			else
				maMatrix[iRow][iCol] = 0;
		}
	}
}
void CRotationMatrix3::SetRandom(double dMin, double dMax)
{
	double span = dMax - dMin;
	for (int iRow = 0; iRow < 3; iRow++)
	{
		for (int iCol = 0; iCol < 3; iCol++)
			maMatrix[iRow][iCol] = dMin + rand() * span / RAND_MAX;
	}
}
void CRotationMatrix3::Print(FILE *pf)const
{
	fprintf(pf, "<CRotationMatrix3> %s\n", (const char*)msName);
	for (int iRow = 0; iRow < 3; iRow++)
	{
		for (int iCol = 0; iCol < 3; iCol++)
			fprintf(pf, umsPrintFormat, maMatrix[iRow][iCol]);
		fprintf(pf, "\n");
	}
	if (umbPrintDeterminant)
		fprintf(pf, "Determinant %f\n", Determinant());
	fprintf(pf, "\n");
}
double CRotationMatrix3::TestInverse(const CRotationMatrix3 &a, FILE *pf)
{
	a.Print(pf);

	fprintf(pf,"Determinant %f\n\n", a.Determinant());


	CRotationMatrix3 b("b");
	b.Inverse(a);
	b.Print(pf);

	CRotationMatrix3 result("result");
	result.Multiply(a,b);
	result.Print(pf);

	double maxDiff = result.MaxDiff(umIdent);

	fprintf(pf,"MaxDiff %f\n", maxDiff);
	fprintf(pf,"----------------------------\n");
	return maxDiff;
}
void CRotationMatrix3::TestInverse(int n)
{
	CLogFile log("CRotationMatrix3__TestInverse");

	CRotationMatrix3 id("id");
	double maxDiff = TestInverse(id, log.mpf);
	int iMax = 0;

	for (int i = 1; i <= n; i++)
	{
		CRotationMatrix3 a("a");
		a.SetRandom(0,10);
		double diff = TestInverse(a, log.mpf);
		if (diff > maxDiff)
		{
			fprintf(log.mpf, "New Max Diff at %d\n\n", i);
			iMax = i;
			maxDiff = diff;
		}
	}
	fprintf(log.mpf, "Max Diff %f at %d\n\n", maxDiff, iMax);
}
//-------------------------------------------------------------
// CRotationMatrix4
//-------------------------------------------------------------
 
CRotationMatrix4::CRotationMatrix4(const char *zName)
: CRotationMatrix(zName)
{
	SetIdent();
}
CRotationMatrix4::CRotationMatrix4(const CRotationMatrix4 &other)
: CRotationMatrix(other.msName)
{
	memcpy(maMatrix[0],other.maMatrix[0],sizeof(maMatrix));
}
CRotationMatrix4::~CRotationMatrix4(void)
{
}
void CRotationMatrix4::SetIdent(void)
{
	memset(maMatrix[0],0,sizeof(maMatrix));
	maMatrix[0][0] = 1;
	maMatrix[1][1] = 1;
	maMatrix[2][2] = 1;
	maMatrix[3][3] = 1;
}
void CRotationMatrix4::SetTransform(double x, double y, double z)
{
	SetIdent();
	maMatrix[3][0] = x;
	maMatrix[3][1] = y;
	maMatrix[3][2] = z;
}
void CRotationMatrix4::SetInverseTransform(const CRotationMatrix4 &other)
{
	SetIdent();
	maMatrix[3][0] = - other.maMatrix[3][0];
	maMatrix[3][1] = - other.maMatrix[3][1];
	maMatrix[3][2] = - other.maMatrix[3][2];
}
double *CRotationMatrix4::GetStart(void)
{
	return maMatrix[0];
}
CRotationMatrix4 & CRotationMatrix4::operator =(CRotationMatrix3 &other3x3)
{
	SetIdent();
	for (int iRow = 0; iRow < 3; iRow++)
	{
		for (int iCol = 0; iCol < 3; iCol++)
		{
			maMatrix[iRow][iCol] = other3x3.maMatrix[iRow][iCol];
		}
	}
	return *this;
}
CRotationMatrix4 & CRotationMatrix4::operator =(CRotationMatrix4 &other)
{
	for (int iRow = 0; iRow < 4; iRow++)
	{
		for (int iCol = 0; iCol < 4; iCol++)
		{
			maMatrix[iRow][iCol] = other.maMatrix[iRow][iCol];
		}
	}
	return *this;
}
void CRotationMatrix4::Multiply(const CRotationMatrix4 &a, const CRotationMatrix4 &b)
{
	for (int iRow = 0; iRow < 4; iRow++)
	{
		for (int iCol = 0; iCol < 4; iCol++)
		{
			double sum = 0;
			for (int i = 0; i < 4; i++)
				sum += a.maMatrix[iRow][i] * b.maMatrix[i][iCol];

			maMatrix[iRow][iCol] = sum;
		}
	}
}
void CRotationMatrix4::Multiply(const CRotationMatrix4 &other)
{
	CRotationMatrix4 tmp(*this);
	Multiply(tmp, other);
}
double CRotationMatrix4::MaxDiff(const CRotationMatrix4 &other)
{
	double maxDiff = 0;
	for (int iRow = 0; iRow < 4; iRow++)
	{
		for (int iCol = 0; iCol < 4; iCol++)
		{
			double diff = CMyMath::AbsDiff(maMatrix[iRow][iCol], other.maMatrix[iRow][iCol]);
			if (diff > maxDiff)
				maxDiff = diff;
		}
	}
	return maxDiff;
}
void CRotationMatrix4::Print(FILE *pf)const
{
	fprintf(pf, "<CRotationMatrix4> %s\n", (const char*)msName);
	for (int iRow = 0; iRow < 4; iRow++)
	{
		for (int iCol = 0; iCol < 4; iCol++)
			fprintf(pf, umsPrintFormat, maMatrix[iRow][iCol]);
		fprintf(pf, "\n");
	}
	fprintf(pf, "\n");
}
double CRotationMatrix4::TestInverse(const CRotationMatrix4 &a, const CRotationMatrix4 &b, FILE *pf)
{
	a.Print(pf);
	//fprintf(pf,"Determinant %f\n\n", a.Determinant());

	b.Print(pf);

	CRotationMatrix4 result("result");
	result.Multiply(a,b);
	result.Print(pf);

	double maxDiff = result.MaxDiff(umIdent);

	fprintf(pf,"MaxDiff %f\n", maxDiff);
	fprintf(pf,"----------------------------\n");
	return maxDiff;
}
void CRotationMatrix4::TestInverse(int /* n */)
{
	CLogFile log("CRotationMatrix4__TestInverse");
	fprintf (log.mpf, "============== Inverting Ident \n");
	CRotationMatrix4 ident("Ident");
	TestInverse(ident,ident,log.mpf);

	fprintf (log.mpf, "\n============== Inverting Trans \n");
	double x = (double)(rand() % 10000) / 10;
	double y = (double)(rand() % 10000) / 10;
	double z = (double)(rand() % 10000) / 10;
	CRotationMatrix4 trans("trans");
	trans.SetTransform(x,y,z);

	CRotationMatrix4 transInv("transInv");
	transInv.SetTransform(-x,-y,-z);
	TestInverse(trans,transInv,log.mpf);

	fprintf (log.mpf, "\n============== Inverting Trans 1 \n");
	x = (double)(rand() % 10000) / 10;
	y = (double)(rand() % 10000) / 10;
	z = (double)(rand() % 10000) / 10;
	CRotationMatrix4 trans1("trans1");
	trans1.SetTransform(x,y,z);

	CRotationMatrix4 transInv1("transInv1");
	transInv1.SetTransform(-x,-y,-z);
	TestInverse(trans1,transInv1,log.mpf);

	fprintf (log.mpf, "\n============== Inverting 3*3 \n");
	CRotationMatrix3 rot3("rot3");
	rot3.SetRandom(0.1,10);
	CRotationMatrix3 rot3i("rot3i");
	rot3i.Inverse(rot3);
	CRotationMatrix4 rot4("rot4");
	rot4 = rot3;
	CRotationMatrix4 rot4i("rot4i");
	rot4i = rot3i;
	TestInverse(rot4,rot4i,log.mpf);

	fprintf (log.mpf, "\n============== Inverting (trans * 3*3rot)\n");
	CRotationMatrix4 combined("combined");
	combined.Multiply(trans, rot4);

	CRotationMatrix4 combinedInverse("combinedInverse");
	combinedInverse.Multiply(rot4i,transInv);
	TestInverse(combined,combinedInverse,log.mpf);

	fprintf (log.mpf, "\n============== Inverting (trans * trans1)\n");
	combined.Multiply(trans, trans1);

	combinedInverse.Multiply(transInv1,transInv);
	TestInverse(combined,combinedInverse,log.mpf);

	fprintf (log.mpf, "\n============== Inverting (trans * Ident * trans1)\n");
	combined.Multiply(trans, umIdent);
	combined.Multiply(trans1);

	combinedInverse.Multiply(transInv1,umIdent);
	combinedInverse.Multiply(transInv);
	TestInverse(combined,combinedInverse,log.mpf);

	fprintf (log.mpf, "\n============== Inverting (trans * 3*3rot * trans1)\n");
	combined.Multiply(trans, rot4);
	combined.Multiply(trans1);

	combinedInverse.Multiply(transInv1,rot4i);
	combinedInverse.Multiply(transInv);
	TestInverse(combined,combinedInverse,log.mpf);
}