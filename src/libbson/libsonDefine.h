/*
 * libsonCmd.h
 *
 *  Created on: Oct 21, 2014
 *      Author: zad
 */

#ifndef LIBSONCMD_H_
#define LIBSONCMD_H_

#define ELEMENT2PROCESS 8

#define TYPE_TEXT 1
#define TYPE_TEXT_DIR "text"

#define TYPE_AUDIO 2
#define TYPE_AUDIO_DIR "audio"

#define TYPE_VIDEO 3
#define TYPE_VIDEO_DIR "video"

#define TYPE_IMGL 4
#define TYPE_IMG_DIR "img"

#define TYPE_HTML 5
#define TYPE_HTML_DIR "html"

#define TYPE_MAX (TYPE_HTML+1)

#define HASH_FIELDS 7
#define HASH_FIELD_TYPE "type"
#define HASH_FIELD_CHECKSUM "checksum"
#define HASH_FIELD_SHA1 "sha1"
#define HASH_FIELD_PATH "path"
#define HASH_FIELD_TITLE "title"
#define HASH_FIELD_DATE "date"
#define HASH_FIELD_HEADLINE "headline"
#define HASH_FIELD_CONTENT "content"
#define HASH_FIELD_FRAGMENT "frag"
#define HASH_FIELD_PAYLOAD "payload"

typedef struct _file_signature
{
   char * signature;
   char * h_name;
} file_signature;

enum
{
   FRAGMENT_WILDCHAR = -1,
   FRAGMENT_STRIP = (FRAGMENT_WILDCHAR - 1),
   FRAGMENT_LOG = (FRAGMENT_STRIP - 1),
   FRAGMENT_VALID = FRAGMENT_LOG,
};
#endif /* LIBSONCMD_H_ */
