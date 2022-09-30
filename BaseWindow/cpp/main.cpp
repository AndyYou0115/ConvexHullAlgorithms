#include <windows.h>

#include <Windowsx.h>
#include <d2d1.h>

#include <list>
#include <memory>
using namespace std;
#include <vector>
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

        if (uMsg == WM_CREATE)
        {
            CreateWindow(TEXT("BUTTON"), TEXT("Minkowski Difference Demo"), WS_CHILD | WS_VISIBLE, 10, 10, 200, 30, hwnd, (HMENU)MinDiffDemo, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Minkowski Sum Demo"), WS_CHILD | WS_VISIBLE, 10, 110, 200, 30, hwnd, (HMENU)MinSumDemo, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Quickhull"), WS_CHILD | WS_VISIBLE, 10, 210, 200, 30, hwnd, (HMENU)QuickHull, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Point Convex Hull"), WS_CHILD | WS_VISIBLE, 10, 310, 200, 30, hwnd, (HMENU)PointCH, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("GJK"), WS_CHILD | WS_VISIBLE, 10, 410, 200, 30, hwnd, (HMENU)GJK, NULL, NULL);
        }
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
    ID2D1Factory *pFactory;
    ID2D1HwndRenderTarget *pRenderTarget;
    ID2D1SolidColorBrush *pBrush;
    D2D1_ELLIPSE ellipses[15];
    D2D1_RECT_F rectangle;
	vector<D2D1_ELLIPSE> hull;

    void CalculateLayout();
    HRESULT CreateGraphicsResources();
    void DiscardGraphicsResources();
    void OnPaint();
    void Resize();
	int findSide(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, D2D1_ELLIPSE p);
	int dist(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, D2D1_ELLIPSE p);
	void quickHull(D2D1_ELLIPSE pointlist[], D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, int side);
	D2D1_ELLIPSE CreateQuickHull(D2D1_ELLIPSE pointlist[15]); 

public:
    MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL)
    {
    }
    PCWSTR ClassName() const { return L"Convex Hull Algorithms"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
// Recalculate drawing layout when the size of the window changes.

void MainWindow::CalculateLayout()
{
    if (pRenderTarget != NULL)
    {
        D2D1_SIZE_F size = pRenderTarget->GetSize();
        const float x = size.width / 4;
        const float y = size.height / 4;

        const float radius = 10.0;

        rectangle = D2D1::RectF(
            230,
            0,
            250,
            size.height);
        for (int i = 0; i < 15; i++)
        {
            // ellipses[i] = &D2D1::Ellipse(D2D1::Point2F(x + rand() % 100 - 100, y + rand() % 100 - 100), radius, radius);
            ellipses[i] = D2D1::Ellipse(D2D1::Point2F(x + rand() % int(size.width - 50) + 250, y + rand() % int(size.height - 50) + 50), radius, radius);
        }
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
            const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);

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
        for (int i = 0; i < 15; i++)
        {
            pRenderTarget->FillEllipse(ellipses[i], pBrush);
        }

        pRenderTarget->FillRectangle(&rectangle, pBrush);

        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
        EndPaint(m_hwnd, &ps);
    }
}

D2D1_ELLIPSE MainWindow::CreateQuickHull(D2D1_ELLIPSE pointlist[15]) 
{
	D2D1_ELLIPSE left = pointlist[0];
	D2D1_ELLIPSE right = pointlist[0];
	int ileft = 0;
	int iright = 0;

	//Go through the list of ellipses to find the extreme top, left, right, bottom points
	for(int i=1; i < 15; i++) {
		D2D1_ELLIPSE pt = pointlist[i];
		if(pt.point.x < left.point.x)
			ileft = i;
		if(pt.point.x > right.point.x)
			iright = i;
	}

	quickHull(pointlist, pointlist[ileft], pointlist[iright], 1);
	quickHull(pointlist, pointlist[ileft], pointlist[iright], -1);
}

int MainWindow::findSide(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, D2D1_ELLIPSE p)
{
	int val = (p.point.y - p1.point.y) * (p2.point.x - p1.point.x) - 
	(p2.point.y - p1.point.y) * (p.point.x - p1.point.x);

	if(val > 0)
		return 1;
	else
		return -1;
}

int MainWindow::dist(D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, D2D1_ELLIPSE p) 
{
	return abs((p.point.y - p1.point.y) * (p2.point.x - p1.point.x) - 
	(p2.point.y - p1.point.y) * (p.point.x - p1.point.x));
}

void MainWindow::quickHull(D2D1_ELLIPSE pointlist[], D2D1_ELLIPSE p1, D2D1_ELLIPSE p2, int side)
{
	int ifarthest = -1;
	int maxdist = 0;

	for(int i=0; i<15; i++)
	{
		int current = dist(p1, p2, pointlist[i]);
		if(findSide(p1, p2, pointlist[i]) == side && current > maxdist)
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

	quickHull(pointlist, pointlist[ifarthest], p1, -findSide(pointlist[ifarthest], p1, p2));
    quickHull(pointlist, pointlist[ifarthest], p2, -findSide(pointlist[ifarthest], p2, p1));
}

void MainWindow::Resize()
{
    if (pRenderTarget != NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        pRenderTarget->Resize(size);
        CalculateLayout();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
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
			CreateQuickHull(ellipses);
			//Draw convex hull based on points in vector hull
        }
        else if (LOWORD(wParam) == PointCH)
        {
            MessageBox(NULL, L"PointCH", L"AH! I GOT PRESSED", MB_ICONINFORMATION);
        }
        else if (LOWORD(wParam) == GJK)
        {
            MessageBox(NULL, L"GJK", L"AH! I GOT PRESSED", MB_ICONINFORMATION);
        }
        return 0;
    }

    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        OnPaint();
        return 0;

        // Other messages not shown...

    case WM_SIZE:
        Resize();
        return 0;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}
