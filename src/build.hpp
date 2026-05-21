#pragma once
#include <string>

struct BuildSettings
{
    std::string compilerPath = "g++";
    std::string compilerFlags = "-std=c++20";

#ifdef _WIN32
    std::string outputBinary = "generated_program.exe";
#else
    std::string outputBinary = "generated_program";
#endif

    std::string sourceFile = "generated.cpp";
};

struct BuildResult
{
    bool success = false;
    std::string output;
};
BuildResult BuildProgram(const BuildSettings &buildSettings);

struct RunResult
{
    bool success = false;
    std::string output;
};
RunResult RunProgram(const std::string& exeFile);
