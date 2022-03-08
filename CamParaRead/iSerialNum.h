#pragma once

#pragma comment (lib, "setupapi.lib")
#pragma comment (lib, "cfgmgr32.lib")
#pragma comment (lib, "strmiids.lib")
#include <initguid.h>

#include <windows.h>  

#include <setupapi.h> 
#include <cfgmgr32.h>
#include <usbiodef.h>

#include <regex>

#include <stdio.h>    


#include <dshow.h>
#include <atlconv.h>
//#include "DistinguishCamera.h"
#include <iostream>
#include <ks.h>
#include <Ksproxy.h>
//#include <Softusbif.h>

#pragma warning(disable : 4996)
#define GUID_CAMERA_STRING L"{65e8773d-8f56-11d0-a3b9-00a0c9223196}"
#if 0
static char* GuidToString(const GUID& guid);
static BOOL GetParentDeviceInstanceId(_Out_ PWCHAR pszParentDeviceInstanceId,
                                      _Out_ PDEVINST phParentDeviceInstanceId,
                                      _In_ DEVINST hCurrentDeviceInstanceId);
static int CheckSerialNumber(std::string serialNumber);
static int getSerialNumber(std::string childDistanceId);
#endif
bool getCameraOrderBySerialNumber(std::vector<int>* order, int numOfCamera);
bool getCamOccupied();