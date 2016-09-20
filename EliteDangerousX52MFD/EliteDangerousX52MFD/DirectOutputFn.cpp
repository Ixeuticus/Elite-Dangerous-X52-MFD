// DirectOutputFn.cpp : Contains all the functions used to communicate with the X52 Pro MFD
#include "stdafx.h"
#include "DirectOutputFn.h"

#undef max

using namespace std;

// Constructor -> Loads in DirectOutput.dll
DirectOutputFn::DirectOutputFn()
{
	cout << "Created DirectOutputFn constructor.\n";
	cout << "Loading DirectOutput libaray... ";

	dll = LoadLibrary(TEXT("../DirectOutput.dll"));
	if (NULL != dll)
	{
		cout << "DONE.\n";
	}
	else
	{
		cout << "FAILED.\n";
	}
}

// Deconstructor -> Frees DirectOutput.dll
DirectOutputFn::~DirectOutputFn()
{
	cout << "Freeing DirectOutput library... ";
	if (!FreeLibrary(dll))
	{
		cout << "FAILED.\n";
	}
	else
	{
		cout << "DONE.\n";
	}

	cout << "Removed DirectOutputFn constructor.\n";
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// Public Functions

/*
	PARAMETERS: const wchar_t * wszPluginName = Name of this application
	RETURNS: HRESULT hr == status indicator to determine if passed

	FUNCTION: Initialized DirectOutput library
*/
HRESULT DirectOutputFn::Initialize(const wchar_t * wszPluginName)
{
	cout << "Initialzing DirectOutput library... ";

	Pfn_DirectOutput_Initialize initDOFn = (Pfn_DirectOutput_Initialize)GetProcAddress(dll, "DirectOutput_Initialize");
	hr = initDOFn(wszPluginName);
	
	return hr;
}

/*
	PARAMETERS: none
	RETURNS: HRESULT hr == status indicator to determine if passed

	FUNCTION: Deinitializes the DirectOutput library
*/
HRESULT DirectOutputFn::Deinitialize()
{
	cout << "Deinitializing DirectOutput... ";
	Pfn_DirectOutput_Deinitialize deInitfunc = (Pfn_DirectOutput_Deinitialize)GetProcAddress(dll, "DirectOutput_Deinitialize");
	hr = deInitfunc();

	return hr;
}

/*
	PARAMETERS: none
	RETURNS: none

	FUNCTION: Gets the currently selected device and adds it to the m_devices list
*/
void DirectOutputFn::RegisterDevice()
{
	cout << "Registering device callback... ";
	Pfn_DirectOutput_RegisterDeviceCallback fnRegisterDeviceCallback = (Pfn_DirectOutput_RegisterDeviceCallback)GetProcAddress(dll, "DirectOutput_RegisterDeviceCallback");
	hr = fnRegisterDeviceCallback(OnDeviceChanged, this);
	if (hr == S_OK)
	{
		cout << "DONE.\n";
	}
	else
	{
		cout << "FAILED.\n";
	}

	cout << "Enumerating devices... ";
	Pfn_DirectOutput_Enumerate fnEnumerate = (Pfn_DirectOutput_Enumerate)GetProcAddress(dll, "DirectOutput_Enumerate");
	hr = fnEnumerate(OnEnumerateDevice, this);
	if (FAILED(hr))
	{
		cout << "FAILED.\n";
	}
	else
	{
		cout << "DONE.\n";
	}
}

/*
	PARAMETERS: none
	RETURNS: none

	FUNCTION: Determines what device type is connected based on the m_devices array. Sort of hard coded to be the X52 Pro since that is all I am looking for
*/
void DirectOutputFn::GetDeviceType()
{
	cout << "Getting device... ";
	Pfn_DirectOutput_GetDeviceType fnGetDeviceType = (Pfn_DirectOutput_GetDeviceType)GetProcAddress(dll, "DirectOutput_GetDeviceType");
	GUID typeGUID = { 0 };
	for (DeviceList::iterator it = m_devices.begin(); it != m_devices.end(); it++)
	{
		hr = fnGetDeviceType(*it, &typeGUID);
		if (FAILED(hr))
		{
			cout << "FAILED.\n";
		}

		if (typeGUID == DeviceType_X52Pro)
		{
			cout << "Got device X52Pro.\n";
		}
	}
}

/*
	PARAMETERS: wchar_t* filepath == full pathname to the desired profile 
	RETURNS: HRESULT hr == status indicator to determine if passed

	FUNCTION: Sets the X52 Pro to the desired profile. This might be unneccessary as I will only be using the screen functions at the moment and not changing any keybindings or lights
*/
HRESULT DirectOutputFn::setDeviceProfile(wchar_t* filepath)
{
	cout << "Setting the device profile... ";
	Pfn_DirectOutput_SetProfile fnSetProfile = (Pfn_DirectOutput_SetProfile)GetProcAddress(dll, "DirectOutput_SetProfile");
	void * hdevice = m_devices[0];
	size_t size = wcslen(filepath);
	hr = fnSetProfile(hdevice, size, filepath);
	return hr;
}

/*
	PARAMETERS: int pageNumber == page to add to display
			const DWORD flag == for setting the page active or not
	RETURNS: HRESULT hr == status indicator to determine if passed

	FUNCTION: The MFD works on a page system that can be scrolled through. Page 0 being the main page, page 1 being the next and so on.
			The flags are listed as:
				FLAG_SET_AS_ACTIVE == sets the page to be active
				0 == will not change the active page
*/

HRESULT DirectOutputFn::setPage(int pageNumber, const DWORD flag)
{
	cout << "Adding page... ";
	Pfn_DirectOutput_AddPage fnSetPage = (Pfn_DirectOutput_AddPage)GetProcAddress(dll, "DirectOutput_AddPage");
	void * hdevice = m_devices[0];
	hr = fnSetPage(hdevice, pageNumber, flag);
	return hr;
}

/*
	PARAMETERS: int pageNumber == pageNumber to set the string on
			int stringLineID == line on the MFD to display the string 
								0 -> line1
								1 -> line2
								2 -> line3
			wchar_t* stringToOutput == string to display on the MFD
	RETURNS: HRESULT hr == status indicator to determine if passed.

	FUNCTION: Sends a string to the MFD depending on the page and linenumber. 
				** I can't seem to figure out how to add strings on non-active pages, so strings have to be set on the active page
*/
HRESULT DirectOutputFn::setString(int pageNumber, int stringLineID, wchar_t * stringToOutput)
{
	cout << "Setting string... ";
	Pfn_DirectOutput_SetString fnSetString = (Pfn_DirectOutput_SetString)GetProcAddress(dll, "DirectOutput_SetString");
	void * hDevice = m_devices[0];
	size_t stringLength = wcslen(stringToOutput);
	hr = fnSetString(hDevice, pageNumber, stringLineID, stringLength, stringToOutput);
	return hr;
}

/*
	PARAMETERS: none
	RETURNS: HRESULT hr == status indicator to determine if passed.

	FUNCTION: Registers a handle so the device can let this program know the right scroll wheel moved up or down
*/
HRESULT DirectOutputFn::registerSoftBtnCallback()
{
	cout << "Registering soft button callback... ";
	Pfn_DirectOutput_RegisterSoftButtonCallback fnRegSoftBtn = (Pfn_DirectOutput_RegisterSoftButtonCallback)GetProcAddress(dll, "DirectOutput_RegisterSoftButtonCallback");
	hr = fnRegSoftBtn(m_devices[0], OnSoftButtonChanged, this);
	return hr;
}

/*
	PARAMETERS: none
	RETURNS: HRESULT hr == status indicator to determine if passed.

	FUNCTION: Registers a handle so the device can let this program know when the left page wheel is used
*/
HRESULT DirectOutputFn::registerPageCallback()
{
	cout << "Registering page callback... ";
	Pfn_DirectOutput_RegisterPageCallback fnRegPageCallback = (Pfn_DirectOutput_RegisterPageCallback)GetProcAddress(dll, "DirectOutput_RegisterPageCallback");
	hr = fnRegPageCallback(m_devices[0], OnPageChanged, this);
	return hr;
}

/*
	PARAMETERS: none
	RETURNS: HRESULT hr == status indicator to determine if passed.

	FUNCTION: Unregisters the right scroll wheel handle. Cleanup function.
*/
HRESULT DirectOutputFn::unRegisterSoftBtnCallback()
{
	cout << "Unregistering soft button callback... ";
	Pfn_DirectOutput_RegisterSoftButtonCallback fnRegSoftBtn = (Pfn_DirectOutput_RegisterSoftButtonCallback)GetProcAddress(dll, "DirectOutput_RegisterSoftButtonCallback");
	hr = fnRegSoftBtn(m_devices[0], NULL, NULL);
	return hr;
}

/*
	PARAMETERS: none
	RETURNS: HRESULT hr == status indicator to determine if passed.

	FUNCTION: Unregisters the right scroll wheel handle. Cleanup function.
*/
HRESULT DirectOutputFn::unRegisterPageCallback()
{
	cout << "Unregistering page callback... ";
	Pfn_DirectOutput_RegisterPageCallback fnRegPageCallback = (Pfn_DirectOutput_RegisterPageCallback)GetProcAddress(dll, "DirectOutput_RegisterPageCallback");
	hr = fnRegPageCallback(m_devices[0], NULL, NULL);
	return hr;
}

/*
	PARAMETERS: none
	RETURNS: currentPage == the currently selected active page on the MFD. See setString() as to why this is neccessary

	FUNCTION: Returns the currently selected page on the MFD back to the main function.
*/
int DirectOutputFn::getCurrentPage()
{
	return currentPage;
}

/*
	PARAMETERS: none
	RETURNS: none

	FUNCTION: This function prints out the selected string based on the predefined page limits in the EliteDangerousX52MFD.cpp. It is not hardcoded only something I selected, I am not sure the hard limit of the device yet.
			So far this is neccessary since I cannot yet figure out why strings can't be set on inactive created pages, I always return an error. 
			Also, the defualt profile page cannot be removed so there will always be an extra page that cannot be removed.
			Plus reprinting the same string too fast causes the display to crash...
*/
void DirectOutputFn::handlePageChange()
{
	switch (currentPage)
	{
	case 0:
		setString(0, 0, TEXT("Greetings CMDR"));
		setString(0, 1, TEXT("Page 0"));
		break;
	case 1:
		setString(1, 1, TEXT("Page 1"));
		break;
	case 2:
		setString(2, 2, TEXT("Page 2"));
		break;
	case 3:
		setString(3, 0, TEXT("Page 3"));
		break;
	case 4:
		setString(4, 0, TEXT("Page 4"));
		break;
	case 5:
		setString(5, 0, TEXT("Page 5"));
	default:
		break;
	}
}



// Private Functions
void __stdcall DirectOutputFn::OnEnumerateDevice(void * hDevice, void * pCtxt)
{
	DirectOutputFn* pThis = (DirectOutputFn*)pCtxt;
	pThis->m_devices.push_back(hDevice);
}

void __stdcall DirectOutputFn::OnDeviceChanged(void * hDevice, bool bAdded, void * pCtxt)
{
	DirectOutputFn* pThis = (DirectOutputFn*)pCtxt;
	if (bAdded)
	{
		// device has been added, add to list of devices
		{
			TCHAR tsz[1024];
			_sntprintf_s(tsz, sizeof(tsz) / sizeof(tsz[0]), sizeof(tsz) / sizeof(tsz[0]), _T("DeviceAdded(%p)\n"), hDevice);
			OutputDebugString(tsz);
		}
		pThis->m_devices.push_back(hDevice);
	}
	else
	{
		// device has been removed, remove from list of devices
		{
			TCHAR tsz[1024];
			_sntprintf_s(tsz, sizeof(tsz) / sizeof(tsz[0]), sizeof(tsz) / sizeof(tsz[0]), _T("DeviceRemoved(%p)\n"), hDevice);
			OutputDebugString(tsz);
		}
		for (DeviceList::iterator it = pThis->m_devices.begin(); it != pThis->m_devices.end(); ++it)
		{
			if (*it == hDevice)
			{
				pThis->m_devices.erase(it);
				break;
			}
		}
	}
}

void __stdcall DirectOutputFn::OnPageChanged(void * hDevice, DWORD dwPage, bool bSetActive, void * pCtxt)
{
	DirectOutputFn* pThis = (DirectOutputFn*)pCtxt;
	cout << "Page change.\n";
	cout << "bsetActive == " << bSetActive << endl;
	cout << "dwPage == " << dwPage << endl;
	pThis->currentPage = dwPage;
	pThis->handlePageChange();
}

void __stdcall DirectOutputFn::OnSoftButtonChanged(void * hDevice, DWORD dwButtons, void * pCtxt)
{
	DirectOutputFn* pThis = (DirectOutputFn*)pCtxt;
	if (dwButtons & 0x00000002)
	{
		++pThis->m_scrollpos;
		cout << "Scroll ++";
	}
	else if (dwButtons & 0x0000004)
	{
		--pThis->m_scrollpos;
		cout << "Scroll --";
	}
}






