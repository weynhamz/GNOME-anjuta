#ifndef __CREGEX_H__
#define __CREGEX_H__
#include <regex.h>
#include <string>
#include <vector>
#include <string.h>

class  compiled_regex
{
protected:
    regex_t regex;
    int errnum;
public:
    compiled_regex(const char *pattern,int cflags=0)
    {
        errnum=regcomp(&regex,pattern,cflags);
    }
    virtual ~compiled_regex()
    {
        if(errnum==0) regfree(&regex);
    }
}; 

class reported_regex : public compiled_regex
{
public:
    reported_regex(const char *pat,int cflags):compiled_regex(pat,cflags){};
    bool verify_regex(ostream& str) const
    {
        char buff[160];
        if(errnum != 0)
        {
            regerror(errnum,&regex,buff,160);
            str<<buff<<endl;
            return false;
        }
        return true;
    }
};

class executed_regex:public reported_regex
{
    int nsubs;
    vector<string> subs;
public:
    executed_regex(const char *pat,int cflags,int ns):reported_regex(pat,cflags)
    {
        nsubs=ns;
    }
    vector<string>::const_iterator begin() const {return subs.begin();}
    vector<string>::const_iterator end() const {return subs.end();}
    int regexec(const char *match,int flags=0)
    {
        regmatch_t * matches=NULL;
        if(nsubs > 0)
            matches=new regmatch_t[nsubs+1];
        subs.clear();
        int res= ::regexec(&regex,match,nsubs==0 ? 0 : nsubs+1,matches,flags);
        if(res==0 && matches != NULL)
        {
            string tmpstr;
            int i;
            for(i=1;i<=nsubs;i++)
            {
                tmpstr="";
                if(matches[i].rm_so >= 0 && matches[i].rm_eo <= (int)strlen(match))
                {
                    copy_n(match+matches[i].rm_so,
                           matches[i].rm_eo-matches[i].rm_so,
                           insert_iterator<string>(tmpstr,tmpstr.begin()));
                }
                subs.push_back(tmpstr);
            }
        }
        if(matches != NULL) delete matches;
        return res;
    }
};

#endif

