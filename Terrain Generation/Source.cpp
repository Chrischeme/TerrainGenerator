#include <GL/glut.h>
#include <iostream>
#include "time.h"
#include "TextureHelper.h"
#include "SoftwareAgents.h"

float pi = 3.14159265358979323846f;

typedef struct Position {
	Position() {};
	Position(float x1, float y1, float z1, float w1) {
		x = x1;
		y = y1;
		z = z1;
		w = w1;
	};
	float x;
	float y;
	float z;
	float w;
} PosVector;

typedef struct Mousestr
{
	Mousestr() { x = y = prevx = prevy = 0; };
	bool rbutton, lbutton;
	int x, y, prevx, prevy;
} Mouse;

PosVector CameraPos, CameraMove;
Mouse mouse;
clock_t timeChange, timeCur, timePrev;
bool keys[256];
HINSTANCE globalHINS;
HWND globalHWND;
RECT globalRECT;
HDC globalHDC;
HGLRC globalHRC;
float alpha = 0.0f;
float beta = 0.0f;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
	switch (message){
	case WM_CREATE:
		break;

	case WM_SIZE:
		break;

	case WM_KEYDOWN:
		keys[wParam] = true;
		break;

	case WM_KEYUP:
		keys[wParam] = false;
		break;

	case WM_MOUSEMOVE:
		if (MK_LBUTTON & wParam) { mouse.lbutton = true; }
		else { mouse.lbutton = false; }
		if (MK_RBUTTON & wParam) { mouse.rbutton = true; }
		else { mouse.rbutton = false; }
		mouse.prevx = mouse.x;
		mouse.prevy = mouse.y;
		mouse.x = int(LOWORD(lParam));
		mouse.y = int(HIWORD(lParam));
		break;


	case WM_PAINT:
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

HWND createWindow(HINSTANCE hInstance) {
	HWND hwnd;
	WNDCLASS wcex;
	memset(&wcex, 0, sizeof(WNDCLASS));

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "FirstWindowClass";

	RegisterClass(&wcex);

	DWORD dw = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	globalHINS = hInstance;

	RECT glwindow;
	glwindow.left = 0;
	glwindow.right = 1200;
	glwindow.top = 0;
	glwindow.bottom = 800;
	AdjustWindowRect(&glwindow, dw, false);

	hwnd = CreateWindow("FirstWindowClass", "Terrain Generation", dw, 0, 0, glwindow.right - glwindow.left, glwindow.bottom - glwindow.top, NULL, NULL, hInstance, NULL);
	if (!hwnd) return NULL;
	ShowWindow(hwnd, SW_SHOWNORMAL);
	UpdateWindow(hwnd);
	SetFocus(hwnd);
	return hwnd;
}

bool setHDC(HDC hdc) {
	PIXELFORMATDESCRIPTOR pfd = { 0 };
	int pixelformat;

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);	
	pfd.nVersion = 1;						
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.dwLayerMask = PFD_MAIN_PLANE;			
	pfd.iPixelType = PFD_TYPE_RGBA;				
	pfd.cColorBits = 24;				
	pfd.cDepthBits = 24;				
	pfd.cAccumBits = 0;							
	pfd.cStencilBits = 0;						
	if ((pixelformat = ChoosePixelFormat(hdc, &pfd)) == false) {
		MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK);
		return false;
	}
	if (SetPixelFormat(hdc, pixelformat, &pfd) == false) {
		MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK);
		return false;
	}
	return true;
}

void ResizeGLWindow(int width, int height) {
	if (height == 0)
	{
		height = 1;
	}

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 1, 10000.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void initOpenGL(int width, int height)
{
	globalHDC = GetDC(globalHWND);
	if (!setHDC(globalHDC)) {
		PostQuitMessage(0);
	}
	globalHRC = wglCreateContext(globalHDC);	
	wglMakeCurrent(globalHDC, globalHRC);		
	ResizeGLWindow(width, height);	
}

void CreateScene() {
	initAgents();
	for (int z = 0; z < MAP_Z; z++)
	{
		for (int x = 0; x < MAP_X; x++)
		{
			terrain[x][z][0] = float(x)*MAP_SCALE;
			terrain[x][z][1] = (float)40;
			terrain[x][z][2] = -float(z)*MAP_SCALE;
			sea[x][z][0] = float(x)*MAP_SCALE;
			sea[x][z][1] = (float)50;
			sea[x][z][2] = -float(z)*MAP_SCALE;
		}
	}
	CoastlineAgent();
	BeachAgent();
	MountainAgent();
	RiverAgent();
}

void init(HWND hwnd) {
	globalHWND = hwnd;
	GetClientRect(globalHWND, &globalRECT);
	initOpenGL(globalRECT.right, globalRECT.bottom);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel(GL_SMOOTH);							
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);				
	glClearDepth(1.0f);									
	glEnable(GL_DEPTH_TEST);							                              
	glEnable(GL_TEXTURE_2D);							
	glEnable(GL_CULL_FACE);

	CreateScene();
	CameraPos = Position(0.0f, 30.0f, 0.0f, 1.0f);
	timeCur = timePrev = clock();
	srand(GLuint(timeCur));
}

void changeTime() {
	timeCur = clock();
	timeChange = timeCur - timePrev;
	timePrev = timeCur;
}

void normalize4dVec(Position vec) {
	float div = sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
	vec.x = vec.x / div;
	vec.y = vec.y / div;
	vec.z = vec.z / div;
}

void cameraControl() {
	if (mouse.lbutton) {
		alpha += 1.0f* float((mouse.x - mouse.prevx));
		beta += 1.0f* float((mouse.y - mouse.prevy));
		mouse.prevx = mouse.x;
		mouse.prevy = mouse.y;

		if (alpha>360.0f) {
			alpha -= 360.0f;
		}
		if (alpha<0) {
			alpha += 360.0f;
		}
		if (beta>89.9f) {
			beta = 89.9f;
		}
		if (beta<-89.9f) {
			beta = (-89.9f);
		}
	}

	CameraMove = Position(0.0f, 0.0f, 0.0f, 0.0f);

	if (keys['W']) {
		CameraMove.x += 1.0f*sinf(alpha / 180.0f*pi)*cosf(beta / 180.0f*pi);
		CameraMove.y += -1.0f*sinf(beta / 180.0f*pi);
		CameraMove.z += -1.0f*cosf(alpha / 180.0f*pi)*cosf(beta / 180.0f*pi);
		normalize4dVec(CameraMove);
	}
	if (keys['S']) {
		CameraMove.x += -1.0f*sinf(alpha / 180.0f*pi)*cosf(beta / 180.0f*pi);
		CameraMove.y += 1.0f*sinf(beta / 180.0f*pi);
		CameraMove.z += 1.0f*cosf(alpha / 180.0f*pi)*cosf(beta / 180.0f*pi);
		normalize4dVec(CameraMove);
	}
	if (keys['A']) {
		CameraMove.x += -1.0f*cosf(alpha / 180.0f*pi)*cosf(beta / 180.0f*pi);
		CameraMove.z += -1.0f*sinf(alpha / 180.0f*pi)*cosf(beta / 180.0f*pi);
		normalize4dVec(CameraMove);
	}
	if (keys['D']) {
		CameraMove.x += +1.0f*cosf(alpha / 180.0f*pi)*cosf(beta / 180.0f*pi);
		CameraMove.z += +1.0f*sinf(alpha / 180.0f*pi)*cosf(beta / 180.0f*pi);
		normalize4dVec(CameraMove);
	}

	CameraMove.x = CameraMove.x * 0.05f * float(timeChange) * 20;
	CameraMove.y = CameraMove.y * 0.05f * float(timeChange) * 20;
	CameraMove.z = CameraMove.z * 0.05f * float(timeChange) * 20;
	CameraPos.x = CameraPos.x + CameraMove.x;
	CameraPos.y = CameraPos.y + CameraMove.y;
	CameraPos.z = CameraPos.z + CameraMove.z;
	CameraPos.w = 1.0f;
}

void render() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glRotatef(beta, 1, 0, 0);
	glRotatef(alpha, 0, 1, 0);
	glTranslatef(0.0f - CameraPos.x, 0.0f - CameraPos.y, 0.0f - CameraPos.z);
	
	LoadGrass();
	for (int z = 0; z < MAP_Z - 1; z++)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < MAP_X - 1; x++)
		{
			glColor3f(terrain[x][z][1] / 255.0f, terrain[x][z][1] / 255.0f, terrain[x][z][1] / 255.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(terrain[x][z][0], terrain[x][z][1], terrain[x][z][2]);

			glTexCoord2f(1.0f, 0.0f);
			glColor3f(terrain[x + 1][z][1] / 255.0f, terrain[x + 1][z][1] / 255.0f, terrain[x + 1][z][1] / 255.0f);
			glVertex3f(terrain[x + 1][z][0], terrain[x + 1][z][1], terrain[x + 1][z][2]);

			glTexCoord2f(0.0f, 1.0f);
			glColor3f(terrain[x][z + 1][1] / 255.0f, terrain[x][z + 1][1] / 255.0f, terrain[x][z + 1][1] / 255.0f);
			glVertex3f(terrain[x][z + 1][0], terrain[x][z + 1][1], terrain[x][z + 1][2]);

			glColor3f(terrain[x + 1][z + 1][1] / 255.0f, terrain[x + 1][z + 1][1] / 255.0f, terrain[x + 1][z + 1][1] / 255.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(terrain[x + 1][z + 1][0], terrain[x + 1][z + 1][1], terrain[x + 1][z + 1][2]);
		}
		glEnd();
	}

	LoadWater();
	for (int z = 0; z < MAP_Z - 1; z++)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < MAP_X - 1; x++)
		{
			glColor3f(sea[x][z][1] / 255.0f, sea[x][z][1] / 255.0f, sea[x][z][1] / 255.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(sea[x][z][0], sea[x][z][1], sea[x][z][2]);

			glTexCoord2f(1.0f, 0.0f);
			glColor3f(sea[x + 1][z][1] / 255.0f, sea[x + 1][z][1] / 255.0f, sea[x + 1][z][1] / 255.0f);
			glVertex3f(sea[x + 1][z][0], sea[x + 1][z][1], sea[x + 1][z][2]);

			glTexCoord2f(0.0f, 1.0f);
			glColor3f(sea[x][z + 1][1] / 255.0f, sea[x][z + 1][1] / 255.0f, sea[x][z + 1][1] / 255.0f);
			glVertex3f(sea[x][z + 1][0], sea[x][z + 1][1], sea[x][z + 1][2]);

			glColor3f(sea[x + 1][z + 1][1] / 255.0f, sea[x + 1][z + 1][1] / 255.0f, sea[x + 1][z + 1][1] / 255.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(sea[x + 1][z + 1][0], sea[x + 1][z + 1][1], sea[x + 1][z + 1][2]);
		}
		glEnd();
	}

	glPopMatrix();

	SwapBuffers(globalHDC);
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int nCmdShow) {
	HWND hwnd;
	hwnd = createWindow(hInstance);
	if (hwnd == NULL) return true;
	init(hwnd);
	MSG msg;
	CameraPos.x = 2500;
	CameraPos.y = 1000;
	CameraPos.z = -2500;
	while (true) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			changeTime();
			cameraControl();
			render();
		}
	}
	return 0;
}