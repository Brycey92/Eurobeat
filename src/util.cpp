#include "eurobeat.h"
#include "util.h"

#include <cctype>
#include <chrono>

//#include <boost/algorithm/string.hpp>
//#include <boost/filesystem.hpp>
//#include <filesystem>
//#include "poco/directoryiterator.h"

using namespace std;
//using namespace boost::filesystem;
//using namespace boost::algorithm;
//using namespace Poco;

string to_lower(string str) {
	string retStr = str;
	for(unsigned int i = 0; i < str.length(); i++) {
		retStr[i] = tolower(str[i]);
	}
	return retStr;
}

string getRandFile(vector<string> audioFiles) {
	if(!audioFiles.empty()) {
		//srand(time(NULL));
		generator = default_random_engine(chrono::system_clock::now().time_since_epoch().count());
		uniform_int_distribution<int> distribution(0,audioFiles.size() - 1);
		return audioFiles[distribution(generator)];
	}
	else {
		return NULL;
	}
}

void enumerateFiles(vector<string>* audioFiles) {
	//C++17
	/*for (auto &p : filesystem::directory_iterator(CUT_MUSIC_DIR)) {
		if(p.is_regular_file()) {
			audioFiles->push_back(p.path())
			#if DEBUG
			cout << "Adding " << p.path << " to file list.\n";
			#endif
		}
	}*/
	//boost
	/*for(auto p : directory_iterator(CUT_MUSIC_DIR)) {
		if(is_regular_file(p)) {
			audioFiles->push_back(p.path());
			#if DEBUG
			cout << "Adding " << p.path().string() << " to file list.\n";
			#endif
		}
	}*/
	//poco
	/*for(auto p : DirectoryIterator(CUT_MUSIC_DIR)) {
		if(Path(p).isFile()) {
			audioFiles->push_back(p);
			#if DEBUG
			cout << "Adding " << p << " to file list.\n";
			#endif
		}
	}*/
	
	audioFiles->push_back("dejavu.ogg");
}
