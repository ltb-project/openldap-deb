/*
 * ppm.c for OpenLDAP
 *
 * See LICENSE, README and INSTALL files
 */

#include <stdlib.h>             // for type conversion, such as atoi...
#include <regex.h>              // for matching allowedParameters / conf file
#include <string.h>
#include <ctype.h>
#include <portable.h>
#include <slap.h>

#if defined(DEBUG)
#include <syslog.h>
#endif

#ifndef CONFIG_FILE
#define CONFIG_FILE                       "/etc/openldap/ppm.conf"
#endif

#define DEFAULT_QUALITY                   3
#define MEMORY_MARGIN                     50
#define MEM_INIT_SZ                       64
#define FILENAME_MAX_LEN                  512
#define DN_MAX_LEN                        512

#define CONF_MAX_SIZE                      50
#define PARAM_MAX_LEN                      16
#define VALUE_MAX_LEN                      60

#define PARAM_PREFIX_CLASS                "class-"
#define TOKENS_DELIMITERS                 " ,;-_£\t"


#define PASSWORD_TOO_LONG_SZ \
  "Password for dn=\"%s\" is too long (%d / %d)"
#define PASSWORD_QUALITY_SZ \
  "Password for dn=\"%s\" does not pass required number of strength checks (%d of %d)"
#define PASSWORD_CRITERIA \
  "Password for dn=\"%s\" has not reached the minimum number of characters (%d) for class %s"
#define PASSWORD_FORBIDDENCHARS \
  "Password for dn=\"%s\" contains %d forbidden characters in %s"
#define RDN_TOKEN_FOUND \
  "Password for dn=\"%s\" contains tokens from the RDN"
#define BAD_PASSWORD_SZ \
  "Bad password for dn=\"%s\" because %s"



typedef union genValue {
    int iVal;
    char sVal[VALUE_MAX_LEN];
} genValue;

typedef enum {
    typeInt,
    typeStr
} valueType;

typedef struct params {
    char param[PARAM_MAX_LEN];
    valueType iType;
} params;

// allowed parameters loaded into configuration structure
// it also contains the type of the corresponding value
params allowedParameters[5] = {
    {"^maxLength", typeInt},
    {"^minQuality", typeInt},
    {"^checkRDN", typeInt},
    {"^forbiddenChars", typeStr},
    {"^class-.*", typeStr}
};


// configuration structure, containing a parameter, a value,
// a corresponding min and minForPoint indicators if necessary
// and a type for the value (typeInt or typeStr)
typedef struct conf {
    char param[PARAM_MAX_LEN];
    valueType iType;
    genValue value;
    int min;
    int minForPoint;
} conf;


int min(char *str1, char *str2);
static void read_config_file(conf * fileConf, int *numParam, char *ppm_config_file);
int check_password(char *pPasswd, char **ppErrStr, Entry * pEntry);
void storeEntry(char *param, char *value, valueType valType, 
           char *min, char *minForPoint, conf * fileConf, int *numParam);
int typeParam(char* param);
genValue* getValue(conf *fileConf, int numParam, char* param);

void
strcpy_safe(char *dest, char *src, int length_dest)
{
    if(src == NULL)
    {
        dest[0] = '\0';
    }
    else
    {
        int length_src = strlen(src);
        int n = (length_dest < length_src) ? length_dest : length_src;
        // Copy the string — don’t copy too many bytes.
        strncpy(dest, src, n);
        // Ensure null-termination.
        dest[n] = '\0';
    }
}

genValue*
getValue(conf *fileConf, int numParam, char* param)
{
    int i = 0;

    // First scan parameters
    for (i = 0; i < numParam; i++) {
        if ((strlen(param) == strlen(fileConf[i].param))
            && (strncmp(param, fileConf[i].param, strlen(fileConf[i].param))
                == 0)) {
            return &(fileConf[i].value);
        }
    }
    return NULL;
}

void
storeEntry(char *param, char *value, valueType valType, 
           char *min, char *minForPoint, conf * fileConf, int *numParam)
{
    int i = 0;
    int iMin;
    int iMinForPoint;
    if (min == NULL || strcmp(min,"") == 0)
      iMin = 0;
    else
      iMin = atoi(min);

    if (minForPoint == NULL || strcmp(minForPoint,"") == 0)
      iMinForPoint = 0;
    else
      iMinForPoint = atoi(minForPoint);

    // First scan parameters
    for (i = 0; i < *numParam; i++) {
        if ((strlen(param) == strlen(fileConf[i].param))
            && (strncmp(param, fileConf[i].param, strlen(fileConf[i].param))
                == 0)) {
            // entry found, replace values
            if(valType == typeInt)
                fileConf[i].value.iVal = atoi(value);
            else
                strcpy_safe(fileConf[i].value.sVal, value, VALUE_MAX_LEN);
            fileConf[i].min = iMin;
            fileConf[i].minForPoint = iMinForPoint;
#if defined(DEBUG)
            if(valType == typeInt)
                syslog(LOG_NOTICE, "ppm:  Accepted replaced value: %d",
                               fileConf[i].value.iVal);
            else
                syslog(LOG_NOTICE, "ppm:  Accepted replaced value: %s",
                               fileConf[i].value.sVal);
#endif
            return;
        }
    }
    // entry not found, add values
    strcpy_safe(fileConf[*numParam].param, param, PARAM_MAX_LEN);
    fileConf[*numParam].iType = valType;
    if(valType == typeInt)
        fileConf[i].value.iVal = atoi(value);
    else
        strcpy_safe(fileConf[i].value.sVal, value, VALUE_MAX_LEN);
    fileConf[*numParam].min = iMin;
    fileConf[*numParam].minForPoint = iMinForPoint;
    ++(*numParam);
#if defined(DEBUG)
            if(valType == typeInt)
                syslog(LOG_NOTICE, "ppm:  Accepted new value: %d",
                               fileConf[*numParam].value.iVal);
            else
                syslog(LOG_NOTICE, "ppm:  Accepted new value: %s",
                               fileConf[*numParam].value.sVal);
#endif
}

int
typeParam(char* param)
{
    int i;
    int n = sizeof(allowedParameters)/sizeof(params);

    regex_t regex;
    int reti;

    for(i = 0 ; i < n ; i++ )
    {
        // Compile regular expression
        reti = regcomp(&regex, allowedParameters[i].param, 0);
        if (reti) {
#if defined(DEBUG)
            syslog(LOG_ERR, "ppm: Cannot compile regex: %s",
                   allowedParameters[i].param);
#endif
            exit(EXIT_FAILURE);
        }
        
        // Execute regular expression
        reti = regexec(&regex, param, 0, NULL, 0);
        if (!reti)
        {
            regfree(&regex);
            return i;
        } 
        regfree(&regex);
    }
    return n;
}

static void
read_config_file(conf * fileConf, int *numParam, char *ppm_config_file)
{
    FILE *config;
    char line[260] = "";
    int nParam = 0;       // position of found parameter in allowedParameters
    int sAllowedParameters = sizeof(allowedParameters)/sizeof(params);

#if defined(DEBUG)
        syslog(LOG_NOTICE, "ppm: Opening file %s", ppm_config_file);
#endif
    if ((config = fopen(ppm_config_file, "r")) == NULL) {
#if defined(DEBUG)
        syslog(LOG_ERR, "ppm: Opening file %s failed", ppm_config_file);
#endif

    }

    while (fgets(line, 256, config) != NULL) {
        char *start = line;
        char *word, *value;
        char *min, *minForPoint;;

        while (isspace(*start) && isascii(*start))
            start++;

        if (!isascii(*start))
            continue;
        if (start[0] == '#')
            continue;

        if ((word = strtok(start, " \t"))) {
            if ((value = strtok(NULL, " \t")) == NULL)
                continue;
            if (strchr(value, '\n') != NULL)
                strchr(value, '\n')[0] = '\0';
            min = strtok(NULL, " \t");
            if (min != NULL)
                if (strchr(min, '\n') != NULL)
                    strchr(min, '\n')[0] = '\0';
            minForPoint = strtok(NULL, " \t");
            if (minForPoint != NULL)
                if (strchr(minForPoint, '\n') != NULL)
                    strchr(minForPoint, '\n')[0] = '\0';


            nParam = typeParam(word); // search for param in allowedParameters
            if (nParam != sAllowedParameters) // param has been found
            {
#if defined(DEBUG)
                syslog(LOG_NOTICE,
                   "ppm: Param = %s, value = %s, min = %s, minForPoint= %s",
                   word, value, min, minForPoint);
#endif

                storeEntry(word, value, allowedParameters[nParam].iType,
                           min, minForPoint, fileConf, numParam);
            }
            else
            {
#if defined(DEBUG)
                syslog(LOG_NOTICE,
                   "ppm: Parameter '%s' rejected", word);
#endif
            }

        }
    }

    fclose(config);
}

static int
realloc_error_message(char **target, int curlen, int nextlen)
{
    if (curlen < nextlen + MEMORY_MARGIN) {
#if defined(DEBUG)
        syslog(LOG_WARNING,
               "ppm: Reallocating szErrStr from %d to %d", curlen,
               nextlen + MEMORY_MARGIN);
#endif
        ber_memfree(*target);
        curlen = nextlen + MEMORY_MARGIN;
        *target = (char *) ber_memalloc(curlen);
    }

    return curlen;
}

// Does the password contains a token from the RDN ?
int
containsRDN(char* passwd, char* DN)
{
    char lDN[DN_MAX_LEN];
    char * tmpToken;
    char * token;
    regex_t regex;
    int reti;
 
    strcpy_safe(lDN, DN, DN_MAX_LEN);
 
    // Extract the RDN from the DN
    tmpToken = strtok(lDN, ",+");
    tmpToken = strtok(tmpToken, "=");
    tmpToken = strtok(NULL, "=");
 
    // Search for each token in the password */
    token = strtok(tmpToken, TOKENS_DELIMITERS);
 
    while (token != NULL)
    {
      if (strlen(token) > 2)
      {
        // Compile regular expression
        reti = regcomp(&regex, token, REG_ICASE);
        if (reti) {
 #if defined(DEBUG)
          syslog(LOG_ERR, "ppm: Cannot compile regex: %s", token);
 #endif
          exit(EXIT_FAILURE);
        }
      }
 
      // Execute regular expression
      reti = regexec(&regex, passwd, 0, NULL, 0);
      if (!reti)
      {
        regfree(&regex);
        return 1;
      }
 
      regfree(&regex);
      token = strtok(NULL, TOKENS_DELIMITERS);
    }
 
    return 0;
}


int
check_password(char *pPasswd, char **ppErrStr, Entry * pEntry)
{
    
#if defined(DEBUG)
    syslog(LOG_NOTICE, "ppm: Entry %s", pEntry->e_nname.bv_val);
#endif

    char *szErrStr = (char *) ber_memalloc(MEM_INIT_SZ);
    int mem_len = MEM_INIT_SZ;
    int numParam = 0; // Number of params in current configuration

    int maxLength;
    int minQuality;
    int checkRDN;
    char forbiddenChars[VALUE_MAX_LEN];
    int nForbiddenChars = 0;
    int nQuality = 0;
    int nbInClass[CONF_MAX_SIZE];
    int i;
    char ppm_config_file[FILENAME_MAX_LEN];

    /* Determine config file */
    strcpy_safe(ppm_config_file, getenv("PPM_CONFIG_FILE"), FILENAME_MAX_LEN);
    if (ppm_config_file[0] == '\0') {
      strcpy_safe(ppm_config_file, CONFIG_FILE, FILENAME_MAX_LEN);
    }
#if defined(DEBUG)
    syslog(LOG_NOTICE, "ppm: reading config file from %s", ppm_config_file);
#endif

    for (i = 0; i < CONF_MAX_SIZE; i++)
        nbInClass[i] = 0;

    /* Set default values */
    conf fileConf[CONF_MAX_SIZE] = {
        {"maxLength", typeInt, {.iVal = 0}, 0, 0
         }
        ,
        {"minQuality", typeInt, {.iVal = DEFAULT_QUALITY}, 0, 0
         }
        ,
        {"checkRDN", typeInt, {.iVal = 0}, 0, 0
         }
        ,
        {"forbiddenChars", typeStr, {.sVal = ""}, 0, 0
         }
        ,
        {"class-upperCase", typeStr, {.sVal = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"}, 0, 1
         }
        ,
        {"class-lowerCase", typeStr, {.sVal = "abcdefghijklmnopqrstuvwxyz"}, 0, 1
         }
        ,
        {"class-digit", typeStr, {.sVal = "0123456789"}, 0, 1
         }
        ,
        {"class-special", typeStr,
         {.sVal = "<>,?;.:/!§ù%*µ^¨$£²&é~\"#'{([-|è`_\\ç^à@)]°=}+"}, 0, 1
         }
    };
    numParam = 8;

    /* Read config file */
    read_config_file(fileConf, &numParam, ppm_config_file);

    maxLength = getValue(fileConf, numParam, "maxLength")->iVal;
    minQuality = getValue(fileConf, numParam, "minQuality")->iVal;
    checkRDN = getValue(fileConf, numParam, "checkRDN")->iVal;
    strcpy_safe(forbiddenChars,
                getValue(fileConf, numParam, "forbiddenChars")->sVal,
                VALUE_MAX_LEN);


    /*The password must have at least minQuality strength points with one
     * point granted if the password contains at least minForPoint characters for each class
     * It must contains at least min chars of each class
     * It must not contain any char in forbiddenChar */

    if(maxLength != 0 && strlen(pPasswd) > maxLength) {
      // constraint is not satisfied... goto fail
      mem_len = realloc_error_message(&szErrStr, mem_len,
                                      strlen(PASSWORD_TOO_LONG_SZ) +
                                      strlen(pEntry->e_name.bv_val) + 
                                      2 * sizeof(maxLength));
      sprintf(szErrStr, PASSWORD_TOO_LONG_SZ, pEntry->e_name.bv_val,
              (int)strlen(pPasswd), maxLength);
      goto fail;
      
    }

    for (i = 0; i < strlen(pPasswd); i++) {

        int n;
        for (n = 0; n < numParam; n++) {
            if (strstr(fileConf[n].param, "class-") != NULL) {
                if (strchr(fileConf[n].value.sVal, pPasswd[i]) != NULL) {
                    ++(nbInClass[n]);
                }
            }
        }
        if (strchr(forbiddenChars, pPasswd[i]) != NULL) {
            nForbiddenChars++;
        }
    }

    // Password checking done, now loocking for minForPoint criteria
    for (i = 0; i < CONF_MAX_SIZE; i++) {
        if (strstr(fileConf[i].param, "class-") != NULL) {
            if ((nbInClass[i] >= fileConf[i].minForPoint)
                && strlen(fileConf[i].value.sVal) != 0) {
                // 1 point granted
                ++nQuality;
#if defined(DEBUG)
                syslog(LOG_NOTICE, "ppm: 1 point granted for class %s",
                       fileConf[i].param);
#endif
            }
        }
    }

    if (nQuality < minQuality) {
        mem_len = realloc_error_message(&szErrStr, mem_len,
                                        strlen(PASSWORD_QUALITY_SZ) +
                                        strlen(pEntry->e_name.bv_val) + 4);
        sprintf(szErrStr, PASSWORD_QUALITY_SZ, pEntry->e_name.bv_val,
                nQuality, minQuality);
        goto fail;
    }
    // Password checking done, now loocking for constraintClass criteria
    for (i = 0; i < CONF_MAX_SIZE; i++) {
        if (strstr(fileConf[i].param, "class-") != NULL) {
            if ((nbInClass[i] < fileConf[i].min) &&
                 strlen(fileConf[i].value.sVal) != 0) {
                // constraint is not satisfied... goto fail
                mem_len = realloc_error_message(&szErrStr, mem_len,
                                                strlen(PASSWORD_CRITERIA) +
                                                strlen(pEntry->e_name.bv_val) + 
                                                2 + PARAM_MAX_LEN);
                sprintf(szErrStr, PASSWORD_CRITERIA, pEntry->e_name.bv_val,
                        fileConf[i].min, fileConf[i].param);
                goto fail;
            }
        }
    }

    // Password checking done, now loocking for forbiddenChars criteria
    if (nForbiddenChars > 0) {  // at least 1 forbidden char... goto fail
        mem_len = realloc_error_message(&szErrStr, mem_len,
                                        strlen(PASSWORD_FORBIDDENCHARS) +
                                        strlen(pEntry->e_name.bv_val) + 2 +
                                        VALUE_MAX_LEN);
        sprintf(szErrStr, PASSWORD_FORBIDDENCHARS, pEntry->e_name.bv_val,
                nForbiddenChars, forbiddenChars);
        goto fail;
    }

    // Password checking done, now looking for checkRDN criteria
    if (checkRDN == 1 && containsRDN(pPasswd, pEntry->e_name.bv_val))
    // RDN check enabled and a token from RDN is found in password: goto fail
    {
        mem_len = realloc_error_message(&szErrStr, mem_len,
                                        strlen(RDN_TOKEN_FOUND) +
                                        strlen(pEntry->e_name.bv_val));
        sprintf(szErrStr, RDN_TOKEN_FOUND, pEntry->e_name.bv_val);

        goto fail;
    }

    *ppErrStr = strdup("");
    ber_memfree(szErrStr);
    return (LDAP_SUCCESS);

  fail:
    *ppErrStr = strdup(szErrStr);
    ber_memfree(szErrStr);
    return (EXIT_FAILURE);

}
