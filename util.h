#ifndef _H_UTIL
#define _H_UTIL

#define LOG(fmt,...) util::Log::getInstance().print( __FILE__, __LINE__, fmt, __VA_ARGS__ )
#define LOGSYS() util::Log::getInstance().sysprint(__FILE__, __LINE__, GetLastError() )

//#define PROFILE_ENABLE

#ifdef PROFILE_ENABLE
	#define PROFILE_START { util::Timer _tim; _tim.start();
	#define PROFILE_STOP(msg) _tim.stop(); LOG("%s: %d", msg, (int)floor(_tim.microseconds())); }
#else
	#define PROFILE_START
	#define PROFILE_STOP(msg)
#endif

namespace util {

template<class T> T highvalues(T arg) { return std::numeric_limits<T>::max(); }
template<class T> T lowvalues(T arg) { return std::numeric_limits<T>::min(); }

template<typename T> void zeroMem(T& obj) { memset(&obj,0,sizeof(T)); }
void getProcessorInfo(DWORD& logical, DWORD& cores);

template<class T> T clamp(T value, T min, T max) { 
	return value<min?min:value>max?max:value;
}

template<class T> T lerp(T min, T max, T mod) {
	return min + (max-min)*mod;
}

void printError();

//---------------------------------------------------------------------------
class Timer
{
	public:
		Timer() : started(false) { 
			QueryPerformanceFrequency(&freq); }
		
		void start() { 
			started = true;
			QueryPerformanceCounter(&a); ticks.QuadPart = 0; }

		void stop() { 
			if (started) {
				QueryPerformanceCounter(&b); ticks.QuadPart = b.QuadPart - a.QuadPart;
			} else {
				ticks.QuadPart = 0;
			}
			started = false;
		}

		double seconds() { 
			return (double)ticks.QuadPart/(double)freq.QuadPart; } 

		double milliseconds() { 
			return (double)(ticks.QuadPart * 1000) / (double)freq.QuadPart; }

		double microseconds() { 
			return (double)(ticks.QuadPart * 1000000) / (double)freq.QuadPart; }

		double nanoseconds() {
			return (double)(ticks.QuadPart * 1000000000) / (double)freq.QuadPart; }

		double diffPercent(Timer& rhs) {
			return (double)(rhs.ticks.QuadPart - ticks.QuadPart)/(double)ticks.QuadPart*100.0; }

	private:
		LARGE_INTEGER a,b,freq,ticks;
		bool started;
};

//---------------------------------------------------------------------------
class CriticalSection
{
public:
	CriticalSection() {
		InitializeCriticalSection(&m_lock); }

	~CriticalSection() {
		DeleteCriticalSection(&m_lock); }

	void lock() {
		EnterCriticalSection(&m_lock); }

	void unlock() {
		LeaveCriticalSection(&m_lock); }

private:
	CRITICAL_SECTION m_lock;
};

//---------------------------------------------------------------------------
class FrameRate
{
	static const int NUM_SAMPLES = 100;

	public:
		static FrameRate& getInstance()
		{
			static FrameRate instance;
			return instance;
		}

		float framerate()
		{
			static float rThou = 1.0f/1000.0f;
			
			float timeMs = 0.0f;

			for ( int i=0; i<NUM_SAMPLES; ++i )
				timeMs += m_time[i];
			
			return NUM_SAMPLES/(timeMs*rThou);
		}

		void update(float ms)
		{
			if ( ++m_timeIdx == NUM_SAMPLES )
				m_timeIdx = 0;

			m_time[m_timeIdx] = ms;
		}

		const char* toString() { 
			snprintf(m_buf, _countof(m_buf), "%d fps", (int)framerate() );
			return m_buf;
		}

	private:
		FrameRate() : m_timeIdx(-1)
		{
			memset( &m_time, 0, NUM_SAMPLES*sizeof(m_time[0]));
		}
		FrameRate(const FrameRate&) = delete;
		void operator=(const FrameRate&) = delete;

		float m_time[NUM_SAMPLES];
		int m_timeIdx;
		char m_buf[32];
};

//---------------------------------------------------------------------------
// number of elements in array
template<typename T, size_t N>
size_t countof( T (&arr)[N] ) { return std::extent<T[N]>::value; }

// size of array
template<typename T, size_t N>
size_t arraysize( T (&arr)[N] ) { return countof(arr) * sizeof(T); } 

//---------------------------------------------------------------------------
class Log
{
	public:
		static Log& getInstance()
		{
			static Log instance;
			return instance;
		}
		
		void enable(bool s) { m_enabled = s; }
		bool isEnabled() { return m_enabled; }
		void print( const char* file, int line, const char* fmt,  ... );
		void sysprint( const char* file, int line, DWORD err );

	private:
		
		Log()
		: m_enabled(false)
		{
			fopen_s(&fpLog,"log.txt","wt");
		}

		~Log()
		{
			if (fpLog) fclose(fpLog);
		}

		void operator=(const Log&) = delete;
		Log(const Log&) = delete;

		FILE *fpLog;
		bool m_enabled;
};
			
//---------------------------------------------------------------------------
// allocate memory and copy string to it, caller owns the memory
const char* strAllocCopy( const char* src, size_t* numCharsExclZero=0 );

//---------------------------------------------------------------------------
// read a file into a null terminated buffer 
class StringFile
{
	public:
		StringFile( const char* filename );

		virtual ~StringFile() { 
			delete[] m_buf;
			delete[] m_filename; }

		const char * filename() {
			return m_filename; }

		size_t len() {
			return m_len; } // length of data excluding trailing null

		char* data(unsigned int off=0)	{
			assert(off<len());
			return m_buf+off; } 

		virtual bool isValid()	{
			return m_len && m_buf; }

		const char* eof()	{
			assert(m_buf!=0);
			return &m_buf[m_len]; } // ptr to the trailing null

	protected:
		char* m_buf;
		size_t m_len;
		const char* m_filename;
};
	
//---------------------------------------------------------------------------
// look up a virtual key code and return its name
class VirtualKeyDictionary
{
	typedef std::map<UINT,const char*> DICTIONARY;

	public:
		static VirtualKeyDictionary& getInstance() {
			static VirtualKeyDictionary instance;
			return instance;
		}

		const char* lookup(UINT virtualKey) {
			DICTIONARY::iterator iter = m_dictionary.find(virtualKey); 
			if ( iter != m_dictionary.end()) {
				return iter->second;
			} else {
				return "unknown key";
			}
		}


	private:
		void operator=(const VirtualKeyDictionary&) = delete;
		VirtualKeyDictionary(const VirtualKeyDictionary&) = delete;
		VirtualKeyDictionary();
		std::map<UINT,const char*> m_dictionary;
};

}; //namespace util

#endif

