#!/usr/bin/env python3
"""Host regressions for the production download helpers; no network required."""
from pathlib import Path
import subprocess
import sys
import tempfile

source = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1] / 'Quake2/data'
http = (source / 'HttpClient.cpp').read_text()
archive = (source / 'Archive.cpp').read_text()

def section(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]

code = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>
#include "HttpClient.h"
constexpr uint32_t kReadBuffer=4096, kHnetChunk=4096;
constexpr int MBEDTLS_ERR_NET_RECV_FAILED=-76, MBEDTLS_ERR_SSL_TIMEOUT=-26624;
constexpr int MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY=-30848, MBEDTLS_ERR_SSL_CONN_EOF=-29312;
constexpr int MBEDTLS_ERR_SSL_WANT_READ=-26880, MBEDTLS_ERR_SSL_WANT_WRITE=-26752;
void StoreFormat(char* dst,uint32_t n,const char* fmt,...) {
    va_list args;va_start(args,fmt);vsnprintf(dst,n,fmt,args);va_end(args);
}
bool StoreStrCopy(char* dst,uint32_t n,const char* src){
    if(strlen(src)>=n)return false;strcpy(dst,src);return true;
}
bool StoreStrStartsWith(const char* s,const char* prefix){return strncmp(s,prefix,strlen(prefix))==0;}
bool StoreStrStartsWithCase(const char* s,const char* prefix){return strncasecmp(s,prefix,strlen(prefix))==0;}
uint32_t StoreStrLen(const char* s){return strlen(s);}
bool StoreParseU32(const char* s,uint32_t* out){
    char* end;unsigned long n=strtoul(s,&end,10);if(end==s||n>UINT32_MAX)return false;
    *out=(uint32_t)n;return true;
}
void SetError(char* e,uint32_t n,const char* s){StoreStrCopy(e,n,s);}
bool ContainsCase(const char* s,const char* needle){return strstr(s,needle)!=nullptr;}
std::string wire;size_t wirePos;int wireEnd,readCalls,g_ssl;
int mbedtls_ssl_read(int*,unsigned char* out,size_t n){
    ++readCalls;if(wirePos==wire.size())return wireEnd;
    n=std::min(n,wire.size()-wirePos);memcpy(out,wire.data()+wirePos,n);wirePos+=n;return n;
}
uint32_t ticks;uint32_t syscall_get_time_ms(){return ticks;}
void syscall_sleep(int n){ticks+=n;}
int g_socketFd=1, recvMode=-2, closeCalls;
namespace Hnet {
constexpr uint32_t MSG_DONTWAIT=8;
int Recv(int,unsigned char*,uint32_t,uint32_t flags){assert(flags==MSG_DONTWAIT);return recvMode;}
int Close(int){++closeCalls;return 0;}
}
'''
code += section(http, 'int TlsRecv(', 'bool TlsEnsureReady(')
code += section(http, 'struct Reader {', 'bool BuildRedirectUrl(')
code += section(http, 'bool ReadFixedBody(', 'bool ReadUntilEof(')
code += section(http, 'bool ReadRangeNumber(', 'bool SendRequest(')
code += r'''
struct ParsedUrl {char host[128];char path[2816];uint16_t port;};
std::string request;
bool TlsWriteAll(const uint8_t* p,uint32_t n,char*,uint32_t){request.assign((const char*)p,n);return true;}
'''
code += section(http, 'bool SendRequest(', 'struct RetrySinkContext {')
code += r'''
constexpr uint32_t kMaxUrl=3072,kMaxRedirects=6;
bool ParseUrl(const char*,ParsedUrl* url){*url=ParsedUrl{"example.org","/demo.exe",443};return true;}
bool TlsOpen(const ParsedUrl&,char*,uint32_t){return true;}
void TlsClose(){}
void SetErrorCode(char* error,uint32_t cap,const char* prefix,int32_t code){StoreFormat(error,cap,"%s %d",prefix,code);}
bool BuildRedirectUrl(const ParsedUrl&,const char*,char*,uint32_t){return false;}
bool ReadChunkedBody(Reader*,StoreBodySink,void*,char*,uint32_t){return false;}
bool ReadUntilEof(Reader*,StoreBodySink,void*,char*,uint32_t){return false;}
'''
code += section(http, 'static bool StoreHttpGetOnce(', '\nbool StoreHttpGet(')
code += r'''
typedef void (*ArchiveProgress)(const char*);
struct mbedtls_sha256_context {std::string data;};
bool writeFail,shaFail,hashFail;int calls,scenario,unlinkCalls,renameCalls,closes;
std::string disk;std::vector<uint32_t> offsets;
void mbedtls_sha256_init(mbedtls_sha256_context* s){s->data.clear();}
int mbedtls_sha256_starts_ret(mbedtls_sha256_context*,int){return 0;}
void mbedtls_sha256_free(mbedtls_sha256_context*){}
int mbedtls_sha256_update_ret(mbedtls_sha256_context* s,const uint8_t* p,uint32_t n){
    if(shaFail)return -1;s->data.append((const char*)p,n);return 0;
}
int syscall_open(const char*,int){disk.clear();return 1;}
int syscall_close(uint32_t){++closes;return 0;}
int syscall_unlink(const char*){++unlinkCalls;return 0;}
int syscall_rename(const char*,const char*){++renameCalls;return 0;}
constexpr int O_WRONLY=1,O_CREAT=2,O_TRUNC=4;
bool StoreWriteAll(int,const uint8_t* p,uint32_t n){
    if(writeFail)return false;disk.append((const char*)p,n);return true;
}
bool StoreHexEqual(const char* a,const char* b){return strcmp(a,b)==0;}
void Report(ArchiveProgress f,const char* s){if(f)f(s);}
'''
code += section(archive, 'struct ShaSink {', 'void SetError(')
code += r'''
void UpdateProgress(ShaSink*,const char*){}
bool FinishSha(ShaSink* sink,char* digest){
    if(!shaFail)assert(sink->sha.data==disk);strcpy(digest,hashFail?"wrong":"correct");return true;
}
bool StoreHttpGet(const char*,const char*,StoreBodySink sink,void* context,
                  StoreHttpResponse*,char* error,uint32_t cap,uint32_t offset,uint32_t total){
    offsets.push_back(offset);++calls;assert(total==6);
    if(scenario==1 && offset){
        assert(!ValidateResumeResponse(200,"",offset,total,true,total));
        SetError(error,cap,"invalid range");return false;
    }
    if(offset){
        char range[64];snprintf(range,sizeof(range),"bytes %u-5/6",offset);
        assert(ValidateResumeResponse(206,range,offset,total,true,total-offset));
    }
    std::string data=scenario==2?"x":(offset?"def":"abc");
    if(!sink(context,(const uint8_t*)data.data(),data.size()))return false;
    if(offset && scenario!=2)return true;
    SetError(error,cap,"interrupted");return false;
}
'''
code += section(archive, 'bool DownloadSink(', 'struct ZipSource {')
code += r'''
Reader feed(const std::string& text,int end){wire=text;wirePos=0;wireEnd=end;readCalls=0;return Reader{};}
void reset(int which=0){
    calls=unlinkCalls=renameCalls=closes=0;scenario=which;offsets.clear();
    writeFail=shaFail=hashFail=false;
}
bool download(char* error){return DownloadFile("https://test/a","part","final",6,"correct","demo",nullptr,error,256);}
int main(){
    char error[256],location[128],range[128];uint32_t n;int32_t status;bool chunked,has;
    ParsedUrl url={"example.org","/demo.exe",443};
    assert(SendRequest(url,"*/*",error,sizeof(error),3));
    assert(request.find("Range: bytes=3-\r\n")!=std::string::npos);
    assert(SendRequest(url,"*/*",error,sizeof(error),0));
    assert(request.find("Range:")==std::string::npos);
    Reader r=feed("HTTP/1.1 206 Partial Content\r\nContent-Length: 3\r\nContent-Range: bytes 3-5/6\r\n\r\n",0);
    assert(ReadHeaders(&r,&status,&chunked,&has,&n,location,sizeof(location),range,sizeof(range),error,sizeof(error)));
    assert(ValidateResumeResponse(status,range,3,6,has,n));
    for(const char* bad:{"bytes 0-5/6","bytes 3-4/6","bytes 3-5/7","bytes 4294967296-5/6","bytes 3-5/*","bytes 3-5/6junk","bytes 3-5","bytes 3-","bytes "})
        assert(!ValidateResumeResponse(206,bad,3,6,true,3));
    assert(!ValidateResumeResponse(200,"",3,6,true,6));
    assert(!ValidateResumeResponse(206,"bytes 3-5/6",3,6,true,4));
    assert(!ValidateResumeResponse(206,"bytes 0-5/6",0,6,true,6));
    assert(ValidateResumeResponse(200,"",0,6,true,6));
    assert(!ValidateResumeResponse(200,"",0,6,true,7));
    std::string delivered;
    auto collect=[](void* ctx,const uint8_t* p,uint32_t n){((std::string*)ctx)->append((const char*)p,n);return true;};
    feed("HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nabcdef",0);
    assert(!StoreHttpGetOnce("https://test/a","*/*",collect,&delivered,nullptr,error,sizeof(error),3,6));
    assert(delivered.empty());
    feed("HTTP/1.1 206 Partial Content\r\nContent-Length: 3\r\nContent-Range: bytes 2-4/6\r\n\r\ndef",0);
    assert(!StoreHttpGetOnce("https://test/a","*/*",collect,&delivered,nullptr,error,sizeof(error),3,6));
    assert(delivered.empty());
    feed("HTTP/1.1 206 Partial Content\r\nContent-Length: 3\r\nContent-Range: bytes 3-5/6\r\n\r\ndef",0);
    assert(StoreHttpGetOnce("https://test/a","*/*",collect,&delivered,nullptr,error,sizeof(error),3,6));
    assert(delivered=="def");
    r=feed("abc",MBEDTLS_ERR_NET_RECV_FAILED);
    assert(!ReadFixedBody(&r,6,nullptr,nullptr,error,sizeof(error)));
    assert(strstr(error,"TLS read failed: -76 after 3 / 6 bytes"));
    r=feed("abc",0);assert(!ReadFixedBody(&r,6,nullptr,nullptr,error,sizeof(error)));
    assert(strstr(error,"Truncated HTTP body: 3 / 6 bytes"));
    r=feed("abcdef",0);assert(ReadFixedBody(&r,6,nullptr,nullptr,error,sizeof(error)));
    unsigned char byte;ticks=UINT32_MAX-100;recvMode=-2;
    assert(TlsRecv(nullptr,&byte,1)==MBEDTLS_ERR_SSL_TIMEOUT && closeCalls==1 && g_socketFd==-1);
    g_socketFd=1;recvMode=0;assert(TlsRecv(nullptr,&byte,1)==0);
    recvMode=-1;assert(TlsRecv(nullptr,&byte,1)==MBEDTLS_ERR_NET_RECV_FAILED);
    reset();assert(download(error));assert(disk=="abcdef" && offsets==std::vector<uint32_t>({0,3}));
    assert(renameCalls==1 && closes==1);
    reset(1);assert(!download(error));assert(disk=="abc" && renameCalls==0 && calls==4);
    reset(2);assert(!download(error));assert(calls==4 && disk=="xxxx" && renameCalls==0);
    reset();writeFail=true;assert(!download(error));assert(calls==1 && disk.empty());
    reset();shaFail=true;assert(!download(error));assert(calls==1 && renameCalls==0);
    reset();hashFail=true;assert(!download(error));assert(disk=="abcdef" && renameCalls==0);
    reset();ShaSink sink={};sink.expected=2;uint8_t bytes[3]={};
    assert(!DownloadSink(&sink,bytes,3) && sink.failed && disk=="abcdef");
    puts("download range, interruption, deadline, integrity, and sink-failure regressions passed");
}
'''
with tempfile.TemporaryDirectory(prefix='quake2-download-test-') as tmp:
    path=Path(tmp)/'test.cpp';path.write_text(code)
    binary=Path(tmp)/'test'
    subprocess.run(['g++','-std=c++17','-O1','-g','-fsanitize=undefined','-I',str(source),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
