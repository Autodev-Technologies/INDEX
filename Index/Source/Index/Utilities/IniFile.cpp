#include "Precompiled.h"
#include "IniFile.h"
#include "Core/OS/FileSystem.h"
#include "Core/StringUtilities.h"

#include <filesystem>
#include <fstream>

Index::IniFile::IniFile(const std::string& filePath)
    : m_FilePath(filePath)
{
    Load();
}

void Index::IniFile::Reload()
{
    RemoveAll();
    Load();
}

bool Index::IniFile::Remove(const std::string& key)
{
    if(IsKeyExisting(key))
    {
        m_Data.erase(key);
        return true;
    }

    return false;
}

void Index::IniFile::RemoveAll()
{
    m_Data.clear();
}

bool Index::IniFile::IsKeyExisting(const std::string& key) const
{
    return m_Data.find(key) != m_Data.end();
}

void Index::IniFile::RegisterPair(const std::string& key, const std::string& value)
{
    RegisterPair(std::make_pair(key, value));
}

void Index::IniFile::RegisterPair(const std::pair<std::string, std::string>& pair)
{
    m_Data.insert(pair);
}

std::vector<std::string> Index::IniFile::GetFormattedContent() const
{
    std::vector<std::string> result;

    for(const auto& [key, value] : m_Data)
        result.push_back(key + "=" + value);

    return result;
}

void Index::IniFile::Load()
{
    if(m_FilePath.empty())
        return;

    auto fileString = Index::FileSystem::ReadTextFile(m_FilePath);
    auto lines      = Index::StringUtilities::GetLines(fileString);

    for(auto& line : lines)
    {
        if(IsValidLine(line))
        {
            // line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
            RegisterPair(ExtractKeyAndValue(line));
        }
    }
}

void Index::IniFile::Rewrite() const
{
    if(m_FilePath.empty())
        return;

    std::stringstream stream;
    for(const auto& [key, value] : m_Data)
        stream << key << "=" << value << std::endl;

    FileSystem::WriteTextFile(m_FilePath, stream.str());
}

std::pair<std::string, std::string> Index::IniFile::ExtractKeyAndValue(const std::string& p_line) const
{
    std::string key;
    std::string value;

    std::string* currentBuffer = &key;

    for(auto& c : p_line)
    {
        if(c == '=')
            currentBuffer = &value;
        else
            currentBuffer->push_back(c);
    }

    return std::make_pair(key, value);
}

bool Index::IniFile::IsValidLine(const std::string& attributeLine) const
{
    if(attributeLine.size() == 0)
        return false;

    if(attributeLine[0] == '#' || attributeLine[0] == ';' || attributeLine[0] == '[')
        return false;

    if(std::count(attributeLine.begin(), attributeLine.end(), '=') != 1)
        return false;

    return true;
}

bool Index::IniFile::StringToBoolean(const std::string& value) const
{
    return (value == "1" || value == "T" || value == "t" || value == "True" || value == "true");
}
