#include "ImgCvt.h"

MATRIX s_Mat;
MATRIX t_Mat;
MATRIX rX_Mat;
MATRIX rY_Mat;
MATRIX rZ_Mat;
MATRIX TMat;

vector<SUBNODE>	nodes;
vector<SUBNODE>	tNodes;
vector<SUBNODE>	eNodes;

double welding = 1e-1;
double maxDepth = 0;
double minDepth = 1e+10;
double x_Rotate = 0.52359877559829887307710723054658;
double y_Rotate = 0.52359877559829887307710723054658;
double z_Rotate = 0.52359877559829887307710723054658;
double normalFactor = 9.0f;
double zoomFactor = 1.1f;

double		depth[RESIZED_WIDTH][RESIZED_HEIGHT];
int			indexMap1[RESIZED_WIDTH][RESIZED_HEIGHT];
int			indexMap2[RESIZED_WIDTH][RESIZED_HEIGHT];
VERTEX		vertMap[RESIZED_WIDTH][RESIZED_HEIGHT]; 
char		renderImg[RESIZED_HEIGHT][RESIZED_WIDTH * 4];
double		roi_Depth = 1.0f;
int			gridInv = 0;

VECTOR3D dirV;
VECTOR3D tDirV;

void normalizeDepth() {
	maxDepth = -1;
	minDepth = 1e+10;
	for (int i = 0; i < RESIZED_HEIGHT; i++) {
		for (int j = 0; j < RESIZED_WIDTH; j++) {
			if (depth[i][j] > 0) {
				if (depth[i][j] > maxDepth) maxDepth = depth[i][j];
				if (depth[i][j] < minDepth) minDepth = depth[i][j];
			}
		}
	}

	for (int i = 0; i < RESIZED_HEIGHT; i++) {
		for (int j = 0; j < RESIZED_WIDTH; j++) {
			if (depth[i][j] > 0) {
				depth[i][j] = 255 - (depth[i][j] - minDepth) * 255.0f / (maxDepth - minDepth);
			}
		}
	}
}

double doMagnitudeForVertex(VERTEX *v) {
	return sqrt(v->val[0] * v->val[0] + v->val[1] * v->val[1] + v->val[2] * v->val[2]);
}

double doMagnitude(VECTOR3D *v) {
	return sqrt(v->val[0] * v->val[0] + v->val[1] * v->val[1] + v->val[2] * v->val[2]);
}

double doMagnitude(VERTEX *v) {
	return sqrt(v->val[0] * v->val[0] + v->val[1] * v->val[1] + v->val[2] * v->val[2]);
}

void GetBound(VERTEX *mx, VERTEX *mn, vector<SUBNODE> *nodes) {
	for (int n = 0; n < nodes->size(); n++) {
		for (int i = 0; i < nodes->at(n).vertex.size(); i++) {
			VERTEX v = nodes->at(n).vertex[i];
			if (n == 0 && i == 0)
			{
				*mn = v;
				*mx = v;
			}
			else{
				if (mn->val[0] > v.val[0]) mn->val[0] = v.val[0];
				if (mn->val[1] > v.val[1]) mn->val[1] = v.val[1];
				if (mn->val[2] > v.val[2]) mn->val[2] = v.val[2];
				if (mx->val[0] < v.val[0]) mx->val[0] = v.val[0];
				if (mx->val[1] < v.val[1]) mx->val[1] = v.val[1];
				if (mx->val[2] < v.val[2]) mx->val[2] = v.val[2];
			}
		}
	}
	normalFactor = doMagnitude(&doMinus(mx, mn)) / 20.0f;
}

void OptimizeMesh() {
	vector<SUBNODE>	nd;
	VERTEX mx, mn;

	GetBound(&mx, &mn, &nodes);
	welding = normalFactor / 10.0f;
	for (int i = 0; i < nodes.size(); i++) {
		SUBNODE sn;

		for (int j = 0; j < nodes[i].vertex.size(); j++) {
			VECTOR3D v1 = VECTOR3D(nodes[i].vertex[j].val[0], nodes[i].vertex[j].val[1], nodes[i].vertex[j].val[2]);
			nodes[i].vertex[j].optIndex = sn.vertex.size();
			bool flag = false;
			for (int k = 0; k < sn.vertex.size(); k++) {
				VECTOR3D v2 = VECTOR3D(sn.vertex[k].val[0], sn.vertex[k].val[1], sn.vertex[k].val[2]);
				double m = doMagnitude(&doMinus(&v1, &v2));
				if (m <= welding) {
					nodes[i].vertex[j].optIndex = k;
					flag = true;
					break;
				}
			}
			if (!flag) {
				sn.vertex.push_back(VERTEX(v1.val[0], v1.val[1], v1.val[2]));
			}
		}
		for (int j = 0; j < nodes[i].index.size(); j++) {
			TRIANGLE t = nodes[i].index[j];
			for (int k = 0; k < 3; k++) {
				t.idx[k] = nodes[i].vertex[t.idx[k]].optIndex;
			}
			if (t.idx[0] != t.idx[1] && t.idx[1] != t.idx[2] && t.idx[0] != t.idx[2]) {
				sn.index.push_back(t);
			}
		}
		nd.push_back(sn);
		nodes[i].index.clear();
		nodes[i].vertex.clear();
	}
	nodes.clear();
	for (int i = 0; i < nd.size(); i++) {
		SUBNODE sn;
		for (int j = 0; j < nd[i].vertex.size(); j++) {
			sn.vertex.push_back(nd[i].vertex[j]);
		}
		for (int j = 0; j < nd[i].index.size(); j++) {
			sn.index.push_back(nd[i].index[j]);
		}
		nodes.push_back(sn);
	}
	GetBound(&mx, &mn, &nodes);
	//FindConnectivity();
}

void init3DOpt() {
	gridInv = 3;
	roi_Depth = 2.0f;
	zoomFactor = 1.1f;
	x_Rotate = 0.0f;// 0.52359877559829887307710723054658;
	y_Rotate = 0.0f;// 0.52359877559829887307710723054658;
	z_Rotate = 0.0f;// 0.52359877559829887307710723054658;
	for (int i = 0; i < tNodes.size(); i++) {
		tNodes[i].index.clear();
		tNodes[i].vertex.clear();
	}
	tNodes.clear();
	for (int i = 0; i < eNodes.size(); i++) {
		eNodes[i].index.clear();
		eNodes[i].vertex.clear();
	}
	eNodes.clear();
	for (int i = 0; i < nodes.size(); i++) {
		nodes[i].index.clear();
		nodes[i].vertex.clear();
	}
	nodes.clear();
}

void init_Render() {
	minDepth = 1e+10;
	maxDepth = 0;
	for (int i = 0; i < tNodes.size(); i++) {
		tNodes[i].index.clear();
		tNodes[i].vertex.clear();
	}
	tNodes.clear();
	for (int n = 0; n < nodes.size(); n++) {
		SUBNODE sb;
		for (int m = 0; m < nodes[n].vertex.size(); m++) {
			VERTEX v = nodes[n].vertex[m];
			sb.vertex.push_back(v);
		}
		tNodes.push_back(sb);
	}
	for (int i = 0; i < RESIZED_HEIGHT; i++) {
		for (int j = 0; j < RESIZED_WIDTH; j++) {
			indexMap1[i][j] = -1;
			indexMap2[i][j] = -1;
		}
	}
}

VERTEX doMinus(VERTEX *a, VERTEX *b) {
	VERTEX r = VERTEX(a->val[0] - b->val[0], a->val[1] - b->val[1], a->val[2] - b->val[2]);

	return r;
}

VECTOR3D doMinus(VECTOR3D *a, VECTOR3D *b) {
	VECTOR3D r = VECTOR3D(a->val[0] - b->val[0], a->val[1] - b->val[1], a->val[2] - b->val[2]);

	return r;
}

VERTEX doCross(VERTEX *a, VERTEX *b) {
	VERTEX r = VERTEX(a->val[1] * b->val[2] - a->val[2] * b->val[1], 
		a->val[2] * b->val[0] - a->val[0] * b->val[2],
		a->val[0] * b->val[1] - a->val[1] * b->val[0]);

	return r;
}

VECTOR3D doCross(VECTOR3D *a, VECTOR3D *b) {
	VECTOR3D r = VECTOR3D(a->val[1] * b->val[2] - a->val[2] * b->val[1],
		a->val[2] * b->val[0] - a->val[0] * b->val[2],
		a->val[0] * b->val[1] - a->val[1] * b->val[0]);

	return r;
}

VECTOR3D doCross(VECTOR3D *a, VERTEX *b) {
	VECTOR3D r = VECTOR3D(a->val[1] * b->val[2] - a->val[2] * b->val[1],
		a->val[2] * b->val[0] - a->val[0] * b->val[2],
		a->val[0] * b->val[1] - a->val[1] * b->val[0]);

	return r;
}

void xRotateMatrix(double alpha) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) rX_Mat.val[i][j] = 1; else rX_Mat.val[i][j] = 0;
		}
	}
	rX_Mat.val[1][1] = cos(alpha); rX_Mat.val[1][2] = sin(alpha);
	rX_Mat.val[2][1] = -sin(alpha); rX_Mat.val[2][2] = cos(alpha);
}

void yRotateMatrix(double alpha) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) rY_Mat.val[i][j] = 1; else rY_Mat.val[i][j] = 0;
		}
	}
	rY_Mat.val[0][0] = cos(alpha); rY_Mat.val[0][2] = sin(alpha);
	rY_Mat.val[2][0] = -sin(alpha); rY_Mat.val[2][2] = cos(alpha);
}

void zRotateMatrix(double alpha) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) rZ_Mat.val[i][j] = 1; else rZ_Mat.val[i][j] = 0;
		}
	}
	rZ_Mat.val[0][0] = cos(alpha); rZ_Mat.val[0][1] = sin(alpha);
	rZ_Mat.val[1][0] = -sin(alpha); rZ_Mat.val[1][1] = cos(alpha);
}

void tMatrix(VECTOR3D t) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) t_Mat.val[i][j] = 1; else t_Mat.val[i][j] = 0;
		}
	}
	t_Mat.val[3][0] = t.val[0]; t_Mat.val[3][1] = t.val[1]; t_Mat.val[3][2] = t.val[2];
}

void sMatrix(VECTOR3D s) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) s_Mat.val[i][j] = 1; else s_Mat.val[i][j] = 0;
		}
	}
	s_Mat.val[0][0] = s.val[0]; s_Mat.val[1][1] = s.val[1]; s_Mat.val[2][2] = s.val[2];
}

MATRIX makeConcatination(MATRIX *a, MATRIX *b) {
	MATRIX ret;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			ret.val[i][j] = 0;
			for (int k = 0; k < 4; k++) {
				ret.val[i][j] += a->val[i][k] * b->val[k][j];
			}
		}
	}
	return ret;
}

void makeTransformMatrix(double rx, double ry, double rz) {
	xRotateMatrix(rx); yRotateMatrix(ry); zRotateMatrix(rz);
	TMat = rX_Mat;
	TMat = makeConcatination(&TMat, &rY_Mat);
	TMat = makeConcatination(&TMat, &rZ_Mat);
}

void doSubnodeTransform(PSUBNODE node, PMATRIX mat) {
	for (int m = 0; m < node->vertex.size(); m++) {
		VERTEX v = node->vertex[m];
		VERTEX r;
		r.val[0] = 0; r.val[1] = 0; r.val[2] = 0; r.val[3] = 0;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				r.val[i] += v.val[j] * mat->val[j][i];
			}
		}
		node->vertex[m] = r;
	}

}

void doTransform(vector<SUBNODE> *nodes, MATRIX *mat) {
	for (int n = 0; n < nodes->size(); n++) {
		doSubnodeTransform(&nodes->at(n), mat);
	}

	tDirV.val[0] = 0; tDirV.val[1] = 0; tDirV.val[2] = 0; tDirV.val[3] = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			tDirV.val[i] += dirV.val[j] * mat->val[j][i];
		}
	}
}

void doWorldTransform() {

}

void doSubodeTransform(SUBNODE *node, VECTOR3D tr, VECTOR3D rt, VECTOR3D sc) {
	tMatrix(tr);
	sMatrix(sc);
	xRotateMatrix(rt.val[0]); yRotateMatrix(rt.val[1]); zRotateMatrix(rt.val[2]);
	TMat = rX_Mat;
	TMat = makeConcatination(&TMat, &rY_Mat);
	TMat = makeConcatination(&TMat, &rZ_Mat);
	TMat = makeConcatination(&TMat, &s_Mat);
	TMat = makeConcatination(&TMat, &t_Mat);
	doSubnodeTransform(node, &TMat);
}

bool isValid(VERTEX *e, VERTEX *p) {
	double d = e->val[0] * p->val[0] + e->val[1] * p->val[1] + e->val[2] * p->val[2] + e->val[3];
	if (d == 0)
		return true;
	else
		return false;
}

void doNormalize(VERTEX *v) {
	double m = sqrt(v->val[0] * v->val[0] + v->val[1] * v->val[1] + v->val[2] * v->val[2]);
	v->val[0] /= m;
	v->val[1] /= m;
	v->val[2] /= m;
}

void doNormalize(VECTOR3D *v) {
	double m = sqrt(v->val[0] * v->val[0] + v->val[1] * v->val[1] + v->val[2] * v->val[2]);
	v->val[0] /= m;
	v->val[1] /= m;
	v->val[2] /= m;
}

void doFindEuqation() {
	for (int n = 0; n < nodes.size(); n++) {
		for (int m = 0; m < nodes[n].vertex.size(); m++) {
			tNodes[n].vertex[m].nor = VECTOR3D(0, 0, 0);
			tNodes[n].vertex[m].ntriCnt = 0;
		}
	}
	for (int i = 0; i < eNodes.size(); i++) {
		eNodes[i].index.clear();
		eNodes[i].vertex.clear();
	}
	eNodes.clear();
	for (int n = 0; n < nodes.size(); n++) {
		SUBNODE sb;
		for (int m = 0; m < nodes[n].index.size(); m++) {
			VERTEX p1 = tNodes[n].vertex[nodes[n].index[m].idx[0]];
			VERTEX p2 = tNodes[n].vertex[nodes[n].index[m].idx[1]];
			VERTEX p3 = tNodes[n].vertex[nodes[n].index[m].idx[2]];
			VERTEX v1 = doMinus(&p2, &p1);
			VERTEX v2 = doMinus(&p3, &p1);
			VERTEX e = doCross(&v1, &v2);
			doNormalize(&e);
			e.val[3] = -(e.val[0] * p1.val[0] + e.val[1] * p1.val[1] + e.val[2] * p1.val[2]);
			TRIANGLE tr = TRIANGLE(0, 0, 0);

			tr.param[0] = e.val[0]; tr.param[1] = e.val[1]; tr.param[2] = e.val[2]; tr.param[3] = e.val[3];
			for (int p = 0; p < 3; p++) {
				tNodes[n].vertex[nodes[n].index[m].idx[p]].nor.val[0] += e.val[0];
				tNodes[n].vertex[nodes[n].index[m].idx[p]].nor.val[1] += e.val[1];
				tNodes[n].vertex[nodes[n].index[m].idx[p]].nor.val[2] += e.val[2];
				tNodes[n].vertex[nodes[n].index[m].idx[p]].ntriCnt++;
			}
			sb.index.push_back(tr);
		}
		eNodes.push_back(sb);
	}
	for (int n = 0; n < nodes.size(); n++) {
		for (int m = 0; m < nodes[n].vertex.size(); m++) {
			tNodes[n].vertex[m].nor.val[0] /= (float)(tNodes[n].vertex[m].ntriCnt);
			tNodes[n].vertex[m].nor.val[1] /= (float)(tNodes[n].vertex[m].ntriCnt);
			tNodes[n].vertex[m].nor.val[2] /= (float)(tNodes[n].vertex[m].ntriCnt);
		}
	}
}

void doProjection(VERTEX *a) {
	double c = sqrt(3);
	a->val[0] = (RESIZED_WIDTH / 2) + (a->val[0] * c / a->val[2]) * (RESIZED_WIDTH / 2);
	a->val[1] = (RESIZED_HEIGHT / 2) + (a->val[1] * c / a->val[2]) * (RESIZED_HEIGHT / 2);
	a->val[2] = 1;
}

void doProjection(VECTOR3D *a) {
	double c = sqrt(3);
	a->val[0] = (RESIZED_WIDTH / 2) + (a->val[0] * c / a->val[2]) * (RESIZED_WIDTH / 2);
	a->val[1] = (RESIZED_HEIGHT / 2) + (a->val[1] * c / a->val[2]) * (RESIZED_HEIGHT / 2);
}

void correctCenter() {
	VERTEX mn, mx;
	VERTEX center;
	double c = sqrt(3);
	double d = 0;

	center.val[0] = 0; center.val[1] = 0; center.val[2] = 0; center.val[3] = 1;
	GetBound(&mx, &mn, &tNodes);
	

	center.val[0] = (mx.val[0] + mn.val[0]) / 2;
	center.val[1] = (mx.val[1] + mn.val[1]) / 2;
	center.val[2] = (mx.val[2] + mn.val[2]) / 2;

	double wx = mx.val[0] - mn.val[0];
	double wy = mx.val[1] - mn.val[1];
	double wz = mx.val[2] - mn.val[2];

	if (wx >= wy && wx >= wz) {
		dirV = VECTOR3D(1, 0, 0);
	}
	else if (wy >= wx && wy >= wz) {
		dirV = VECTOR3D(0, 1, 0);
	}
	else if (wz >= wx && wz >= wy) {
		dirV = VECTOR3D(0, 0, 1);
	}

	float tz = (mx.val[2] - mn.val[2]) * 0.01;

	for (int n = 0; n < tNodes.size(); n++) {
		for (int m = 0; m < tNodes[n].vertex.size(); m++) {
			VERTEX v = tNodes[n].vertex[m];
			v.val[0] -= center.val[0]; v.val[1] -= center.val[1]; v.val[2] += tz;
			tNodes[n].vertex[m] = v;
		}
	}

	for (int n = 0; n < tNodes.size(); n++) {
		for (int m = 0; m < tNodes[n].vertex.size(); m++) {
			VERTEX v = tNodes[n].vertex[m];
			float t = abs(v.val[0]) > abs(v.val[1]) ? abs(v.val[0]) : abs(v.val[1]);
			t *= zoomFactor;
			t *= c;
			t = t - v.val[2];
			if (t > d) d = t;
		}
	}
	for (int n = 0; n < tNodes.size(); n++) {
		for (int m = 0; m < tNodes[n].vertex.size(); m++) {
			VERTEX v = tNodes[n].vertex[m];
			v.val[2] += d;
			tNodes[n].vertex[m] = v;
		}
	}
}

bool isInside(VERTEX *p, VERTEX *a) {
	VERTEX pp[3];
	VERTEX v1 = doMinus(&p[1], &p[0]);
	VERTEX v2 = doMinus(a, &p[0]);
	pp[0] = doCross(&v1, &v2);
	v1 = doMinus(&p[2], &p[1]);
	v2 = doMinus(a, &p[1]);
	pp[1] = doCross(&v1, &v2);
	v1 = doMinus(&p[0], &p[2]);
	v2 = doMinus(a, &p[2]);
	pp[2] = doCross(&v1, &v2);
	if ((pp[0].val[2] > 0 && pp[1].val[2] > 0 && pp[2].val[2] > 0) ||
		(pp[0].val[2] < 0 && pp[1].val[2] < 0 && pp[2].val[2] < 0)) 
		return true;
	return false; 
}

void PushVertex(int i, int j, int n, int m, double x, double y, double z) {
	depth[j][i] = z;
	indexMap1[j][i] = n;
	indexMap2[j][i] = m;
	vertMap[j][i] = VERTEX(x, y, z);
	VERTEX p1 = tNodes[n].vertex[nodes[n].index[m].idx[0]];
	VERTEX p2 = tNodes[n].vertex[nodes[n].index[m].idx[1]];
	VERTEX p3 = tNodes[n].vertex[nodes[n].index[m].idx[2]];
	VERTEX v1 = VERTEX(x - p1.val[0], y - p1.val[1], z - p1.val[2]);
	VERTEX v2 = VERTEX(x - p2.val[0], y - p2.val[1], z - p2.val[2]);
	VERTEX v3 = VERTEX(x - p3.val[0], y - p3.val[1], z - p3.val[2]);
	VERTEX l1 = VERTEX(p2.val[0] - p1.val[0], p2.val[1] - p1.val[1], p2.val[2] - p1.val[2]);
	VERTEX l2 = VERTEX(p3.val[0] - p2.val[0], p3.val[1] - p2.val[1], p3.val[2] - p2.val[2]);
	VERTEX l3 = VERTEX(p1.val[0] - p3.val[0], p1.val[1] - p3.val[1], p1.val[2] - p3.val[2]);
	
	double m1 = doMagnitudeForVertex(&v1);
	double m2 = doMagnitudeForVertex(&v2);
	double m3 = doMagnitudeForVertex(&v3);
	double ml1 = doMagnitudeForVertex(&l1);
	double ml2 = doMagnitudeForVertex(&l2);
	double ml3 = doMagnitudeForVertex(&l3);
	
	VERTEX c1 = doCross(&v1, &l1);
	VERTEX c2 = doCross(&v2, &l2);
	VERTEX c3 = doCross(&v3, &l3);

	double aa = doMagnitudeForVertex(&c1) / ml1;
	double bb = doMagnitudeForVertex(&c2) / ml2;
	double cc = doMagnitudeForVertex(&c3) / ml3;
	double s = doMagnitudeForVertex(&doCross(&l1, &l2));
	double a = s / ml1;
	double b = s / ml2;
	double c = s / ml3;

	aa /= a; bb /= b; cc /= c;
	/*
	aa *= (PI / 2.0f);
	bb *= (PI / 2.0f);
	bb *= (PI / 2.0f);
	aa = sin(aa); bb = sin(bb); cc = sin(cc);
	*/
	VECTOR3D nor1;
	VECTOR3D nor2;
	VECTOR3D nor3;

	nor1.val[0] = p1.nor.val[0] * bb; nor1.val[1] = p1.nor.val[1] * bb; nor1.val[2] = p1.nor.val[2] * bb;
	nor2.val[0] = p2.nor.val[0] * cc; nor2.val[1] = p2.nor.val[1] * cc; nor2.val[2] = p2.nor.val[2] * cc;
	nor3.val[0] = p3.nor.val[0] * aa; nor3.val[1] = p3.nor.val[1] * aa; nor3.val[2] = p3.nor.val[2] * aa;
	VECTOR3D nor;
	nor.val[0] = nor1.val[0] + nor2.val[0] + nor3.val[0];
	nor.val[1] = nor1.val[1] + nor2.val[1] + nor3.val[1];
	nor.val[2] = nor1.val[2] + nor2.val[2] + nor3.val[2];
	m1 = doMagnitude(&nor);
	vertMap[j][i].nor = VECTOR3D(nor.val[0] / m1, nor.val[1] / m1, nor.val[2] / m1);
}

void doRender() {
	double c = 1 / sqrt(3);
	
	init_Render();
	correctCenter();
	makeTransformMatrix(x_Rotate, y_Rotate, z_Rotate);
	doTransform(&tNodes, &TMat);
	correctCenter();
	doFindEuqation();
	for (int i = 0; i < RESIZED_WIDTH; i++) {
		for (int j = 0; j < RESIZED_HEIGHT; j++) {
			depth[i][j] = 0;
		}
	}
	for (int n = 0; n < nodes.size(); n++) {
		for (int m = 0; m < nodes[n].index.size(); m++)
		{
			VERTEX p[3];

			p[0] = tNodes[n].vertex[nodes[n].index[m].idx[0]];
			p[1] = tNodes[n].vertex[nodes[n].index[m].idx[1]];
			p[2] = tNodes[n].vertex[nodes[n].index[m].idx[2]];

			double mxX, mnX, mxY, mnY, mnZ, mxZ;
			for (int i = 0; i < 3; i++) {
				doProjection(&p[i]);

				if (i == 0) {
					mxX = p[i].val[0]; mnX = p[i].val[0];
					mxY = p[i].val[1]; mnY = p[i].val[1];
					mxZ = p[i].val[2]; mnZ = p[i].val[2];
				}
				else{
					if (p[i].val[0] > mxX) mxX = p[i].val[0];
					if (p[i].val[0] < mnX) mnX = p[i].val[0];
					if (p[i].val[1] > mxY) mxY = p[i].val[1];
					if (p[i].val[1] < mnY) mnY = p[i].val[1];
					if (p[i].val[2] > mxZ) mxZ = p[i].val[2];
					if (p[i].val[2] < mnZ) mnZ = p[i].val[2];
				}
			}
			VERTEX v1 = doMinus(&p[2], &p[0]);
			VERTEX v2 = doMinus(&p[1], &p[0]);
			v1 = doCross(&v1, &v2);
			if (v1.val[2] < 0) continue;
			int imxX = (int)mxX;
			int imnX = (int)mnX;
			int imxY = (int)mxY;
			int imnY = (int)mnY;
			mxZ *= 1.5f;
			VERTEX pp[3];
			pp[0] = p[0]; pp[0].val[2] = 0;
			pp[1] = p[1]; pp[1].val[2] = 0;
			pp[2] = p[2]; pp[2].val[2] = 0;
			for (int i = imnX - 1; i <= imxX + 1; i++) {
				if (i < 0 || i >= RESIZED_WIDTH) continue;
				for (int j = imnY - 1; j < imxY + 1; j++) {
					if (j < 0 || j >= RESIZED_HEIGHT) continue;
					VERTEX a;
					a.val[0] = i + 0.5; a.val[1] = j + 0.5; a.val[2] = 0;
					
					if (isInside(pp, &a)) 
					{
						double x = i - RESIZED_WIDTH / 2;
						double y = j - RESIZED_HEIGHT / 2;
						x = x / (double)(RESIZED_WIDTH / 2) * c;
						y = y / (double)(RESIZED_HEIGHT / 2) * c;
						double z = eNodes[n].index[m].param[0] * x + eNodes[n].index[m].param[1] * y + eNodes[n].index[m].param[2];
						if (z != 0) {
							z = -eNodes[n].index[m].param[3] / z;
							x = x * z;
							y = y * z;
							if (z > 0) {
								if (z > maxDepth) maxDepth = z;
								if (z < minDepth) minDepth = z;
								if (depth[j][i] == 0) {
									PushVertex(i, j, n, m, x, y, z);
								}
								else if (z < depth[j][i]) {
									PushVertex(i, j, n, m, x, y, z);
								}
							}
						}
					}
				}
			}
		}
	}
}

