#include "../inc/logging.hpp"
#include <cstdint>
#include <cstdlib>
#include <thread>
#include <optional>
#include <iostream>
#include <iomanip>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <array>
#include "../inc/ini.h"
#define SETBIT(x, y) (x |= (1 << y))
#define CLEARBIT(x, y) (x &= ~(1 << y))
#define GETBIT(x, y) (x & (1 << y))
#define BIT_MASK (0xf000000000000000)
#define OFFSET(x) (x * 4)
#define GET_TILE(b, x) (uint8_t)(((b << OFFSET(x)) & BIT_MASK) >> OFFSET(15))
#define SET_TILE(b, x, v) ( b = (~(~b | (BIT_MASK >> OFFSET(x))) | ((BIT_MASK & ((uint64_t)v << OFFSET(15))) >> OFFSET(x)) ))

// https://stackoverflow.com/a/5100745
template< typename T >
std::string int_to_hex( T i )
{
  std::stringstream stream;
  stream << "0x" 
         << std::setfill ('0') << std::setw(sizeof(T)*2) 
         << std::hex << i;
  return stream.str();
}

// https://dev.to/aggsol/calling-shell-commands-from-c-8ej
std::string execCommand(const std::string cmd, int& out_exitStatus) {
    out_exitStatus = 0;
    auto pPipe = ::popen(cmd.c_str(), "r");
    if(pPipe == nullptr)
    {
        throw std::runtime_error("Cannot open pipe");
    }

    std::array<char, 256> buffer;

    std::string result;

    while(not std::feof(pPipe))
    {
        auto bytes = std::fread(buffer.data(), 1, buffer.size(), pPipe);
        result.append(buffer.data(), bytes);
    }

    auto rc = ::pclose(pPipe);

    if(WIFEXITED(rc))
    {
        out_exitStatus = WEXITSTATUS(rc);
    }

    return result;
}

std::optional<int> get_int(){
	int res;
	std::string input;
	std::getline(std::cin, input);
	try{
		res = std::stoi(input);
	}
	catch(std::invalid_argument){
		return {};
	}
	return res;
}
std::optional<bool> get_bool(){
	std::string input;
	std::getline(std::cin, input);
	std::transform(input.begin(), input.end(), input.begin(), toupper);
	if(input == "Y" || input == "TRUE" || input == "T" || input == "YES"){
		return true;
	}
	if(input == "N" || input == "FALSE" || input == "F" || input == "NO"){
		return false;
	}
	return {};
}

std::vector<std::uint64_t> winstate_to_vec(std::uint64_t winstate, int inp){
	std::vector<std::uint64_t> res;
	std::uint64_t tmp = winstate;
	for(int i = 0; i < 16; i++){
		if(GET_TILE(winstate, i) == 0xe){
			SET_TILE(tmp, i, inp);
			res.push_back(tmp);
			SET_TILE(tmp, i, 0);
		}
	}
	return res;
}

std::string vec_to_str(std::vector<std::uint64_t> vec){
	std::string res;
	for(std::uint64_t &b : vec){
		res += int_to_hex(b);
	}
	return res;
}

int main(){
	mINI::INIFile generated("cablegen.conf");
	mINI::INIStructure ini;
	std::uint64_t initial[4]{ // LL, DPDgap, 2x4, 3x4
		0xff12ff2120000001,
		0xfff2fff120000001,
		0xffffffff00020010,
		0xffff000100020010
	};
	std::uint64_t winstate_loc[4]{
		0xffe0ff0000000000,
		0xfffefffe00000000,
		0xffffffffe0000000,
		0xffffe00000000000
	};
	Logger logger;
	std::string cablegen;
	std::string name;
	logger.log("Welcome to cablegen config generator!", Logger::INFO);
	logger.log("First, input the location of cablegen", Logger::INFO);
	std::getline(std::cin, cablegen);
	logger.log("Testing cablegen...", Logger::INFO);
	int exit = 0;
	std::string res = execCommand(cablegen + " test", exit);
	if(exit){
		logger.log("Could not validate cablegen install!", Logger::FATAL);
		logger.log("tip: this is either because the passed location doesn't exist, or because the tests aren't passing.", Logger::INFO);
		std::exit(EXIT_FAILURE);
	}
	logger.log("What should this table be called?", Logger::INFO);
	std::getline(std::cin, name);
	ini["Generate"]["dir"] = name;
	ini["Solve"]["dir"] = name;
	logger.log("What formation? \n(1) 4422 / LL \n(2) 4411 / DPDgap \n(3) 0044 / 2x4 \n(4) 0444 / 3x4", Logger::INFO);
	int formation = 0;
	do{
		formation = get_int().value_or(0);
	}while(!formation || formation < 0 || formation > 4);
	logger.log("Should it be (1) free, (2) locked, or (3) variant?", Logger::INFO);
	int type = 0;
	do{
		type = get_int().value_or(0);
	}while(!type || type < 0 || type > 3);
	switch(type){
		case 1:
			ini["Cablegen"]["free_formation"] = "true";
			ini["Cablegen"]["ignore_f"] = "false";
			break;
		case 2:
			ini["Cablegen"]["free_formation"] = "false";
			ini["Cablegen"]["ignore_f"] = "false";
			break;
		case 3:
			ini["Cablegen"]["free_formation"] = "true";
			ini["Cablegen"]["ignore_f"] = "true";
			break;
	}
	logger.log("What should the goal tile be? (1 for 2, 2 for 4, so on)", Logger::INFO);
	int goal = 0;
	do{
		goal = get_int().value_or(0);
	}while(!goal || type < 0 || type > 14);
	
	logger.log("Finally, how many cores should be used? (enter to use max)", Logger::INFO);
	int cores = 0;
	do{
		cores = get_int().value_or(std::thread::hardware_concurrency());
	}while(!cores || cores < 0);
	ini["Cablegen"]["Cores"] = std::to_string(cores);

	logger.log("Writing initial to ./generated_initial...", Logger::INFO);
	execCommand(cablegen + " write generated_initial " + int_to_hex(initial[formation - 1]), exit);
	if(exit){
		logger.log("Failed generating initial!", Logger::FATAL);
		std::exit(EXIT_FAILURE);
	}

	logger.log("Writing winstate to ./generated_winstate...", Logger::INFO);
	execCommand(cablegen + " write generated_winstate " + vec_to_str(winstate_to_vec(winstate_loc[formation - 1], goal)), exit);
	if(exit){
		logger.log("Failed generating winstate!", Logger::FATAL);
		std::exit(EXIT_FAILURE);
	}
	logger.log("Writing config file to ./cablegen.conf...", Logger::INFO);
	ini["Generate"]["initial"] = "generated_initial";
	ini["Solve"]["winstates"] = "generated_winstate";
	auto generationres = generated.generate(ini, true);
	logger.log("Done!", Logger::INFO);

}
