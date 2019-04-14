#include "Settings.h"
#include <fstream>
#include <iostream>

Settings::Settings(std::string settingsFileName) : m_settingsFileName(settingsFileName)
{
	std::ifstream inFile(m_settingsFileName);
	std::string line;
	if (inFile.good())
	{
		while (std::getline(inFile, line))
		{
			if (line.length() > 0 && line[0] != '#')
			{
				auto equalPos = line.find('=');
				if (equalPos != std::string::npos)
				{
					std::string key = line.substr(0, equalPos);
					std::string value = line.substr(equalPos + 1, line.length() - (equalPos + 1));
					std::cout << "Settings: Key=" << key << " Value=" << value << "\n";

					if (m_settings.m_rootFolder.first == key)
					{
						m_settings.m_rootFolder.second = value;
						std::cout << key << " updated to " << value << "\n";
					}

				}
			}
		}
	}
}

Settings::~Settings()
{

}
