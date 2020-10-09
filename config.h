#ifndef _H_CONFIG
#define _H_CONFIG

#include "util.h"

class Config
{
	public:
		static Config& getInstance() {
			static Config instance;
			return instance;
		}
		
		typedef enum { C_BIND, C_NUM_IDENTIFIERS } IDENTIFIER; 
		static const char* s_identifiers[C_NUM_IDENTIFIERS];

//		typedef enum { C_

		struct ConfigData
		{
			IDENTIFIER type;
			union {
				struct {
					int vkKey;
					int vkBindTo;
				} bind;
			};
		};

		typedef std::vector<ConfigData> VALUELIST;
		typedef std::unordered_map<IDENTIFIER,VALUELIST*> CONFIG;

		bool loadScript(const char* filename);

		const VALUELIST* getConfig(IDENTIFIER id) {
			CONFIG::iterator iter = m_config.find(id);
			return iter == m_config.end() ? 0 : iter->second;
		}

		typedef int (*ENUMCONFIGCALLBACK)(IDENTIFIER id, const VALUELIST* values, LPVOID param);
		void enumConfig(ENUMCONFIGCALLBACK fn,LPVOID param=0);

		void printConfig() {
			enumConfig(print);
		}

		const char* identifierToString(IDENTIFIER id) {
			if (id>=0 && id<C_NUM_IDENTIFIERS) {
				return s_identifiers[id];
			} else {
				return "null";
			}
		}

	private:
		void operator=(const Config&) = delete;
		Config(const Config&) = delete;

		Config();
		~Config();

		void clear();

		static int print(IDENTIFIER id, const VALUELIST* values, LPVOID param);
		util::StringFile* m_file;
		std::vector<char*> m_lines;
		CONFIG m_config;
};


#endif
