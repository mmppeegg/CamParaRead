// CamParaRead.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define MAX_STRING_SIZE 256

#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <Windows.h>
#include <dshow.h>
//#include <atlconv.h>
//#include <stdio.h>
#include <string>
//#include <cfgmgr32.h>
#include <vector>
#include <setupapi.h> 
#include <hidsdi.h>
#include <hidpi.h>
#include <ks.h>
#include <Ksproxy.h>
#include <comdef.h>

// my header
#include "AddTimeStamp.h"
#include "iSerialNum.h"

// Pre-compiler-definition
#define CameraControl_LowLightCompensation 19

#pragma warning(disable : 4996)
//#pragma comment (lib, "cfgmgr32.lib")
#pragma comment(lib, "strmiids.lib")
// functions declare
std::string ConvertBSTRToMBS(BSTR bstr);
void _DeleteMediaType(AM_MEDIA_TYPE* pmt);

// Direct show read/write testing
void myCamControl()
{
    // example get/set camera settings by Direct show(from Kevin H M)
    // for playing
    IGraphBuilder* pGraphBuilder;
    ICaptureGraphBuilder2* pCaptureGraphBuilder2;
    IMediaControl* pMediaControl;
    IBaseFilter* pDeviceFilter = NULL;

    // to select a video input device
    ICreateDevEnum* pCreateDevEnum = NULL;
    IEnumMoniker* pEnumMoniker = NULL;
    IMoniker* pMoniker = NULL;
    ULONG nFetched = 0;

    // initialize COM
    std::cout << "Now start initializing COM. \r\n";
    CoInitialize(nullptr);
    std::cout << "Now finish initializing COM. \r\n";

    //
    // selecting a device
    //

    // Create CreateDevEnum to list device
    CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, (PVOID*)&pCreateDevEnum);

    // Create EnumMoniker to list VideoInputDevice 
    pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);
    if (pEnumMoniker == NULL) {
        // this will be shown if there is no capture device
        printf("no device\n");
        //return 0;
        return;
    }

    // reset EnumMoniker
    pEnumMoniker->Reset();

    // get each Moniker
    while (pEnumMoniker->Next(1, &pMoniker, &nFetched) == S_OK)
    {
        IPropertyBag* pPropertyBag;
        TCHAR devname[256];
        LPSTR devname1 = NULL;

        // bind to IPropertyBag
        pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropertyBag);

        VARIANT var;
        VARIANT varName;
        VariantInit(&varName);

        // get FriendlyName
        pPropertyBag->Read(L"FriendlyName", &varName, 0);
        std::string strFName = ConvertBSTRToMBS(varName.bstrVal);

        // vv - we can take advantage of this to identify each camera if there is no serial number embedded in camera itself.
        // MSDN: https://docs.microsoft.com/en-us/windows/win32/directshow/selecting-a-capture-device
        // DevicePath - The "DevicePath" property is not a human-readable string, but is guaranteed to be unique 
        // for each video capture device on the system. 
        // You can use this property to distinguish between two or more instances of the same model of device.

        std::cout << strFName << "\r\n";
        VariantClear(&varName);
        //std::cout << "select this device ? [y] or [n]" << "\r\n";
        //printf("  select this device ? [y] or [n]\r\n");
        //int ch = getchar();

        // you can start playing by 'y' + return key
        // if you press the other key, it will not be played.
        //if (ch == 'y') {
        std::string w2000("W2000");
        std::string w2050("W2050");
        if (strFName.find(w2000) != std::string::npos || 
            strFName.find(w2050) != std::string::npos)
        {
            // Bind Monkier to Filter
            pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pDeviceFilter);
        }
        //pDeviceFilter->

        //IKsPropertySet  GG;
        //GG->Get(KSPROPERTY_PIN_CINSTANCES)



        // release
        pMoniker->Release();
        pPropertyBag->Release();

        if (pDeviceFilter != NULL) {
            // go out of loop if getchar() returns 'y'
            break;
        }
    }

    if (pDeviceFilter != NULL) {
        //
        // PLAY
        //

        // create FilterGraph
        CoCreateInstance(CLSID_FilterGraph,
            NULL,
            CLSCTX_INPROC,
            IID_IGraphBuilder,
            (LPVOID*)&pGraphBuilder);

        // create CaptureGraphBuilder2
        CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC,
            IID_ICaptureGraphBuilder2,
            (LPVOID*)&pCaptureGraphBuilder2);

        //============================================================
        //===========  MY CODE  ======================================
        //=============================================================
        HRESULT hr;// = CoInitialize(0);
        IAMStreamConfig* pConfig = NULL;
        hr = pCaptureGraphBuilder2->FindInterface(&PIN_CATEGORY_CAPTURE, 0, pDeviceFilter, IID_IAMStreamConfig, (void**)&pConfig);

        #if 0 // 這裡可以不處理，但不處理的話畫出來的視窗會比較小，但對於控制/讀取camera是沒問題的
        int iCount = 0, iSize = 0;
        hr = pConfig->GetNumberOfCapabilities(&iCount, &iSize);

        // Check the size to make sure we pass in the correct structure.
        
        if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
        {
            // Use the video capabilities structure.

            for (int iFormat = 0; iFormat < iCount; iFormat++)
            {
                VIDEO_STREAM_CONFIG_CAPS scc;
                AM_MEDIA_TYPE* pmtConfig;
                hr = pConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);
                if (SUCCEEDED(hr))
                {
                    /* Examine the format, and possibly use it. */
                    if ((pmtConfig->majortype == MEDIATYPE_Video) &&
                        (pmtConfig->subtype == MEDIASUBTYPE_RGB24) &&
                        (pmtConfig->formattype == FORMAT_VideoInfo) &&
                        (pmtConfig->cbFormat >= sizeof(VIDEOINFOHEADER)) &&
                        (pmtConfig->pbFormat != NULL))
                    {
                        VIDEOINFOHEADER* pVih = (VIDEOINFOHEADER*)pmtConfig->pbFormat;
                        // pVih contains the detailed format information.
                        LONG lWidth = pVih->bmiHeader.biWidth;
                        LONG lHeight = pVih->bmiHeader.biHeight;
                    }
                    if (iFormat == 26) { //2 = '1280x720YUV' YUV, 22 = '1280x800YUV', 26 = '1280x720RGB'
                        hr = pConfig->SetFormat(pmtConfig);
                    }
                    // Delete the media type when you are done.
                    _DeleteMediaType(pmtConfig);
                }
            }
        }
        #endif

#if 0   // (works)Query the capture filter for the IAMCameraControl interface.
        IAMCameraControl* pCameraControl = 0;
        hr = pDeviceFilter->QueryInterface(IID_IAMCameraControl, (void**)&pCameraControl);
        if (FAILED(hr))
        {
            // The device does not support IAMCameraControl
            std::cout << "The device does not support IAMCameraControl" << "\r\n";
        }
        else
        {
            long Min, Max, Step, Default, Flags, Val;
            // Get the range and default values 
            hr = pCameraControl->GetRange(CameraControl_Exposure, &Min, &Max, &Step, &Default, &Flags);
            //hr = pCameraControl->GetRange(CameraControl_Focus, &Min, &Max, &Step, &Default, &Flags);
            //hr = pCameraControl->GetRange(CameraControl_Iris, &Min, &Max, &Step, &Default, &Flags);
            // 低光補償看起來也是可以用，但應該只有default有意義(預設值為1?)
            //hr = pCameraControl->GetRange(CameraControl_LowLightCompensation, &Min, &Max, &Step, &Default, &Flags);
            if (SUCCEEDED(hr))
            {
                #if 1
                //hr = pCameraControl->Get(CameraControl_Tilt, &Val, &Flags);
                //hr = pCameraControl->Get(CameraControl_LowLightCompensation, &Val, &Flags); // It works而且不受auto-exposure勾選的影響
                //hr = pCameraControl->Get(CameraControl_Focus, &Val, &Flags);
                hr = pCameraControl->Get(CameraControl_Exposure, &Val, &Flags);

                std::cout << "Tile value is: " << Val << " auto value: " << Flags << "\r\n";
                #endif
                #if 0 // vv20210929 here we can set camera parameter (control)
                hr = pCameraControl->Set(CameraControl_Exposure, -11, CameraControl_Flags_Manual); // Min = -11, Max = 1, Step = 1
                hr = pCameraControl->Set(CameraControl_Focus, 10, CameraControl_Flags_Manual);
                #endif
            }
        }
#endif // Camera control (works!)

#if 0   // (Works) Query the capture filter for the IAMVideoProcAmp interface.
        IAMVideoProcAmp* pProcAmp = 0;
        hr = pDeviceFilter->QueryInterface(IID_IAMVideoProcAmp, (void**)&pProcAmp);
        if (FAILED(hr))
        {
            // The device does not support IAMVideoProcAmp
        }
        else
        {
            long Min, Max, Step, Default, Flags, Val;


            // Get the range and default values 
            hr = pProcAmp->GetRange(VideoProcAmp_Brightness, &Min, &Max, &Step, &Default, &Flags);
            //hr = pProcAmp->GetRange(VideoProcAmp_BacklightCompensation, &Min, &Max, &Step, &Default, &Flags);
            //hr = pProcAmp->GetRange(VideoProcAmp_Contrast, &Min, &Max, &Step, &Default, &Flags);
            //hr = pProcAmp->GetRange(VideoProcAmp_Saturation, &Min, &Max, &Step, &Default, &Flags);
            //hr = pProcAmp->GetRange(VideoProcAmp_Sharpness, &Min, &Max, &Step, &Default, &Flags);
            //hr = pProcAmp->GetRange(VideoProcAmp_WhiteBalance, &Min, &Max, &Step, &Default, &Flags);
            if (SUCCEEDED(hr))
            {
                #if 1 // vv20210929 here we can get camera parameter (Video Prom Amp)
                //hr = pProcAmp->Get(VideoProcAmp_Brightness, &Val, &Flags);
                //std::cout << "Brightness value is: " << Val << " auto value" << Flags << "\r\n";
                hr = pProcAmp->Get(VideoProcAmp_WhiteBalance, &Val, &Flags);
                std::cout << "WhiteBalance value is: " << Val << " auto value" << Flags << "\r\n";
                #endif


                #if 0  // vv20210929 here we can set the camera parameter (Video Prom Amp)
                hr = pProcAmp->Set(VideoProcAmp_Brightness, 142, VideoProcAmp_Flags_Manual);
                hr = pProcAmp->Set(VideoProcAmp_BacklightCompensation, 0, VideoProcAmp_Flags_Manual);
                hr = pProcAmp->Set(VideoProcAmp_Contrast, 4, VideoProcAmp_Flags_Manual);
                hr = pProcAmp->Set(VideoProcAmp_Saturation, 100, VideoProcAmp_Flags_Manual);
                hr = pProcAmp->Set(VideoProcAmp_Sharpness, 0, VideoProcAmp_Flags_Manual);
                hr = pProcAmp->Set(VideoProcAmp_WhiteBalance, 2800, VideoProcAmp_Flags_Manual);
                #endif
            }
        }
#endif // Pro Amp control (works!) 

        // set FilterGraph
        hr = pCaptureGraphBuilder2->SetFiltergraph(pGraphBuilder);
        if (!SUCCEEDED(hr))
            MessageBox(NULL, L"SetFiltergraph failed!", L"Failed message", MB_OK);

        // get MediaControl interface
        hr = pGraphBuilder->QueryInterface(IID_IMediaControl, (LPVOID*)&pMediaControl);
        if (!SUCCEEDED(hr))
            MessageBox(NULL, L"get MediaControl interface failed!", L"Failed message", MB_OK);

        // add device filter to FilterGraph
        hr = pGraphBuilder->AddFilter(pDeviceFilter, L"Device Filter");
        if (!SUCCEEDED(hr))
            MessageBox(NULL, L"AddFilter failed!", L"Failed message", MB_OK);

        // create Graph
        hr = pCaptureGraphBuilder2->RenderStream(&PIN_CATEGORY_CAPTURE, NULL, pDeviceFilter, NULL, NULL);
        if (!SUCCEEDED(hr))
            MessageBox(NULL, L"RenderStream failed!", L"Failed message", MB_OK);

        //hr = pMediaControl->GetState(); // 這個不能拿來判斷是否被占用

        // start playing
        //OAFilterState gg;
        //hr = pMediaControl->GetState(0, &gg);
        //UINT pctinfo; // 這看起來也不行
        //hr = pMediaControl->GetTypeInfoCount(&pctinfo);
        hr = pMediaControl->Run();
        if (!SUCCEEDED(hr))
        { // 會有小視窗閃一下
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            //MessageBox(NULL, L"Run() failed!", L"Failed message", MB_OK);
            std::cout << "Run failed!! The return code is:" << std::hex << hr << "\r\n";
        }
        else
        {
            // to block execution
            // without this messagebox, the graph will be stopped immediately
            MessageBox(NULL,
                L"Without this messagebox, the graph will be stopped immediately",
                L"Block",
                MB_OK);
        }

        // release
        pMediaControl->Release();
        pCaptureGraphBuilder2->Release();
        pGraphBuilder->Release();
    }

    // release
    pEnumMoniker->Release();
    pCreateDevEnum->Release();

    // finalize COM
    CoUninitialize();

    std::cout << "-------------------CamParaRead is end now-------------------\r\n";
    //return 0;
}


// Release the format block for a media type.
void _FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0)
    {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL)
    {
        // pUnk should not be used.
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}


// Delete a media type structure that was allocated on the heap.
void _DeleteMediaType(AM_MEDIA_TYPE* pmt)
{
    if (pmt != NULL)
    {
        _FreeMediaType(*pmt);
        CoTaskMemFree(pmt);
    }
}

std::string ConvertWCSToMBS(const wchar_t* pstr, long wslen)
{
    int len = ::WideCharToMultiByte(CP_ACP, 0, pstr, wslen, NULL, 0, NULL, NULL);

    std::string dblstr(len, '\0');
    len = ::WideCharToMultiByte(CP_ACP, 0 /* no flags */,
        pstr, wslen /* not necessary NULL-terminated */,
        &dblstr[0], len,
        NULL, NULL /* no default char */);

    return dblstr;
}

std::string ConvertBSTRToMBS(BSTR bstr)
{
    int wslen = ::SysStringLen(bstr);
    return ConvertWCSToMBS((wchar_t*)bstr, wslen);
}

//-----------------------------------------------------------------------------
void* buf_alloc(int size)
{
    void* buf;

    if (NULL == (buf = malloc(size)))
        std::cout << "Out of memory! \n";
        //error_exit("out of memory");

    return buf;
}

//-----------------------------------------------------------------------------
void buf_free(void* buf)
{
    free(buf);
}

//int dbg_enumerate(debugger_t* debuggers, int size)
int dbg_enumerate(int size)
{
    GUID hid_guid;
    HDEVINFO hid_dev_info;
    HIDD_ATTRIBUTES hid_attr;
    SP_DEVICE_INTERFACE_DATA dev_info_data;
    PSP_DEVICE_INTERFACE_DETAIL_DATA detail_data;
    DWORD detail_size;
    HANDLE handle;
    int rsize = 0;

//#define GUID_CAMERA_STRING L"{65e8773d-8f56-11d0-a3b9-00a0c9223196}"
#define GUID_CAMERA_STRING1 L"{ca3e7ab9-b4c3-4ae6-8251-579ef933890f}"
    GUID guid;
    CLSIDFromString(GUID_CAMERA_STRING, &guid);



    HidD_GetHidGuid(&hid_guid); // 這個看起來
//#define GUID_CAMERA_ {ca3e7ab9-b4c3-4ae6-8251-579ef933890f}
    hid_dev_info = SetupDiGetClassDevs(&hid_guid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    //hid_dev_info = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

    dev_info_data.cbSize = sizeof(dev_info_data);

    for (int i = 0; i < size; i++)
    {
        if (FALSE == SetupDiEnumDeviceInterfaces(hid_dev_info, 0, &hid_guid, i, &dev_info_data))
            break;

        SetupDiGetDeviceInterfaceDetail(hid_dev_info, &dev_info_data, NULL, 0,
            &detail_size, NULL);

        detail_data = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buf_alloc(detail_size);
        detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        SetupDiGetDeviceInterfaceDetail(hid_dev_info, &dev_info_data, detail_data,
            detail_size, NULL, NULL);

        handle = CreateFile(detail_data->DevicePath, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

        if (INVALID_HANDLE_VALUE != handle)
        {
            hid_attr.Size = sizeof(hid_attr);
            HidD_GetAttributes(handle, &hid_attr);

            //if (DBG_VID == hid_attr.VendorID && DBG_PID == hid_attr.ProductID)
            //if (0x047D == hid_attr.VendorID && 0x80B3 == hid_attr.ProductID) // vv20211004
            //if (0x047D == hid_attr.VendorID && 0x8068 == hid_attr.ProductID)
            if (0x17E9 == hid_attr.VendorID && 0x4306 == hid_attr.ProductID)
            {
                wchar_t wstr[MAX_STRING_SIZE];
                char str[MAX_STRING_SIZE];

                //debuggers[rsize].path = strdup(detail_data->DevicePath);

                HidD_GetSerialNumberString(handle, (PVOID)wstr, MAX_STRING_SIZE);
                wcstombs(str, wstr, MAX_STRING_SIZE);
                //debuggers[rsize].serial = strdup(str);

                HidD_GetManufacturerString(handle, (PVOID)wstr, MAX_STRING_SIZE);
                wcstombs(str, wstr, MAX_STRING_SIZE);
                //debuggers[rsize].manufacturer = strdup(str);

                HidD_GetProductString(handle, (PVOID)wstr, MAX_STRING_SIZE);
                wcstombs(str, wstr, MAX_STRING_SIZE);
                //debuggers[rsize].product = strdup(str);

                rsize++;
            }

            CloseHandle(handle);
        }

        buf_free(detail_data);
    }

    SetupDiDestroyDeviceInfoList(hid_dev_info);

    return rsize;
}

HRESULT selectDev()
{
    bool bFound = false;
    // Create the System Device Enumerator.
    HRESULT hr;
    ICreateDevEnum* pSysDevEnum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, (void**)&pSysDevEnum);
    if (FAILED(hr))
    {
        return hr;
    }

    // Obtain a class enumerator for the video compressor category.
    IEnumMoniker* pEnumCat = NULL;
    //hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoCompressorCategory, &pEnumCat, 0);
    hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumCat, 0);

    if (hr == S_OK)
    {
        // Enumerate the monikers.
        IMoniker* pMoniker = NULL;
        ULONG cFetched;
        while (pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
        {
            IPropertyBag* pPropBag;
            //if(pMoniker != NULL)
                hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);

            if (SUCCEEDED(hr))
            {
                // To retrieve the filter's friendly name, do the following:
                VARIANT varName;
                VariantInit(&varName);
                hr = pPropBag->Read(L"FriendlyName", &varName, 0);
                if (SUCCEEDED(hr))
                {
                    // Display the name in your UI somehow.
                    //std::cout << "Got friendly names!! \n"; // vv: works with CLSID_VideoCompressorCategory
                    std::string tmpStr = ConvertBSTRToMBS(varName.bstrVal);
                    //std::string w2050("W2050 Full HD Auto Focus Pro Webcam"); // Solo
                    std::string w2050("W2050");
                    
                    if (tmpStr.find(w2050) != std::string::npos) {
                        std::cout << "Got Kensington Solo! \n";
                        bFound = true;

                        hr = pPropBag->Read(L"DevicePath", &varName, 0); // works
                        if (SUCCEEDED(hr))
                        {
                            // The device path is not intended for display.
                            printf("Device path: %S\n", varName.bstrVal);
                            //VariantClear(&varName);
                        }

                        //IAMVideoProcAmp* pProcAmp = 0;
                        //hr = pMoniker->QueryInterface(IID_IAMVideoProcAmp, (void**)&pProcAmp);
                        //VariantClear(&varName);
                    }
                }
                VariantClear(&varName);

                // To create an instance of the filter, do the following:
                if (bFound == true) {
                    IBaseFilter* pFilter = NULL;
                    //hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pFilter);
                    hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pFilter);

                    // Now add the filter to the graph. (TBD)
                    //if (SUCCEEDED(hr))
                    //{
                    //    hr = m_pGraph->AddFilter(pCap, L"Capture Filter");
                    //}
                }                
                
                
                    //IAMVideoProcAmp* pProcAmp = 0;
                    //hr = pMoniker->QueryInterface(IID_IAMVideoProcAmp, (void**)&pProcAmp);                    
                


                //Remember to release pFilter later.
                pPropBag->Release();
            }
            pMoniker->Release();
        }
        pEnumCat->Release();
    }
    pSysDevEnum->Release();

    return hr;
}
#if 1
bool occupiedDetect()
{
    #if 0 // (under testing...)
    KSPROPERTY ksProp;
    DWORD numPins;
    DWORD bytesReturned;
    ksProp.Set = KSPROPSETID_Pin;
    ksProp.Id = KSPROPERTY_PIN_CTYPES;
    ksProp.Flags = KSPROPERTY_TYPE_GET;

    if (DeviceIoControl(devHandle, IOCTL_KS_PROPERTY, &ksProp, sizeof(ksProp), &numPins, sizeof(numPins), &bytesReturned, NULL)) {

        KSP_PIN ksPin;
        ksPin.Reserved = 0;
        ksPin.Property.Set = KSPROPSETID_Pin;
        ksPin.Property.Id = KSPROPERTY_PIN_CINSTANCES;
        ksPin.Property.Flags = KSPROPERTY_TYPE_GET;

        for (DWORD pin = 0; pin < numPins; ++pin) {

            KSPIN_CINSTANCES pinInstance;
            ksPin.PinId = pin;

            if (DeviceIoControl(devHandle, IOCTL_KS_PROPERTY, &ksPin, sizeof(ksPin), &pinInstance, sizeof(pinInstance), &bytesReturned, NULL)) {
                printf("Pin %d Possible: %d Current: %d\n", pin, pinInstance.PossibleCount, pinInstance.CurrentCount);
            }
        }
    }
    #endif

    return true;
}
#endif

int main()
{
    std::cout << "Start trying read my camera parameters!!\n";

    // get if a specied cam is occupied
    //getCamOccupied();

#if 1 // works! Pro Amps and Camera control adjust!
    myCamControl();
#endif

#if 1 // Get serial number
    std::vector<int>* order = new std::vector<int>(10, -1);
    getCameraOrderBySerialNumber(order, 1);
    
#endif

#if 0 // get serial number test (不可用)
    dbg_enumerate(100); // 使用HidD_xxxx 無法抓到camera，因為camera沒有分類到hid裝置
#endif

#if 0 // Get device path
    CoInitialize(nullptr);
    HRESULT hr = selectDev();
#endif


#if 0 // get parameters
    HWND hTrackbar; // Handle to the trackbar control. 
// Initialize hTrackbar (not shown).

// Query the capture filter for the IAMVideoProcAmp interface.
    IAMVideoProcAmp* pProcAmp = 0;
    hr = pCap->QueryInterface(IID_IAMVideoProcAmp, (void**)&pProcAmp);
    if (FAILED(hr))
    {
        // The device does not support IAMVideoProcAmp, so disable the control.
        EnableWindow(hTrackbar, FALSE);
    }
    else
    {
        long Min, Max, Step, Default, Flags, Val;

        // Get the range and default value. 
        hr = m_pProcAmp->GetRange(VideoProcAmp_Brightness, &Min, &Max, &Step,
            &Default, &Flags);
        if (SUCCEEDED(hr))
        {
            // Get the current value.
            hr = m_pProcAmp->Get(VideoProcAmp_Brightness, &Val, &Flags);
        }
        if (SUCCEEDED(hr))
        {
            // Set the trackbar range and position.
            SendMessage(hTrackbar, TBM_SETRANGE, TRUE, MAKELONG(Min, Max));
            SendMessage(hTrackbar, TBM_SETPOS, TRUE, Val);
            EnableWindow(hTrackbar, TRUE);
        }
        else
        {
            // This property is not supported, so disable the control.
            EnableWindow(hTrackbar, FALSE);
        }
    }
#endif
}