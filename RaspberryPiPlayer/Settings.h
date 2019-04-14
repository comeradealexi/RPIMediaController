#pragma once
#include <string>
#include <algorithm>
#include <vector>
namespace RaspberryPiPlayer
{
	using StringPair = std::pair<const std::string, std::string>;
	struct SettingContents
	{
		SettingContents()
		{
		}
		StringPair m_rootFolder = StringPair("RootFolder", "/home/pi/externalhdd/TV Shows");
	};
}

class Settings
{
public:
	Settings(std::string settingsFileName);
	~Settings();

	const std::string& GetRootFolder() const { return m_settings.m_rootFolder.second; }
	const char* GetRootFolderC() const { return m_settings.m_rootFolder.second.c_str(); }

private:
	const std::string m_settingsFileName;
	RaspberryPiPlayer::SettingContents m_settings;
};