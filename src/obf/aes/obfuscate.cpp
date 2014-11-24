/**
 ******************************************************************************
 *
 * @file parser.cpp
 *
 * This code is part of the String Obfuscation Project by Michael Haephrati 
 * The source code is licensed under COMMON DEVELOPMENT AND DISTRIBUTION LICENSE (CDDL)Version 1.1
 * © 2012 Michael Haephrati - haephrati@gmail.com
 *
 * @brief @~english Definitions of function for encode/decode string.
 *
 *
 * extern "C" wchar_t* __ODC__( const char * );
 ******************************************************************************
 * @n @b encoding:    US-ASCII
 * @n @b tabsize:     8 (not used)
 * @n @b indentation: 2
 */

#include <stdio.h>
#include <time.h>
#include <string>
#include <list>

#define _MAX_DRIVE 256
#define _MAX_PATH 256
#define _MAX_EXT 256
typedef std::list<std::string> strList;

extern "C" int AES_encode_w( unsigned int key_start, const wchar_t * plainString, unsigned char * outbuf, unsigned outlen );
extern "C" int AES_encode_a( unsigned int key_start, const wchar_t * plainString, unsigned char * outbuf, unsigned outlen );

#define SUBST_NAME  "extern \"C\" wchar_t* __ODC__( const char * );"
#define SUBST_NAME_A  "extern \"C\" char* __ODA__( const char * );"

/**
 * Parse solution and extract fullpath filename of project.
 */
static strList parseSolution( const char * solName  )
{
  strList result;
  static char drive[_MAX_DRIVE];
  static char somepath[_MAX_PATH];
  static char buffer[_MAX_PATH];
  static char path[_MAX_PATH];
  static char ext[_MAX_EXT];

  _splitpath( solName, drive, somepath, buffer, ext );

  FILE * f = fopen( solName, "r" );
  if( NULL == f )
  {
    printf("ERROR: Solution %s is missing or unavailable.\n", solName );
    exit(1);
  }

  while( !feof(f) )
  {
    char * res = fgets( buffer, sizeof(buffer), f );
    if( NULL == res )
      continue;

    if( NULL != strstr(buffer, "Project(") )
    {
      char * ptrName = strchr( buffer, '=' );
      char * ptrFile = strchr( ptrName, ',' );
      *ptrFile++ = 0;
      char * ptrEnd  = strchr( ptrFile, ',' );
      *ptrEnd++  = 0;
      
      while( ('=' == *ptrName) 
           ||(' ' == *ptrName)
           ||('"' == *ptrName) ) ptrName++;
      if( '"' == ptrName[strlen(ptrName)-1] )
        ptrName[strlen(ptrName)-1] = 0;

      while( (' ' == *ptrFile)
           ||('"' == *ptrFile) ) ptrFile++;
      if( '"' == ptrFile[strlen(ptrFile)-1] )
        ptrFile[strlen(ptrFile)-1] = 0;

      _makepath( path, drive, somepath, ptrFile, NULL );
      
      result.push_back( std::string(path) );
    }
  }

  fclose(f);

  return result;
}

/**
 * Parse project and extract fullpath source filename from project.
 */
static strList parseProject( const char * projName  )
{
  strList result;
  static char drive[_MAX_DRIVE];
  static char somepath[_MAX_PATH];
  static char buffer[_MAX_PATH];
  static char path[_MAX_PATH];
  static char ext[_MAX_EXT];

  _splitpath( projName, drive, somepath, buffer, ext );

  FILE * f = fopen( projName, "r" );
  if( NULL == f )
  {
    printf("ERROR: Project %s is missing or unavailable.\n", projName );
    exit(1);
  }

  while( !feof(f) )
  {
    char * res = fgets( buffer, sizeof(buffer), f );
    if( NULL == res )
      continue;

    if( (NULL != strstr(buffer, "<ClInclude Include="))
      ||(NULL != strstr(buffer, "<ClCompile Include=")) )
    {
      char * ptrName = strchr( buffer, '=' );
      char * ptrName1 = strstr( buffer, "/>" );
	  if( NULL != ptrName1 ) *ptrName1 = 0;

      while( ('=' == *ptrName) 
           ||(' ' == *ptrName)
           ||('"' == *ptrName) ) ptrName++;
      while( ('"' == ptrName[strlen(ptrName)-1])
		   ||(' ' == ptrName[strlen(ptrName)-1])
           ||('\n' == ptrName[strlen(ptrName)-1]))
        ptrName[strlen(ptrName)-1] = 0;

      _makepath( path, drive, somepath, ptrName, NULL );
      result.push_back( std::string(path) );
    }
  }

  fclose(f);

  return result;
}

/**
 * Recoding source file.
 */
static void performFile( const char *filename )
{
  /**
   * check filename.
   */
  static char oldName[_MAX_PATH];
  static char newName[_MAX_PATH];
  static wchar_t buffer[1024];
  static wchar_t copybuf[1024]=L"/*";
  static wchar_t bufout[1024];
  static unsigned char encr_str[2048];
  static char ext[_MAX_EXT];

  _splitpath( filename, NULL, NULL, NULL, ext );
  
  if( (0 != strcmp(ext,".cpp"))
    &&(0 != strcmp(ext,".c"))
    &&(0 != strcmp(ext,".h"))
    &&(0 != strcmp(ext,".hpp")) )
  {
    printf("WARNING: File '%s' can't been prepared.\n", filename);
    return;
  }

  strcpy( oldName, filename ); strcat( oldName, ".old" );
  strcpy( newName, filename ); strcat( newName, ".new" );

  FILE * fin = fopen( filename, "r, ccs=UNICODE" );
  FILE * fout = fopen( newName, "w, ccs=UNICODE" );
  if( NULL == fin )
  {
    printf("ERROR: File %s is missing or unavailable.\n", filename );
    exit(1);
  }
  if( NULL == fout )
  {
    printf("ERROR: File %s is unavailable.\n", newName );
    exit(1);
  }

  fputws( _T(SUBST_NAME) _T("\n"), fout );
  fputws( _T(SUBST_NAME_A) _T("\n"), fout );

  bool longComment = false;
  while( !feof(fin) )
  {
    if( NULL == fgetws( buffer, sizeof(buffer)/sizeof(wchar_t), fin ) )
      continue;
    /** Break newline */
    if( L'\n' == buffer[wcslen(buffer)-1] )
      buffer[wcslen(buffer)-1] = 0;

    if( 0 != wcsstr( buffer, _T(SUBST_NAME) ) )
    {
      printf("WARNING: File '%s' can't been prepared because it already obfuscate.\n", filename);
      goto ex;
    }

    if( (0 != wcsstr( buffer, _T("#include") ))
	  ||(0 != wcsstr( buffer, _T("#error") ))
	  ||(0 != wcsstr( buffer, _T("#pragma") )) )
    {
      fputws( buffer, fout );
      fputwc( L'\n', fout );
    }
    else
    {
      wchar_t * subStr = NULL;
      int inIdx = 0;
      int outIdx = 0;
      bool backSlachPrefix = false;
      bool shortComment = false;

	  memcpy( &copybuf[2], buffer, sizeof(buffer)-4);
  	  wcscat( copybuf, L"*/" );

      while( (0!=buffer[inIdx])
         &&(inIdx<sizeof(buffer)/sizeof(wchar_t)) )
      {
        if( (!backSlachPrefix)
		&&(L'\\' == buffer[inIdx]) )
        {
          backSlachPrefix = true;
          inIdx++;
          continue;
        }
        
        if( (NULL == subStr)
          &&(!backSlachPrefix)
          &&(&buffer[inIdx] == wcsstr(&buffer[inIdx], L"/*")) )
        {
          longComment = true;
          // Start long comment
          bufout[outIdx++] = buffer[inIdx++];
          bufout[outIdx++] = buffer[inIdx++];
          continue;
        }

        if( (NULL == subStr)
          &&(!backSlachPrefix)
          &&(&buffer[inIdx] == wcsstr(&buffer[inIdx], L"*/")) )
        {
          longComment = false;
          // Start long comment
          bufout[outIdx++] = buffer[inIdx++];
          bufout[outIdx++] = buffer[inIdx++];
          continue;
        }


        if( (NULL == subStr)
          &&(!backSlachPrefix)
          &&(&buffer[inIdx] == wcsstr(&buffer[inIdx], L"//")) )
        {
          shortComment = true;
          // Start long comment
          bufout[outIdx++] = buffer[inIdx++];
          bufout[outIdx++] = buffer[inIdx++];
          continue;
        }

        if( (L'"' == buffer[inIdx])
          &&(!backSlachPrefix) 
          &&(!shortComment)
          &&(!longComment) )
        {
          if( NULL != subStr )
          {
            static wchar_t bf[1024];
            bool wideChar = false;
            /** This is end of string. Perform substitution. */
            buffer[inIdx++] = 0;

			      /**
			       * Check outIdx
			       */

			      bufout[outIdx]=0;

			      if( (outIdx>3)
			        &&(NULL!=wcsstr( &bufout[outIdx-3], L"_T(")) )
			      {
				      if( L')' != buffer[inIdx] )
				      {
					      // @todo copy original and WARN
					      wcscat( bufout, L"_T(\"" );
					      wcscat( bufout, subStr );
					      wcscat( bufout, L"\"" );
                          outIdx = wcslen(bufout);
                          printf(" WARNING: Can't substitute '%ws'\n", subStr );
					      subStr = NULL;
					      continue;
				      }
				      else
				      {
					      outIdx -= 3;
					      buffer[inIdx++] = 0;
   			              bufout[outIdx]=0;
                          wideChar = true;
				      }
			      }
			      if( (outIdx>5)
			        &&(NULL!=wcsstr( &bufout[outIdx-5], L"TEXT(")) )
			      {
				      if( L')' != buffer[inIdx] )
				      {
					      // @todo copy original and WARN
					      wcscat( bufout, L"TEXT(\"" );
					      wcscat( bufout, subStr );
					      wcscat( bufout, L"\"" );
                          outIdx = wcslen(bufout);
                          printf(" WARNING: Can't substitute '%ws'\n", subStr );
					      subStr = NULL;
					      continue;
				      }
				      else
				      {
					      outIdx -= 5;
					      buffer[inIdx++] = 0;
   			              bufout[outIdx]=0;
                          wideChar = true;
				      }
			      }
			      else if( (outIdx>1)
			        &&( L'L' == bufout[outIdx-1]) )
			      {
				      outIdx--;
				      bufout[outIdx]=0;
              wideChar = true;
            }

            subStr++;
            if( wideChar )
              printf("FOUND UNICODE: '%ws'\n", subStr );
            else
              printf("FOUND ASCII: '%ws'\n", subStr );

			if( 0 != copybuf[2] )
			{
			  fputws( copybuf, fout );
              fputwc( L'\n', fout );
			  copybuf[2] = 0;
			}
            /* unescape simple */
            {
              int i=0,j=0;
              while( 0 != subStr[i] )
              {
                if( (L'\\' == subStr[i])
                  &&(0 != subStr[i+1]) )
                {
                  i++;
                  switch(subStr[i++])
                  {
                    case L'a': subStr[j++] = L'\a'; break;
                    case L'b': subStr[j++] = L'\b'; break;
                    case L'f': subStr[j++] = L'\f'; break;
                    case L'r': subStr[j++] = L'\r'; break;
                    case L't': subStr[j++] = L'\t'; break;
                    case L'v': subStr[j++] = L'\v'; break;
                    case L'\'': subStr[j++] = L'\''; break;
                    case L'\"': subStr[j++] = L'"'; break;
                    case L'\\': subStr[j++] = L'\\'; break;
                    case L'n': subStr[j++] = L'\n'; break;
                    case L'\?': subStr[j++] = L'?'; break;
                  }
                }
                else
                  subStr[j++] = subStr[i++];
              }
              subStr[j] = 0;
            }

            int i,count;
            unsigned short outChar = 'A';

            if( wideChar )
            {
              count = AES_encode_w( rand(), subStr, encr_str, sizeof(encr_str) );
              wcscat(bufout, L"__ODC__(\"" );
              outIdx = wcslen(bufout);
            }
            else
            {
              count = AES_encode_a( rand(), subStr, encr_str, sizeof(encr_str) );
              wcscat(bufout, L"__ODA__(\"" );
              outIdx = wcslen(bufout);
            }
 
            for( i = 0; i<count;i++ )
            {
              outChar += ((encr_str[i]&0xF0)>>4);
              if( outChar >= 0x7E )
                outChar -= (0x7E - 0x20);

              if( ('\"' == outChar)
                ||('\\' == outChar)
                ||('\?' == outChar) )
                bufout[outIdx++]= L'\\';
              bufout[outIdx++]= (wchar_t) outChar;
              outChar += (encr_str[i]&0x0F);
              if( outChar >= 0x7E )
                outChar -= (0x7E - 0x20);

              if( ('\"' == outChar)
                ||('\\' == outChar)
                ||('\?' == outChar) )
                bufout[outIdx++]= L'\\';
              bufout[outIdx++]= (wchar_t) outChar;
            }
            bufout[outIdx]=0;
            wcscat(bufout, L"\")" );
            outIdx = wcslen(bufout);

            subStr = NULL;
            continue;
          }
          else
          {
            /** Begin of string. */
           subStr = &buffer[inIdx];
           if( (NULL != wcsstr( buffer, L"[]" ))
              &&(wcsstr( buffer, L"[]" ) < subStr) )
            {
               printf(" WARNING: Can't substitute text into '%ws' because it initialization probably.\n", buffer );
               subStr = NULL;
               shortComment = true; // This string was copied 'as is'
            }
          }
        }


        if( NULL == subStr )
		{
		  if( backSlachPrefix )
		    bufout[outIdx++] = L'\\';
          bufout[outIdx++] = buffer[inIdx];
		}

        backSlachPrefix = false;
        inIdx++;
      } /* while along string */
    
      if( NULL != subStr )
      {
        printf("ERROR: in file '%s' string '%s' is not finished\n", filename, subStr );
        break;
      }

      bufout[outIdx] = 0;
      fputws( bufout, fout );
      fputwc( L'\n', fout );
    } /*if*/
  }

  fclose(fin); fin = NULL;
  fclose(fout); fout = NULL;

#ifndef _DEBUG
  /** Perform renaming */
  rename( filename, oldName );
  rename( newName, filename );
#endif
ex:
  if( NULL != fin ) fclose(fin);
  if( NULL != fout ) fclose(fout);
#ifndef _DEBUG
  remove(newName);
#endif
}

/**
 * argv[1] must been $(SolutionPath)
 */

void main(int argc, char* argv[])
{
  srand( (unsigned)time( NULL ) );

  strList projectList = parseSolution( argv[1] );
  strList::iterator itProj = projectList.begin();
  while( itProj != projectList.end() )
  {
    printf("Project: %s\n", itProj->c_str());
    strList fileName = parseProject( itProj->c_str() );
    strList::iterator itFile = fileName.begin();
    while( itFile != fileName.end() )
    {
      printf("  File %s\n", itFile->c_str());
      performFile( itFile->c_str() );
      itFile++;
    }
    fileName.clear();
    itProj++;
  }

  projectList.clear();
}
