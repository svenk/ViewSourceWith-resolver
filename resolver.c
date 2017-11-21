/**
 * A Firefox Quantum ViewSourceWith replacement for Windows.
 * When compiled, this behaves like an editor, i.e. you call
 * it with a file as an argument. It tries to extract then
 * the real file to open.
 *
 *
 * COMPILATION
 * ===========
 *
 * On Windows, compile with MinGW either as simple as
 *
 *   g++ resolver.c
 *
 * or sophisticatedly as
 *
 *   g++ -Os s -mwindows -static -o resolver.exe resolver.c
 *
 * where -mwindows hides the opening console window,
 *       -static   removes mingw dll dependencies (file size goes from 80K -> 1.7MB)
 *       -Os -s    decreases the size from 1.7MB -> 964K
 * use UPX and call "upx resolver.exe"  to further decrease from 900K -> 300K
 *
 * Check DLL dependencies with
 *   objdump -p resolver.exe | grep 'DLL Name'
 * In a static build, there is only kernel32.dll, user32.dll, msvcrt.dll.
 *
 *
 * INTEGRATION IN FIREFOX
 * ======================
 *
 * Edit about:config and search for "view_source.editor". Put the full path to the
 * exe there. Make soure you have a "conf.ini" in the exe directory for setting your
 * editor's path.
 *
 * Written by SvenK at 2017-11-21.
 **/

#include <stdio.h>
#include <fstream>
#include <ctime>
#include <windows.h>
#include <string.h>
#include <tchar.h>
#include <cctype>
#include <sstream>

using namespace std;

// global variables
stringstream err; // for quickly composing error messages
ofstream log; // for logging from any function
string logfilename; 
string inifile; // location of the .ini file
string inisection; // where the global ini section comes from

/// string to lowercase
std::string lower(std::string& input) {
	std::string data = input;
	for(int i=0;i<data.length();i++) data[i]=tolower(data[i]);
	//std::transform(data.begin(), data.end(), data.begin(), ::tolower);
	return data;
}

// quick string contains
bool contains(std::string haystack, std::string needle) {
	return haystack.find(needle) != std::string::npos;
}

std::string time_as_string() {
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80];

  time (&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(buffer,sizeof(buffer),"%d-%m-%Y %I:%M:%S",timeinfo);
  std::string str(buffer);
  return str;
}

// Quick and dirty
bool file_exists(const std::string& name) {
    return ifstream(name.c_str()).good();
}

std::string readIniSetting(std::string key, std::string defaultValue) {
	char result[10000];
	result[0] = 0;
	GetPrivateProfileString((char*)inisection.c_str(), (char*)key.c_str(), (char*)defaultValue.c_str(),
	    result, 10000, (char*)inifile.c_str());
	log << "Read ini setting '" << inisection << "/" << key << "' = '" << result << "'\n";
	return string(result);
}

// get path to this exe
std::string getexepath() {
  char result[ MAX_PATH ];
  return std::string( result, GetModuleFileName( NULL, result, MAX_PATH ) );
}

std::string dirnameOf(const string& fname) {
     size_t pos = fname.find_last_of("\\/");
     return (std::string::npos == pos) ? "" : fname.substr(0, pos);
}

void message(string msg){
   MessageBox( NULL, TEXT(msg.c_str()), TEXT("Resolver code"), MB_OK );
}

// convenience: Write as message(err<<"foo");
void message(ostream& msg) { message(((stringstream&)msg).str()); }

// not what you find on stackoverflow: self-written system() without console window opening
int windows_system(char *cmd) {
   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   ZeroMemory(&si, sizeof(si));
   ZeroMemory(&pi, sizeof(pi));
   si.cb = sizeof(si);
   
   if(!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
          message(err << "Create Process failed: " << GetLastError() << ". Inspect "<< logfilename << " for further information.");
   }
}

void open_in_notebook(std::string filename) {
   stringstream cmd;
   cmd << readIniSetting("notepad", "notepad.exe") << " " << filename;
   // don't write the following two lines in a single line, otherweise the
   // string storage is lost already in the next line.
   string cmds = cmd.str();
   char* cmdp = (char*)(cmds.c_str());
   log << "Starting process: '"<< cmds << "'\n";
   windows_system(cmdp);
}

int main(int argc, char** argv) {
   string exe = getexepath();
   string exedir = dirnameOf(exe);
   
   inifile = exedir + "/conf.ini"; // expect configuration here
   inisection = "resolver";
   if(!file_exists(inifile)) {
       message(err << "Expected ini file '"<< inifile <<"', not there.");
	   return 1;
   }
   
   // open file in current directory
   logfilename = exedir + "\\" + readIniSetting("log_file", "last.log");
   log.open(logfilename.c_str(), ios::trunc);
   log << "Last job log\n";
   log << "Of: " << exe << "\n";
   log << "In: " << exedir << "\n";
   log << "At: "<<time_as_string() << "\n";
   log << "Called with argc=" << argc << " arguments\n";
   for(int i=0; i<argc; i++) {
	  log << "Argument "<<i<<": '"<<argv[i]<<"'\n";
   }
   
   // try to open the given file input.
   if(argc != 2) {
	   message(err << "Usage: " << argv[0] << " <path to file>.");
	   return 1;
   }
   
   ifstream given;
   const std::string filename = argv[1];
   
   if(!file_exists(filename)) {
       message(err << "Passed filename '"<<filename<<"' does not exist");
	   return 1;
   }
   given.open(filename.c_str());
   string line;
   string meta_name = readIniSetting("meta_name", "t29.localfile");
   while(getline(given, line)) {
	   std::string lline = lower(line);
	   // poor man's HTML parsing. Assuming meta tags in a single line
	   if(contains(lline, "meta") &&
		  contains(lline, "name") &&
		  contains(lline, "content")) {
			  log << "Found meta line: " << line << "\n";
			  if(contains(lline, meta_name.c_str())) {
			     int attrstart = line.find("content") + string("content='").length();
				 int attrend = line.find_first_of("\"'", attrstart);
				 string localfile = line.substr(attrstart, attrend-attrstart);
				 log << "Found "<< meta_name << "='" << localfile << "'\n";
				 if(!file_exists(localfile)) {
					 message(err << "Could extract filename '"<< localfile << "' from meta tag '"<< meta_name<< "', but the file does not exist.");
				 }
				 open_in_notebook(localfile);
				 return 0;
			  }
			  
		  }
   }
   
   message(err << "Could not find meta tag '"<<meta_name<< "' in given input file '"<< filename << "'. Therefore, I directly open the given input file. This is probably not what you want.");
   open_in_notebook(filename);
}