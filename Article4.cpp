//--------------------------------------------------------------------------------------
// File: Artlcle4.cpp
// Установка матриц трансформаций и камеры
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11.h>
#include <d2d1.h>
#include <dwrite.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include "resource.h"
#include <vector>
#include <iostream>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

//--------------------------------------------------------------------------------------
// Структуры
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};


struct ConstantBuffer
{
    XMMATRIX mWorld;
    XMMATRIX mView;
    XMMATRIX mProjection;
    float time;
};


//--------------------------------------------------------------------------------------
// Глобальные переменные
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = NULL;
HWND                    g_hWnd = NULL;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device* g_pd3dDevice = NULL;
ID3D11DeviceContext* g_pImmediateContext = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;
ID3D11Texture2D* g_pDepthStencil = NULL;
ID3D11DepthStencilView* g_pDepthStencilView = NULL;
ID3D11VertexShader* g_pVertexShader = NULL;
ID3D11PixelShader* g_pPixelShader = NULL;
ID3D11InputLayout* g_pVertexLayout = NULL;
ID3D11Buffer* g_pVertexBuffer = NULL;
ID3D11Buffer* g_pVertexBufferFigure2 = NULL;
ID3D11Buffer* g_pIndexBuffer = NULL;
ID3D11Buffer* g_pIndexBufferFigure2 = NULL;
ID3D11Buffer* g_pConstantBuffer = NULL;
XMMATRIX                g_World;
XMMATRIX                g_View;
XMMATRIX                g_Projection;

float                   cameraX;
float                   cameraY;
float                   cameraZ;
float                   cameraRotX;
float                   cameraRotY;
XMVECTOR                At;
XMVECTOR                Up;


//Dynamic
int                     firstFigureCount;
SimpleVertex* firstFigureBuffer;


//UI
ID2D1Factory* pD2DFactory = NULL;
ID2D1RenderTarget* pRT = NULL;
ID2D1SolidColorBrush* pWhiteBrush = NULL;
ID2D1SolidColorBrush* pRedBrush = NULL;

//Text 
IDWriteFactory* m_pDWriteFactory = NULL;
IDWriteTextFormat* m_pTextFormat = NULL;


//Controls

POINT firstCursorPlace;


//--------------------------------------------------------------------------------------
// Объявления функций
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
HRESULT InitGeometry();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();



//Shader
void ChangeColor(float);

//SkyBox



//--------------------------------------------------------------------------------------
// Главная функция программы. Происходят все инициализации, и затем выполняется
// цикл обработки сообщений
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (FAILED(InitWindow(hInstance, nCmdShow)))
        return 0;

    if (FAILED(InitDevice()))
    {
        CleanupDevice();
        return 0;
    }
    if (FAILED(InitGeometry()))
    {
        return 0;
    }



    // Цикл обработки сообщений
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return (int)msg.wParam;
}


//--------------------------------------------------------------------------------------
// Создание окна
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"Article4";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hWnd = CreateWindow(L"Article4", L"Кутасевич 924404: Различные фигуры, камера, реакция на клавиатуру и мышь, текст", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
        NULL);
    if (!g_hWnd)
        return E_FAIL;

    ShowWindow(g_hWnd, nCmdShow);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Вспомогательная функция для компилирования шейдеров DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
    if (FAILED(hr))
    {
        if (pErrorBlob != NULL)
        {
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
            char* a = (char*)pErrorBlob->GetBufferPointer();
        }
        if (pErrorBlob) pErrorBlob->Release();
        return hr;
    }
    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Инициализация Direct3D устройства
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(g_hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    createDeviceFlags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#pragma region 3dContext

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return hr;

    // Создание рендер-таргета
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr))
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);

    pBackBuffer->Release();
    if (FAILED(hr))
        return hr;

    // Создание поверхности для Z-буфера
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
    if (FAILED(hr))
        return hr;

    // Создание z-буфреа
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
    if (FAILED(hr))
        return hr;

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports(1, &vp);
#pragma endregion

    //UI
#pragma region UI

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
    if (FAILED(hr))
        return hr;




    IDXGISurface* pBackBuffer2 = NULL;

    hr = g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer2));
    if (SUCCEEDED(hr))
    {

        FLOAT dpiX;
        FLOAT dpiY;
        pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);
        D2D1_RENDER_TARGET_PROPERTIES props =
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
                dpiX,
                dpiY
            );


        hr = pD2DFactory->CreateDxgiSurfaceRenderTarget(pBackBuffer2, &props, &pRT);


        if (SUCCEEDED(hr)) {
            pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pWhiteBrush);
            pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &pRedBrush);
        }
    }

#pragma endregion

    //Text

#pragma region Text

    static const WCHAR msc_fontName[] = L"Colibri";
    static const FLOAT msc_fontSize = 50;


    // Create a DirectWrite factory.
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory));

    if (SUCCEEDED(hr))
    {
        // Create a DirectWrite text format object.
        hr = m_pDWriteFactory->CreateTextFormat(
            msc_fontName,
            NULL,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            msc_fontSize,
            L"", //locale
            &m_pTextFormat
        );

        if (SUCCEEDED(hr))
        {
            // Center the text horizontally and vertically.
            m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

            m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        }
    }



#pragma endregion

    return S_OK;
}
//--------------------------------------------------------------------------------------
// Инициализация геометрии
//--------------------------------------------------------------------------------------
HRESULT InitGeometry()
{
    RECT rc;
    GetClientRect(g_hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    // Загружаем шейдеры
    ID3DBlob* pVSBlob = NULL;
    HRESULT hr;
    hr = CompileShaderFromFile(L"Article4.fx", "VS", "vs_4_0", &pVSBlob);
    if (FAILED(hr))
    {
        MessageBox(NULL,
            L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.asd", L"Error", MB_OK);
        return hr;
    }

    // Вершинный шейдер
    hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return hr;
    }


    // Определение формата вершинного буфера
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    // Создание формата буфера
    hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), &g_pVertexLayout);
    pVSBlob->Release();
    if (FAILED(hr))
        return hr;

    // Установка формата буфера
    g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

    // Пиксельный шейдер
    ID3DBlob* pPSBlob = NULL;
    hr = CompileShaderFromFile(L"Article4.fx", "PS", "ps_4_0", &pPSBlob);
    if (FAILED(hr))
    {
        MessageBox(NULL,
            L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.qwe", L"Error", MB_OK);
        return hr;
    }

    // Пиксельный шейдер
    hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader);
    pPSBlob->Release();
    if (FAILED(hr))
        return hr;

    // Создание геометрии для вершинного буфера
    SimpleVertex vertices[] =
    {

        { XMFLOAT3(0.0f, 1.0f,  0.0f),       XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)     }, // вершина 0
        { XMFLOAT3(0.0f, 0.45f,  0.9f),      XMFLOAT4(0.0f, 0.5f, 1.0f, 1.0f)     }, // вершина 1
        { XMFLOAT3(-0.85f, 0.45f,  0.28f),   XMFLOAT4(-1.0f, 0.5f, 0.2f, 1.0f)    }, // вершина 2
        { XMFLOAT3(-0.52f,  0.45f,  -0.72f), XMFLOAT4(-0.5f, 0.5f, -0.5f, 1.0f)   }, // вершина 3
        { XMFLOAT3(0.52f,  0.45f,  -0.72f),  XMFLOAT4(0.5f, 0.5f, -0.5f, 1.0f)    }, // вершина 4
        { XMFLOAT3(0.85f, 0.45f,  0.28f),    XMFLOAT4(1.0f, 0.5f, 0.2f, 1.0f)     }, // вершина 5

        { XMFLOAT3(-0.52f,  -0.45f,  0.72f), XMFLOAT4(-0.2f, -0.5f, 0.8f, 1.0f)   }, // вершина 6
        { XMFLOAT3(-0.85f, -0.45f,  -0.27f), XMFLOAT4(-1.0f, -0.5f, -0.2, 1.0f)   }, // вершина 7
        { XMFLOAT3(0.0f, -0.45f,  -0.9f),    XMFLOAT4(0.0f, -0.5f, -1.0f, 1.0f)   }, // вершина 8
        { XMFLOAT3(0.85f, -0.45f,  -0.27f),  XMFLOAT4(1.0f, -0.5f, -0.2, 1.0f)    }, // вершина 9
        { XMFLOAT3(0.52f,  -0.45f,  0.72f),  XMFLOAT4(0.2f, -0.5f, 0.8f, 1.0f)    }, // вершина 10
        { XMFLOAT3(0.0f, -1.0f,  0.0f),      XMFLOAT4(0.0f, -1.0f, 0.0f, 1.0f)    }  // вершина 11 


    };
    firstFigureCount = 12;

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(SimpleVertex) * 12;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
    //bd.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;



    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
    if (FAILED(hr))
        return hr;

    firstFigureBuffer = new SimpleVertex[firstFigureCount];
    for (int i = 0; i < firstFigureCount; i++)
    {
        firstFigureBuffer[i] = vertices[i];
    }



    // Создание индексного буфера
    WORD indices[] =
    {
        1,0,2,	/* Треугольник 1 = vertices[0], vertices[2], vertices[3] */
        2,0,3,	/* Треугольник 2 = vertices[0], vertices[3], vertices[4] */
        3,0,4,	/* и т. д. */
        4,0,5,
        5,0,1,

        1,2,6,
        2,3,7,
        3,4,8,
        4,5,9,
        5,1,10,

        1,6,10,
        6,2,7,
        7,3,8,
        8,4,9,
        9,5,10,

        11,6,7,
        7, 8, 11,
        11,8,9,
        11,9,10,
        11,10,6
    };
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * 60;        // 36 vertices needed for 12 triangles in a triangle list
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer);
    if (FAILED(hr))
        return hr;


    //создание геометрии для второй фигуры
    SimpleVertex vertices2[] =
    {
        { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, -1.0f, -1.0f),  XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
    };



    bd.ByteWidth = sizeof(SimpleVertex) * 8;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    InitData.pSysMem = vertices2;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBufferFigure2);
    if (FAILED(hr))
        return hr;


    // Создание индексного буфера второй фигуры

    WORD indices2[] =
    {
        3,1,0,
        2,1,3,

        0,5,4,
        1,5,0,

        3,4,7,
        0,4,3,

        1,6,5,
        2,6,1,

        2,7,6,
        3,7,2,

        6,4,5,
        7,4,6
    };

    bd.ByteWidth = sizeof(WORD) * 36;
    InitData.pSysMem = indices2;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBufferFigure2);
    if (FAILED(hr))
        return hr;


    // Установка индексного буфера
    g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    // Установка типа примитив
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Создание буфера констант шейдера
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pConstantBuffer);
    if (FAILED(hr))
        return hr;

    cameraX = 0.0f;
    cameraY = 1.0f;
    cameraZ = 10.0f;

    cameraRotX = 0.0f;
    cameraRotY = 0.0f;


    // Установка матриц
    g_World = XMMatrixIdentity();
    XMVECTOR Eye = XMVectorSet(cameraX, cameraY, cameraZ, 0.0f);
    XMVECTOR At = XMVectorSet(cameraX, cameraY, 0.0f, 0.0f);
    Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    g_View = XMMatrixRotationY(cameraRotX) * XMMatrixRotationX(cameraRotY) * XMMatrixLookAtLH(Eye, At, Up);
    g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f);


    GetCursorPos(&firstCursorPlace);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Очистка переменных
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if (g_pImmediateContext) g_pImmediateContext->ClearState();

    if (g_pConstantBuffer) g_pConstantBuffer->Release();
    if (g_pVertexBuffer) g_pVertexBuffer->Release();
    if (g_pVertexBufferFigure2) g_pVertexBufferFigure2->Release();
    if (g_pIndexBuffer) g_pIndexBuffer->Release();
    if (g_pIndexBufferFigure2) g_pIndexBufferFigure2->Release();
    if (g_pVertexLayout) g_pVertexLayout->Release();
    if (g_pVertexShader) g_pVertexShader->Release();
    if (g_pPixelShader) g_pPixelShader->Release();
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
    if (pD2DFactory) pD2DFactory->Release();
    if (pRT) pRT->Release();
    if (pWhiteBrush) pWhiteBrush->Release();
    if (m_pDWriteFactory) m_pDWriteFactory->Release();
    if (m_pTextFormat) m_pTextFormat->Release();
    if (firstFigureBuffer) delete[] firstFigureBuffer;
}


//--------------------------------------------------------------------------------------
// Процедура обработки сообщений Windows
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}




void ChangeColor(float colorVar)
{
    SimpleVertex* outp = new SimpleVertex[firstFigureCount];
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
    for (int i = 0; i < firstFigureCount; i++)
    {
        outp[i] = firstFigureBuffer[i];
        outp[i].Color.x *= sin(colorVar);
    }

    g_pImmediateContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, outp, sizeof(SimpleVertex) * firstFigureCount);

    g_pImmediateContext->Unmap(g_pVertexBuffer, 0);
    delete[] outp;
}


//--------------------------------------------------------------------------------------
// Значения связанные с камерой
//--------------------------------------------------------------------------------------
float orbit = 0.0f;
float rotation = 0.0f;
float height = 0.0f;
bool goUp = true;


float color = 0.01f;
float increase = 0.005f;
float sens = 0.003f;


//--------------------------------------------------------------------------------------
// Рендер
//--------------------------------------------------------------------------------------
void Render()
{

    //
    // Очистка рендер-таргета
    //
    float ClearColor[4] = { 0.1f, 0.0f, 0.0f, 1.0f }; // цвет
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
    g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    if (GetKeyState(0x57) & 0x8000)
    {
        cameraY += increase;
    }

    else if (GetKeyState(0x53) & 0x8000)
    {
        cameraY -= increase;
    }

    if (GetKeyState(0x41) & 0x8000)
    {
        cameraX += increase;
    }
    else if (GetKeyState(0x44) & 0x8000)
    {
        cameraX -= increase;
    }
    if (GetKeyState(0x5A) & 0x8000)
    {
        cameraZ -= increase;
    }
    else if (GetKeyState(0x58) & 0x8000)
    {
        cameraZ += increase;
    }


    POINT cursorPos;
    GetCursorPos(&cursorPos);
    cameraRotX = cursorPos.x - firstCursorPlace.x;
    cameraRotX *= sens;
    cameraRotY = cursorPos.y - firstCursorPlace.y;
    cameraRotY *= sens;

    // Изменение позиции камеры на орбите
    float radius = 9.0f;

    // Инициализация матрицы камеры из орбитальных данных её координат
    XMVECTOR Eye = XMVectorSet(cameraX, cameraY, cameraZ, 0.0f);
    XMVECTOR At = XMVectorSet(cameraX, cameraY, 0.0f, 0.0f);
    g_View = XMMatrixRotationY(cameraRotX) * XMMatrixRotationX(cameraRotY) * XMMatrixLookAtLH(Eye, At, Up);


    //
    // Установка контсант шейдера
    //
    ConstantBuffer cb;
    cb.mWorld = XMMatrixTranspose(g_World);
    cb.mView = XMMatrixTranspose(g_View);
    cb.mProjection = XMMatrixTranspose(g_Projection);
    cb.time = color;
    g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, NULL, &cb, 0, 0);

    //
    // Рендер 
    //

    // Установка вершинного буфера на первую фигуру
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    // Установка индексного буфера на первую фигуру
    g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
    g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);

    cb.mWorld = XMMatrixTranspose(XMMatrixTranslation(6.0f, 0, 0));
    cb.mWorld *= XMMatrixRotationY(rotation);

    g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, NULL, &cb, 0, 0);
    g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
    g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
    g_pImmediateContext->DrawIndexed(60, 0, 0);       // 12 вершин образуют 4 полигона, по три вершины на полигон


    //установка вершинного буфера на вторую фигуру
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferFigure2, &stride, &offset);

    //установка индексного буфера на вторую фигуру
    g_pImmediateContext->IASetIndexBuffer(g_pIndexBufferFigure2, DXGI_FORMAT_R16_UINT, 0);


    cb.mWorld = XMMatrixTranspose(XMMatrixTranslation(0, 2, 0));
    cb.mWorld *= XMMatrixRotationX(rotation * 2.5);
    g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, NULL, &cb, 0, 0);
    g_pImmediateContext->DrawIndexed(36, 0, 0);   // 24 вершины образуют 6 полигонов, по три вершины на полигон


    // Текст

    pRT->BeginDraw();
    static const WCHAR firstText[] = L"Кутасевич 924404";
    UINT size1 = ARRAYSIZE(firstText) - 1;
    pRT->DrawText(firstText, size1, m_pTextFormat, D2D1::RectF(200, 50, 650, 250), pWhiteBrush, D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
    pRT->EndDraw();

    pRT->BeginDraw();
    static const WCHAR secondText[] = L"Программирование";
    UINT size2 = ARRAYSIZE(secondText) - 1;
    pRT->DrawText(secondText, size2, m_pTextFormat, D2D1::RectF(180, 450, 650, 550), pRedBrush, D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
    pRT->EndDraw();

    //
    // Вывод на экран содержимого рендер-таргета
    //

    g_pSwapChain->Present(0, 0);
}
