#include <windows.h>

#include <Windowsx.h>
#include <d2d1.h>
#include <cstdlib>
#include <list>
#include <stdio.h>
#include <memory>
#include <iostream>
using namespace std;
#include <vector>
using namespace std;
#include <algorithm>
using namespace std;

#pragma comment(lib, "d2d1")
template <class T>

void SafeRelease(T **ppT)
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
	static void Initialize(ID2D1Factory *pFactory)
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
		DERIVED_TYPE *pThis = NULL;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
			pThis = (DERIVED_TYPE *)pCreate->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
			pThis->m_hwnd = hwnd;
		}
		else
		{
			pThis = (DERIVED_TYPE *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
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
		WNDCLASS wc = {0};

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
	HCURSOR hCursor;
	ID2D1Factory *pFactory;
	ID2D1HwndRenderTarget *pRenderTarget;
	D2D1_SIZE_F prevSize;
	D2D1_POINT_2F ptMouse;
	ID2D1SolidColorBrush *pBrush;
	ID2D1SolidColorBrush *rectBrush;
	D2D1_ELLIPSE ellipses[15];
	int lengthOfEllipses;
	int selectedPoint;
	D2D1_RECT_F rectangle;
	float zoom = 1;
	vector<D2D1_ELLIPSE> hull;
    vector<HRESULT> edges;

	void SortPoints();
	void CreateButtons();
	void CalculateLayout();
	HRESULT CreateGraphicsResources();
	void DiscardGraphicsResources();
	void OnPaint();
	void Resize();
	void RandPoints();
	void OnLButtonDown(int x, int y);
	void OnMouseMove(int pixelX, int pixelY);
	BOOL HitTest(float x, float y);
	int findSide(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, D2D1_ELLIPSE p);
	int dist(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, D2D1_ELLIPSE p);
	void quickHull(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, int side);
	void CreateQuickHull(); 
	void DrawHull();
	
public:
	MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL)
	{
	}
	PCWSTR ClassName() const { return L"Convex Hull Algorithms"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

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

        rectangle = D2D1::RectF(
            230,
            0,
            250,
            size.height);

		// for (int i = 0; i < 15; i++)
		// {
		//     ellipses[i].point.x = diffInWidth;
		//     ellipses[i].point.y = diffInHeight;
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
		hull = vector<D2D1_ELLIPSE>();
		RandPoints();
		CreateQuickHull();
		DrawHull();
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
void MainWindow::OnLButtonDown(int x, int y)
{
	const float dipX = DPIScale::PixelsToDipsX(x);
	const float dipY = DPIScale::PixelsToDipsY(y);
	int hitIndex = HitTest(dipX, dipY);
	if (hitIndex != -1)
	{
		// we hit something
		selectedPoint = hitIndex;
	}
}

void MainWindow::CreateQuickHull() 
{
	hull = vector<D2D1_ELLIPSE>();
	D2D1_ELLIPSE left = ellipses[0];
	D2D1_ELLIPSE right = ellipses[0];
	int ileft = 0;
	int iright = 0;

	//Go through the list of ellipses to find the extreme top, left, right, bottom points
	for(int i=1; i < 15; i++) {
		D2D1_ELLIPSE pt = ellipses[i];
		if(pt.point.x < left.point.x)
			ileft = i;
		if(pt.point.x > right.point.x)
			iright = i;
	}

	quickHull(ellipses[ileft], ellipses[iright], 1);
	quickHull(ellipses[ileft], ellipses[iright], -1);
}

int MainWindow::findSide(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, D2D1_ELLIPSE p)
{
	int val = (p.point.y - p1.point.y) * (p2.point.x - p1.point.x) - 
	(p2.point.y - p1.point.y) * (p.point.x - p1.point.x);

	if(val > 0)
		return 1;
	if (val < 0)
		return -1;
	return 0;
}

int MainWindow::dist(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, D2D1_ELLIPSE p) 
{
	return abs((p.point.y - p1.point.y) * (p2.point.x - p1.point.x) - 
	(p2.point.y - p1.point.y) * (p.point.x - p1.point.x));
}

void MainWindow::quickHull(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, int side)
{
	int ifarthest = -1;
	int maxdist = 0;

	for(int i=0; i<15; i++)
	{
		int current = dist(p1, p2, ellipses[i]);
		if(findSide(p1, p2, ellipses[i]) == side && current > maxdist)
		{
			ifarthest = i;
			maxdist = current;
		}
	}

	if(ifarthest == -1)
	{
		hull.push_back(p1);
		hull.push_back(p2);
		
		return;
	}
	quickHull(ellipses[ifarthest], p1, -findSide(ellipses[ifarthest], p1, p2));
    quickHull(ellipses[ifarthest], p2, -findSide(ellipses[ifarthest], p2, p1));
}

void MainWindow::SortPoints()
{
	D2D1_ELLIPSE min = hull[0];
	for(int i=1; i<hull.size(); i++)
	{
		if(hull[i].point.x > min.point.x)
		{
			min = hull[i];
			D2D1_ELLIPSE temp = hull[0];
			hull[0] = min;
			hull[i] = temp;
		}
	}

	int y = hull[0].point.y;
	int x = hull[0].point.x;
	for(int j=1; j<hull.size(); j++)
	{
		for(int k=j+1; k<hull.size(); k++)
		{
			if((atan((hull[j].point.y-y)/(hull[j].point.x-x))) > (atan((hull[k].point.y-y)/(hull[k].point.x-x))))
			{
				D2D1_ELLIPSE temp = hull[j];
				hull[j] = hull[k];
				hull[k] = temp;
			}
		}
	}
}

void MainWindow::DrawHull()
{
	//Sort the points based on x value
	SortPoints();
	// char buffer[254] ={0};
	// sprintf(buffer, "BUFFER SIZE %i", hull.size());
	// MessageBoxA(NULL, buffer, "BUFF size", NULL);
	// for(int i=0; i < hull.size(); i++)
   	// 	std::cout << hull.at(i).point.x << ',' << hull.at(i).point.y << endl;
	
	//Draw the convex hull with the sorted point array
	PAINTSTRUCT ps;
	BeginPaint(m_hwnd, &ps);
	pRenderTarget->BeginDraw();

	HRESULT hr = S_OK;
	const D2D1_COLOR_F yellow = D2D1::ColorF(D2D1::ColorF::CadetBlue, 1.0f);
	hr = pRenderTarget->CreateSolidColorBrush(yellow, &pBrush);

	for(int i=0; i<hull.size()-1; i++)
	{
		// if (SUCCEEDED(hr)) {
		// 	CreateWindow(L"STATIC", L"SUCCEEDED", WS_VISIBLE | WS_CHILD | WS_BORDER, 200, 300, 300, 25, m_hwnd, NULL, NULL, NULL);
		// }
		//edges.push_back(
			pRenderTarget->DrawLine(
				D2D1::Point2F(static_cast<FLOAT>(hull[i].point.x), static_cast<FLOAT>(hull[i].point.y)),
				D2D1::Point2F(static_cast<FLOAT>(hull[i+1].point.x), static_cast<FLOAT>(hull[i+1].point.y)),
				pBrush,
				1.5f
			);
	//);		
	}

	//Draw the last edge from last point in the list to the first point in the list
	//edges.push_back(
	pRenderTarget->DrawLine(
        D2D1::Point2F(static_cast<FLOAT>(hull[hull.size()-1].point.x), static_cast<FLOAT>(hull[hull.size()-1].point.y)),
		D2D1::Point2F(static_cast<FLOAT>(hull[0].point.x), static_cast<FLOAT>(hull[0].point.y)),
        pBrush,
        0.5f
	);
	pRenderTarget->EndDraw();
	EndPaint(m_hwnd, &ps);
	//);	
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
			MessageBox(NULL, L"MinDiffDemo", L"AH! I GOT PRESSED", MB_ICONINFORMATION);
		}
		else if (LOWORD(wParam) == MinSumDemo)
		{
			MessageBox(NULL, L"MinSumDemo", L"AH! I GOT PRESSED", MB_ICONINFORMATION);
		}
		else if (LOWORD(wParam) == QuickHull)
		{
			RandPoints();
			CreateQuickHull();
			DrawHull();
		}
		else if (LOWORD(wParam) == PointCH)
		{
			RandPoints();
			MessageBox(NULL, L"PointCH", L"AH! I GOT PRESSED", MB_ICONINFORMATION);
		}
		else if (LOWORD(wParam) == GJK)
		{
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
		if (pRenderTarget != NULL)
		{
			RECT rc;
			GetClientRect(m_hwnd, &rc);

			D2D1_SIZE_U size;
			if (zoom > 0)
			{
				size == D2D1::SizeU(rc.right * zoom, rc.bottom * zoom);
			}
			else if (zoom < 0)
			{
				size = D2D1::SizeU(rc.right / zoom, rc.bottom / zoom);
			}

			pRenderTarget->Resize(size);
			CalculateLayout();
			InvalidateRect(m_hwnd, NULL, FALSE);
		}
	}
		// Other messages not shown...

	case WM_SIZE:
		Resize();
		return 0;
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}