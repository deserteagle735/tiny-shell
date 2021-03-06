#include "process.h"
#include "macro.h"

// Use in Suspend and Resume process
typedef LONG (NTAPI * NtSuspendProcess)(IN HANDLE ProcessHandle);
typedef LONG (NTAPI * NtResumeProcess)(IN HANDLE ProcessHandle);

PROCESS_INFORMATION* piPointer;

// Function to printf errors while working with processes
void printError() {
    DWORD eNum;
    char sysMsg[256];
    char* p;
    //Get error
    eNum = GetLastError( );
    FormatMessage(  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, eNum,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                    sysMsg, 256, NULL );

  // Trim the end of the line and terminate it with a null
  p = sysMsg;
  while( ( *p > 31 ) || ( *p == 9 ) )
    ++p;
  do { *p-- = 0; } while( ( p >= sysMsg ) &&
                          ( ( *p == '.' ) || ( *p < 33 ) ) );

  // Display the message
  printf("\n  WARNING: Operation failed with error %d (%s)", eNum, sysMsg );
}

// List process
void ListProcess(int argc, char** argv) {
    if (argc > 2) {
        _WARNING_MANY_ARG_("list");
    }
    int searchAllProcess = 0; // 0 = do not list system process
                            // 1 = list all process
    if (argc == 2) {
        // search all
        if (!strcmp(argv[1], "all")) {
            searchAllProcess = 1;
        }
        // invalid command 
        else {
            printf("Invalid argument for \'%s\'. Try \'%s all\'\n", argv[0], argv[0]);
            return;
        }
    }

    /* List process
        take a napshot (picture) of whole system,
        use pe32 as a iterator to walk process by process through the snapshot */
    HANDLE hProcess;
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if( hProcessSnap == INVALID_HANDLE_VALUE )
    {
        //Can not take snapshot
        printError();
        return;
    }
    // Set the size of the structure before using it.
    pe32.dwSize = sizeof( PROCESSENTRY32 );
    // Retrieve information about the first process,
    // and exit if unsuccessful
    if( !Process32First( hProcessSnap, &pe32 ) )
    {
        printError(); // show cause of failure
        CloseHandle( hProcessSnap );          // clean the snapshot object
        return;
    }
    // Now walk the snapshot of processes, and
    // display information about each process in turn
    if (searchAllProcess == 0) {
        // Do not list system process
        do {
            // Try open process
            hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID );
            if (hProcess != NULL) {
                // Not a system process
                printf( "\nPROCESS NAME:  %s", pe32.szExeFile );
                printf( "\n  Process ID        = %d", pe32.th32ProcessID );
                printf( "\n  Thread count      = %d",   pe32.cntThreads );
            }
        } while( Process32Next( hProcessSnap, &pe32 ) ); //Go to next process
    }
    else {
        // List all process, included system process
        do {
            printf( "\nPROCESS NAME:  %s", pe32.szExeFile );
            printf( "\n  Process ID        = %d", pe32.th32ProcessID );
            printf( "\n  Thread count      = %d",   pe32.cntThreads );
        } while( Process32Next( hProcessSnap, &pe32 ) ); // Go to next process
    }
    CloseHandle( hProcessSnap ); // Close snapshot
}

/*  Search a process by name
    As ListProcess, this function take a snapshot of the whole system 
    then walk through to find the fisrt process that have matched name 
*/
PROCESS SearchProcessName(char* processName) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    // Inittialize an empty process
    PROCESS p = {"noname", 0, 0};
    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if( hProcessSnap == INVALID_HANDLE_VALUE )
    {
        printError();
        return p;
    }
    // Set the size of the structure before using it.
    pe32.dwSize = sizeof( PROCESSENTRY32 );
    // Retrieve information about the first process,
    // and exit if unsuccessful
    if( !Process32First( hProcessSnap, &pe32 ) )
    {
        printError(); // show cause of failure
        CloseHandle( hProcessSnap );          // clean the snapshot object
        return p;
    }
    // Now walk the snapshot of processes, and
    // display information about each process in turn
    do {
        if (strcmp(pe32.szExeFile, processName) == 0) {
            //There is a matched process
            strcpy(p.name, pe32.szExeFile);
            p.processID = pe32.th32ProcessID;
            p.threadCount = pe32.cntThreads;
            return p;
        }
    } while( Process32Next( hProcessSnap, &pe32 ) );
    CloseHandle( hProcessSnap );
    return p;
    // If can not find a suitable process return p {"noname", 0, 0}
}

/* Search for all process that have matched name, then print them out
*/
BOOL SearchAllProcessName(char* processName) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    BOOL found = FALSE;
    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if( hProcessSnap == INVALID_HANDLE_VALUE )
    {
        printError();
        return found;
    }
    // Set the size of the structure before using it.
    pe32.dwSize = sizeof( PROCESSENTRY32 );
    // Retrieve information about the first process,
    // and exit if unsuccessful
    if( !Process32First( hProcessSnap, &pe32 ) )
    {
        printError(); // show cause of failure
        CloseHandle( hProcessSnap );          // clean the snapshot object
        return found;
    }
    // Now walk the snapshot of processes, and
    // display information about each process in turn
    do {
        if (strcmp(pe32.szExeFile, processName) == 0) {
            //There is a matched process, print it out
            printf( "\nPROCESS NAME:  %s", pe32.szExeFile );
            printf( "\n  Process ID        = %d", pe32.th32ProcessID );
            printf( "\n  Thread count      = %d",   pe32.cntThreads );
            found = TRUE;
        }
    } while( Process32Next( hProcessSnap, &pe32 ) );
    CloseHandle( hProcessSnap );
    return found;
    // If can not find a suitable process return p {"noname", 0, 0}
}

/*  Search for a process by process ID
    As ListProcess, this function take a snapshot of the whole system 
    then walk through to find the fisrt process that have matched name 
*/
PROCESS SearchProcessID(int processID) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    //Initialize anempty process
    PROCESS p = {"noname", 0, 0};
    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if( hProcessSnap == INVALID_HANDLE_VALUE )
    {
        printError();
        return p;
    }
    // Set the size of the structure before using it.
    pe32.dwSize = sizeof( PROCESSENTRY32 );
    // Retrieve information about the first process,
    // and exit if unsuccessful
    if( !Process32First( hProcessSnap, &pe32 ) )
    {
        printError(); // show cause of failure
        CloseHandle( hProcessSnap );          // clean the snapshot object
        return p;
    }
    // Now walk the snapshot of processes, and
    // display information about each process in turn
    do {
        if (pe32.th32ProcessID == processID) {
            // if there is matched process
            strcpy(p.name, pe32.szExeFile);
            p.processID = pe32.th32ProcessID;
            p.threadCount = pe32.cntThreads;
            return p;
        }
    } while( Process32Next( hProcessSnap, &pe32 ) );
    CloseHandle( hProcessSnap );
    return p;
    // If can not find a suitable process return p {"noname", 0, 0}
}

/* Search for a process in 2 ways:
    1, by name
        search "process_name"
    2. by processID
        search pid "processID"
*/
void SearchProcess(int argc, char** argv) {  
    if (argc > 3) {
        _WARNING_MANY_ARG_("search");
    }
    if (argc < 2) {
        _WARNING_FEW_ARG_("search");
    }
    // Search by name
    if (argc == 2) {
        char* processName = argv[1];
        BOOL found = SearchAllProcessName(processName);    
        if (found == FALSE) {
            // there is no thread means can not find
            printf("Process not found\n");
        }
    }
    // Search by process ID
    else if (argc ==3) {
        if (!strcmp(argv[1], "pid")) {
            //second argument is "pid"
            int processID = atoi(argv[2]);
            PROCESS p = SearchProcessID(processID);
            if (p.threadCount == 0) {
                printf("Process not found\n");
            }
            else {
                printf("\nPROCESS NAME:  %s\n", p.name );
                printf("  Process ID        = %d\n", p.processID );
                printf("  Thread count      = %d\n", p.threadCount );
            }
        }
        // invalid command
        else{
            printf("Invalid argument for \'%s\'. Try \'%s pid process_ID\'\n", argv[0], argv[0]);
            return;
        }
    }
}

/*  Kill a process in 2 ways:
    1. by name
        kill "process_name"
    2. ny process ID
        kill pid "process_ID"
*/
void KillProcess(int argc, char** argv) {  
    if (argc < 2) {
        _WARNING_FEW_ARG_("kill");
    }
    if (argc > 3) {
        _WARNING_MANY_ARG_("kill");
    }
    //Kill by name
    if (argc == 2) {
        char* processName = argv[1];
        // Search for process by mane
        PROCESS p = SearchProcessName(argv[1]);
        if (p.threadCount == 0) {
            // thread count = 0 means can not find process
            printf("Process not found\n");
            return;
        }
        // take handle of process
        HANDLE hProcess = OpenProcess( PROCESS_TERMINATE, TRUE, p.processID );
        if (hProcess == NULL) 
            //Error can not take handle
            printError();
        else {
            // terminate process
            printf("Process terminated\n");
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }
    }
    //Kill by process ID
    else if (argc == 3) {
        if (!strcmp(argv[1], "pid")) {
            // second argument is "pid"
            int processID = atoi(argv[2]);
            // take handle of process 
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, TRUE, processID );
            if (hProcess == NULL) 
                // error can not find process
                printError();
            else {
                // terminate process 
                printf("Process terminated\n");
                TerminateProcess(hProcess, 0);
                CloseHandle(hProcess);
            }
        }
        else {
            // invalid command           
            printf("Invalid argument for \'%s\'. Try \'%s pid process_ID\'\n", argv[0], argv[0]);
            return;
        }        
    }   
}

/* If found, stop it using NtSuspendProcess in ntdll.dll 
*/

void SuspendProcessID(int processID) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL) {
        printError();
        return;
    }
    // copy address of function NtSuspendProcess in ntdll.dll to pfnMtSuspendProcess
    NtSuspendProcess pfnNtSuspendProcess = 
        (NtSuspendProcess)GetProcAddress(GetModuleHandle("ntdll"), "NtSuspendProcess");
    //Stop process 
    pfnNtSuspendProcess(hProcess);
    printf("Process Stopped\n");
    CloseHandle(hProcess);
}

/* Stop a process in 2 way:
    1. by name
        stop "process_name"
    2. by process_id
        stop pid "process_id"
*/
void StopProcess(int argc, char** argv) {
    if (argc < 2) {
        _WARNING_FEW_ARG_("stop");
    }
    if (argc > 3) {
        _WARNING_MANY_ARG_("stop");
    }
    // stop by name
    if (argc == 2) {
        //Search for first process that have matched name
        PROCESS p = SearchProcessName(argv[1]);
        if (p.threadCount == 0) {
            // can not find process
            printf("Process not found\n");
        }
        else {
            // if there is a process, stop it by process_id
            SuspendProcessID(p.processID);
        }
    }
    //stop by process_id
    else if (argc ==3) {
        // if second argument is "pid"
        if (!strcmp(argv[1], "pid")) {
            int processID = atoi(argv[2]);
            // stop process
            SuspendProcessID(processID);
        }
        // invalid command
        else {
            printf("Invalid argument for \'%s\'. Try \'%s pid process_ID\'\n", argv[0], argv[0]);
            return;
        }
    }
}

/* Resume a process by process_id using NtResumeProcess in  ntdll.dll 
*/
void ResumeProcessID(int processID) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL) {
        // if can not open process
        printError();
        return;
    }
    // Copy address of function NtResumeProcess in ntdll.dll to pfnNtResumeProcess 
    NtResumeProcess pfnNtResumeProcess = 
        (NtResumeProcess)GetProcAddress(GetModuleHandle("ntdll"), "NtResumeProcess");
    //Resume process
    pfnNtResumeProcess(hProcess);
    printf("Process Resumed\n");
    CloseHandle(hProcess);
}

/* Resume proces in 2 way:
    1. by name
        resume "process_name"
    2. by process_id
        resume pid "process_id"
*/
void ResumeProcess(int argc, char** argv) {
    if (argc < 2) {
        _WARNING_FEW_ARG_("resume");
    }
    if (argc > 3) {
        _WARNING_MANY_ARG_("resume");
    }
    // Resume by name
    if (argc == 2) {
        // search for first process have matched name
        PROCESS p = SearchProcessName(argv[1]);
        if (p.threadCount == 0) {
            printf("Process not found\n");
        }
        // if there is process
        else {
            ResumeProcessID(p.processID);
        }
    }
    // Resume by process_id 
    else if (argc ==3) {
        //second argument id pid
        if (!strcmp(argv[1], "pid")) {
            int processID = atoi(argv[2]);
            // resume process by process_id
            ResumeProcessID(processID);
        }
        //invalid command
        else {
            printf("Invalid argument for \'%s\'. Try \'%s pid process_ID\'\n", argv[0], argv[0]);
            return;
        }
    }
}

/* capture Ctrl_C event, and terminate process 
*/
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
        // Handle the CTRL-C signal. 
        case CTRL_C_EVENT:
            TerminateProcess(piPointer->hProcess, 0);
            CloseHandle(piPointer->hProcess);
            CloseHandle(piPointer->hThread);
            return TRUE;
        default:
            return FALSE;

    }
}

/* Create background process
*/
void CreateProcessBackground(int argc, char** argv, char* command){
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    ZeroMemory( &pi, sizeof(pi) );
    if(!CreateProcess(  NULL, 
                    command, 
                    NULL, 
                    NULL, 
                    FALSE, 
                    CREATE_NEW_CONSOLE, 
                    NULL, 
                    NULL, 
                    &si, 
                    &pi)) {
        // if can not create process 
        printf("\'%s\' is not recognized as an internal or external command, operable program or batch file.\n", argv[0]);
    }
}

/* Create forground process and set Ctrl_C handler 
*/
void CreateProcessForeground(int argc, char** argv, char* command){
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    ZeroMemory( &pi, sizeof(pi) );
    if(!CreateProcess(  NULL, 
                    command, 
                    NULL, 
                    NULL, 
                    FALSE, 
                    NORMAL_PRIORITY_CLASS, 
                    NULL, 
                    NULL, 
                    &si, 
                    &pi)) {
        // if can not create process
        printf("\'%s\' is not recognized as an internal or external command, operable program or batch file.\n", argv[0]);
    }
    else{
        // piPointer is a global variable  
        piPointer = &pi;
        // Set Ctrl_C handler
        SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler, TRUE );
        // Wait until the new process terminate
        WaitForSingleObject(pi.hProcess, INFINITE);
    }
}

