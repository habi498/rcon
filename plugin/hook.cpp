/*
    Copyright (C) 2023  habi

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include "funchook.h"

#include "main.h"
extern char* rcon_password;
extern bool rcon_init;
#pragma comment( lib, "psapi.lib" )
#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#ifdef WIN32
typedef int (__stdcall *recvfrom_func)(
                   SOCKET   s,
                 char* buf,
                    int      len,
                    int      flags,
                   sockaddr* from,
     int* fromlen
);


#else
typedef ssize_t (*recvfrom_func)(int sockfd, void* buf, size_t len, int flags,
    struct sockaddr* src_addr, socklen_t* addrlen);
#endif
recvfrom_func recvfrom_original;


#ifdef WIN32
int __stdcall recvfrom_hook(SOCKET s, char* buf, int len, int flags,
    sockaddr* from, int* fromlen) {
#else
ssize_t recvfrom_hook(int s, void* message, size_t len,
    int flags, struct sockaddr* from,
    socklen_t* fromlen) {
    char* buf = ( char*)message;
#endif
    int r= recvfrom_original(s, buf, len, flags, from, fromlen);
    if (len < 11)return r;
    if (buf[0] == 'V' && buf[1] == 'C' && buf[2] == 'M' && buf[3] == 'P' &&
        (buf[10] == 'r'))
    {
        
        if (buf[10] == 'r')
        {
            //Login as RCON Admin
            struct sockaddr_in* addr_in = (struct sockaddr_in*)from;
            char* ip = inet_ntoa(addr_in->sin_addr);
            short passlen, cmdlen;
            if (len < 13)goto login_failed;
             passlen = (unsigned char)buf[11] * 256 + (unsigned char)buf[12];//the vcmp style!
           // printf("passlen is %d and len is %d\n", passlen,len);
            if (len < (13 + passlen))goto login_failed;
            char buffer1[512];
            if (passlen > (sizeof(buffer1) - 1))
                goto login_failed;
            memcpy(buffer1, buf + 13, passlen);
            buffer1[passlen] = '\0';
            //Get command
            char buffer2[512];
            if (len < (15 + passlen))return r;
            cmdlen = (unsigned char)buf[13 + passlen] * 256 + (unsigned char)buf[13 + passlen + 1];
            if (cmdlen > (sizeof(buffer2) - 1))
                return r;
            if (len < (13 + passlen + 2 + cmdlen))
                return r;
            memcpy(buffer2, buf + 13 + passlen + 2, cmdlen);
            buffer2[cmdlen] = '\0';
            char reply[512];
            memcpy(reply, buf, 11);
            if (strcmp(rcon_password, buffer1) == 0)
            {
                //Passwords match
               //printf("RCON: Admin logged in from %s\n", ip);
               
                if (strcmp(buffer2, "L") == 0)
                {
                    //interactive
                    reply[11] = 0; reply[12] = 2;
                    memcpy(reply + 13, "OK", 2); 
                    sendto(s, reply, 15, 0, from, *fromlen);
                    printf("RCON admin connected to server\n");
                }
                else
                {
                    char _reply[1024];
                    OnRCONCommand(buffer2, ip,_reply, sizeof(_reply));
                    char buffer2[512];
                    memcpy(buffer2, reply, 11);
                    int replylen = strlen(_reply);
                    buffer2[11] = (char)((replylen >> 8) & 0xFF);
                    buffer2[12] = (char)(replylen & 0xFF);
                    memcpy(buffer2 + 13, _reply, replylen);
                    sendto(s, buffer2, 13 + replylen, 0, from, *fromlen);
                }
               return r;
            }
            else { 
                OnRCONLoginFailed(ip);
                if (strcmp(buffer2, "L") == 0)
                {
                    //interactive
                    reply[11] = 0; reply[12] = 2;
                    memcpy(reply + 13, "NO", 2); 
                    sendto(s, reply, 15, 0, from, *fromlen);
                    return r;
                }
                else
                {
                    return r;
                }
                
            }
        login_failed:
            printf("RCON: Login failed from %s\n", ip);
            return r;
        }
    }
    return r;
}
int rcon_install_hooks()
{
    funchook_t* funchook = funchook_create();
    int rv;

    /* Prepare hooking.
     * The return value is used to call the original send function
     * in send_hook.
     */
    #ifdef WIN32
    HINSTANCE libr=LoadLibrary("ws2_32.dll");
    
    //recvfrom_func = recvfrom;
    recvfrom_original= (recvfrom_func)GetProcAddress(libr, "recvfrom");
#else 
    recvfrom_original = (recvfrom_func)recvfrom;
#endif
    void** target = (void**)&recvfrom_original;
    rv = funchook_prepare(funchook, target, (void*)recvfrom_hook);
    if (rv != 0) {
        printf("Error on funchook_prepare 2\n");
        exit(0);
    }



    /* Install hooks.
     * The first 5-byte code of send() and recv() are changed respectively.
     */
    rv = funchook_install(funchook, 0);
    if (rv != 0) {
        printf("Error on funchook_install\n");
        exit(0);
    }

    return 1;
}