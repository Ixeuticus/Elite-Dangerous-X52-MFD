// EliteDangerousX52MFD.cpp : Entry point

/*
	EliteDangerousX52MFD v 1.0
	Special Thanks to:
		Frontier for Elite Dangerous
		Saitek for the use and development of the SDK to run this project
		Andargor for work on Elite Dangerous Companion Emulator (https://github.com/Andargor/edce-client)
		Niels Lohmann for work on JSON for Modern C++ (https://github.com/nlohmann/json)
*/

#include "stdafx.h"
#include <chrono>
#include <sys/stat.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <sstream>
#include <thread>
#include "DirectOutputFn.h"
#include "JSONDataStructure.h"

using namespace std;

// DirectOutput function object
// Creation of this object automatically loads in DirectOutput but it still needs to be initialized
DirectOutputFn fn;

// Accessor to JSON file
JSONDataStructure jsonDataClass;

TCHAR profileFilepath[260];
TCHAR edceScriptFilepath[260];
TCHAR edceJSONFilepath[260];
TCHAR defaultDirectory[260];
TCHAR edceDirectory[260];

bool foundProfile = false;
bool closeOnWindowX = false;

int timer;

// Internal Functions
void checkHR(HRESULT hr);
void txtFileCheck();
void getFilepaths();
void createTxtFile();
bool getFilePathName(bool isProfile);
void contactEDCE();
void cleanupAndClose();
BOOL controlHandler(DWORD fdwCtrlType);

int main()
{
	// Setup control handling, if app is closed by other means
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)controlHandler, TRUE);

	// Initialize DirectOutput
	checkHR(fn.Initialize(L"EliteDangerousX52MFD"));

	// Registers and enumerates device
	fn.RegisterDevice();

	// Gets the enumerated device
	fn.GetDeviceType();

	// .txt file check
	txtFileCheck();

	// Set the profile
	if (foundProfile)
	{
		checkHR(fn.setDeviceProfile(profileFilepath));
	}
	else
	{
		cout << "Couldn't link a profile since one was not provided!\n";
	}
	
	// Register right soft button clicks and scrolls
	checkHR(fn.registerSoftBtnCallback());

	// Register page change callback
	checkHR(fn.registerPageCallback());
	cout << "Setup Complete.\n\n";

	// Add 5 pages
	for (int i = 0; i < 4; i++)
	{
		if (i == 0)
		{
			checkHR(fn.setPage(0, FLAG_SET_AS_ACTIVE));
		}
		else
		{
			checkHR(fn.setPage(i, 0));
		}
	}

	if (edceDirectory[0] == _T('\0'))
	{
		cout << "Can't locate the edce directory. Please delete EDX52Settings.txt and restart the application.\n";
		cout << "\nPress enter to deinitialize, cleanup, and quit.\n";
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		cleanupAndClose();
		return 0;
	}

	jsonDataClass.isFirstTime = true;
	jsonDataClass.readStoreJSON(edceDirectory, defaultDirectory, edceJSONFilepath);

	fn.setString(0, 0, jsonDataClass.pg0.cmdrPage0Info[0]);
	fn.setString(0, 1, jsonDataClass.pg0.cmdrPage0Info[1]);
	fn.setString(0, 2, jsonDataClass.pg0.cmdrPage0Info[2]);

	jsonDataClass.isFirstTime = false;

	// Main loop, run once per designated cooldown time
	bool isEliteRunning = true;
	LPCTSTR appName = TEXT("Elite Dangerous Launcher");
	do
	{
		contactEDCE();
		if (FindWindow(NULL, appName) == NULL)
		{
			isEliteRunning = false;
		}
		if (closeOnWindowX)
		{
			isEliteRunning = false;
		}
		system("cls");
	} while (isEliteRunning);

	if (closeOnWindowX == false)
	{
		cout << "\nPress enter to deinitialize, cleanup, and quit.\n";
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		cleanupAndClose();
	}
 
    return 0;
}

/*
	PARAMETERS: HRESULT hr == some functions from DirectOutput return a HRESULT value, this just checks if it pass/fail and ouputs to the console
	RETURNS: none

	FUNCTION: Checks result of the function if it returns an HRESULT value

	Read more about HRESULT return types at
	Microsoft (MSDN) -> https://msdn.microsoft.com/en-us/library/windows/desktop/aa378137(v=vs.85).aspx
	Wikipedia -> https://en.wikipedia.org/wiki/HRESULT
*/
void checkHR(HRESULT hr) 
{
	if (hr == S_OK)
	{
		cout << "DONE.\n";
	}
	else
	{
		cout << "FAILED/ hr = " << hr << endl;
	}
}

void txtFileCheck()
{
	// Create .txt file for filepaths to
	// Profile to use
	// edce_client.py
	cout << "\nLooking for txt file... ";
	char *filename = "EDX52Settings.txt";
	struct stat buffer;
	if (stat(filename, &buffer) == 0 )
	{
		cout << "FOUND.\nLoading filepaths for X52 profile and EDCE client.\n";
		foundProfile = true;
		ifstream myFile("EDX52Settings.txt");
		string line;
		int lineNumber = 0;
		size_t strSize;
		if (myFile.is_open())
		{
			while (getline(myFile, line))
			{
				cout << line << endl;
				switch (lineNumber)
				{
				case 0:
					strSize = line.length();
					for (size_t i = 0; i < strSize; i++)
					{
						profileFilepath[i] = line[i];
					}
					lineNumber++;
					break;
				case 1:
					strSize = line.length();
					for (size_t i = 0; i < strSize; i++)
					{
						edceScriptFilepath[i] = line[i];
					}
					lineNumber++;
					break;
				case 2:
					strSize = line.length();
					for (size_t i = 0; i < strSize; i++)
					{
						edceJSONFilepath[i] = line[i];
					}
					lineNumber++;
					break;
				case 3:
					strSize = line.length();
					for (size_t i = 0; i < strSize; i++)
					{
						defaultDirectory[i] = line[i];
					}
					lineNumber++;
					break;
				case 4:
					strSize = line.length();
					for (size_t i = 0; i < strSize; i++)
					{
						edceDirectory[i] = line[i];
					}
					break;
				default:
					break;
				}
			}
			myFile.close();
			cout << endl;
		}
	}
	else {
		// Couldn't find file. Need to create it
		// Get and save the filepaths
		getFilepaths();
		// Create the txt file with the filepaths
		createTxtFile();
		// Read in the newly created txt file
		txtFileCheck();
	}
}

void getFilepaths()
{
	// Get the current directory so it can be restored after the files have been found
	GetCurrentDirectory(MAX_PATH, defaultDirectory);

	// Get filepaths for the profile to be used, the location of the edce_client.py and create a filepath to last.json
	cout << "Couldn't find file. Creating file \"EDX52Settings.txt\"...\n\n";
	cout << "Please select your profile to use. This will allow use of pre-assigned keybindings, colors, settings, etc.\n";
	cout << "The default location is -> C:\\Users\\Public\\Public Documents\\SmartTechnology Profiles\n";
	if (getFilePathName(true))
	{
		cout << "Got profile filepath: ";
		wcout << profileFilepath << endl;
		foundProfile = true;
	}
	else
	{
		cout << "Couldn't get the profile or the operation was canceled.\n";
		foundProfile = false;
	}

	cout << "Please find the EDCE folder which contains edce_client.py.\n";
	if (getFilePathName(false))
	{
		cout << "Got EDCE script location: ";
		wcout << edceScriptFilepath << endl;

		// remove edce_client.py from previous selection and replace with last.json
		cout << "Creating filepath for last.json from the previous selection: ";
		// Copy to a temp char array
		char tempArray[sizeof(edceScriptFilepath)];
		for (size_t i = 0; i < sizeof(edceScriptFilepath); i++)
		{
			tempArray[i] = edceScriptFilepath[i];
		}
		PathRemoveFileSpecA(tempArray);
		size_t tempArraySize = strlen(tempArray) + 1;
		wchar_t *wcString = new wchar_t[tempArraySize];
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, wcString, tempArraySize, tempArray, _TRUNCATE);
		wcscpy_s(edceDirectory, wcString);
		delete[] wcString;

		strcat_s(tempArray, "\\last.json\0");
		tempArraySize = strlen(tempArray) + 1;
		wchar_t *wcStr = new wchar_t[tempArraySize];
		convertedChars = 0;
		mbstowcs_s(&convertedChars, wcStr, tempArraySize, tempArray, _TRUNCATE);
		wcscpy_s(edceJSONFilepath, wcStr);
		delete[] wcStr;
		wcout << edceJSONFilepath << endl;

		cout << "Got needed filepaths. Creating txt file to save these paths.\n\n";
	}
	else
	{
		cout << "Couldn't get the script location or the operation was canceled.\n\n";
	}
	// Restore the starting directory
	SetCurrentDirectory(defaultDirectory);
}

void createTxtFile() {
	// Create the txt file
	cout << "Creating txt file...";
	TCHAR currentDirectoy[MAX_PATH];
	TCHAR txtFile[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, currentDirectoy);
	swprintf_s(txtFile, _T("%s\\EDX52Settings.txt"), currentDirectoy);
	HANDLE hFile = CreateFile(txtFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// Add the filepaths
	cout << "DONE.\nWriting to txt file selected locations.\n";
	wofstream myFile;
	myFile.open("EDX52Settings.txt");
	myFile << profileFilepath << "\n";
	myFile << edceScriptFilepath << "\n";
	myFile << edceJSONFilepath << "\n";
	myFile << defaultDirectory << "\n";
	myFile << edceDirectory << "\n";
	myFile.close();
	cout << "Wrote to txt file.\n\n";
}


bool getFilePathName(bool isProfile)
{
	OPENFILENAME ofn;
	HWND hwnd = 0;

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	if (isProfile)
	{
		ofn.lpstrFile = profileFilepath;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(profileFilepath);
		ofn.lpstrFilter = _T("PR0 File (.pr0)\0*.pr0*\0\0");
	}
	else
	{
		ofn.lpstrFile = edceScriptFilepath;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(edceScriptFilepath);
		ofn.lpstrFilter = _T("Python File (.py)\0*.py*\0\0");
	}

	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = L"C:\\";
	ofn.Flags = OFN_PATHMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn) == TRUE) {
		return true;
	}
	else
	{
		return false;
	}
}


void contactEDCE()
{
	// Do this once a minute and then update pages with relevant data
	// Need to set the directory from the script location
	SetCurrentDirectory(edceDirectory);
	
	// Run the script
	_wsystem(edceScriptFilepath);

	// Restore working directory
	SetCurrentDirectory(defaultDirectory);

	// Update information
	jsonDataClass.readStoreJSON(edceDirectory, defaultDirectory, edceJSONFilepath);
	jsonDataClass.updateCurrentPage(edceDirectory, defaultDirectory, edceJSONFilepath, fn);

	// Wait for 2.5 minutes
	cout << "Cooldown: 2.5 minutes\n";
	using namespace std::this_thread;
	using namespace std::chrono;
	// Reset timer
	timer = 150;
	while (timer != 0)
	{
		sleep_until(system_clock::now() + 1s);
		timer--;
		cout << timer << "..";
	}
	cout << endl;
}

void cleanupAndClose() {
	//Unregister the callbacks
	checkHR(fn.unRegisterSoftBtnCallback());
	checkHR(fn.unRegisterPageCallback());

	// Deinitialize DirectOutput
	checkHR(fn.Deinitialize());
}

BOOL controlHandler(DWORD fdwCtrlType) {
	switch (fdwCtrlType)
	{
		// Handle the CTRL-C signal. 
	case CTRL_C_EVENT:
		cout << "Ctrl-C event\n\n";
		closeOnWindowX = true;
		cleanupAndClose();
		timer = 1;
		return(TRUE);

		// CTRL-CLOSE: confirm that the user wants to exit on 'X' button click on window. 
	case CTRL_CLOSE_EVENT:
		cout << "Ctrl-Close event\n\n";
		closeOnWindowX = true;
		cleanupAndClose();
		timer = 1;
		return(TRUE);

		// Pass other signals to the next handler. 
	case CTRL_BREAK_EVENT:
		cout << "Ctrl-Break event\n\n";
		return FALSE;

	case CTRL_LOGOFF_EVENT:
		cout << "Ctrl-Logoff event\n\n";
		return FALSE;

	case CTRL_SHUTDOWN_EVENT:
		cout << "Ctrl-Shutdown event\n\n";
		return FALSE;

	default:
		return FALSE;
	}
}