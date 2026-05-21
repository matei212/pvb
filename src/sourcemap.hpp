#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

struct CodeGenResult
{
    std::string code;
    std::unordered_map<uint32_t, int> blockToLine;
    std::unordered_map<int, std::vector<uint32_t>> lineToBlocks;

    std::vector<uint32_t> blocksForLine(int line) const
    {
        auto it = lineToBlocks.find(line);
        if (it == lineToBlocks.end())
            return {};
        return it->second;
    }

    int lineForBlock(uint32_t blockId) const
    {
        auto it = blockToLine.find(blockId);
        if (it == blockToLine.end())
            return 0;
        return it->second;
    }
};
