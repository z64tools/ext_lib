#include <ext_lib.h>

#ifdef _WIN32
  #include "regex/xregex.h"
#else
  #include <regex.h>
#endif

char* Regex(const char* str, const char* pattern, RegexFlag flag) {
	regex_t reg;
	regmatch_t* match;
	u32 matchNum = 0;
	char* ret = NULL;
	
	if (flag & REGFLAG_MATCH_NUM)
		matchNum = (flag & REGFLAG_NUMMASK );
	
	match = malloc(sizeof(regmatch_t) * (matchNum + 1));
	
	Log("comp");
	if (regcomp(&reg, pattern, REG_EXTENDED)) {
		printf_warning("regex: compilation error");
		printf_warning("pattern: \"%s\"", pattern);
		goto done;
	}
	
	Log("exec");
	if (regexec(&reg, str, matchNum + 1, match, 0))
		goto done;
	
	if (flag & REGFLAG_START) {
		ret = (char*)str + match[matchNum].rm_so;
	} else if (flag & REGFLAG_END) {
		ret = (char*)str + match[matchNum].rm_eo;
	} else if (flag & REGFLAG_COPY) {
		ret = strndup(str + match[matchNum].rm_so, match[matchNum].rm_eo - match[matchNum].rm_so);
	}
	
done:
	regfree(&reg);
	free(match);
	
	return ret;
}
