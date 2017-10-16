#include <iostream>
#include <fstream>
#include <mutex>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <string>
#include <stdio.h>
#include <cstring>
#include <functional>
#include <algorithm>
#include <map>
#include <atomic>
#include <sys/file.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstdarg>
#include <vector>
#include "HtmlHeader.h"

typedef std::map<int, std::string> LookupList;

/*

Project Plan:

SENDING KEYBOARD COMMANDS TO OMXPLAYER:

mkfifo OMXPLAYERFIFO //Call this on Start

Write To OMXPLAYERFIFO / FOPEN IT TO START OMXPLAYER PLAYING. (echo -n p > OMXPLAYERFIFO) for example...

CLOSE FIFO handle.

Upon program exit, delete the OMXPLAYERFIFO??

//WEB SERVER?
There can be a PHP script which fopens the fifo and writes to it directly. Remember to close it though!

//The acutal OMXPLayerFifo program will just do a straight system(omxplayer) call.

(In the event of a quit of skip command being sent, a key stroke of 'q' should be written to the pipe first).

//For communicating with the actual oxmplayer program, it can be done by either opening the stdin file and writing to it directly, or with sockets. It'll probably be easier to just write to the input file.

*/

//int globalVariable = 0;

/*
TO MOUNT HDD:

run: sudo mount /dev/sda1 /mnt

then it'll be in the /mnt dir

Dir I actually want: "/mnt/TV Shows/" and "/mnt/Films/"

*/

//File System Dir Thingy
class LogClass
{
private:
	std::ofstream m_LogFile;
	std::recursive_mutex m_LogMutex;

public:
	//Allows writing to ofstream as well as cout!
	template <typename T>
	std::ostream& operator<<(const T& TData)
	{
		std::unique_lock<std::recursive_mutex> lk(m_LogMutex);
		m_LogFile << TData;
		std::cout << TData;
		return m_LogFile;
	}

	void Printf(const char* __fmt, ...)
	{
		std::unique_lock<std::recursive_mutex> lk(m_LogMutex);

		static char cLogBuffer[1024 * 1024];
		if (m_LogFile.good())
		{
			va_list argptr;
			va_start(argptr, __fmt);
			vsprintf(cLogBuffer, __fmt, argptr);
			va_end(argptr);

			m_LogFile << cLogBuffer;
			std::cout << cLogBuffer;
		}
	}

	void OpenLog()
	{
		#ifdef DEBUG
			#define BUILD "DEBUG"
		#else
			#define BUILD "RELEASE"
		#endif

		//Use epoc time as a file log stamp
		std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
		std::string strName = "Log_" BUILD "_";
		strName += std::to_string(ms.count());
		strName += ".txt";

		std::unique_lock<std::recursive_mutex> lk(m_LogMutex);
		m_LogFile.open(strName.c_str());
	}

	void CloseLog()
	{
		*this << "Exiting Program With Normal Termination\n";

		std::unique_lock<std::recursive_mutex> lk(m_LogMutex);
		if (m_LogFile.good())
			m_LogFile.close();
	}
} g_LogClass;

template<typename T1, typename T2>
struct RAII
{
	RAII(T1 t1, T2 t2) : m_t2(t2)
	{
		t1();
	}
	~RAII()
	{
		m_t2();
	}

private:
	T2 m_t2;
};
typedef RAII<std::function<void()>, std::function<void()>> RAII_VOID;


enum class PlayState : uint32_t
{
	IDLE,
	PLAYING,
};

enum class OMXCommandList : uint32_t
{
	STOP,
	PAUSE,
	VOLUME_UP,
	VOLUME_DOWN,
	MUTE
};

/*function... might want it in some class?*/
int getdir(std::string dir, std::vector<std::string> &files)
{
	DIR *dp;
	struct dirent *dirp;
	if ((dp = opendir(dir.c_str())) == NULL) {
		g_LogClass << "Error(" << errno << ") opening " << dir << std::endl;
		return errno;
	}

	while ((dirp = readdir(dp)) != NULL) {
		files.push_back(std::string(dirp->d_name));
	}
	closedir(dp);
	return 0;
}

bool ShouldIgnoreDir(const char* psz)
{
	if (psz[0] == '.' && (psz[1] == '\0' || (psz[1] == '.' && psz[2] == '\0')))
		return true;
	return false;
}

//Lists valid file extensions to use for omxplayer.
bool ShouldIgnoreReg(const char* psz)
{
	int len = strlen(psz);

	if (len > 4)
	{
		len--;
		if (tolower(psz[len]) == '4' && tolower(psz[len - 1]) == 'p' && tolower(psz[len - 2]) == 'm' && tolower(psz[len - 3]) == '.') //mp4
			return false;
		if (tolower(psz[len]) == 'i' && tolower(psz[len - 1]) == 'v' && tolower(psz[len - 2]) == 'a' && tolower(psz[len - 3]) == '.') //avi
			return false;
		if (tolower(psz[len]) == 'v' && tolower(psz[len - 1]) == 'k' && tolower(psz[len - 2]) == 'm' && tolower(psz[len - 3]) == '.') //mkv
			return false;
		if (tolower(psz[len]) == 'v' && tolower(psz[len - 1]) == '4' && tolower(psz[len - 2]) == 'm' && tolower(psz[len - 3]) == '.') //m4v
			return false;
	}

	return true;
}

void RecursivelyBuildListHtml(char* pszPath, FILE* pOutputFile, const char* pPrevName, int iDepth, int& ID, LookupList& lookupList)
{
	char tmpPath[1024];
	DIR* dp = opendir(pszPath);
	dirent* dirp;

	if (dp == nullptr)
		return;

	while ((dirp = readdir(dp)) != NULL) 
	{
		if (dirp->d_type == DT_DIR || dirp->d_type == DT_REG)
		{
			if (dirp->d_type == DT_DIR && ShouldIgnoreDir(dirp->d_name))
				continue;

			if (dirp->d_type == DT_REG && ShouldIgnoreReg(dirp->d_name))
				continue;

			snprintf(tmpPath, sizeof(tmpPath), "%s/%s", pszPath, dirp->d_name);

			const char* displayType = iDepth >= 0 ? "display: none;" : "display: ;";
			const char* szName = dirp->d_type == DT_REG ? "name=\"file\"" : "";

			if (dirp->d_type == DT_REG) 
				lookupList[ID] = tmpPath;

			fprintf(pOutputFile, "<li>" "<img style=\"width:15px; height:15px; background - color: lightblue;\" src=\"down.png\" onclick=\"dropdownfunction(this)\"></img>" "<label><input %s onclick=\"UpdateMarkings(this)\" type=\"checkbox\" id=\"%s\" class=\"checkboxClass\">%s</label>\n", szName, std::to_string(ID++).c_str(), dirp->d_name);
			if (dirp->d_type == DT_DIR)
			{
				fprintf(pOutputFile, "<ul style=\"%s\">\n", displayType);
				RecursivelyBuildListHtml(tmpPath, pOutputFile, dirp->d_name, iDepth + 1, ID, lookupList);
				fprintf(pOutputFile, "</ul>\n");
			}
			fprintf(pOutputFile, "</li>\n");
		}
	}
}

void SaveMapToFile(LookupList& lookupList)
{
	typedef std::pair<int, std::string> MapEntry;
	std::vector<MapEntry> sortedList;

	for (const auto& e : lookupList)
		sortedList.push_back(e);

	std::sort(sortedList.begin(), sortedList.end(), [](const MapEntry& m1, const MapEntry& m2) -> bool { return m1.first < m2.first; });

	std::ofstream m_LogFile;
	m_LogFile.open("mapEntriesList.txt");
	if (m_LogFile.good())
	{
		for (const auto& e : sortedList)
			m_LogFile << e.first << " : " << e.second << std::endl;
		m_LogFile.close();
	}
}

void GenerateHtml(LookupList& lookupList)
{
	FILE* pOutputFile = fopen("index.html", "wb");
	if (pOutputFile)
	{

		fprintf(pOutputFile, k_szHeader);

		int iID = 0;
		lookupList[iID] = "/mnt/TV Shows";

		fprintf(pOutputFile, "<ul>\n");
		fprintf(pOutputFile, "<li>" "<img style=\"width:15px; height:15px; background - color: lightblue;\" src=\"right.png\" onclick=\"dropdownfunction(this)\"></img>" "<label><input onclick=\"UpdateMarkings(this)\" type=\"checkbox\" id=\"%s\" class=\"checkboxClass\">%s</label>\n", std::to_string(iID++).c_str(), "TV Shows");
		fprintf(pOutputFile, "<ul>\n");
		RecursivelyBuildListHtml("/mnt/TV Shows", pOutputFile, "TV Shows", 0, iID, lookupList);
		fprintf(pOutputFile, "</ul>\n");
		fprintf(pOutputFile, "</li>\n");
		fprintf(pOutputFile, "</ul>\n");

		fprintf(pOutputFile,k_szFooter);

		fclose(pOutputFile);

		SaveMapToFile(lookupList);
	}
}

const char* g_kHDDRootDir = "";
const char* g_kPathToPlay = "";
const char* g_kFIFOName = "AH_OMXPlayerFIFO";

class PlayerClass
{
public:
	PlayerClass() : 
		m_bKillProgram(false), 
		m_bInputAvailable(false), 
		m_omxDataAvailable(false)
	{
		std::srand(std::time(0));
		EnsureMounted();
		GenerateHtml(m_lookupList);
		g_LogClass << "m_lookupList size: " << m_lookupList.size() << std::endl;
		m_pOMXPlayThread = new std::thread(&PlayerClass::OmxThread, this);
		m_pInputThread = new std::thread(&PlayerClass::InputThread, this);
		BroadcastCurrentVideo("");
	}
	~PlayerClass()
	{
		m_bKillProgram = true;

		if (m_pOMXPlayThread)
		{
			m_pOMXPlayThread->join();
			delete m_pOMXPlayThread;
		}
		
		if (m_pInputThread)
		{
			m_pInputThread->join();
			delete m_pInputThread;
		}
	}
	void EnsureMounted();
	void IssueOMXCommand(OMXCommandList cmd);
	void InputThread();
	void BroadcastCurrentVideo(std::string str);
	void ExtractPlayData(std::string& str);
	void OMXPlay();
	bool PlayNewVideo();
	void OmxThread();

	void ProcessNewVideoRequest();
	void UpdateIdle();
	void UpdatePlaying();
	void Run();
private:
	PlayState			m_eState = PlayState::IDLE;
	LookupList			m_lookupList;
	std::vector<int>	m_playList;

private:
	std::thread*		m_pOMXPlayThread = nullptr;
	FILE*				m_pOMXInputFile = nullptr;
	std::atomic_bool	m_omxDataAvailable;
	std::string			m_strActiveVideo;

private:
	std::thread*		m_pInputThread = nullptr;
	std::atomic_bool	m_bKillProgram;
	std::atomic_bool	m_bInputAvailable;
	std::string			m_strInput;
};

void PlayerClass::EnsureMounted()
{
	auto pFile = opendir("/mnt");
	if (pFile)
		closedir(pFile);
	else 
		system("mount /dev/sda1 /mnt");
}

void PlayerClass::IssueOMXCommand(OMXCommandList cmd)
{
	g_LogClass.Printf("Issuing OMX Command? m_pOMXPlayThread: %x m_omxDataAvailable: %s m_pOMXInputFile: %x\n", m_pOMXPlayThread, m_omxDataAvailable ? "Y" : "N", m_pOMXInputFile);
	if (m_pOMXPlayThread && m_omxDataAvailable && m_pOMXInputFile)
	{
		g_LogClass << "Writing To File \n";
		switch (cmd)
		{
		case OMXCommandList::STOP:
			g_LogClass << "Stopping\n";
			fwrite("q", 1, 1, m_pOMXInputFile);
			break;
		case OMXCommandList::PAUSE:
			g_LogClass << "pausing\n";
			fwrite("p", 1, 1, m_pOMXInputFile);
			break;
		case OMXCommandList::VOLUME_UP:
			g_LogClass << "vol up\n";
			fwrite("=", 1, 1, m_pOMXInputFile);
			break;
		case OMXCommandList::VOLUME_DOWN:
			g_LogClass << "vol down\n";
			fwrite("-", 1, 1, m_pOMXInputFile);
			break;
		case OMXCommandList::MUTE:
			break;
		default:
			break;
		}
		fflush(m_pOMXInputFile);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	g_LogClass << "Finished Issue OMX Command\n";
}

void PlayerClass::ExtractPlayData(std::string& str)
{
	char cbuffer[1024 * 1024];
	strcpy(cbuffer, str.c_str());

	char* szPtr = cbuffer;
	g_LogClass << "Extracting Play Data: ";
	int iQuoteCount = 0;
	const char* szPtrPrev = nullptr;
	while (*szPtr != '\0')
	{
		if (*szPtr == '"')
		{
			iQuoteCount++;

			if (iQuoteCount == 1)
			{
				szPtrPrev = szPtr + 1;
			}
			if (iQuoteCount == 2)
			{
				char curChar = *szPtr;
				*szPtr = '\0';
				g_LogClass.Printf("%s, ", szPtrPrev);
				//printf("%i\n", stoi(szPtrPrev));
				int iVal = std::stoi(szPtrPrev);
				m_playList.push_back(iVal);
				*szPtr = curChar;
				iQuoteCount = 0;
			}
		}

		szPtr++;
	}
	g_LogClass << "\n";
}

bool PlayerClass::PlayNewVideo()
{
	if (m_playList.size() > 0)
	{
		int iRandomNumber = std::rand() % m_playList.size();
		const std::string& fileToPlay = m_lookupList[m_playList[iRandomNumber]];
		m_playList.erase(m_playList.begin() + iRandomNumber);
		g_LogClass << "The video to be played is: " << fileToPlay << "ID: " << m_playList[iRandomNumber] << std::endl;
		m_strActiveVideo = fileToPlay;
		BroadcastCurrentVideo(fileToPlay);
		m_omxDataAvailable = true;
		m_pOMXInputFile = fopen("AH_OMXPlayerFIFO", "w");
		m_eState = PlayState::PLAYING;
		return true;
	}
	BroadcastCurrentVideo("");
	m_eState = PlayState::IDLE;
	return false;
}

void PlayerClass::InputThread()
{
	system("mkfifo thefifofile");

	while (m_bKillProgram == false)
	{
		g_LogClass << "Input Thread Waiting for Input Pid is: " << std::to_string(getpid()) << std::endl;

		FILE* pFile = fopen("thefifofile", "r");
		if (pFile)
		{
			char cbuffer[1024 * 1024];
			fgets(cbuffer, sizeof(cbuffer), pFile);
			m_strInput = cbuffer;
			g_LogClass << "Received Input of: \"" << m_strInput << "\"" << std::endl;

			m_bInputAvailable = true;
			fclose(pFile);
		}
		else
			g_LogClass << "did not open file" << std::endl;

		while (m_bInputAvailable == true)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void PlayerClass::BroadcastCurrentVideo(std::string str)
{
	FILE* pFile = fopen("currentvideo.txt", "wb");
	if (pFile)
	{
		if (str.length() < 2)
		{
			fprintf(pFile, '\0');
		}
		else //Gets just the file name, reads from the last slash
		{
			const char* szLastShash = str.c_str();
			const char* pPtr = str.c_str();

			while (*pPtr)
			{
				if (*pPtr == '/' || *pPtr == '\\')
					szLastShash = pPtr;
				pPtr++;
			}

			szLastShash++;
			g_LogClass << "Broadcasting To File: " << szLastShash;
			fprintf(pFile, szLastShash);
		}

		fclose(pFile);
	}
}

void PlayerClass::OmxThread()
{
	while (m_bKillProgram == false)
	{
		if (m_omxDataAvailable)
		{
			char cbuffer[1024];
			sprintf(cbuffer, "omxplayer \"%s\" -o hdmi -b < AH_OMXPlayerFIFO", m_strActiveVideo.c_str());
			g_LogClass << "OmxThread: Playing video with the following command: " << cbuffer << std::endl;
			system(cbuffer);
			m_omxDataAvailable = false;
			m_pOMXInputFile = nullptr;
			g_LogClass.Printf("Finished OMX Player Playback\n");
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

void PlayerClass::ProcessNewVideoRequest()
{
	m_playList.clear();
	g_LogClass << "STATE IDLE: Received Play Request" << std::endl;
	IssueOMXCommand(OMXCommandList::STOP);
	ExtractPlayData(m_strInput);
	PlayNewVideo();
}

void PlayerClass::UpdateIdle()
{
	if (m_bInputAvailable)
	{
		//We are idle and we've got a new play request.
		if (m_strInput[0] == 'p')
		{
			PlayerClass::ProcessNewVideoRequest();
		}
		m_bInputAvailable = false; 
	}
}

void PlayerClass::UpdatePlaying()
{
	//Video has finished playing.
	if (m_omxDataAvailable == false)
	{
		PlayNewVideo();
	}

	if (m_bInputAvailable)
	{
		g_LogClass << "STATE PLAYING: Received Input" << std::endl;
		if (m_strInput[0] == 'p')
		{
			PlayerClass::ProcessNewVideoRequest();
		}
		else if (m_strInput[0] == 'q')
		{
			IssueOMXCommand(OMXCommandList::STOP);
			m_playList.clear();
		}
		else if (m_strInput[0] == 'n')
		{
			IssueOMXCommand(OMXCommandList::STOP);
		}
		else if (m_strInput[0] == 'P')
		{
			IssueOMXCommand(OMXCommandList::PAUSE);
		}
		else if (m_strInput[0] == '=')
		{
			IssueOMXCommand(OMXCommandList::VOLUME_UP);
		}
		else if (m_strInput[0] == '-')
		{
			IssueOMXCommand(OMXCommandList::VOLUME_DOWN);
		}
		m_bInputAvailable = false;
	}
}

void PlayerClass::Run()
{
	while (m_bKillProgram == false)
	{
		switch (m_eState)
		{
		case PlayState::IDLE:
			UpdateIdle();
			break;
		case PlayState::PLAYING:
			UpdatePlaying();
			break;
		default:
			m_bInputAvailable = false;
			break;
		}
	}
}

void PublishPID()
{
	FILE* pFile = fopen("mypid.txt", "wb");
	if (pFile)
	{
		fprintf(pFile, std::to_string(getpid()).c_str());
		fclose(pFile);
	}
}

int main()
{
	g_LogClass.OpenLog();
	g_LogClass << "Starting Program...\n";
	PublishPID();
	PlayerClass plyr;
	plyr.Run();
	g_LogClass.CloseLog();
	return 0;
}

#if 0



int main()
{
	RAII_VOID fifoCreate
	(
		[]() { system("mkfifo AH_OMXPlayerFIFO"); }, 
		[]() { /*system("remove AH_OMXPlayerFIFO");*/ }
	);

	std::thread playVideo([]() {system("omxplayer /opt/vc/src/hello_pi/hello_video/test.h264 -o hdmi < AH_OMXPlayerFIFO"); });

	FILE* pFIFO = fopen("AH_OMXPlayerFIFO", "w");
	//fwrite(" ", 1, 1, pFIFO);
	//fflush(pFIFO);

	std::this_thread::sleep_for(std::chrono::seconds(6));
	//rewind(pFIFO);
	//rewind(pFIFO);
	fwrite("p", 1, 1, pFIFO);
	fflush(pFIFO);

	std::this_thread::sleep_for(std::chrono::seconds(2));
	//rewind(pFIFO);
	//rewind(pFIFO);
	fwrite("p", 1, 1, pFIFO);
	fflush(pFIFO);

// 	fwrite(".", 1, 1, pFIFO);
// 	fflush(pFIFO);

	//std::this_thread::sleep_for(std::chrono::seconds(1));
	//fwrite("p", 2, 1, pFIFO);
	playVideo.join();

	return 0;

 	Log() << "Hello World!\n";
	//int iRet = 
	//return 0;
	

	//int pforkid = fork();
	//std::cout << "fork ID: " << pforkid << std::endl;
	//if (pforkid == 0)                // child
	//{
		//globalVariable = 1;
		std::thread strThread([]() {
			Log() << "Starting Video!\n";
			int iret = system("omxplayer /opt/vc/src/hello_pi/hello_video/test.h264 -o hdmi --no-keys");
			Log() << "VIDEO IS FINISHED" << std::endl;
		});
		
		//std::this_thread::sleep_for(std::chrono::seconds(1));
		//globalVariable = 0;
		//return 0;
	//}
	//else if (pforkid < 0)            // failed to fork
	//{
	//	// Throw exception
	//	std::cout << "FAILED TO FORK " << pforkid << std::endl;
	//}
	//else                                   // parent
	//{
	//	std::cout << "Waiting For Video!\n";
	//	// Code only executed by parent process
	//	while (globalVariable == 1)
	//	{
	//		std::this_thread::sleep_for(std::chrono::seconds(1));
	//		std::cout << "Video Is Running...\n";
	//	}
	//}

	int oxmplayerpid = -1;
	std::string strCMD = "ps";
	std::string result = "";
	char buffer[1024];
	FILE* pipe = popen(strCMD.c_str(), "r");
	if (pipe)
	{
		while (!feof(pipe))
			if (fgets(buffer, 256, pipe) != nullptr)
			{
				if (strstr(buffer, "omxplayer") != nullptr)
				{
					char * pch;
					char cBufferCopy[1024];
					strcpy(cBufferCopy, buffer);
					char* pCbcpy = cBufferCopy;
					while (*pCbcpy != 0)
						if (*pCbcpy == ' ')
						{
							*pCbcpy = 0; break;
						}
						else pCbcpy++;
						
					oxmplayerpid = std::stoi(cBufferCopy);
				}

				result += buffer;
			}
		pclose(pipe);
	}
//	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::string strCmdKill = "sudo kill " + std::to_string(oxmplayerpid);
	int iKillVideo = system(strCmdKill.c_str());
	Log() << "Ran Command: " << strCmdKill << std::endl;
	Log() << "Command Is Running, process ID is:" << getpid() << std::endl;
	Log() << "Active Jobs:" << std::endl << result << std::endl;
 	
	strThread.join();
 	return 0;
 }
 //#include <iostream>
 //#include <string>
 //
 //// Required by for routine
 //#include <sys/types.h>
 //#include <unistd.h>
 //
 //#include <stdlib.h>   // Declaration for exit()
 //
 //using namespace std;
 //
 //int globalVariable = 2;
 //
 //main()
 //{
 //	std::cout << "START Current PID:" << getpid() << std::endl;
 //
 //	string sIdentifier;
 //	int    iStackVariable = 20;
 //
 //	pid_t pID = fork();
 //	std::cout << "Fork PID is: " << pID << std::endl;
 //	std::cout << "Current PID:" << getpid() << std::endl;
 //	if (pID == 0)                // child
 //	{
 //		// Code only executed by child process
 //
 //		sIdentifier = "Child Process: ";
 //		globalVariable++;
 //		iStackVariable++;
 //	}
 //	else if (pID < 0)            // failed to fork
 //	{
 //		cerr << "Failed to fork" << endl;
 //		exit(1);
 //		// Throw exception
 //	}
 //	else                                   // parent
 //	{
 //		// Code only executed by parent process
 //
 //		sIdentifier = "Parent Process:";
 //	}
 //
 //	// Code executed by both parent and child.
 //
 //	cout << sIdentifier;
 //	cout << " Global variable: " << globalVariable;
 //	cout << " Stack variable: " << iStackVariable << endl;
 //}
#endif