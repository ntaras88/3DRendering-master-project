#pragma once


#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <math.h>
#include <jni.h>

using namespace std;

#define RESIZED_WIDTH	600
#define RESIZED_HEIGHT	600
#define ROI				5
#define	PI				3.1415926535897932384626433832795


typedef struct _VECTOR3D_
{
	_VECTOR3D_() {

	}

	_VECTOR3D_(double x, double y, double z) {
		val[0] = x; val[1] = y; val[2] = z;
	}
	double	val[3];
}VECTOR3D, *PVECTOR3D;

typedef struct _VERTEX_
{
	_VERTEX_() {
		val[0] = 0; val[1] = 0; val[2] = 0; val[3] = 1;
		ntriCnt = 0;
		optIndex = -1;
	}

	_VERTEX_(double x, double y, double z) {
		val[0] = x; val[1] = y; val[2] = z; val[3] = 1;
		ntriCnt = 0;
		optIndex = -1;
	}
	double		val[4];
	VECTOR3D	nor;
	int			ntriCnt;
	int			optIndex;
}VERTEX, *PVERTEX;

typedef struct _TRIANGLE_
{
	_TRIANGLE_() {
		idx[0] = -1; idx[1] = -1; idx[2] = -1;
	}

	_TRIANGLE_(int idx1, int idx2, int idx3) {
		idx[0] = idx1; idx[1] = idx2; idx[2] = idx3;
	}
	int		idx[3];
	double	param[4];
}TRIANGLE, *PTRIANGLE;

typedef struct _SUBNODE_
{
	vector<VERTEX>		vertex;
	vector<TRIANGLE>	index;
}SUBNODE, *PSUBNODE;

typedef struct _MATRIX_
{
	double val[4][4];
}MATRIX, *PMATRIX;

enum FBXENUM
{
	SUCCESS_IMPORT,
	INVALID_FBX_FILE,
	UNSURPPORTED_FBX_VERSION,
	UNSURPPORTED_PROPERTY_TYPE,
	INVALID_VERTEX_PAIR,
	INVALID_POLYGON_PAIR,
	COMPRESSED_PROPERTY_FOUND
};


extern vector<SUBNODE>	nodes;

extern double	depth[RESIZED_WIDTH][RESIZED_HEIGHT];
extern int		indexMap1[RESIZED_WIDTH][RESIZED_HEIGHT];
extern int		indexMap2[RESIZED_WIDTH][RESIZED_HEIGHT];
extern VERTEX	vertMap[RESIZED_WIDTH][RESIZED_HEIGHT];
extern char		renderImg[RESIZED_HEIGHT][RESIZED_WIDTH * 4];
extern jint		renderResult[RESIZED_HEIGHT * RESIZED_WIDTH * 4];

extern double	maxDepth;
extern double	minDepth;
extern double	x_Rotate;
extern double	y_Rotate;
extern double	z_Rotate;
extern double	zoomFactor;
extern double	normalFactor;
extern double	roi_Depth;
extern int		gridInv;

extern vector<SUBNODE>	nodes;
extern vector<SUBNODE>	tNodes;
extern vector<SUBNODE>	eNodes;

extern VECTOR3D tDirV;
extern float	gridThickness;
extern int		gridNum;

void init3DOpt();
void OptimizeMesh(double optRate);

//3D Opt
void correctCenter();
void doFindEuqation();
void doRender(bool bFast);
void init_Render(bool bRelative);
void normalizeDepth();
void doSubodeTransform(SUBNODE *node, VECTOR3D tr, VECTOR3D rt, VECTOR3D sc);
void doProjection(VERTEX *a);
VERTEX doCross(VERTEX *a, VERTEX *b);
VECTOR3D doCross(VECTOR3D *a, VECTOR3D *b);
void doNormalize(VERTEX *v);
void doNormalize(VECTOR3D *v);
double doMagnitude(VECTOR3D *v);
double doMagnitudeForVertex(VERTEX *v);
void doProjection(VECTOR3D *a);
VERTEX doMinus(VERTEX *a, VERTEX *b);
VECTOR3D doMinus(VECTOR3D *a, VECTOR3D *b);
void DrawAllNormal(int interval);
void DrawMesh();
void DrawGrid();

class libImgCvt
{
public:
	const char * getPlatformABI();
	libImgCvt();
	~libImgCvt();
};

