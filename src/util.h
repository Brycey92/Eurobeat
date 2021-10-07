#ifndef UTIL
#define UTIL

#include <random>
#include <string>
#include <vector>

std::string to_lower(std::string str);

static std::default_random_engine generator;

std::string getRandFile(std::vector<std::string> audioFiles);

void enumerateFiles(std::vector<std::string>* audioFiles);

#endif