/*
 * =====================================================================================
 *
 *       Filename:  encript.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/11/2014 12:28:53
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zad (), wtfrtfmdiy@gmail.com
 *   Organization:  ht
 *
 * =====================================================================================
 */

#include "common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>     // std::cout
#include <fstream>      // std::fstream
#include <string>
#include <sstream> 
#include <iomanip>
using namespace std;
std::string ToHex(const std::string& s, bool upper_case)
{
  ostringstream ret;
  for (std::string::size_type i = 0; i < s.length(); ++i)
  {
    int z = s[i]&0xff;
    ret << std::hex << std::setfill('0') << std::setw(2) << (upper_case ? std::uppercase : std::nouppercase) << z;
  }
  return ret.str();
}
string HexToStr(char* s)
{
  ostringstream ret;
  if(s!=NULL)
  {

    int size = strlen(s);
    char z[3];
    z[2]='\0';
    for (string::size_type i = 0; i < size;)
    {
      std::stringstream ss;
      unsigned int n=0;
      z[0]= s[i];
      z[1]= s[i+1];
      ss << std::hex << z;
      ss >> n;
      ret << (char) n;
      i+=2;
    }
  }
  return ret.str();
}

void test(char *key,char* text)
{
  rc4_ks_t keyrc4;
  char *buffer = new char [(size_t) strlen((char *)key)];
  char output1[strlen(text)];
  memset(buffer,0,sizeof(buffer));
  memset(output1,0,sizeof(output1));
  rc4_setks((uint8_t*)key,(size_t) strlen((char *)key), &keyrc4);
  rc4_crypt((uint8_t*) output1,(size_t) strlen((char *)text), &keyrc4);
  strcpy( output1,text);
  rc4_crypt((uint8_t*)output1,(size_t) sizeof(output1), &keyrc4);
  std::string tohexed = ToHex(std::string(output1, strlen(text)), true);
  //printf("[%d]key=\"%s\"  text=\"%s\"\n",strlen(key),key,text);
  printf("%s\n",tohexed.c_str());

  memset(buffer,0,sizeof(buffer));
  rc4_setks((uint8_t*)key, strlen((char *)key), &keyrc4);
  rc4_crypt((uint8_t*)buffer,(size_t) strlen((char *)key), &keyrc4);
  std::string dehexed = HexToStr((char*)tohexed.c_str());
  memcpy(output1,dehexed.c_str(),(size_t) sizeof(output1));
  //printf("[%d]",sizeof(output1));
  rc4_crypt((uint8_t*)output1,(size_t) sizeof(output1), &keyrc4);
  //printf("%s\n",output1);
}

void usage(char* prg){
  printf("%s\n",prg);
  printf("commands available:\n");
  printf("\ttest key plaitext\n");
  printf("\tcrypt infile outfile key <skip>\n");
}
int crypt(char *dst,char *src, char *key,int skip)
 {
  rc4_ks_t keyrc4;
  std::ifstream iin (src, std::ifstream::binary);
  std::ofstream iout (dst, std::ifstream::binary);
  printf("crypt key=%s [%d]",key,(int)strlen((char *)key));
  if (iin) {
    char * buffer = new char [CHUNK_SIZE];
    int length=0;
    memset(buffer,0,sizeof(buffer));
   
    rc4_setks((uint8_t*)key, strlen((char *)key), &keyrc4);
    rc4_crypt((uint8_t*)buffer, CHUNK_SIZE, &keyrc4);
    if(skip){
         iin.read (buffer,skip);
         length = (int)iin.gcount() ;
         iout.write(buffer,length);
       }
    do{
      memset(buffer,0,sizeof(buffer));
      iin.read (buffer,CHUNK_SIZE);
      length = (int)iin.gcount() ;
      rc4_crypt((uint8_t*)buffer, length, &keyrc4);
      iout.write(buffer,length);
    }while(!iin.eof());

    iin.close();
    iout.close();

    delete[] buffer;
  }
  return 0;
}
int main(int argc, char **argv){
  int print_help=0;
  if(argc<2){
    print_help=1;
  }else{
    if(strncmp(argv[1],"test",4)==0){
      if(argc==4){
        test(argv[2],argv[3]);
      }else{
        print_help=1;
      }
    }else if(strncmp(argv[1],"crypt",5)==0){
      if(argc>4){
        if(argc==6){
          crypt(argv[3],argv[2],argv[4],atoi(argv[5]));
        }else{
          crypt(argv[3],argv[2],argv[4],0);
        }
      }else{
        print_help=1;
      }
    }else{
      printf("unsupported command %s\n",argv[1]);
    }
  }
  if(print_help){
    usage(argv[0]);
  }
}

void rc4_setks(const uint8_t *buf, size_t len, rc4_ks_t *ks)
{
  uint8_t j = 0;
  uint8_t *state = ks->state;
  int i;

  for (i = 0;  i < 256; ++i)
    state[i] = i;

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
  unsigned int  i;

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

