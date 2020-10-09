#include "stdafx.h"
#include "config.h"
#include "util.h"

const char* Config::s_identifiers[Config::C_NUM_IDENTIFIERS] = {
	"BIND"
};

//---------------------------------------------------------------------------
// CONFIG::CLEAR
//---------------------------------------------------------------------------
void Config::clear()
{
	for (CONFIG::iterator iter = m_config.begin(); iter != m_config.end(); ++iter) {
		delete iter->second;
	}
	m_config.clear();
	m_lines.clear();
	delete m_file;
	m_file = 0;
}
//---------------------------------------------------------------------------
// ::CONFIG
//---------------------------------------------------------------------------
Config::Config() : m_file(0) 
{
}
//---------------------------------------------------------------------------
// ::~CONFIG
//---------------------------------------------------------------------------
Config::~Config() 
{
	clear();
}
//---------------------------------------------------------------------------
// CONFIG::ENUMCONFIG
//---------------------------------------------------------------------------
void Config::enumConfig(ENUMCONFIGCALLBACK fn, LPVOID param/*=0*/)
{
	for (CONFIG::iterator iter = m_config.begin(); iter != m_config.end(); ++iter) {
		if (0 == fn(iter->first,iter->second,param)) {
			return;
		}
	}
}
//---------------------------------------------------------------------------
// CONFIG::PRINT
//---------------------------------------------------------------------------
int Config::print(IDENTIFIER id, const VALUELIST* values, LPVOID param)
{
	Config& self = Config::getInstance();
	for (VALUELIST::const_iterator iter = values->begin(); iter != values->end(); ++iter) {
		switch (id) {
			case C_BIND: {
//				printf("%s %d %d\n", self.identifierToString(id), iter->layout.windowNumber, iter->layout.value );
				break;
			}
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
// CONFIG::LOADSCRIPT
//---------------------------------------------------------------------------
bool Config::loadScript(const char* filename) 
{
	clear();

	m_file = new util::StringFile(filename);
	if (!m_file->isValid()) {
		return false;
	}
	
	// get lines
	char* context = 0;
	char* token = strtok_s(m_file->data(),"\r\n",&context);
	while (token) {
		m_lines.push_back(token);
		token = strtok_s(NULL,"\r\n",&context);
	}

	// break out config values
	for (size_t i=0; i<m_lines.size(); ++i) {
		char* context = 0;
		char* token = strtok_s(m_lines[i]," \t",&context);
		std::vector<char*> values;
		while (token && *token!='#') {
			values.push_back(token);
			token = strtok_s(NULL," \t",&context);
		}
		if (!values.size()) {
			continue;
		}
		IDENTIFIER id = C_NUM_IDENTIFIERS;
		for (int j=0; j<C_NUM_IDENTIFIERS; ++j) {
			if (!strcmp(values[0],s_identifiers[j])) {
				id = (IDENTIFIER)j;
				break;
			}
		}
		if (id == C_NUM_IDENTIFIERS) {
			printf("config.txt: config type not recognised: %s\n", values[0] );
			return false;
		}
		switch (id) {
			case C_BIND: {
				if (values.size() != 3) {
					printf("config.txt: config \"BIND\" requires 2 fields, found %d\n", values.size()-1);
					return false;
				}
				ConfigData data;
				data.type = C_BIND;
				VALUELIST* valueList;
				CONFIG::iterator iter = m_config.find(C_BIND);
				if (iter != m_config.end()) {
					valueList = iter->second;
				} else {
					valueList = new VALUELIST;
					m_config.insert(CONFIG::value_type(C_BIND,valueList));
				}
				valueList->push_back(data);
				break;
			}
		}
	}
	return true;
}
