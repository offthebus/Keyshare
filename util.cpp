#include "stdafx.h"
#include "util.h"

namespace util {

void printError() 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + 40) * sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("SYSTEM ERROR: %d: %s"), 
        dw, lpMsgBuf); 
	
		printf("%s\n",(char*)lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

struct LogBuffers
{
	char fname[MAX_PATH];
   char ext[_MAX_EXT]; ;
	char msg[1024];
	char dbg[1024];
} 
g_buffers;
	
//---------------------------------------------------------------------------
void Log::sysprint( const char* file, int line, DWORD err )
{
	char buf[128];
	print(file,line,"SYSTEM ERROR: %s", strerror_s(buf,_countof(buf)) );
}

//---------------------------------------------------------------------------
void Log::print( const char* file, int line, const char* fmt, ... )
{
	if ( !m_enabled )
		return;

	_splitpath_s( file, NULL, 0, NULL, 0, g_buffers.fname, countof(g_buffers.fname), g_buffers.ext, countof(g_buffers.ext) );

	va_list args;
	va_start( args, fmt );
	vsprintf_s( g_buffers.msg, countof(g_buffers.msg), fmt, args ); 

	if ( fpLog )
		fprintf_s(fpLog, "%s%s(%d): %s\n", g_buffers.fname, g_buffers.ext, line, g_buffers.msg);

//	printf("%s%s(%d): %s\n", g_buffers.fname, g_buffers.ext, line, g_buffers.msg);

	if ( IsDebuggerPresent() )
	{
		sprintf_s( g_buffers.dbg, countof(g_buffers.dbg), "\n**** %s%s(%d): %s\n", file, g_buffers.ext, line, g_buffers.msg ); 
		OutputDebugString(g_buffers.dbg);
	}
}

//---------------------------------------------------------------------------
const char* strAllocCopy( const char* src, size_t* numCharsExclZero/*=0*/ )
{
	size_t len = strlen( src );

	if ( numCharsExclZero )
		*numCharsExclZero = len;

	char* ptr;
	ptr = new char[ len+1 ];

	strcpy_s( ptr, len+1, src );
	return (const char*)ptr;
}

//---------------------------------------------------------------------------
StringFile::StringFile( const char* filename )
: m_buf(0), m_len(0), m_filename(0)
{
	char *full = _fullpath(NULL,filename,MAX_PATH+2);
	m_filename = strAllocCopy(filename);

	FILE* fp;
	fopen_s( &fp, full, "rb" );

	if ( !fp )
	{
		char buf[128];
		strerror_s( buf, _countof(buf), errno );
		LOG("ERROR: %s: %s", full, buf );
		return;
	}

	fseek( fp, 0, SEEK_END );
	size_t len = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	if ( !len )
	{
		LOG("ERROR: %s: zero length file", full );
		fclose(fp);
		return;
	}

	m_buf = new char[ len + 1 ];

	if ( len == fread( m_buf, 1, len, fp ) )
	{
		m_len = len;
		m_buf[ len ] = 0;
	}
	else
	{
		char buf[128];
		strerror_s( buf, _countof(buf), errno );
		LOG("ERROR: %s: %s", full, buf );
		delete[] m_buf;
		m_buf = 0;
	}

	fclose(fp);
}

typedef BOOL (WINAPI *LPFN_GLPI)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, 
    PDWORD);
//
// Helper function to count set bits in the processor mask.
DWORD CountSetBits(ULONG_PTR bitMask)
{
    DWORD LSHIFT = sizeof(ULONG_PTR)*8-1;
    DWORD bitSetCount = 0;
//    ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;    
//    DWORD i;
//    
//    for (i = 0; i <= LSHIFT; ++i)
//    {
//        bitSetCount += ((bitMask & bitTest)?1:0);
//        bitTest/=2;
//    }
//

	do
	{
		bitSetCount += bitMask&1;
		bitMask >>= 1;
	}
	while (LSHIFT--);
	 
	return bitSetCount;
}

//---------------------------------------------------------------------------
void getProcessorInfo(DWORD& logical, DWORD& cores)
{
    LPFN_GLPI glpi;
    BOOL done = FALSE;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
    DWORD returnLength = 0;
    DWORD logicalProcessorCount = 0;
    DWORD numaNodeCount = 0;
    DWORD processorCoreCount = 0;
    DWORD processorL1CacheCount = 0;
    DWORD processorL2CacheCount = 0;
    DWORD processorL3CacheCount = 0;
    DWORD processorPackageCount = 0;
    DWORD byteOffset = 0;
    PCACHE_DESCRIPTOR Cache;

	 logical = cores = 0;

    glpi = (LPFN_GLPI) GetProcAddress(
                            GetModuleHandle(TEXT("kernel32")),
                            "GetLogicalProcessorInformation");
    if (NULL == glpi) 
    {
        return;
    }

    while (!done)
    {
        DWORD rc = glpi(buffer, &returnLength);

        if (FALSE == rc) 
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
            {
                if (buffer) 
                    free(buffer);

                buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
                        returnLength);

                if (NULL == buffer) 
                {
                    return;
                }
            } 
            else 
            {
                return;
            }
        } 
        else
        {
            done = TRUE;
        }
    }

    ptr = buffer;

    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) 
    {
        switch (ptr->Relationship) 
        {
        case RelationNumaNode:
            // Non-NUMA systems report a single record of this type.
            numaNodeCount++;
            break;

        case RelationProcessorCore:
            processorCoreCount++;

            // A hyperthreaded core supplies more than one logical processor.
            logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
            break;

        case RelationCache:
            // Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
            Cache = &ptr->Cache;
            if (Cache->Level == 1)
            {
                processorL1CacheCount++;
            }
            else if (Cache->Level == 2)
            {
                processorL2CacheCount++;
            }
            else if (Cache->Level == 3)
            {
                processorL3CacheCount++;
            }
            break;

        case RelationProcessorPackage:
            // Logical processors share a physical package.
            processorPackageCount++;
            break;

        default:
            break;
        }
        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }

	 logical = logicalProcessorCount;
	 cores	= processorCoreCount;

    //_tprintf(TEXT("\nGetLogicalProcessorInformation results:\n"));
    //_tprintf(TEXT("Number of NUMA nodes: %d\n"), 
    //         numaNodeCount);
    //_tprintf(TEXT("Number of physical processor packages: %d\n"), 
    //         processorPackageCount);
    //_tprintf(TEXT("Number of processor cores: %d\n"), 
    //         processorCoreCount);
    //_tprintf(TEXT("Number of logical processors: %d\n"), 
    //         logicalProcessorCount);
    //_tprintf(TEXT("Number of processor L1/L2/L3 caches: %d/%d/%d\n"), 
    //         processorL1CacheCount,
    //         processorL2CacheCount,
    //         processorL3CacheCount);
    
    free(buffer);

}

//---------------------------------------------------------------------------
// ::VIRTUALKEYDICTIONARY
//---------------------------------------------------------------------------
VirtualKeyDictionary::VirtualKeyDictionary()
{
	m_dictionary.insert(DICTIONARY::value_type(VK_BACK,"Backspace"));
	m_dictionary.insert(DICTIONARY::value_type(VK_TAB,"Tab"));
	m_dictionary.insert(DICTIONARY::value_type(VK_CLEAR,"Clear"));
	m_dictionary.insert(DICTIONARY::value_type(VK_RETURN,"Return"));
	m_dictionary.insert(DICTIONARY::value_type(VK_SHIFT,"Shift"));
	m_dictionary.insert(DICTIONARY::value_type(VK_CONTROL,"Control"));
	m_dictionary.insert(DICTIONARY::value_type(VK_MENU,"Alt"));
	m_dictionary.insert(DICTIONARY::value_type(VK_PAUSE,"Pause"));

	m_dictionary.insert(DICTIONARY::value_type(VK_CAPITAL,""));

	m_dictionary.insert(DICTIONARY::value_type(VK_ESCAPE,"Esc"));
	m_dictionary.insert(DICTIONARY::value_type(VK_SPACE,"Space"));
	m_dictionary.insert(DICTIONARY::value_type(VK_LEFT,"Left"));
	m_dictionary.insert(DICTIONARY::value_type(VK_UP,"Up"));
	m_dictionary.insert(DICTIONARY::value_type(VK_RIGHT,"Right"));
	m_dictionary.insert(DICTIONARY::value_type(VK_DOWN,"Down"));


	m_dictionary.insert(DICTIONARY::value_type(VK_PRINT,"Print"));
	m_dictionary.insert(DICTIONARY::value_type(VK_EXECUTE,"Execute"));
	m_dictionary.insert(DICTIONARY::value_type(VK_SNAPSHOT,"Snapshot"));
	m_dictionary.insert(DICTIONARY::value_type(VK_INSERT,"Insert"));
	m_dictionary.insert(DICTIONARY::value_type(VK_DELETE,"Delete"));
	m_dictionary.insert(DICTIONARY::value_type(VK_HELP,"Help"));
	m_dictionary.insert(DICTIONARY::value_type(VK_SELECT,"Select"));

	m_dictionary.insert(DICTIONARY::value_type(0x30,"0"));
	m_dictionary.insert(DICTIONARY::value_type(0x31,"1"));
	m_dictionary.insert(DICTIONARY::value_type(0x32,"2"));
	m_dictionary.insert(DICTIONARY::value_type(0x33,"3"));
	m_dictionary.insert(DICTIONARY::value_type(0x34,"4"));
	m_dictionary.insert(DICTIONARY::value_type(0x35,"5"));
	m_dictionary.insert(DICTIONARY::value_type(0x36,"6"));
	m_dictionary.insert(DICTIONARY::value_type(0x37,"7"));
	m_dictionary.insert(DICTIONARY::value_type(0x38,"8"));
	m_dictionary.insert(DICTIONARY::value_type(0x39,"9"));
	m_dictionary.insert(DICTIONARY::value_type(0x41,"A"));
	m_dictionary.insert(DICTIONARY::value_type(0x42,"B"));
	m_dictionary.insert(DICTIONARY::value_type(0x43,"C"));
	m_dictionary.insert(DICTIONARY::value_type(0x44,"D"));
	m_dictionary.insert(DICTIONARY::value_type(0x45,"E"));
	m_dictionary.insert(DICTIONARY::value_type(0x46,"F"));
	m_dictionary.insert(DICTIONARY::value_type(0x47,"G"));
	m_dictionary.insert(DICTIONARY::value_type(0x48,"H"));
	m_dictionary.insert(DICTIONARY::value_type(0x49,"I"));
	m_dictionary.insert(DICTIONARY::value_type(0x4A,"J"));
	m_dictionary.insert(DICTIONARY::value_type(0x4B,"K"));
	m_dictionary.insert(DICTIONARY::value_type(0x4C,"L"));
	m_dictionary.insert(DICTIONARY::value_type(0x4D,"M"));
	m_dictionary.insert(DICTIONARY::value_type(0x4E,"N"));
	m_dictionary.insert(DICTIONARY::value_type(0x4F,"O"));
	m_dictionary.insert(DICTIONARY::value_type(0x50,"P"));
	m_dictionary.insert(DICTIONARY::value_type(0x51,"Q"));
	m_dictionary.insert(DICTIONARY::value_type(0x52,"R"));
	m_dictionary.insert(DICTIONARY::value_type(0x53,"S"));
	m_dictionary.insert(DICTIONARY::value_type(0x54,"T"));
	m_dictionary.insert(DICTIONARY::value_type(0x55,"U"));
	m_dictionary.insert(DICTIONARY::value_type(0x56,"V"));
	m_dictionary.insert(DICTIONARY::value_type(0x57,"W"));
	m_dictionary.insert(DICTIONARY::value_type(0x58,"X"));
	m_dictionary.insert(DICTIONARY::value_type(0x59,"Y"));
	m_dictionary.insert(DICTIONARY::value_type(0x5A,"Z"));


	m_dictionary.insert(DICTIONARY::value_type(VK_LWIN,"Lwin"));
	m_dictionary.insert(DICTIONARY::value_type(VK_RWIN,"Rwin"));
	m_dictionary.insert(DICTIONARY::value_type(VK_APPS,"Apps"));
	m_dictionary.insert(DICTIONARY::value_type(VK_SLEEP,"Sleep"));

	m_dictionary.insert(DICTIONARY::value_type(VK_NUMPAD0,"Numpad0"));
	m_dictionary.insert(DICTIONARY::value_type(VK_NUMPAD1,"Numpad1"));
	m_dictionary.insert(DICTIONARY::value_type(VK_NUMPAD2,"Numpad2"));
	m_dictionary.insert(DICTIONARY::value_type(VK_NUMPAD3,"Numpad3"));
	m_dictionary.insert(DICTIONARY::value_type(VK_NUMPAD4,"Numpad4"));
	m_dictionary.insert(DICTIONARY::value_type(VK_NUMPAD5,"Numpad5"));
	m_dictionary.insert(DICTIONARY::value_type(VK_NUMPAD6,"Numpad6"));
	m_dictionary.insert(DICTIONARY::value_type(VK_NUMPAD7,"Numpad7"));
	m_dictionary.insert(DICTIONARY::value_type(VK_NUMPAD8,"Numpad8"));
	m_dictionary.insert(DICTIONARY::value_type(VK_NUMPAD9,"Numpad9"));
	m_dictionary.insert(DICTIONARY::value_type(VK_MULTIPLY,"Multiply"));
	m_dictionary.insert(DICTIONARY::value_type(VK_ADD,"Add"));
	m_dictionary.insert(DICTIONARY::value_type(VK_SUBTRACT,"Subtract"));
	m_dictionary.insert(DICTIONARY::value_type(VK_DECIMAL,"Decimal"));
	m_dictionary.insert(DICTIONARY::value_type(VK_DIVIDE,"Divide"));
	m_dictionary.insert(DICTIONARY::value_type(VK_F1,"F1"));
	m_dictionary.insert(DICTIONARY::value_type(VK_F2,"F2"));
	m_dictionary.insert(DICTIONARY::value_type(VK_F3,"F3"));
	m_dictionary.insert(DICTIONARY::value_type(VK_F4,"F4"));
	m_dictionary.insert(DICTIONARY::value_type(VK_F5,"F5"));
	m_dictionary.insert(DICTIONARY::value_type(VK_F6,"F6"));
	m_dictionary.insert(DICTIONARY::value_type(VK_F7,"F7"));
	m_dictionary.insert(DICTIONARY::value_type(VK_F8,"F8"));
	m_dictionary.insert(DICTIONARY::value_type(VK_F9,"F9"));
	m_dictionary.insert(DICTIONARY::value_type(VK_F10,"F10"));
	m_dictionary.insert(DICTIONARY::value_type(VK_F11,"F11"));
	m_dictionary.insert(DICTIONARY::value_type(VK_F12,"F12"));

	m_dictionary.insert(DICTIONARY::value_type(VK_NUMLOCK,"Numlock"));
	m_dictionary.insert(DICTIONARY::value_type(VK_SCROLL,"Vscroll"));

	m_dictionary.insert(DICTIONARY::value_type(VK_LSHIFT,"Lshift"));
	m_dictionary.insert(DICTIONARY::value_type(VK_RSHIFT,"Rshift"));
	m_dictionary.insert(DICTIONARY::value_type(VK_LCONTROL,"Lcontrol"));
	m_dictionary.insert(DICTIONARY::value_type(VK_RCONTROL,"Rcontrol"));


	m_dictionary.insert(DICTIONARY::value_type(VK_OEM_1,"Oem_1"));
	m_dictionary.insert(DICTIONARY::value_type(VK_OEM_2,"Oem_2"));
	m_dictionary.insert(DICTIONARY::value_type(VK_OEM_3,"Oem_3"));
	m_dictionary.insert(DICTIONARY::value_type(VK_OEM_4,"Oem_4"));
	m_dictionary.insert(DICTIONARY::value_type(VK_OEM_5,"Oem_5"));
	m_dictionary.insert(DICTIONARY::value_type(VK_OEM_6,"Oem_6"));
	m_dictionary.insert(DICTIONARY::value_type(VK_OEM_7,"Oem_7"));
	m_dictionary.insert(DICTIONARY::value_type(VK_OEM_8,"Oem_8"));
	m_dictionary.insert(DICTIONARY::value_type(VK_OEM_102,"Oem_102"));
	m_dictionary.insert(DICTIONARY::value_type(VK_OEM_PLUS,"Oem_plus"));
	m_dictionary.insert(DICTIONARY::value_type(VK_OEM_COMMA,"Oem_comma"));
	m_dictionary.insert(DICTIONARY::value_type(VK_OEM_MINUS,"Oem_minus"));
	m_dictionary.insert(DICTIONARY::value_type(VK_OEM_PERIOD,"Oem_period"));
}

}; // namespace ut
