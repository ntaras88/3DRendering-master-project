#include "libImgCvt.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <dlfcn.h>

void *assimpHandle = NULL;

const char *(*AIGetErrorString)();
const aiScene *(*AIImportFile)(char *, unsigned int);
void(*AIIdentityMatrix4)(aiMatrix4x4 *);
void(*AIReleaseImport)(const aiScene *);
void(*AIMultiplyMatrix4)(aiMatrix4x4 *, const aiMatrix4x4 *);
void(*AITransformVecByMatrix4)(aiVector3D *, const aiMatrix4x4 *);

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "libImgCvt", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "libImgCvt", __VA_ARGS__))

const		aiScene* scene = NULL;

extern "C" {
	/* This trivial function returns the platform ABI for which this dynamic native library is compiled.*/
	const char * libImgCvt::getPlatformABI()
	{
	#if defined(__arm__)
	#if defined(__ARM_ARCH_7A__)
	#if defined(__ARM_NEON__)
		#define ABI "armeabi-v7a/NEON"
	#else
		#define ABI "armeabi-v7a"
	#endif
	#else
		#define ABI "armeabi"
	#endif
	#elif defined(__i386__)
		#define ABI "x86"
	#else
		#define ABI "unknown"
	#endif
		LOGI("This dynamic shared library is compiled with ABI: %s", ABI);
		return "This native library is compiled with ABI: %s" ABI ".";
	}

	char *jstring2string(JNIEnv *env, jstring jStr) {
		if (!jStr)
			return "";

		const jclass stringClass = env->GetObjectClass(jStr);
		const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
		const jbyteArray stringJbytes = (jbyteArray)env->CallObjectMethod(jStr, getBytes, env->NewStringUTF("UTF-8"));

		size_t length = (size_t)env->GetArrayLength(stringJbytes);
		char *ret = (char *)env->GetByteArrayElements(stringJbytes, NULL);
		ret[length] = 0;
		
		env->DeleteLocalRef(stringJbytes);
		env->DeleteLocalRef(stringClass);
		return ret;
	}

	JNIEXPORT jstring JNICALL Java_com_library_imgcvt_MainActivity_HelloJNI
	(JNIEnv *env, jobject obj, jstring a, jstring b) {
		std::string str1 = jstring2string(env, a);
		std::string str2 = jstring2string(env, b);
		char *str = strcat((char *)str1.c_str(), str2.c_str());
		return env->NewStringUTF(str);
	}

	JNIEXPORT jboolean JNICALL Java_com_library_imgcvt_MainActivity_IsDllFound(JNIEnv *env, jobject obj) {
		return true;
	}

	JNIEXPORT jint JNICALL Java_com_library_imgcvt_MainActivity_InitLibrary(JNIEnv *env, jobject obj) {
		void *assimpHandle = NULL;
		assimpHandle = dlopen("libassimp.so", RTLD_LAZY);
		if (assimpHandle == NULL) {
			return 1;
		}
		dlerror();

		AIImportFile = NULL;
		AIIdentityMatrix4 = NULL;
		AIReleaseImport = NULL;
		AIMultiplyMatrix4 = NULL;
		AITransformVecByMatrix4 = NULL;
		AIGetErrorString = NULL;

		AIImportFile = (const aiScene *(*)(char *, unsigned int))dlsym(assimpHandle, "aiImportFile");
		if (AIImportFile == NULL) {
			return 2;
		}
		dlerror();
		AIIdentityMatrix4 = (void(*)(aiMatrix4x4 *))dlsym(assimpHandle, "aiIdentityMatrix4");
		if (AIIdentityMatrix4 == NULL) {
			return 3;
		}
		dlerror();
		AIReleaseImport = (void(*)(const aiScene *))dlsym(assimpHandle, "aiReleaseImport");
		if (AIReleaseImport == NULL) {
			return 4;
		}
		dlerror();
		AIMultiplyMatrix4 = (void(*)(aiMatrix4x4 *, const aiMatrix4x4 *))dlsym(assimpHandle, "aiMultiplyMatrix4");
		if (AIMultiplyMatrix4 == NULL) {
			return 5;
		}
		dlerror();
		AITransformVecByMatrix4 = (void(*)(aiVector3D *, const aiMatrix4x4 *))dlsym(assimpHandle, "aiTransformVecByMatrix4");
		if (AITransformVecByMatrix4 == NULL)
		{
			return 6;
		}
		dlerror();
		AIGetErrorString = (const char *(*)())dlsym(assimpHandle, "aiGetErrorString");
		if (AIGetErrorString == NULL) {
			return 7;
		}
		dlerror();
		return 0;
	}

	JNIEXPORT jboolean JNICALL Java_com_library_imgcvt_MainActivity_TermLibrary(JNIEnv *env, jobject obj) {
		if (assimpHandle) {
			dlclose(assimpHandle);
			assimpHandle = NULL;
		}
		return true;
	}

	void coatOrdinary() {
		for (int i = 0; i < RESIZED_HEIGHT; i++) {
			for (int j = 0; j < RESIZED_WIDTH; j++) {
				if (indexMap1[i][j] == -1 || indexMap2[i][j] == -1) {
					renderImg[i][j * 4] = 0;
					renderImg[i][j * 4 + 1] = 0;
					renderImg[i][j * 4 + 2] = 0;
					renderImg[i][j * 4 + 3] = 1;
				}
				else {
					VERTEX v1 = VERTEX(1, 1, 1);
					VERTEX v2 = VERTEX(vertMap[i][j].nor.val[0], vertMap[i][j].nor.val[1], vertMap[i][j].nor.val[2]);
					v1 = doCross(&v1, &v2);
					double m = doMagnitudeForVertex(&v1);
					m = m * m * 70.0f;
					if (m > 255) m = 255;
					renderImg[i][j * 4] = m;
					renderImg[i][j * 4 + 1] = m;
					renderImg[i][j * 4 + 2] = m;
					renderImg[i][j * 4 + 3] = 1;
				}
			}
		}
	}

	void coatMetallic() {
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
						if (fabs(depth[i][j] - depth[i + x][j + y]) < roi_Depth) {
							sum++;
						}
					}
				}
				sum /= (double)((2 * ROI + 1) * (2 * ROI + 1));
				sum1 = sum;
				sum *= 255.0f;
				sum1 *= 255.0f;
				renderImg[i][j * 4] = round(sum);
				renderImg[i][j * 4 + 1] = round(sum1);
				renderImg[i][j * 4 + 2] = round(sum);
				renderImg[i][j * 4 + 3] = 1;
			}
		}
	}

	void coatChrome() {
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
						if (fabs(depth[i][j] - depth[i + x][j + y]) < roi_Depth) {
							sum++;
						}
					}
				}
				sum /= (double)((2 * ROI + 1) * (2 * ROI + 1));
				sum = pow(sum, 5);
				sum1 = 0.7f + sum;
				if (sum1 > 1.0f) sum1 = 1.0f;
				sum *= 255.0f;
				sum1 *= 255.0f;
				renderImg[i][j * 4] = round(sum);
				renderImg[i][j * 4 + 1] = round(sum1);
				renderImg[i][j * 4 + 2] = round(sum);
				renderImg[i][j * 4 + 3] = 1;
			}
		}
	}

	void get_bounding_box_for_node(const struct aiNode* nd, aiMatrix4x4* trafo)
	{
		aiMatrix4x4 prev;
		unsigned int n = 0, t;

		prev = *trafo;
		(*AIMultiplyMatrix4)(trafo, &nd->mTransformation);

		for (; n < nd->mNumMeshes; ++n) {
			const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
			SUBNODE subnode;
			for (t = 0; t < mesh->mNumVertices; ++t) {
				aiVector3D tmp = mesh->mVertices[t];
				(*AITransformVecByMatrix4)(&tmp, trafo);
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
			get_bounding_box_for_node(nd->mChildren[n], trafo);
		}
		*trafo = prev;
	}

	JNIEXPORT jint JNICALL Java_com_library_imgcvt_MainActivity_SetRotate(JNIEnv *env, jobject object, jfloat rX, jfloat rY) {
		x_Rotate = rX; y_Rotate = rY; z_Rotate = 0.0f;
		doRender(false);
		normalizeDepth();
		x_Rotate = 0.0f; y_Rotate = 0.0f; z_Rotate = 0.0f;
		return 0;
	}

	JNIEXPORT jstring JNICALL Java_com_library_imgcvt_MainActivity_LoadAsset(JNIEnv *env, jobject object, jstring path, jboolean bOptMesh, jdouble optRate)
	{
		//char *szPath = "/storage/emulated/0/pictures/woman.fbx";
		char *szPath = jstring2string(env, path);
		init3DOpt();
		scene = (*AIImportFile)(szPath, aiProcessPreset_TargetRealtime_Fast);
		aiMatrix4x4 trafo;
		(*AIIdentityMatrix4)(&trafo);
		
		if (scene) {
			get_bounding_box_for_node(scene->mRootNode, &trafo);
			(*AIReleaseImport)(scene);
			if(bOptMesh) OptimizeMesh(optRate);
			init_Render(true);
			correctCenter();
			doRender(false);
			normalizeDepth();
			return NULL;
		}
		return env->NewStringUTF((*AIGetErrorString)());
	}

	JNIEXPORT jboolean JNICALL Java_com_library_imgcvt_MainActivity_SetGridNumber(JNIEnv *env, jobject object, jint nGridNum) {
		gridNum = nGridNum;
		return true;
	}

	JNIEXPORT jboolean JNICALL Java_com_library_imgcvt_MainActivity_SetGridThickness(JNIEnv *env, jobject object, jfloat gridthss) {
		gridThickness = gridthss;
		return true;
	}

	JNIEXPORT jboolean JNICALL Java_com_library_imgcvt_MainActivity_SetROI(JNIEnv *env, jobject object, jdouble roi) {
		roi_Depth = roi;
		return true;
	}

	void Convert2Float() {
		for (int i = ROI; i < RESIZED_HEIGHT - ROI; i++) {
			for (int j = ROI; j < RESIZED_WIDTH - ROI; j++) {
				renderResult[i * (RESIZED_WIDTH * 4) + j * 4] = renderImg[i][j * 4];
				renderResult[i * (RESIZED_WIDTH * 4) + j * 4 + 1] = renderImg[i][j * 4 + 1];
				renderResult[i * (RESIZED_WIDTH * 4) + j * 4 + 2] = renderImg[i][j * 4 + 2];
				renderResult[i * (RESIZED_WIDTH * 4) + j * 4 + 3] = 1;
			}
		}
	}

	jintArray GetJniArray(JNIEnv *env) {
		jintArray outJNIArray = env->NewIntArray(RESIZED_HEIGHT * RESIZED_WIDTH * 4);  // allocate
		if (NULL == outJNIArray) return NULL;
		env->SetIntArrayRegion(outJNIArray, 0, RESIZED_HEIGHT * RESIZED_WIDTH * 4, renderResult);  // copy
		return outJNIArray;
	}

	JNIEXPORT jintArray JNICALL Java_com_library_imgcvt_MainActivity_GetNormalImage(JNIEnv *env, jobject object) {
		DrawAllNormal(RESIZED_HEIGHT / 60);
		Convert2Float();
		return GetJniArray(env);
	}

	JNIEXPORT jintArray JNICALL Java_com_library_imgcvt_MainActivity_GetMeshImage(JNIEnv *env, jobject object) {
		DrawMesh();
		Convert2Float();
		return GetJniArray(env);
	}
	
	JNIEXPORT jintArray JNICALL Java_com_library_imgcvt_MainActivity_GetGridImage(JNIEnv *env, jobject object) {
		DrawGrid();
		Convert2Float();
		return GetJniArray(env);
	}

	JNIEXPORT jintArray JNICALL Java_com_library_imgcvt_MainActivity_GetMetallicImage(JNIEnv *env, jobject obj) {
		coatMetallic();
		Convert2Float();
		return GetJniArray(env);
	}

	JNIEXPORT jintArray JNICALL Java_com_library_imgcvt_MainActivity_GetChromImage(JNIEnv *env, jobject obj) {
		coatChrome();
		Convert2Float();
		return GetJniArray(env);
	}

	JNIEXPORT jintArray JNICALL Java_com_library_imgcvt_MainActivity_GetSurface(JNIEnv *env, jobject obj) {
		coatOrdinary();
		Convert2Float();
		return GetJniArray(env);
	}
	
	void libImgCvt()
	{

	}

	libImgCvt::libImgCvt()
	{

	}

	libImgCvt::~libImgCvt()
	{

	}
}
