#include <array>
#include <cstdio>
#include <memory>

#include "build.hpp"
#include "log.hpp"

BuildResult BuildProgram(const BuildSettings &buildSettings)
{
    BuildResult result;

    std::string outputBinary = buildSettings.outputBinary;
#ifdef _WIN32
if (buildSettings.outputBinary.find(".exe") == std::string::npos)
    outputBinary += ".exe";
#endif

    std::string cmd = buildSettings.compilerPath + " " +
	buildSettings.sourceFile + " " +
	buildSettings.compilerFlags + " " +
	"-o " + outputBinary +
	" 2>&1";

    result.output += cmd + '\n';
    std::array<char, 512> buffer;

#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif

    if (!pipe) {
        result.output = "Failed to start compiler";
        return result;
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
        result.output += buffer.data();
    }

#ifdef _WIN32
    int code = _pclose(pipe);
#else
    int code = pclose(pipe);
#endif

    result.success = (code == 0);
    if (result.success) {
	result.output += "Built successful";
    } else {
	result.output += "Built failed";
    }

    return result;
}

#ifdef _WIN32
RunResult RunProgram(const std::string& exeFile)
{
    RunResult result;

    std::string exe = exeFile;
    if (exe.find(".exe") == std::string::npos)
        exe += ".exe";

    // Launch new cmd terminal and wait for key press
    std::string cmd = "start cmd /C \"" + exe + " & echo. & echo Press any key to exit... & pause\"";

    int ret = std::system(cmd.c_str());
    result.success = (ret == 0);
    result.output = result.success ? "Program launched successfully." : "Failed to launch program in terminal.";
    return result;
}
#else

bool CommandExists(const std::string& cmd)
{
    std::string check = "command -v " + cmd + " >/dev/null 2>&1";
    return (std::system(check.c_str()) == 0);
}

RunResult RunProgram(const std::string& exeFile)
{
    RunResult result;

    const char* terminals[] = { "x-terminal-emulator", "xterm", "gnome-terminal", "konsole", "lxterminal", "mate-terminal", "kitty", "alacritty", "foot" };
    std::string chosenTerminal;

    // Find the first available terminal
    for (auto term : terminals) {
        if (CommandExists(term)) {
            chosenTerminal = term;
            break;
        }
    }

    if (chosenTerminal.empty()) {
        result.success = false;
        result.output = "No terminal emulator found (tried xterm, gnome-terminal, konsole, etc.)";
        return result;
    }

    std::string cmd = chosenTerminal + " -e sh -c './" + exeFile + "; echo; echo Press any key to exit...; read -n 1'";
    int ret = std::system(cmd.c_str());

    result.success = (ret == 0);
    result.output = result.success ? "Program launched successfully." : "Failed to launch program in terminal.";

    return result;
}
#endif
