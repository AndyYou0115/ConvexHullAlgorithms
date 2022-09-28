#include <windows.h>

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
public:
	PCWSTR ClassName() const { return L"Sample Window Class"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

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
			MessageBox(NULL, L"QuickHull", L"AH! I GOT PRESSED", MB_ICONINFORMATION);
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
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(m_hwnd, &ps);
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(m_hwnd, &ps);
	}
	return 0;

	default:
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
	return TRUE;
}
