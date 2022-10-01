#include <windows.h>

#include <Windowsx.h>
#include <d2d1.h>
#include <cstdlib>
#include <list>

#include <memory>
#include <iostream>
using namespace std;
#include <vector>
using namespace std;
#include <algorithm>
using namespace std;

#pragma comment(lib, "d2d1")
template <class T>

void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}
class DPIScale
{
	static float scaleX;
	static float scaleY;

public:
	static void Initialize(ID2D1Factory* pFactory)
	{
		FLOAT dpiX, dpiY;
		pFactory->GetDesktopDpi(&dpiX, &dpiY);
		scaleX = dpiX / 96.0f;
		scaleY = dpiY / 96.0f;
	}

	template <typename T>
	static float PixelsToDipsX(T x)
	{
		return static_cast<float>(x) / scaleX;
	}

	template <typename T>
	static float PixelsToDipsY(T y)
	{
		return static_cast<float>(y) / scaleY;
	}
};

float DPIScale::scaleX = 1.0f;
float DPIScale::scaleY = 1.0f;
struct MyEllipse
{
	D2D1_ELLIPSE ellipse;
	D2D1_COLOR_F color;

	BOOL HitTest(float x, float y)
	{
		const float a = ellipse.radiusX;
		const float b = ellipse.radiusY;
		const float x1 = x - ellipse.point.x;
		const float y1 = y - ellipse.point.y;
		const float d = ((x1 * x1) / (a * a)) + ((y1 * y1) / (b * b));
		return d <= 1.0f;
	}
};
template <class DERIVED_TYPE>

class BaseWindow
{

#define MinDiffDemo 1
#define MinSumDemo 2
#define QuickHull 3
#define PointCH 4
#define GJK 5

public:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		DERIVED_TYPE* pThis = NULL;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
			pThis = (DERIVED_TYPE*)pCreate->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
			pThis->m_hwnd = hwnd;
		}
		else
		{
			pThis = (DERIVED_TYPE*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		}
		if (pThis)
		{
			return pThis->HandleMessage(uMsg, wParam, lParam);
		}
		else
		{
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}

	BaseWindow() : m_hwnd(NULL) {}

	BOOL Create(
		PCWSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = 0,
		HMENU hMenu = 0)
	{
		WNDCLASS wc = { 0 };

		wc.lpfnWndProc = DERIVED_TYPE::WindowProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.lpszClassName = ClassName();

		RegisterClass(&wc);

		m_hwnd = CreateWindowEx(
			dwExStyle, ClassName(), lpWindowName, dwStyle, x, y,
			nWidth, nHeight, hWndParent, hMenu, GetModuleHandle(NULL), this);

		return (m_hwnd ? TRUE : FALSE);
	}

	HWND Window() const { return m_hwnd; }

protected:
	virtual PCWSTR ClassName() const = 0;
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	HWND m_hwnd;
};

class MainWindow : public BaseWindow<MainWindow>
{
	//HONESTLY NO IDEA WHAT THESE DO I DONT THINK THEY ARE EVEN BEING USED
	HCURSOR hCursor;
	ID2D1Factory* pFactory;
	D2D1_POINT_2F ptMouse;
	D2D1_SIZE_F prevSize;

	//MODE SWITCH VARIABLES
	BOOL MDDMode = false;
	BOOL MSDMode = false;
	BOOL QuickHullMode = false;
	BOOL PCHMode = false;
	BOOL GJKMode = false;
	void SetMode(int x);
	int getMode();

	//Main Variables
	ID2D1HwndRenderTarget* pRenderTarget;
	D2D1_RECT_F rectangle;
	float zoom = 1; //Scroll factor

	//BRUSHES
	ID2D1SolidColorBrush* pBrush; //yellow
	ID2D1SolidColorBrush* rectBrush; //grey

	//QUICKHULL | POINT CONVEX HULL
	D2D1_ELLIPSE ellipses[15];
	int lengthOfEllipses;

	D2D1_POINT_2F pointCHTestPoint;



	//USER INTERACTION
	int selectedPoint; //point thats clicked on by mouse


	// vector<D2D1_LINE_JOIN_MITER> edges;

	void CreateButtons();//Makes the buttons
	void CalculateLayout(); //recalcs the layout
	HRESULT CreateGraphicsResources();
	void DiscardGraphicsResources();
	void OnPaint();
	void Resize();
	void RandPoints();
	//USER MOUSE INTERACTIONS:
	void OnLButtonDown(int x, int y);
	void OnMouseMove(int pixelX, int pixelY);
	BOOL HitTest(float x, float y);
	void MouseScroll();

public:
	MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL)
	{
	}
	PCWSTR ClassName() const { return L"Convex Hull Algorithms"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

void MainWindow::SetMode(int x)
{

	if (x == 0)
	{
		MDDMode = true;
		MSDMode = false;
		QuickHullMode = false;
		PCHMode = false;
		GJKMode = false;
	}
	else if (x == 1) {
		MDDMode = false;
		MSDMode = true;
		QuickHullMode = false;
		PCHMode = false;
		GJKMode = false;
	}
	else if (x == 2) {
		MDDMode = false;
		MSDMode = false;
		QuickHullMode = true;
		PCHMode = false;
		GJKMode = false;
	}
	else if (x == 3) {
		MDDMode = false;
		MSDMode = false;
		QuickHullMode = false;
		PCHMode = true;
		GJKMode = false;
	}
	else if (x == 4) {
		MDDMode = false;
		MSDMode = false;
		QuickHullMode = false;
		PCHMode = false;
		GJKMode = true;
	}
}

int MainWindow::getMode() {
	BOOL MDDMode = false;
	BOOL MSDMode = false;
	BOOL QuickHullMode = false;
	BOOL PCHMode = false;
	BOOL GJKMode = false;
	if (MDDMode) {
		return 0;
	}
	else if (MSDMode) {
		return 1;
	}
	else if (QuickHullMode) {
		return 2;
	}
	else if (PCHMode) {
		return 3;
	}
	else if (GJKMode) {
		return 4;
	}
	return -1;
}
BOOL HitTestEllipse(float x, float y, D2D1_ELLIPSE ellipse)
{
	const float a = ellipse.radiusX;
	const float b = ellipse.radiusY;
	const float x1 = x - ellipse.point.x;
	const float y1 = y - ellipse.point.y;
	const float d = ((x1 * x1) / (a * a)) + ((y1 * y1) / (b * b));
	return d <= 1.0f;
}

int MainWindow::HitTest(float x, float y)
{
	const float dipX = DPIScale::PixelsToDipsX(x);
	const float dipY = DPIScale::PixelsToDipsY(y);
	for (int i = 0; i < 15; i++)
	{
		if (HitTestEllipse(x, y, ellipses[i]))
		{
			return i;
		}
	}
	return -1;
}

// Recalculate drawing layout when the size of the window changes.

void MainWindow::CalculateLayout()
{
	if (pRenderTarget != NULL)
	{
		D2D1_SIZE_F size = pRenderTarget->GetSize();
		float scaleFact = (zoom / 100) + 1;
		rectangle = D2D1::RectF(
			230,
			0,
			250,
			size.height);

		// for (int i = 0; i < 15; i++)
		// {
		// 	ellipses[i].point.x *= scaleFact;
		// 	ellipses[i].point.y *= scaleFact;
		// }
	}
}

void MainWindow::OnMouseMove(int pixelX, int pixelY)
{
	const float dipX = DPIScale::PixelsToDipsX(pixelX);
	const float dipY = DPIScale::PixelsToDipsY(pixelY);
	if (selectedPoint != -1)
	{
		// if we got a point
		ellipses[selectedPoint].point.x = dipX;
		ellipses[selectedPoint].point.y = dipY;
		OnPaint();
		CreateButtons();
	}
}
HRESULT MainWindow::CreateGraphicsResources()
{
	HRESULT hr = S_OK;
	if (pRenderTarget == NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		hr = pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, size),
			&pRenderTarget);

		if (SUCCEEDED(hr))
		{
			const D2D1_COLOR_F yellow = D2D1::ColorF(D2D1::ColorF::Yellow, 1.0f);
			hr = pRenderTarget->CreateSolidColorBrush(yellow, &pBrush);
			const D2D1_COLOR_F grey = D2D1::ColorF(D2D1::ColorF::Gray, 1.0f);
			hr = pRenderTarget->CreateSolidColorBrush(grey, &rectBrush);
			if (SUCCEEDED(hr))
			{
				CalculateLayout();
			}
		}
	}
	return hr;
}

void MainWindow::DiscardGraphicsResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&pBrush);
}

void MainWindow::OnPaint()
{
	HRESULT hr = CreateGraphicsResources();

	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);

		pRenderTarget->BeginDraw();

		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
		if (lengthOfEllipses != 0)
		{
			for (int i = 0; i < 15; i++)
			{
				pRenderTarget->FillEllipse(ellipses[i], pBrush);
			}
		}

		pRenderTarget->FillRectangle(&rectangle, rectBrush);

		hr = pRenderTarget->EndDraw();
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
		{
			DiscardGraphicsResources();
		}

		EndPaint(m_hwnd, &ps);
	}
}
void MainWindow::RandPoints()
{
	if (pRenderTarget != NULL)
	{
		D2D1_SIZE_F size = pRenderTarget->GetSize();

		const float radius = 10.0;
		for (int i = 0; i < 15; i++)
		{
			ellipses[i] = D2D1::Ellipse(D2D1::Point2F((rand() % int(size.width / 2) + 200) + 230, rand() % int(size.height - 150) + 50), radius, radius);
		}
		lengthOfEllipses = 15;
		OnPaint();
		CreateButtons();
	}
}
// void MainWindow::DrawHull()
// {
//     //Sort points based on x value
//     auto sortrule = [] (D2D1_ELLIPSE const& e1, D2D1_ELLIPSE const& e2) -> bool
//     {
//         return e1.point.x < e2.point.x;
//     };

//     sort(hull.begin(), hull.end(), sortrule);

//     const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0);
//     pRenderTarget->CreateSolidColorBrush(color, &pBrush);
//     for(int i=0; i < hull.size()-1; i++)
//     {
//          edge = pRenderTarget->DrawLine(
//                 D2D1::Point2F(static_cast<FLOAT>(hull[i].point.x),static_cast<FLOAT>(hull[i].point.y)),
//                 D2D1::Point2F(static_cast<FLOAT>(hull[i+1].point.x),static_cast<FLOAT>(hull[i+1].point.y)),
//                 ,
//                 0.5f
//             )
//         edges.push_back(edge);

//     }
//     edges.push_back();
// }

void MainWindow::Resize()
{
	if (pRenderTarget != NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
		prevSize = pRenderTarget->GetSize();
		pRenderTarget->Resize(size);
		CalculateLayout();
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}
void MainWindow::MouseScroll()
{
	if (pRenderTarget != NULL)
	{
		D2D1_SIZE_F size = pRenderTarget->GetSize();
		float midX = (size.width + 250) / 2;
		float midY = size.height / 2;
		float scaleFact = zoom / 100;

		pRenderTarget->SetTransform(
			D2D1::Matrix3x2F::Scale(
				D2D1::Size(1 + scaleFact, 1 + scaleFact),
				D2D1::Point2F(midX, midY)));

		char buffer[256] = { 0 };
		wchar_t wtext[256];
		sprintf(buffer, "Value is %f %f", ellipses[0].point.x, 1 + scaleFact);
		mbstowcs(wtext, buffer, strlen(buffer) + 1);
		LPWSTR ptr = wtext;
		CreateWindow(L"STATIC", ptr, WS_VISIBLE | WS_CHILD | WS_BORDER, 200, 300, 300, 25, m_hwnd, NULL, NULL, NULL);
		// for (int i = 0; i < 15; i++)
		// {
		// 	double zoomFact = 0.04;
		// 	// SCROLL UP
		// 	if (zoom < 0)
		// 	{

		// 		if (ellipses[i].radiusX < 50)
		// 		{
		// 			ellipses[i].radiusX *= 1 + zoomFact;
		// 			ellipses[i].radiusY *= 1 + zoomFact;
		// 		}

		// 		// if to the left of the midX
		// 		if (ellipses[i].point.x < midX)
		// 		{
		// 			// this adds the zoom
		// 			ellipses[i].point.x *= 1 - zoomFact;
		// 			if (ellipses[i].point.y < midY)
		// 			{
		// 				ellipses[i].point.y *= 1 - zoomFact;
		// 			}
		// 			else if (ellipses[i].point.y > midY)
		// 			{
		// 				ellipses[i].point.y *= 1 + zoomFact;
		// 			}
		// 		}
		// 		else if (ellipses[i].point.x > midX)
		// 		{
		// 			// this adds the zoom
		// 			ellipses[i].point.x *= 1 + zoomFact;
		// 			if (ellipses[i].point.y < midY)
		// 			{
		// 				ellipses[i].point.y *= 1 - zoomFact;
		// 			}
		// 			else if (ellipses[i].point.y > midY)
		// 			{
		// 				ellipses[i].point.y *= 1 + zoomFact;
		// 			}
		// 		}
		// 	} // SCROLL DOWN - zoom out
		// 	if (zoom > 0)
		// 	{

		// 		if (ellipses[i].radiusX > 20)
		// 		{
		// 			ellipses[i].radiusX *= 1 - zoomFact;
		// 			ellipses[i].radiusY *= 1 - zoomFact;
		// 		}

		// 		// if to the left of the midX
		// 		if (ellipses[i].point.x < midX)
		// 		{
		// 			// this adds the zoom
		// 			ellipses[i].point.x *= 1 + zoomFact;
		// 			if (ellipses[i].point.y < midY)
		// 			{
		// 				ellipses[i].point.y *= 1 + zoomFact;
		// 			}
		// 			else if (ellipses[i].point.y > midY)
		// 			{
		// 				ellipses[i].point.y *= 1 - zoomFact;
		// 			}
		// 		}
		// 		else if (ellipses[i].point.x > midX)
		// 		{
		// 			// this adds the zoom
		// 			ellipses[i].point.x *= 1 - zoomFact;
		// 			if (ellipses[i].point.y < midY)
		// 			{
		// 				ellipses[i].point.y * 1 + zoomFact;
		// 			}
		// 			else if (ellipses[i].point.y > midY)
		// 			{
		// 				ellipses[i].point.y *= 1 - zoomFact;
		// 			}
		// 		}
		// 	}
		// }

		OnPaint();
		CreateButtons();
	}
}

void MainWindow::OnLButtonDown(int x, int y)
{
	const float dipX = DPIScale::PixelsToDipsX(x);
	const float dipY = DPIScale::PixelsToDipsY(y);
	int hitIndex = HitTest(dipX, dipY);

	char buffer[256] = { 0 };
	wchar_t wtext[256];
	sprintf(buffer, "Value is %f %f", dipX, dipY);
	mbstowcs(wtext, buffer, strlen(buffer) + 1);
	LPWSTR ptr = wtext;
	CreateWindow(L"STATIC", ptr, WS_VISIBLE | WS_CHILD | WS_BORDER, 200, 500, 300, 25, m_hwnd, NULL, NULL, NULL);
	if (hitIndex != -1)
	{
		// we hit something
		selectedPoint = hitIndex;
	}
}
void MainWindow::CreateButtons()
{
	CreateWindow(TEXT("BUTTON"), TEXT("Minkowski Difference Demo"), WS_CHILD | WS_VISIBLE, 10, 10, 200, 30, m_hwnd, (HMENU)MinDiffDemo, NULL, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("Minkowski Sum Demo"), WS_CHILD | WS_VISIBLE, 10, 110, 200, 30, m_hwnd, (HMENU)MinSumDemo, NULL, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("Quickhull"), WS_CHILD | WS_VISIBLE, 10, 210, 200, 30, m_hwnd, (HMENU)QuickHull, NULL, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("Point Convex Hull"), WS_CHILD | WS_VISIBLE, 10, 310, 200, 30, m_hwnd, (HMENU)PointCH, NULL, NULL);
	CreateWindow(TEXT("BUTTON"), TEXT("GJK"), WS_CHILD | WS_VISIBLE, 10, 410, 200, 30, m_hwnd, (HMENU)GJK, NULL, NULL);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	MainWindow win;

	if (!win.Create(L"Convex Hull Algorithms", WS_OVERLAPPEDWINDOW))
	{
		return 0;
	}

	ShowWindow(win.Window(), nCmdShow);

	// Run the message loop.

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{

	case WM_CREATE:
		if (FAILED(D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
		{
			return -1; // Fail CreateWindowEx.
		}
		selectedPoint = -1;
		lengthOfEllipses = 0;
		CreateButtons();
	case WM_COMMAND:
	{
		// IF BUTTON PRESSURED AND THAT ID IS OUR ID
		if (LOWORD(wParam) == MinDiffDemo)
		{
			if (pRenderTarget != NULL)
			{
				SetMode(0);
				pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
			}
		}
		else if (LOWORD(wParam) == MinSumDemo)
		{
			if (pRenderTarget != NULL)
			{
				SetMode(1);
				pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
			}
		}
		else if (LOWORD(wParam) == QuickHull)
		{
			RandPoints();
			if (pRenderTarget != NULL)
			{
				SetMode(2);
				pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
			}

		}
		else if (LOWORD(wParam) == PointCH)
		{
			RandPoints();
			if (pRenderTarget != NULL)
			{
				SetMode(3);
				pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
			}

		}
		else if (LOWORD(wParam) == GJK)
		{
			if (pRenderTarget != NULL)
			{
				SetMode(4);
				pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
			}
			MessageBox(NULL, L"GJK", L"AH! I GOT PRESSED", MB_ICONINFORMATION);
		}
		return 0;
	}
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT)
		{
			SetCursor(hCursor);
			return TRUE;
		}
		break;
	case WM_LBUTTONDOWN:
		OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
		selectedPoint = -1;
		return 0;
	case WM_DESTROY:
		DiscardGraphicsResources();
		SafeRelease(&pFactory);
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:

		OnPaint();
		return 0;
	case WM_MOUSEWHEEL:
	{
		// how many times the scroll wheel was scrolled
		zoom += (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
		MouseScroll();
	}
	// Other messages not shown...

	case WM_SIZE:
		Resize();
		return 0;
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}