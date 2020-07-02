// ImgCvt.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "ImgCvt.h"


#include <commdlg.h>
#include <CommCtrl.h>
#include <shlobj.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <opencv2\core\core.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\highgui\highgui.hpp>

using namespace cv;
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

char		szFile[MAX_PATH];

const		aiScene* scene = NULL;

UCHAR	imgLG[2400][2400 * 4];
UCHAR	imgSM[120][120 * 4];

char	modelName[MAX_PATH] = "Model";
float	gridThickness = 0.4f;
int		gridNum = 15;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


void ShowResultMatrix(HWND hWnd, Mat *frm) {
	HDC hdc = GetDC(GetDlgItem(hWnd, IDC_MODEL_VIEW));
	HDC hdcMem = CreateCompatibleDC(hdc);
	HGDIOBJ oldBitmap;
	HBITMAP hBmp = CreateBitmap(RESIZED_WIDTH, RESIZED_HEIGHT, 1, 32, frm->data);
	RECT rect;
	GetClientRect(GetDlgItem(hWnd, IDC_MODEL_VIEW), &rect);

	SetStretchBltMode(hdc, HALFTONE);

	oldBitmap = SelectObject(hdcMem, hBmp);

	StretchBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdcMem, 0, 0, RESIZED_WIDTH, RESIZED_HEIGHT, SRCCOPY);

	SelectObject(hdcMem, oldBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(hWnd, hdc);
	DeleteDC(hdc);
	DeleteObject(hBmp);
}

void DrawNormal(Mat *frm, int i, int j, bool flag) {
	if (indexMap1[i][j] == -1 || indexMap2[i][j] == -1) return;
	if (flag) {
		int m = indexMap1[i][j];
		int n = indexMap2[i][j];

		VERTEX v[3];
		VECTOR3D nor[3];
		for (int i = 0; i < 3; i++) {
			int idx = nodes[m].index[n].idx[i];
			v[i] = tNodes[m].vertex[idx];
			nor[i] = tNodes[m].vertex[idx].nor;
			nor[i] = VECTOR3D(v[i].val[0] + nor[i].val[0] * normalFactor, v[i].val[1] + nor[i].val[1] * normalFactor, v[i].val[2] + nor[i].val[2] * normalFactor);
			doProjection(&v[i]);
			doProjection(&nor[i]);
			line(*frm, Point(v[i].val[0], v[i].val[1]), Point(nor[i].val[0], nor[i].val[1]), Scalar(0, 255, 0));
		}
	}
	else{
		VERTEX v1 = vertMap[i][j];
		VERTEX v2 = VERTEX(vertMap[i][j].nor.val[0], vertMap[i][j].nor.val[1], vertMap[i][j].nor.val[2]);
		v2.val[0] *= normalFactor; v2.val[1] *= normalFactor; v2.val[2] *= normalFactor;
		v2.val[0] += v1.val[0]; v2.val[1] += v1.val[1]; v2.val[2] += v1.val[2];
		doProjection(&v1); doProjection(&v2);
		line(*frm, Point(j, i), Point(v2.val[0], v2.val[1]), Scalar(0, 0, 255));
		//circle(*frm, Point(j, i), 3, Scalar(0, 0, 255));
	}
}

void DrawAllNormal(Mat *frm, int interval, bool flag) {
	for (int i = 0; i < RESIZED_HEIGHT; i += interval) {
		for (int j = 0; j < RESIZED_WIDTH; j += interval) {
			DrawNormal(frm, i, j, flag);
		}
	}
}

void DrawGrid(HWND hWnd) {
	if (nodes.size() == 0) return;
	Mat frm = Mat::zeros(RESIZED_HEIGHT, RESIZED_WIDTH, CV_8UC3);

	for (int i = 0; i < RESIZED_HEIGHT; i++) {
		for (int j = 0; j < RESIZED_WIDTH; j++) {
			int m = 255;
			frm.at<Vec3b>(i, j) = Vec3b(m, m, m);
		}
	}

	if (SendMessage(GetDlgItem(hWnd, IDC_SURFACE), BM_GETCHECK, 0, 0)) {
		double mn = 1e+100;
		double mx = -1e+100;

		for (int i = 0; i < RESIZED_HEIGHT; i++) {
			bool f = false;
			for (int j = 0; j < RESIZED_WIDTH; j++) {
				if (indexMap1[i][j] == -1 || indexMap2[i][j] == -1) continue;
				VERTEX v = vertMap[i][j];
				mn = v.val[1];
				f = true;
				break;
			}
			if (f) break;
		}
		for (int i = RESIZED_HEIGHT; i >= 0; i--) {
			bool f = false;
			for (int j = 0; j < RESIZED_WIDTH; j++) {
				if (indexMap1[i][j] == -1 || indexMap2[i][j] == -1) continue;
				VERTEX v = vertMap[i][j];
				mn = v.val[1];
				f = true;
				break;
			}
			if (f) break;
		}
		double stp = (mx - mn) / 15;
		for (int i = 0; i < RESIZED_HEIGHT; i++) {
			for (int j = 0; j < RESIZED_WIDTH; j++) {
				if (indexMap1[i][j] == -1 || indexMap2[i][j] == -1) continue;
				VERTEX v = vertMap[i][j];
				int n = (int)((v.val[1] - mn) / stp);
				if (abs(v.val[1] - stp * n) < 0.01) {
					frm.at<Vec3b>(i, j) = Vec3b(30, 30, 30);
				}
			}
		}
		for (int i = 0; i < RESIZED_HEIGHT; i++) {
			for (int j = 0; j < RESIZED_WIDTH; j++) {
				renderImg[i][j * 4] = frm.at<Vec3b>(i, j).val[0];
				renderImg[i][j * 4 + 1] = frm.at<Vec3b>(i, j).val[1];
				renderImg[i][j * 4 + 2] = frm.at<Vec3b>(i, j).val[2];
				renderImg[i][j * 4 + 3] = 1;
			}
		}
		frm.release();
	}
}

void CreateGrid(HWND hWnd) {
	if (nodes.size() == 0) return;
	Mat frm = Mat::zeros(RESIZED_HEIGHT, RESIZED_WIDTH, CV_8UC3);

	for (int i = 0; i < RESIZED_HEIGHT; i++) {
		for (int j = 0; j < RESIZED_WIDTH; j++) {
			int m = 255;
			frm.at<Vec3b>(i, j) = Vec3b(m, m, m);
		}
	}
	if (SendMessage(GetDlgItem(hWnd, IDC_SURFACE), BM_GETCHECK, 0, 0)) {
		for (int i = 0; i < RESIZED_HEIGHT; i++) {
			for (int j = 0; j < RESIZED_WIDTH; j++) {
				if (indexMap1[i][j] == -1 || indexMap2[i][j] == -1) continue;
				VERTEX v1 = VERTEX(1, 1, 1);
				VERTEX v2 = VERTEX(vertMap[i][j].nor.val[0], vertMap[i][j].nor.val[1], vertMap[i][j].nor.val[2]);
				double m = doMagnitudeForVertex(&doCross(&v1, &v2));
				m = m * m * 70.0f;
				if (m > 255) m = 255;
				frm.at<Vec3b>(i, j) = Vec3b(m, m, m);
			}
		}
	}
	if (SendMessage(GetDlgItem(hWnd, IDC_NORMAL_VERTEX), BM_GETCHECK, 0, 0)) {
		DrawAllNormal(&frm, RESIZED_HEIGHT / 60, true);
	}
	if (SendMessage(GetDlgItem(hWnd, IDC_NORMAL_PIXEL), BM_GETCHECK, 0, 0)) {
		DrawAllNormal(&frm, RESIZED_HEIGHT / 60, false);
	}
	if (SendMessage(GetDlgItem(hWnd, IDC_MESH), BM_GETCHECK, 0, 0)) {
		for (int n = 0; n < nodes.size(); n++) {
			for (int m = 0; m < nodes[n].index.size(); m++) {
				nodes[n].index[m].param[0] = -1.0f;
			}
		}
		for (int i = 0; i < RESIZED_HEIGHT; i++) {
			for (int j = 0; j < RESIZED_WIDTH; j++) {
				if (indexMap1[i][j] == -1 || indexMap2[i][j] == -1) continue;
				int n = indexMap1[i][j];
				int m = indexMap2[i][j];
				if (nodes[n].index[m].param[0] > 0) continue;
				nodes[n].index[m].param[0] = 1.0f;
				VERTEX v1 = tNodes[n].vertex[nodes[n].index[m].idx[0]];
				VERTEX v2 = tNodes[n].vertex[nodes[n].index[m].idx[1]];
				VERTEX v3 = tNodes[n].vertex[nodes[n].index[m].idx[2]];
				doProjection(&v1); doProjection(&v2); doProjection(&v3);
				line(frm, Point(v1.val[0], v1.val[1]), Point(v2.val[0], v2.val[1]), Scalar(255, 0, 0));
				line(frm, Point(v2.val[0], v2.val[1]), Point(v3.val[0], v3.val[1]), Scalar(255, 0, 0));
				line(frm, Point(v3.val[0], v3.val[1]), Point(v1.val[0], v1.val[1]), Scalar(255, 0, 0));
			}
		}
	}


	if (SendMessage(GetDlgItem(hWnd, IDC_CHK_GRID), BM_GETCHECK, 0, 0)) 
	{
		double mn = 1e+100;
		double mx = -1e+100;

		for (int i = 0; i < RESIZED_HEIGHT; i++) {
			for (int j = 0; j < RESIZED_WIDTH; j++) {
				if (indexMap1[i][j] == -1 || indexMap2[i][j] == -1) continue;
				VERTEX v = vertMap[i][j];
				double d = tDirV.val[0] * v.val[0] + tDirV.val[1] * v.val[1] + tDirV.val[2] * v.val[2];
				if (d > mx) mx = d;
				if (d < mn) mn = d;
			}
		}

		double stp = (mx - mn) / gridNum;
		for (int i = 0; i < RESIZED_HEIGHT; i++) {
			for (int j = 0; j < RESIZED_WIDTH; j++) {
				if (indexMap1[i][j] == -1 || indexMap2[i][j] == -1) continue;
				VERTEX v = vertMap[i][j];
				double d = tDirV.val[0] * v.val[0] + tDirV.val[1] * v.val[1] + tDirV.val[2] * v.val[2];
				int n = (int)((d - mn) / stp);
				if (abs(d - stp * n - mn) < stp * gridThickness) {
					frm.at<Vec3b>(i, j) = Vec3b(30, 30, 30);
				}
			}
		}
		/*
		for (int n = 0; n < nodes.size(); n++) {
			for (int m = 0; m < nodes[n].index.size(); m++) {
				nodes[n].index[m].param[0] = -1.0f;
			}
		}
		int delta = gridInv;

		char buf[MAX_PATH];
		GetWindowText(GetDlgItem(hWnd, IDC_INV), buf, MAX_PATH);
		gridInv = atoi(buf);
		for (int i = 1; i < RESIZED_HEIGHT - 1; i+=delta) {
			for (int j = 1; j < RESIZED_WIDTH - 1;j+=delta) {
				if (indexMap1[i][j] == -1 || indexMap2[i][j] == -1) continue;
				int n = indexMap1[i][j];
				int m = indexMap2[i][j];
				if (nodes[n].index[m].param[0] > 0) continue;
				nodes[n].index[m].param[0] = 1.0f;
				int nRetry = 0;
				VECTOR3D op;
			_retry:
				if (nRetry > 50) continue;
				int *idx = nodes[n].index[m].idx;
				VECTOR3D np[5];
				np[0] = VECTOR3D(tNodes[n].vertex[idx[0]].val[0], tNodes[n].vertex[idx[0]].val[1], tNodes[n].vertex[idx[0]].val[2]);
				np[1] = VECTOR3D(tNodes[n].vertex[idx[1]].val[0], tNodes[n].vertex[idx[1]].val[1], tNodes[n].vertex[idx[1]].val[2]);
				np[2] = VECTOR3D(tNodes[n].vertex[idx[2]].val[0], tNodes[n].vertex[idx[2]].val[1], tNodes[n].vertex[idx[2]].val[2]);
				np[3] = np[0];
				np[4] = np[1];

				VECTOR3D nn[5];
				nn[0] = tNodes[n].vertex[idx[0]].nor;
				nn[1] = tNodes[n].vertex[idx[1]].nor;
				nn[2] = tNodes[n].vertex[idx[2]].nor;
				nn[3] = nn[0];
				nn[4] = nn[1];

				int vIdx[5];
				vIdx[0] = 0; vIdx[1] = 1; vIdx[2] = 2; vIdx[3] = 0; vIdx[4] = 1;
				double iv = 30.0f;
				double mn = -1;
				VECTOR3D mnc;
				int mnc1;
				VECTOR3D mnc2;

				bool flag = false;
				for (int k = 0; k < 3; k++) {
					VECTOR3D p0 = np[k];
					VECTOR3D n0 = nn[k];
					VECTOR3D p01 = VECTOR3D(p0.val[0] + n0.val[0], p0.val[1] + n0.val[1], p0.val[2] + n0.val[2]);
					for (int n = 0; n < iv; n++) {
						VECTOR3D p1 = VECTOR3D((np[k + 1].val[0] * (iv - n) + np[k + 2].val[0] * n) / iv,
												(np[k + 1].val[1] * (iv - n) + np[k + 2].val[1] * n) / iv,
												(np[k + 1].val[2] * (iv - n) + np[k + 2].val[2] * n) / iv);
						VECTOR3D n1 = VECTOR3D((nn[k + 1].val[0] * (iv - n) + nn[k + 2].val[0] * n),
												(nn[k + 1].val[1] * (iv - n) + nn[k + 2].val[1] * n),
												(nn[k + 1].val[2] * (iv - n) + nn[k + 2].val[2] * n));
						VECTOR3D p11 = VECTOR3D(p1.val[0] + n1.val[0], p1.val[1] + n1.val[1], p1.val[2] + n1.val[2]);
						doNormalize(&n1);
						doNormalize(&n0);

						VECTOR3D dp = doMinus(&p1, &p0);
						double mdp = doMagnitude(&dp);

						doNormalize(&dp);
						double m1 = doMagnitude(&doCross(&doCross(&n0, &dp), &doCross(&dp, &n1)));
						double m2 = doMagnitude(&doMinus(&p1, &p0));
						double m3 = doMagnitude(&doMinus(&p11, &p01));

						//if (m1 > 0.1f) continue;
						double sa = doMagnitude(&doCross(&n0, &dp));
						double sb = doMagnitude(&doCross(&dp, &n1));
						double sc = doMagnitude(&doCross(&n0, &n1));

						double b = sc / sa;
						double a = sb / sa;
						double c = (b + mdp) * a / b - a;

						if (mn < 0) {
							mn = c; mnc = doMinus(&p1, &p0);
							mnc1 = k; mnc2 = p1;
						}else if (c < mn) {
							mn = c; mnc = doMinus(&p1, &p0);
							mnc1 = k; mnc2 = p1;
						}
					}
				}

				double mg = doMagnitude(&doMinus(&np[mnc1 + 2], &np[mnc1 + 1]));
				int inv = mg * 5 / normalFactor + 2;
				for (int n = 1; n < inv; n++) {
					float ext = 0.0f;
					VECTOR3D p1 = VECTOR3D((np[mnc1 + 1].val[0] * (inv - n) + np[mnc1 + 2].val[0] * n) / inv + mnc.val[0] * ext,
						(np[mnc1 + 1].val[1] * (inv - n) + np[mnc1 + 2].val[1] * n) / inv + mnc.val[1] * ext,
						(np[mnc1 + 1].val[2] * (inv - n) + np[mnc1 + 2].val[2] * n) / inv + mnc.val[2] * ext);
					double m1 = doMagnitude(&doMinus(&mnc2, &np[mnc1 + 1]));
					double m2 = doMagnitude(&doMinus(&p1, &np[mnc1 + 1]));
					if (m1 >= m2) {
						ext = m2 / m1;
					}
					else{
						m1 = doMagnitude(&doMinus(&mnc2, &np[mnc1 + 2]));
						m2 = doMagnitude(&doMinus(&p1, &np[mnc1 + 2]));
						ext = m2 / m1;
					}
					VECTOR3D p0 = VECTOR3D(p1.val[0] - mnc.val[0] * ext, p1.val[1] - mnc.val[1] * ext, p1.val[2] - mnc.val[2] * ext);
					doProjection(&p0);
					doProjection(&p1);
					line(frm, Point(p1.val[0], p1.val[1]), Point(p0.val[0], p0.val[1]), Scalar(30, 30, 30), 1);
				}
			}
		}*/
	}

	for (int i = 0; i < RESIZED_HEIGHT; i++) {
		for (int j = 0; j < RESIZED_WIDTH; j++) {
			renderImg[i][j * 4] = frm.at<Vec3b>(i, j).val[0];
			renderImg[i][j * 4 + 1] = frm.at<Vec3b>(i, j).val[1];
			renderImg[i][j * 4 + 2] = frm.at<Vec3b>(i, j).val[2];
			renderImg[i][j * 4 + 3] = 1;
		}
	}
	//Mat f;
	//cvtColor(frm, f, CV_BGR2BGRA);
	//ShowResultMatrix(hWnd, &f);
	//f.release();
	frm.release();
}

PBITMAPINFO CreateBitmapInfoStruct(HBITMAP hBmp)
{
	BITMAP bmp;
	PBITMAPINFO pbmi;
	WORD    cClrBits;

	// Retrieve the bitmap color format, width, and height.  
	if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp)) return NULL;

	// Convert the color format to a count of bits.  
	cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
	if (cClrBits == 1)
		cClrBits = 1;
	else if (cClrBits <= 4)
		cClrBits = 4;
	else if (cClrBits <= 8)
		cClrBits = 8;
	else if (cClrBits <= 16)
		cClrBits = 16;
	else if (cClrBits <= 24)
		cClrBits = 24;
	else cClrBits = 32;

	// Allocate memory for the BITMAPINFO structure. (This structure  
	// contains a BITMAPINFOHEADER structure and an array of RGBQUAD  
	// data structures.)  

	if (cClrBits < 24)
		pbmi = (PBITMAPINFO)LocalAlloc(LPTR,
		sizeof(BITMAPINFOHEADER) +
		sizeof(RGBQUAD) * (1 << cClrBits));

	// There is no RGBQUAD array for these formats: 24-bit-per-pixel or 32-bit-per-pixel 

	else
		pbmi = (PBITMAPINFO)LocalAlloc(LPTR,
		sizeof(BITMAPINFOHEADER));

	// Initialize the fields in the BITMAPINFO structure.  

	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biWidth = bmp.bmWidth;
	pbmi->bmiHeader.biHeight = bmp.bmHeight;
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
	if (cClrBits < 24)
		pbmi->bmiHeader.biClrUsed = (1 << cClrBits);

	// If the bitmap is not compressed, set the BI_RGB flag.  
	pbmi->bmiHeader.biCompression = BI_RGB;

	// Compute the number of bytes in the array of color  
	// indices and store the result in biSizeImage.  
	// The width must be DWORD aligned unless the bitmap is RLE 
	// compressed. 
	pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits + 31) & ~31) / 8
		* pbmi->bmiHeader.biHeight;
	// Set biClrImportant to 0, indicating that all of the  
	// device colors are important.  
	pbmi->bmiHeader.biClrImportant = 0;
	return pbmi;
}

void CreateBMPFile(LPTSTR pszFile, HBITMAP hBMP)
{
	HANDLE hf;                 // file handle  
	BITMAPFILEHEADER hdr;       // bitmap file-header  
	PBITMAPINFOHEADER pbih;     // bitmap info-header  
	LPBYTE lpBits;              // memory pointer  
	DWORD dwTotal;              // total count of bytes  
	DWORD cb;                   // incremental count of bytes  
	BYTE *hp;                   // byte pointer  
	DWORD dwTmp;
	PBITMAPINFO pbi;
	HDC hDC;

	hDC = CreateCompatibleDC(GetWindowDC(GetDesktopWindow()));
	SelectObject(hDC, hBMP);

	pbi = CreateBitmapInfoStruct(hBMP);
	if (pbi == NULL) return;
	pbih = (PBITMAPINFOHEADER)pbi;
	lpBits = (LPBYTE)GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

	if (!lpBits) return;

	// Retrieve the color table (RGBQUAD array) and the bits  
	// (array of palette indices) from the DIB.  
	if (!GetDIBits(hDC, hBMP, 0, (WORD)pbih->biHeight, lpBits, pbi,
		DIB_RGB_COLORS)) return;

	// Create the .BMP file.  
	hf = CreateFile(pszFile,
		GENERIC_READ | GENERIC_WRITE,
		(DWORD)0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		(HANDLE)NULL);
	if (hf == INVALID_HANDLE_VALUE) return;

	hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M"  
	// Compute the size of the entire file.  
	hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) +
		pbih->biSize + pbih->biClrUsed
		* sizeof(RGBQUAD) + pbih->biSizeImage);
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;

	// Compute the offset to the array of color indices.  
	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) +
		pbih->biSize + pbih->biClrUsed
		* sizeof(RGBQUAD);

	// Copy the BITMAPFILEHEADER into the .BMP file.  
	if (!WriteFile(hf, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER),
		(LPDWORD)&dwTmp, NULL)) return;

	// Copy the BITMAPINFOHEADER and RGBQUAD array into the file.  
	if (!WriteFile(hf, (LPVOID)pbih, sizeof(BITMAPINFOHEADER)
		+ pbih->biClrUsed * sizeof(RGBQUAD),
		(LPDWORD)&dwTmp, (NULL))) return;

	// Copy the array of color indices into the .BMP file.  
	dwTotal = cb = pbih->biSizeImage;
	hp = lpBits;
	if (!WriteFile(hf, (LPSTR)hp, (int)cb, (LPDWORD)&dwTmp, NULL)) return;

	// Close the .BMP file.  
	if (!CloseHandle(hf)) return;

	// Free memory.  
	GlobalFree((HGLOBAL)lpBits);
}


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_IMGCVT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	for (int i = 1; i < RESIZED_HEIGHT - 1; i++) {
		for (int j = 1; j < RESIZED_WIDTH - 1; j++) {
			indexMap1[i][j] = -1; indexMap2[i][j] = -1;
		}
	}
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)WndProc);
	return 1;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IMGCVT));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_IMGCVT);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

void ShowResultImage(HWND hWnd) {
	HDC hdc = GetDC(GetDlgItem(hWnd, IDC_MODEL_VIEW));
	HDC hdcMem = CreateCompatibleDC(hdc);
	HGDIOBJ oldBitmap;
	HBITMAP hBmp = CreateBitmap(RESIZED_WIDTH, RESIZED_HEIGHT, 1, 32, renderImg);
	RECT rect;
	GetClientRect(GetDlgItem(hWnd, IDC_MODEL_VIEW), &rect);

	SetStretchBltMode(hdc, HALFTONE);

	oldBitmap = SelectObject(hdcMem, hBmp);

	StretchBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdcMem, 0, 0, RESIZED_WIDTH, RESIZED_HEIGHT, SRCCOPY);

	SelectObject(hdcMem, oldBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(hWnd, hdc);
	DeleteDC(hdc);
	DeleteObject(hBmp);
}

void coatMaterial(HWND hWnd) {
	if (SendMessage(GetDlgItem(hWnd, IDC_SURFACE), BM_GETCHECK, 0, 0) || SendMessage(GetDlgItem(hWnd, IDC_NORMAL_VERTEX), BM_GETCHECK, 0, 0) ||
		SendMessage(GetDlgItem(hWnd, IDC_NORMAL_PIXEL), BM_GETCHECK, 0, 0) || SendMessage(GetDlgItem(hWnd, IDC_MESH), BM_GETCHECK, 0, 0) || SendMessage(GetDlgItem(hWnd, IDC_CHK_GRID), BM_GETCHECK, 0, 0)) {
		CreateGrid(hWnd);
	}
	else{
		for (int i = ROI; i < RESIZED_HEIGHT - ROI; i++) {
			for (int j = ROI; j < RESIZED_WIDTH - ROI; j++) {
				renderImg[i][j * 4] = 0;
				renderImg[i][j * 4 + 1] = 0;
				renderImg[i][j * 4 + 2] = 0;
				renderImg[i][j * 4 + 3] = 1;
				if (depth[i][j] == 0) {
					continue;
				}
				double sum = 0;
				double sum1 = 0;
				for (int x = -ROI; x <= ROI; x++) {
					for (int y = -ROI; y <= ROI; y++) {
						if (abs(depth[i][j] - depth[i + x][j + y]) < roi_Depth) {
							sum++;
						}
					}
				}
				sum /= (double)((2 * ROI + 1) * (2 * ROI + 1));
				if (SendMessage(GetDlgItem(hWnd, IDC_NEON), BM_GETCHECK, 0, 0)) {
					sum = pow(sum, 5);
					sum1 = 0.7f + sum;
					if (sum1 > 1.0f) sum1 = 1.0f;
				}
				else{
					sum1 = sum;
				}
				sum *= 255.0f;
				sum1 *= 255.0f;
				renderImg[i][j * 4] = round(sum);
				renderImg[i][j * 4 + 1] = round(sum1);
				renderImg[i][j * 4 + 2] = round(sum);
				renderImg[i][j * 4 + 3] = 1;
			}
		}
	}
}

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{

	if (uMsg == BFFM_INITIALIZED)
	{
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}

	return 0;
}

std::string BrowseFolder(std::string saved_path)
{
	TCHAR path[MAX_PATH];

	const char * path_param = saved_path.c_str();

	BROWSEINFO bi = { 0 };
	bi.lpszTitle = ("Browse for folder...");
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = (LPARAM)path_param;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

	if (pidl != 0)
	{
		//get the name of the folder and put it in path
		SHGetPathFromIDList(pidl, path);

		//free memory used
		IMalloc * imalloc = 0;
		if (SUCCEEDED(SHGetMalloc(&imalloc)))
		{
			imalloc->Free(pidl);
			imalloc->Release();
		}

		return path;
	}

	return "";
}

void get_bounding_box_for_node(const struct aiNode* nd, aiMatrix4x4* trafo)
{
	aiMatrix4x4 prev;
	unsigned int n = 0, t;

	prev = *trafo;
	aiMultiplyMatrix4(trafo, &nd->mTransformation);

	for (; n < nd->mNumMeshes; ++n) {
		const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
		SUBNODE subnode;
		for (t = 0; t < mesh->mNumVertices; ++t) {

			aiVector3D tmp = mesh->mVertices[t];
			aiTransformVecByMatrix4(&tmp, trafo);
			VERTEX v = VERTEX(tmp.x, tmp.y, tmp.z);
			subnode.vertex.push_back(v);
		}
		for (int t = 0; t < mesh->mNumFaces; t++) {
			aiFace tmp = mesh->mFaces[t];
			if (tmp.mNumIndices < 3) continue;
			for (int i = 2; i < tmp.mNumIndices; i++) {
				TRIANGLE t = TRIANGLE(tmp.mIndices[0], tmp.mIndices[i - 1], tmp.mIndices[i]);
				subnode.index.push_back(t);
			}
		}
		nodes.push_back(subnode);
	}

	for (n = 0; n < nd->mNumChildren; ++n) {
		get_bounding_box_for_node(nd->mChildren[n] ,trafo);
	}
	*trafo = prev;
}

int loadasset(const char* path)
{
	/* we are taking one of the postprocessing presets to avoid
	spelling out 20+ single postprocessing flags here. */

	scene = aiImportFile(path, aiProcessPreset_TargetRealtime_Fast);
	aiMatrix4x4 trafo;
	aiIdentityMatrix4(&trafo);

	if (scene) {
		if (scene->mRootNode != NULL)
		{
			get_bounding_box_for_node(scene->mRootNode, &trafo);
		}
		aiReleaseImport(scene);
		return 0;
	}
	return 1;
}

void SetTrackBar(HWND hWnd, int ID, int ShowID, double *val) {
	int v = SendMessage(GetDlgItem(hWnd, ID), TBM_GETPOS, 0, 0);
	SetWindowText(GetDlgItem(hWnd, ShowID), toString(v).c_str());
	*val = v * PI / 180.0f;
}

void InitDialog(HWND hWnd) {
	//SetWindowText(GetDlgItem(hWnd, IDC_INV), "3");
	SetWindowText(GetDlgItem(hWnd, IDC_ZOOM), "1");
	SendMessage(GetDlgItem(hWnd, IDC_SLD_ZOOM), TBM_SETRANGE, 0, MAKELPARAM(1, 10));
	SendMessage(GetDlgItem(hWnd, IDC_SLD_ZOOM), TBM_SETPOS, TRUE, 1);

	SetWindowText(GetDlgItem(hWnd, IDC_ROT_X), toString(x_Rotate * 180.0f / PI).c_str());
	SetWindowText(GetDlgItem(hWnd, IDC_ROT_Y), toString(y_Rotate * 180.0f / PI).c_str());
	SetWindowText(GetDlgItem(hWnd, IDC_ROT_Z), toString(z_Rotate * 180.0f / PI).c_str());
	SetWindowText(GetDlgItem(hWnd, IDC_MODEL_NAME), modelName);
	SendMessage(GetDlgItem(hWnd, IDC_CHROME), BM_SETCHECK, BST_CHECKED, 0);

	SendMessage(GetDlgItem(hWnd, IDC_SLD_RX), TBM_SETRANGE, 0, MAKELPARAM(0, 180));
	SendMessage(GetDlgItem(hWnd, IDC_SLD_RY), TBM_SETRANGE, 0, MAKELPARAM(0, 180));
	SendMessage(GetDlgItem(hWnd, IDC_SLD_RZ), TBM_SETRANGE, 0, MAKELPARAM(0, 180));

	SendMessage(GetDlgItem(hWnd, IDC_SLD_RX), TBM_SETPOS, TRUE, 30);
	SendMessage(GetDlgItem(hWnd, IDC_SLD_RY), TBM_SETPOS, TRUE, 30);
	SendMessage(GetDlgItem(hWnd, IDC_SLD_RZ), TBM_SETPOS, TRUE, 30);

	SendMessage(GetDlgItem(hWnd, IDC_GRID_NUM), TBM_SETRANGE, 0, MAKELPARAM(10, 30));
	SendMessage(GetDlgItem(hWnd, IDC_GRID_THICKNESS), TBM_SETRANGE, 0, MAKELPARAM(2, 8));
	SendMessage(GetDlgItem(hWnd, IDC_GRID_NUM), TBM_SETPOS, TRUE, gridNum);
	SendMessage(GetDlgItem(hWnd, IDC_GRID_THICKNESS), TBM_SETPOS, TRUE, gridThickness * 10);

	SendMessage(GetDlgItem(hWnd, IDC_BTN_METALIC), TBM_SETRANGE, 0, MAKELPARAM(0, 1500));
	SendMessage(GetDlgItem(hWnd, IDC_BTN_METALIC), TBM_SETPOS, TRUE, 900);
	SetWindowText(GetDlgItem(hWnd, IDC_METALIC_VAL), toString(3.0f).c_str());
	SendMessage(GetDlgItem(hWnd, IDC_SURFACE), BM_SETCHECK, 0, 0);
	SendMessage(GetDlgItem(hWnd, IDC_MESH), BM_SETCHECK, 0, 0);
	SendMessage(GetDlgItem(hWnd, IDC_NORMAL_VERTEX), BM_SETCHECK, 0, 0);
	SendMessage(GetDlgItem(hWnd, IDC_NORMAL_PIXEL), BM_SETCHECK, 0, 0);
	SendMessage(GetDlgItem(hWnd, IDC_CHK_GRID), BM_SETCHECK, 0, 0);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_INITDIALOG:
		{
			SendMessage(GetDlgItem(hWnd, IDC_OPTIMIZE), BM_SETCHECK, 1, 0);

			InitDialog(hWnd);
		}
		break;
	case WM_NOTIFY:
		{
			wmId = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			switch (wmId)
			{
			case IDC_SLD_ZOOM:
				{
					int v = SendMessage(GetDlgItem(hWnd, IDC_SLD_ZOOM), TBM_GETPOS, 0, 0);
					zoomFactor = 1.0f + v * 0.1f;
					SetWindowText(GetDlgItem(hWnd, IDC_ZOOM), toString(v).c_str());
				}
				break;
			case IDC_BTN_METALIC:
				{
					int v = SendMessage(GetDlgItem(hWnd, IDC_BTN_METALIC), TBM_GETPOS, 0, 0);
					roi_Depth = (1500.0f - (float)v) / 300.0f;
					SetWindowText(GetDlgItem(hWnd, IDC_METALIC_VAL), toString(v / 300.0f).c_str());
				}
				break;
			case IDC_GRID_NUM:
				gridNum = SendMessage(GetDlgItem(hWnd, IDC_GRID_NUM), TBM_GETPOS, 0, 0);
				break;
			case IDC_GRID_THICKNESS:
				gridThickness = SendMessage(GetDlgItem(hWnd, IDC_GRID_THICKNESS), TBM_GETPOS, 0, 0) / 10.0f;
				break;
			case IDC_SLD_RX:
				SetTrackBar(hWnd, IDC_SLD_RX, IDC_ROT_X, &x_Rotate);
				break;
			case IDC_SLD_RY:
				SetTrackBar(hWnd, IDC_SLD_RY, IDC_ROT_Y, &y_Rotate);
				break;
			case IDC_SLD_RZ:
				SetTrackBar(hWnd, IDC_SLD_RZ, IDC_ROT_Z, &z_Rotate);
				break;
			default:
				break;
			}

			//if (wmId == IDC_SLD_RX || wmId == IDC_SLD_RY || wmId == IDC_SLD_RZ || IDC_SLD_ZOOM) 
			{
				doRender();
				normalizeDepth();
			}
			coatMaterial(hWnd);
			ShowResultImage(hWnd);
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		// Parse the menu selections:
		switch (wmId)
		{
		case IDC_MESH:
		case IDC_NORMAL_VERTEX:
		case IDC_NORMAL_PIXEL:
		case IDC_SURFACE:
		case IDC_CHK_GRID:
		//case IDC_INV:
			doRender();
			normalizeDepth();
			coatMaterial(hWnd);
			ShowResultImage(hWnd);
			break;
		case IDC_RENDER:
		//case IDC_ROTATE:
			{
				SendMessage(GetDlgItem(hWnd, IDC_SURFACE), BM_SETCHECK, 0, 0);
				SendMessage(GetDlgItem(hWnd, IDC_MESH), BM_SETCHECK, 0, 0);
				SendMessage(GetDlgItem(hWnd, IDC_NORMAL_VERTEX), BM_SETCHECK, 0, 0);
				SendMessage(GetDlgItem(hWnd, IDC_NORMAL_PIXEL), BM_SETCHECK, 0, 0);
				SendMessage(GetDlgItem(hWnd, IDC_CHK_GRID), BM_SETCHECK, 0, 0);

				char buf[MAX_PATH];
				GetWindowText(GetDlgItem(hWnd, IDC_ROT_X), buf, MAX_PATH);
				x_Rotate = atoi(buf) * PI / 180.0f;
				GetWindowText(GetDlgItem(hWnd, IDC_ROT_Y), buf, MAX_PATH);
				y_Rotate = atoi(buf) * PI / 180.0f;
				GetWindowText(GetDlgItem(hWnd, IDC_ROT_Z), buf, MAX_PATH);
				z_Rotate = atoi(buf) * PI / 180.0f;
				doRender();
				normalizeDepth();
				coatMaterial(hWnd);
				ShowResultImage(hWnd);
			}	
			break;
		case IDC_CHROME:
		case IDC_NEON:
			{
				coatMaterial(hWnd);
				ShowResultImage(hWnd);
			}
			break;
		case IDC_BROWSE:
		{
			OPENFILENAME ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hWnd;
			ofn.lpstrFile = szFile;
			// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
			// use the contents of szFile to initialize itself.
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "fbx\0*.fbx\0obj\0*.obj\0stl\0*.stl";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			GetOpenFileName(&ofn);
			if (!FileExists(szFile)) break;

			SetWindowText(GetDlgItem(hWnd, IDC_FILE_NAME), szFile);
			for (int i = 0; i < strlen(szFile); i++) {
				szFile[i] = toupper(szFile[i]);
			}
			char *p = NULL;
			for (int i = strlen(szFile); i >= 0; i--) {
				if (szFile[i] == '.') {
					p = szFile + i;
				}
			}

			if (p != NULL) {
				init3DOpt();
				InitDialog(hWnd);
				if (loadasset(szFile) != 0) {
					MessageBox(hWnd, "Failed to load Asset!", "Image2Bin", MB_OK);
					break;
				}
				if (SendMessage(GetDlgItem(hWnd, IDC_OPTIMIZE), BM_GETCHECK, 0, 0)) {
					OptimizeMesh();
				}
				doRender();
				normalizeDepth();
				coatMaterial(hWnd);
				ShowResultImage(hWnd);
			}
		}
			break;
		case IDC_SAVE:
		{
			static std::string path("");
			std::string str = BrowseFolder(path);

			if (str.empty()) break;
			path = str;
			GetWindowText(GetDlgItem(hWnd, IDC_MODEL_NAME), modelName, MAX_PATH);
			if (modelName == "") {
				MessageBox(NULL, "Please enter the model name!", "Image COnvertor", MB_OK);
				break;
			}
			if (SendMessage(GetDlgItem(hWnd, IDC_NEON), BM_GETCHECK, 0, 0)) {
				for (int i = 0; i < RESIZED_HEIGHT; i++) {
					for (int j = 0; j < RESIZED_WIDTH; j++) {
						float r = renderImg[i][j * 4];
						float g = renderImg[i][j * 4 + 1];
						float b = renderImg[i][j * 4 + 2];
						renderImg[i][j * 4] = 0.2126 * r + 0.7152 * g + 0.0722 * b;
						renderImg[i][j * 4 + 1] = 0.2126 * r + 0.7152 * g + 0.0722 * b;
						renderImg[i][j * 4 + 2] = 0.2126 * r + 0.7152 * g + 0.0722 * b;
					}
				}
			}
			writeLargeImage(path, 1);
			writeSmallImage(path, 1);
		}
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDCANCEL:
			DestroyWindow(hWnd);
			exit(0);
			break;
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		ShowResultImage(hWnd);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
