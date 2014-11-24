#!/usr/bin/env python

import os
import sys
import datetime
import time
import hashlib
import re
import ntpath
import subprocess


def path_leaf(path):
    head, tail = ntpath.split(path)
    return tail or ntpath.basename(head)


def process_payload(file, outfile, key, encrypter):
    if os.path.exists(outfile):
        os.remove(outfile)
    p = subprocess.Popen([encrypter, 'crypt',file,outfile, key], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    if os.path.exists(outfile):
        print "done %s" % outfile

def process_file(file, outfile,key,encrypter):
    # Open file as file object and read to string
    ifile = open(file, 'r')
    if os.path.exists(outfile):
        os.remove(outfile)
    ofile = open(outfile,'w')
    pattern = r'((?:|[^"])*)(.{0,4})("(?:\\.|[^"\\])*")'
    n = 1
    for line in ifile:
        # works: match = re.search(pattern, line)
        match = re.findall(pattern, line)
        a = re.compile("^#include")

        if (not a.match(line)) and match:
            new_line = ""
            for result in match:
                if len(result) == 3:
                    if ("KKK" in result[0] or "KKK" in result[1]):
                        if not ("ENC(" in result[0] or "ENC(" in result[1]):
                            p = subprocess.Popen([encrypter, 'test', key, result[2][1:-1]], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                            out, err = p.communicate()
                            new_line += ("%s%sENC(%s)" % (result[0], result[1], '"'+out.rstrip("\n")+'"'))
                        else:
                            p = subprocess.Popen([encrypter, 'test', key, result[2][1:-1]], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                            out, err = p.communicate()
                            new_line += ("%s%s%s" % (result[0], result[1], '"'+out.rstrip("\n")+'"'))
                        new_line = new_line.replace('KKK', '')
                    else:
                        new_line += ("%s%s%s" % (result[0], result[1], result[2]))


                    last = result[2]
            if last:
                token = line.split(last);
                new_line += token[len(token)-1]
            new_line = new_line
            #print "%d:%s" % (n, new_line),
            ofile.write(new_line)
        else:
            #print "%d%s" % (n, line.rstrip('\n')),
            ofile.write(line)
        n += 1

    # Close file object
    ifile.close()
    ofile.close()



def check_dir(dir):
    if not os.path.exists(dir):
        d = os.mkdir(dir)
    if os.path.exists(dir):
        return True
    return False

def rot13(s):
    result = "{"
    first = 1
    # Loop over characters.
    for v in s:
        # Convert to number with ord.
        c = ord(v)
        c ^= 0x11
        if first == 1:
            first = 0
        else:
            result += ","
        # Append to result.
        result += "%d" % c
    if not result == "{":
        result += "}"
    print "%s rotted to :\n%s" % (key, result)
    # Return transformation.
    return result

def make_key_enc(key,file):
    #int pass[] = {114,120,112,126,49,124,126,127,117,126};
    template = """#ifndef RC4_ENC_H_
#define RC4_ENC_H_
int pass[] = {key};
int pass_len = {key_len};
char buffer_key[{key_len}+1];
#endif /* RC4_ENC_H_ */
    """
    key_l=len(key)
    key_a = rot13(key)
    context = {
        "key": key_a,
        "key_len": key_l,
        "key_len": key_l,
    }
    with open(file, 'w') as myfile:
        myfile.write(template.format(**context))
        myfile.close()


def usage():
    print("usage :")
    print("\tencstring <inputDir> <outputDir> <key> <encrypter>")
    print("\tencpayload <inputDir> <suffix> <key> <encrypter>")

if __name__ == '__main__':

    if len(sys.argv) < 5:
        usage()
        exit

    cmd = sys.argv[1]
    inDir = sys.argv[2]
    outDir = sys.argv[3]
    key = sys.argv[4]
    encrypter = sys.argv[5]
    print('arg passed cmd=<%s> inputdir=<%s> outdir/suffix=<%s> key<%s> encrypter<%s>' % (cmd, inDir, outDir, key, encrypter))

    if "encstring" in cmd:
        outDirHeader = outDir+"/include/"
        if not check_dir(outDir):
            print ("unable to create %s" % outDir)
            exit
        if not check_dir(outDirHeader):
            print ("unable to create %s" % outDirHeader)
            exit

        for file in os.listdir(inDir):
            if file.endswith(".cpp"):
                print "processing %s -> %s" %(inDir+"/"+file, outDir+"/"+file)
                process_file(inDir+"/"+file, outDir+"/"+file, key, encrypter)
        for file in os.listdir(inDir):
            if file.endswith(".h"):
                print "processing %s -> %s" %(inDir+"/"+file, outDirHeader+"/"+file)
                process_file(inDir+"/"+file, outDirHeader+"/"+file, key, encrypter)

        make_key_enc(key,outDirHeader+"/"+"rc4_enc.h")

    if "encpayload" in cmd:
        for file in os.listdir(inDir):
            if file.endswith(outDir):
                print "skipping %s -> %s" %(inDir, file)
            else:
                print "processing %s -> %s" %(inDir+"/"+file, inDir+"/"+file+outDir)
                process_payload(inDir+"/"+file, inDir+"/"+file+outDir, key, encrypter)