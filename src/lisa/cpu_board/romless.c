/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.6      DEV 2007.12.04                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, 2007 Ray A. Arachelian                          *
*                                All Rights Reserved                                   *
*                                                                                      *
*           This program is free software; you can redistribute it and/or              *
*           modify it under the terms of the GNU General Public License                *
*           as published by the Free Software Foundation; either version 2             *
*           of the License, or (at your option) any later version.                     *
*                                                                                      *
*           This program is distributed in the hope that it will be useful,            *
*           but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*           MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*           GNU General Public License for more details.                               *
*                                                                                      *
*           You should have received a copy of the GNU General Public License          *
*           along with this program;  if not, write to the Free Software               *
*           Foundation, Inc., 59 Temple Place #330, Boston, MA 02111-1307, USA.        *
*                                                                                      *
*                   or visit: http://www.gnu.org/licenses/gpl.html                     *
*                                                                                      *
*                                                                                      *
*                                                                                      *
*                                ROMless boot functions                                *
*                                                                                      *
\**************************************************************************************/


#define IN_ROMLESS_C

#include <vars.h>

#include <reg68k.h>
#include <generator.h>
#include <registers.h>
#include <reg68k.h>
#include <cpu68k.h>

// __CYGWIN__ wrapper added by Ray Arachelian for LisaEm to prevent crashes in reg68k_ext exec
#ifdef __CYGWIN__
 extern uint32 reg68k_pc;
 extern uint32 *reg68k_regs;
 extern t_sr reg68k_sr;
#else
#if (!(defined(PROCESSOR_ARM) || defined(PROCESSOR_SPARC) || defined(PROCESSOR_INTEL) ))
 extern uint32 reg68k_pc;
 extern uint32 *reg68k_regs;
 extern t_sr reg68k_sr;
#endif
#endif

extern long getsectornum(DC42ImageType *F, uint8 side, uint8 track, uint8 sec);
extern char *los_error_code(signed long loserror);

#define FLOP_STAT_INVCMD   0x01  // invalid command
#define FLOP_STAT_INVDRV   0x02  // invalid drive
#define FLOP_STAT_INVSEC   0x03  // invalid sector
#define FLOP_STAT_INVSID   0x04  // invalid side
#define FLOP_STAT_INVTRK   0x05  // invalid track
#define FLOP_STAT_INVCLM   0x06  // invalid clear mask
#define FLOP_STAT_NODISK   0x07  // no disk
#define FLOP_STAT_DRVNOT   0x08  // drive not enabled
#define FLOP_STAT_IRQPND   0x09  // Interrupts pending

#define FLOP_STAT_INVFMT   0x0a  // Invalid format configuration
#define FLOP_STAT_BADROM   0x0b  // ROM Selftest failure
#define FLOP_STAT_BADIRQ   0x0c  // Unexpected IRQ or NMI
#define FLOP_STAT_WRPROT   0x14  // Write protect error
#define FLOP_STAT_BADVFY   0x15  // Unable to verify
#define FLOP_STAT_NOCLMP   0x16  // Unable to clamp disk
#define FLOP_STAT_NOREAD   0x17  // Unable to read
#define FLOP_STAT_NOWRIT   0x18  // Unable to write
#define FLOP_STAT_NOWRITE  0x18  // Unable to write
#define FLOP_STAT_NOUCLP   0x19  // Unable to unclamp/eject
#define FLOP_STAT_NOCALB   0x1A  // Unable to find calibration
#define FLOP_STAT_NOSPED   0x1B  // Unable to adjust speed
#define FLOP_STAT_NWCALB   0x1c  // Unable to write calibration


extern DC42ImageType Floppy_u,                 // in floppy.c, slightly cheating here.
                     Floppy_l;

extern long deinterleave5(long sector);


/* 0=00080000, 20000=000a0000 */
static uint8  romless_loram[0x560]=
{
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x11,0x86,0x00,0xfe,0x07,0x4e,  // 00
    0x00,0xfe,0x06,0xf8,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,  // 10 
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x07,0x36,0x00,0xfe,0x07,0x36,  // 20
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,  // 30 
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,  // 40 
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,  // 50 
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,  // 60 
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x07,0x04,  // 70 
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,  // 80 
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,  // 90 
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,  // a0 
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,  // b0 
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,  // c0 
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,  // d0 
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,  // e0 
    0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,0x00,0xfe,0x06,0xec,  // 100 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  // 110 
    0x00,0x0f,0x80,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  // 120 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  // 130 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  // 140 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  // 150 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  // 160 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  // 170 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,  // 180 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 190 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 1a0 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 1b0 
    0x00,0x00,0xbf,0x01,0x00,0x00,0x00,0x00,0x00,0x80,0xe7,0x20,0x01,0x80,0x90,0x83,  // 1c0 
    0x00,0x00,0x00,0x83,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xff,0x00,0x00,0x00,0x54,  // 1d0 
    0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x54,0x20,0x00,0x00,0x00,  // 1e0 
    0x00,0xfc,0xdd,0x81,0x00,0x0f,0xa9,0x60,0x00,0xfe,0x3c,0x88,0x00,0xfe,0x09,0x16,  // 1f0 
    0x00,0xfe,0x31,0x4e,0x00,0x0f,0x9e,0x20,0x00,0x0f,0xa9,0x06,0x00,0x00,0x00,0x00,  // 200 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 210 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 220 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 230 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 240 
    0x0f,0x0f,0x00,0x02,0x08,0x03,0x00,0x08,0x01,0x00,0x04,0x00,0x05,0x00,0x0f,0x0f,  // 250 
    0x00,0x00,0x01,0x00,0x01,0x06,0x03,0x05,0x00,0x04,0x07,0x00,0x00,0x00,0x00,0x00,  // 260 
    0x00,0x00,0x02,0xb6,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 270 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 280
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 290
    0x00,0x00,0x04,0x7c,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 300
    0x00,0xa8,0xa0,0x00,0x00,0x08,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01, 
    0x80,0xbf,0x80,0xbf,0x80,0xbf,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    0xff,0xff,0x00,0xfe,0x0c,0xb2,0x8f,0x00,0x8f,0x00,0x8f,0x00,0x8f,0x00,0x80,0x00, 
    0x80,0x00,0x80,0x00,0x80,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x8f,0x00,0x8f,0x00,0x8f,0x00,0x8f,0x00,0x80,0x00, 
    0x80,0x00,0x80,0x00,0x80,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x1e,0x00,0x00,0x00, 
    0x00,0x00,0x00,0x12,0x00,0x00,0x00,0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x48, 
    0x00,0xfe,0x3a,0x3d,0x00,0xfe,0x3a,0x3d,0x00,0xfe,0x32,0x18,0x00,0x00,0x00,0xa4, 
    0x00,0xfe,0x32,0x54,0x00,0x00,0x00,0x4e,0x00,0x00,0x00,0xa4,0x55,0x55,0xaa,0xaa, 
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11,0x40,0x00,0xfe,0x3a,0x3d, 
    0x00,0xfe,0x11,0x86,0x00,0x00,0x00,0x80,0x00,0xfe,0x3a,0x3d,0x55,0x55,0xaa,0xaa, 
    0x00,0x00,0x02,0x9e,0x00,0xfe,0x3a,0x3d,0x00,0xfe,0x35,0x50,0x00,0x00,0x00,0x80, 
    0x00,0x00,0x00,0x00,0x00,0xfc,0xdd,0x81,0x00,0xfe,0x1e,0x7e,0x00,0xfe,0x1c,0x32, 
    0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x45,0x00,0x20,0x00,0x00,0xff,0xff,0x00,0x08, 
    0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x45,0x00,0x20,0xff,0xff,0xff,0xff,0xff,0xff, 
    0xff,0xff,0x71,0xff,0xff,0xff,0x60,0x01,0xff,0xff,0x40,0x01,0xff,0xff,0x60,0x01, 
    0xff,0xff,0x71,0xff,0xff,0xff,0x79,0xff,0xff,0xff,0xfd,0xff,0xff,0xff,0xff,0xff, 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, 
    0xff,0xff,0x00,0x40,0x00,0x20,0x00,0x10,0x00,0x0f,0x8b,0x48,0xff,0xff,0xff,0xff, 
    0x1d,0x8a,0x1e,0x40,0xff,0x80,0xff,0xff,0xff,0xff,0x00,0x02,0x80,0xf1,0x00,0x10, 
    0x00,0x10,0x00,0xa0,0x00,0x32,0x00,0xf2,0x00,0x10,0x00,0x32,0x00,0xa0,0x00,0x54, 
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};


void romless_setvia_and_flopram(int profileboot)
{
  
    if (profileboot )  //0=profile, 1=floppy
    {
    // booted from floppy ////////////

    via[1].active=0x01;
    via[1].vianum=0x01;
    via[1].via[0]=0x01;
    via[1].via[1]=0x06;
    via[1].via[2]=0xaf;
    via[1].via[3]=0x00;
    via[1].via[4]=0x00;
    via[1].via[5]=0x00;
    via[1].via[6]=0x00;
    via[1].via[7]=0x00;
    via[1].via[8]=0x00;
    via[1].via[9]=0x00;
    via[1].via[10]=0x0f;
    via[1].via[11]=0x01;
    via[1].via[12]=0xc9;
    via[1].via[13]=0x82;
    via[1].via[14]=0x82;
    via[1].via[15]=0x00;
    via[1].via[16]=0x20;
    via[1].via[17]=0x00;
    via[1].via[18]=0x06;
    via[1].via[19]=0x7c;
    via[1].via[20]=0x00;
    via[1].via[21]=0x00;
    via[1].last_a_accs=0;
    via[1].orapending=1;
    via[1].t1_e=(XTIMER)-1;
    via[1].t1_fired=(XTIMER)12;
    via[1].t2_e=(XTIMER)-1;
    via[1].t2_fired=(XTIMER)0;
    via[1].sr_e=(XTIMER)0;
    via[1].sr_fired=(XTIMER)0;
    via[1].t1_set_cpuclk=(XTIMER)4174038;
    via[1].t2_set_cpuclk=(XTIMER)504058877;
    via[1].t1_fired_cpuclk=(XTIMER)4174166;
    via[1].t2_fired_cpuclk=(XTIMER)504058877;
    via[1].irqnum=0x02;
    via[1].srcount=0x00;
    via[1].ca1=0x00;
    via[1].ca2=0x00;
    via[1].cb1=0x00;
    via[1].cb2=0x00;
    via[2].active=0x01;
    via[2].vianum=0x02;
    via[2].via[0]=0xfa;
    via[2].via[1]=0x00;
    via[2].via[2]=0x9c;
    via[2].via[3]=0x00;
    via[2].via[4]=0x00;
    via[2].via[5]=0x00;
    via[2].via[6]=0x00;
    via[2].via[7]=0x00;
    via[2].via[8]=0x00;
    via[2].via[9]=0x00;
    via[2].via[10]=0x00;
    via[2].via[11]=0x00;
    via[2].via[12]=0x6b;
    via[2].via[13]=0x42;
    via[2].via[14]=0x00;
    via[2].via[15]=0x00;
    via[2].via[16]=0x00;
    via[2].via[17]=0x00;
    via[2].via[18]=0x01;
    via[2].via[19]=0x00;
    via[2].via[20]=0x72;
    via[2].via[21]=0xfa;
    via[2].last_a_accs=1;
    via[2].orapending=0;
    via[2].t1_e=(XTIMER)-1;
    via[2].t1_fired=(XTIMER)7;
    via[2].t2_e=(XTIMER)-1;
    via[2].t2_fired=(XTIMER)0;
    via[2].sr_e=(XTIMER)0;
    via[2].sr_fired=(XTIMER)0;
    via[2].t1_set_cpuclk=(XTIMER)949960;
    via[2].t2_set_cpuclk=(XTIMER)0;
    via[2].t1_fired_cpuclk=(XTIMER)951976;
    via[2].t2_fired_cpuclk=(XTIMER)0;
    via[2].irqnum=0x01;
    via[2].srcount=0x00;
    via[2].ca1=0x00;
    via[2].ca2=0x00;
    via[2].cb1=0x00;
    via[2].cb2=0x00;

    floppy_FDIR=0;
    floppy_6504_wait=0;
    floppy_irq_top=1;
    floppy_irq_bottom=0;
    uint8 romless_floppyram[0xb0]={
          0x00, 0xcc, 0x80, 0x00, 0x00, 0x00, 0xd5, 0x00, 0x00,
          0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd5, 0xc0,
          0xa7, 0x89, 0x64, 0x1e, 0x04, 0x09, 0xa8, 0x64, 0x02,
          0x82, 0x4f, 0x0c, 0x00, 0x00, 0xff, 0xff, 0x00, 0xff,
          0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
          0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0xd5, 0xaa, 0x96, 0xde, 0xaa, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0xff, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0xd5, 0xaa, 0xad, 0xde, 0xaa,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00
        };

        via[2].ProFile->Command=-2;
        via[2].ProFile->StateMachineStep=0x00;
        via[2].ProFile->indexread=0x0218;
        via[2].ProFile->indexwrite=0x0004;
        via[2].ProFile->CMDLine=0x00;
        via[2].ProFile->BSYLine=0x00;
        via[2].ProFile->DENLine=0x01;
        via[2].ProFile->RRWLine=0x00;
        via[2].ProFile->VIA_PA=0x08;
        via[2].ProFile->last_a_accs=0x00;
        
      memcpy(floppy_ram,romless_floppyram,0xb0);

    } // above - booted from floppy
    else
    {  // profile --------------

    if (!via[2].ProFile) return;
    if ( via[2].ProFile->DC42.fd<3 || via[2].ProFile->DC42.fh==NULL) return;

    via[1].active=0x01;
    via[1].vianum=0x01;
    via[1].via[0]=0x01;
    via[1].via[1]=0x06;
    via[1].via[2]=0xaf;
    via[1].via[3]=0x00;
    via[1].via[4]=0x00;
    via[1].via[5]=0x00;
    via[1].via[6]=0x00;
    via[1].via[7]=0x00;
    via[1].via[8]=0x00;
    via[1].via[9]=0x00;
    via[1].via[10]=0x0f;
    via[1].via[11]=0x01;
    via[1].via[12]=0xc9;
    via[1].via[13]=0x82;
    via[1].via[14]=0x82;
    via[1].via[15]=0x00;
    via[1].via[16]=0xa0;
    via[1].via[17]=0x00;
    via[1].via[18]=0x06;
    via[1].via[19]=0x7c;
    via[1].via[20]=0x40;
    via[1].via[21]=0x00;
    via[1].last_a_accs=0;
    via[1].orapending=1;
    via[1].t1_e=(XTIMER)-1;
    via[1].t1_fired=(XTIMER)12;
    via[1].t2_e=(XTIMER)-1;
    via[1].t2_fired=(XTIMER)0;
    via[1].sr_e=(XTIMER)0;
    via[1].sr_fired=(XTIMER)0;
    via[1].t1_set_cpuclk=(XTIMER)4174038;
    via[1].t2_set_cpuclk=(XTIMER)12507185;
    via[1].t1_fired_cpuclk=(XTIMER)4174166;
    via[1].t2_fired_cpuclk=(XTIMER)12507185;
    via[1].irqnum=0x02;
    via[1].srcount=0x00;
    via[1].ca1=0x00;
    via[1].ca2=0x00;
    via[1].cb1=0x00;
    via[1].cb2=0x00;
    via[2].active=0x01;
    via[2].vianum=0x02;
    via[2].via[0]=0xfa;
    via[2].via[1]=0x08;
    via[2].via[2]=0x9c;
    via[2].via[3]=0x00;
    via[2].via[4]=0x00;
    via[2].via[5]=0x00;
    via[2].via[6]=0x00;
    via[2].via[7]=0x00;
    via[2].via[8]=0x00;
    via[2].via[9]=0x00;
    via[2].via[10]=0x00;
    via[2].via[11]=0x00;
    via[2].via[12]=0x6b;
    via[2].via[13]=0x40;
    via[2].via[14]=0x00;
    via[2].via[15]=0x00;
    via[2].via[16]=0x00;
    via[2].via[17]=0x00;
    via[2].via[18]=0x08;
    via[2].via[19]=0x55;
    via[2].via[20]=0x72;
    via[2].via[21]=0xfa;
    via[2].last_a_accs=0;
    via[2].orapending=0;
    via[2].t1_e=(XTIMER)-1;
    via[2].t1_fired=(XTIMER)7;
    via[2].t2_e=(XTIMER)-1;
    via[2].t2_fired=(XTIMER)0;
    via[2].sr_e=(XTIMER)0;
    via[2].sr_fired=(XTIMER)0;
    via[2].t1_set_cpuclk=(XTIMER)949960;
    via[2].t2_set_cpuclk=(XTIMER)0;
    via[2].t1_fired_cpuclk=(XTIMER)951976;
    via[2].t2_fired_cpuclk=(XTIMER)0;
    via[2].irqnum=0x01;
    via[2].srcount=0x00;
    via[2].ca1=0x00;
    via[2].ca2=0x00;
    via[2].cb1=0x00;
    via[2].cb2=0x00;

    floppy_FDIR=0;
    floppy_6504_wait=0;
    floppy_irq_top=1;
    floppy_irq_bottom=1;
    uint8 romless_floppyram[0xb0]={
          0x00, 0x88, 0x80, 0x00, 0x00, 0x01, 0xd5, 0x00, 0x00,
          0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd5, 0xc0,
          0xa7, 0x89, 0x64, 0x1e, 0x04, 0x09, 0xa8, 0x64, 0x02,
          0x82, 0x4f, 0x0c, 0x00, 0x00, 0x00, 0xff, 0x01, 0xff,
          0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0xd5, 0xaa, 0x96, 0xde, 0xaa, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0xff, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0xd5, 0xaa, 0xad, 0xde, 0xaa,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00
          };

      memcpy(floppy_ram,romless_floppyram,0xb0);
        via[2].ProFile->Command=-2;
        via[2].ProFile->StateMachineStep=0x00;
        via[2].ProFile->indexread=0x0218;
        via[2].ProFile->indexwrite=0x0004;
        via[2].ProFile->CMDLine=0x00;
        via[2].ProFile->BSYLine=0x00;
        via[2].ProFile->DENLine=0x01;
        via[2].ProFile->RRWLine=0x00;
        via[2].ProFile->VIA_PA=0x08;
        via[2].ProFile->last_a_accs=0x00;
        
    } // profile //------------------------------------------
    
    
}

void romless_proread(void);
void romless_twgread(void);

int romless_boot(int profileboot)
{

int32 tagsidx, dataidx;
uint8  *blk;

int i;

for (i=0; i<128; i++)
    {
     mmu_all[1][i].sor=0; mmu_all[1][i].slr=0xc00; mmu_all[1][i].changed=3;
     mmu_all[2][i].sor=0; mmu_all[2][i].slr=0xc00; mmu_all[2][i].changed=3;
     mmu_all[3][i].sor=0; mmu_all[3][i].slr=0xc00; mmu_all[3][i].changed=3;
     mmu_all[4][i].sor=0; mmu_all[4][i].slr=0xc00; mmu_all[4][i].changed=3;
    }

mmu_all[1][0].sor=0x400;  mmu_all[1][0].slr=0x700;
mmu_all[1][1].sor=0x500;  mmu_all[1][1].slr=0x700;
mmu_all[1][2].sor=0x600;  mmu_all[1][2].slr=0x700;
mmu_all[1][3].sor=0x700;  mmu_all[1][3].slr=0x700;
mmu_all[1][4].sor=0x800;  mmu_all[1][4].slr=0x700;
mmu_all[1][5].sor=0x900;  mmu_all[1][5].slr=0x700;
mmu_all[1][6].sor=0xa00;  mmu_all[1][6].slr=0x700;
mmu_all[1][7].sor=0xb00;  mmu_all[1][7].slr=0x700;


mmu_all[1][126].slr=0x900;
mmu_all[1][127].slr=0xf00;

memset(lisaram,0xff,1024*1024); 


segment1=0; segment2=0; start=0; 
mmudirty=0xdec0de;
mmudirty_all[0]=0;
mmudirty_all[1]=0xdec0de;
mmudirty_all[2]=0xdec0de;
mmudirty_all[3]=0xdec0de;
mmudirty_all[4]=0xdec0de;
mmuflush(0x2000);

for (i=0; i<0x560; i++) storebyte(i,romless_loram[i]);


lisaram[0x80240]=serialnum240[ 0]>>8; lisaram[0x80241]=serialnum240[ 0] & 0x0f;
lisaram[0x80242]=serialnum240[ 1]>>8; lisaram[0x80243]=serialnum240[ 1] & 0x0f;
lisaram[0x80244]=serialnum240[ 2]>>8; lisaram[0x80245]=serialnum240[ 2] & 0x0f;
lisaram[0x80246]=serialnum240[ 3]>>8; lisaram[0x80247]=serialnum240[ 3] & 0x0f;
lisaram[0x80248]=serialnum240[ 4]>>8; lisaram[0x80249]=serialnum240[ 4] & 0x0f;
lisaram[0x8024a]=serialnum240[ 5]>>8; lisaram[0x8024b]=serialnum240[ 5] & 0x0f;
lisaram[0x8024c]=serialnum240[ 6]>>8; lisaram[0x8024d]=serialnum240[ 6] & 0x0f;
lisaram[0x8024e]=serialnum240[ 7]>>8; lisaram[0x8024f]=serialnum240[ 7] & 0x0f;

lisaram[0x80250]=serialnum240[ 8]>>8; lisaram[0x80251]=serialnum240[ 8] & 0x0f;
lisaram[0x80252]=serialnum240[ 9]>>8; lisaram[0x80253]=serialnum240[ 9] & 0x0f;
lisaram[0x80254]=serialnum240[10]>>8; lisaram[0x80255]=serialnum240[10] & 0x0f;
lisaram[0x80256]=serialnum240[11]>>8; lisaram[0x80257]=serialnum240[11] & 0x0f;
lisaram[0x80258]=serialnum240[12]>>8; lisaram[0x80259]=serialnum240[12] & 0x0f;
lisaram[0x8025a]=serialnum240[13]>>8; lisaram[0x8025b]=serialnum240[13] & 0x0f;
lisaram[0x8025c]=serialnum240[14]>>8; lisaram[0x8025d]=serialnum240[14] & 0x0f;
lisaram[0x8025e]=serialnum240[15]>>8; lisaram[0x8025f]=serialnum240[15] & 0x0f;

// invoke F-Line on ROM calls
memset(lisarom,0xff,0x4000);
lisarom[0x3ffc]=0x02; lisarom[0x3ffd]='H';
cheat_ram_test=0;
romless=1;
fixromchk();

// profile boot:
regs.pc = reg68k_pc = pc24=     0x00020000; 
regs.sp=                        0x00000000;
regs.sr.sr_int=reg68k_sr.sr_int=0x2704; 
reg68k_regs = regs.regs;

romless_setvia_and_flopram(profileboot);

if (profileboot==0)
{
  ProFileType *P=via[2].ProFile;
  
//  lisaram[0x80183]=0;

  if  (!P) {
      messagebox("Can't boot from non-existant ProFile. Create a ProFile, boot from an OS install disk, and install an operating system on the ProFile.", "No ProFile Hard Drive");
      cpu68k_clocks=cpu68k_clocks_stop; regs.stop=1;
      lisa_powered_off();
      return 1;
      }


  reg68k_regs[  1]=deinterleave5(0);
  reg68k_regs[8+1]=0x0001ffec;
  reg68k_regs[8+2]=0x00020000;


  romless_proread();
  if  (lisa_ram_safe_getword(1,0x1ffec+4)!=0xaaaa)  {
      messagebox("Try booting from an OS installation diskette.", "No bootable OS found on ProFile Hard Drive");
      cpu68k_clocks=cpu68k_clocks_stop; regs.stop=1;
      lisa_powered_off();
      return 1;
      }

  reg68k_regs[ 0]=0x0000aaaa;
  reg68k_regs[ 1]=0x00000000;
  reg68k_regs[ 2]=0x01200000;
  reg68k_regs[ 3]=0x0000000a;
  reg68k_regs[ 4]=0x00000003;
  reg68k_regs[ 5]=0x00000073;
  reg68k_regs[ 6]=0x00000010;
  reg68k_regs[ 7]=0x30000000;
  reg68k_regs[ 8]=0x00000100;
  reg68k_regs[ 9]=0x0001ffec;
  reg68k_regs[10]=0x00020000;
  reg68k_regs[11]=0x00fe1f5e;
  reg68k_regs[12]=0x00fe1f66;
  reg68k_regs[13]=0x00007ff8;
  reg68k_regs[14]=0x000fb3be; 
  reg68k_regs[15]=0x00000480;

  lisaram[0x801b3]=2;

//  storeword(0x22e,1);
//lisaram[0x8022e]=3;

}
else
{

  //lisaram[0x80183]=1;
 

  reg68k_regs[ 1]=0x80000000; 
  reg68k_regs[ 9]=0x0001FFF4;
  reg68k_regs[10]=0x00020000;

  romless_twgread();
  if (lisa_ram_safe_getword(1,0x1fff4+4)!=0xaaaa)
     {
         messagebox("This microdiskette is not bootable!", "OS BOOT Aborted");
         cpu68k_clocks=cpu68k_clocks_stop; regs.stop=1;
         lisa_powered_off();
         return 1;
     }

  reg68k_regs[ 0]=0x0000aaaa;
  reg68k_regs[ 1]=0x00000000; 
  reg68k_regs[ 2]=0x00c00000; 
  reg68k_regs[ 3]=0x00000019; 
  reg68k_regs[ 4]=0x00000000; 
  reg68k_regs[ 5]=0x00000073; 
  reg68k_regs[ 6]=0x00000010;
  reg68k_regs[ 7]=0x30000000;
  reg68k_regs[ 8]=0x00000100; 
  reg68k_regs[ 9]=0x0001fff4; 
  reg68k_regs[10]=0x00020000; 
  reg68k_regs[11]=0x00fe1d0e;
  reg68k_regs[12]=0x00fe1d14; 
  reg68k_regs[13]=0x00007ff8; 
  reg68k_regs[14]=0x000fb3be; 
  reg68k_regs[15]=0x00000480;    

  floppy_motor_sounds(0);
  lisaram[0x801b3]=1;

}


int t=60,b=160,l=16,r=70;

if (profileboot==1) b=320;

videolatch=0x2f;
videolatchaddress=0x00178000;

// show a fuzzy desktop with a profile
for (i=0; i<32768; i++)   
    {
        int j=i/90, k=i%90;
      dirtyvidram[i]=0xff; 
      if (j>20)        lisaram[videolatchaddress+i]=(j&1 ? 0x55:0xaa);
      else if (j==19)  lisaram[videolatchaddress+i]=0xff;
      else             lisaram[videolatchaddress+i]=0x00;

     if (profileboot==0)
        {
 
          if (!(  (j>t && j<t+8 && k==l+1)  ||     // corners
                  (j>t && j<t+8 && k==r-1)  ||
                  (j>b-8 && j<b && k==l+1)  || 
                  (j>b-8 && j<b && k==r-1)    )       
                                               && 
                  (j>t && j<b && k>l && k<r)     ) // white box with black border
                          lisaram[videolatchaddress+i]=(j<t+8 || j>b-8 || k<l+2 || k>r-2) ? 0xff:0x00; 

         // profile nametag/led
         if (j>(b-28) && j<(b-20) && ((k>(l+4) && k<(l+15) && k!=(l+6)) || k==l+47)   )   lisaram[videolatchaddress+i]=0xff;

         // feet
         if (j>b  && j<b+8 && (k==l+2 || k==l+10 || k==r-2))  lisaram[videolatchaddress+i]=0xff;
        }
       else
       {
         if (!(j>t && j<t+8 && k==r-1) &&
              (j>t && j<b && k>l && k<r)  )  // border of floppy
                 lisaram[videolatchaddress+i]=(j<t+8 || j>b-8 || k<l+2 || k>r-2) ? 0xff:0x00; 


        if (j>t+16   && j<t+70 && (k==r-20 || k==r-13 ))  lisaram[videolatchaddress+i]=0xff;
        if (j>t+ 8   && j<t+16 && (k> r-19 && k< r-14 ))  lisaram[videolatchaddress+i]=0xff;
        if (j>t+70   && j<t+78 && (k> r-19 && k< r-14 ))  lisaram[videolatchaddress+i]=0xff;

        if (j>t     && j<t+98 && (k==l+11 || k==r-11 ))  lisaram[videolatchaddress+i]=0xff;
        if (j>t+90  && j<t+98 && (k> l+11 && k< r-11 ))  lisaram[videolatchaddress+i]=0xff;

        if (j>b-150 && j<b     && (k==l+8  || k==r- 7 ))  lisaram[videolatchaddress+i]=0xff;
        if (j>b-158 && j<b-150 && (k> l+8  && k< r- 7 ))  lisaram[videolatchaddress+i]=0xff;

    
    
       } 
       }

videoramdirty=32768;

reg68k_sr.sr_struct.c=0;
cpu68k_makeipclist(pc24);

return 0;
}

void romless_vfychksum(void)
{
//ALERT_LOG(0,"Verifying checksum, mode:%d addr:%08x size:%d",reg68k_regs[1],reg68k_regs[8],(reg68k_regs[0]<<1) & 0x0000ffff);
 uint32 loops=reg68k_regs[0];
 reg68k_regs[2]=0;
 reg68k_regs[3]=0;
 
 reg68k_regs[0]=reg68k_regs[0] & 0x0000ffff;

 if (reg68k_regs[1])
      do
        {
         reg68k_regs[2]=(fetchbyte(reg68k_regs[8])<<8)|(fetchbyte(reg68k_regs[8]+2));
         reg68k_regs[8]+=4;

         reg68k_regs[2]=((reg68k_regs[2]>>8) & 0xff00)|(reg68k_regs[2] & 0xff);
         reg68k_regs[3]+=reg68k_regs[2];
         reg68k_regs[3]=wrol(reg68k_regs[3],1);
        }
       while ((reg68k_regs[0]--) != 0xffffffff);
 else
       do
        {
        
        reg68k_regs[2]=fetchword(reg68k_regs[8]);
        reg68k_regs[8]+=2;

        reg68k_regs[3]+=reg68k_regs[2];
        reg68k_regs[3]=wrol(reg68k_regs[3],1);
        }
       while ( (reg68k_regs[0]--) != 0xffffffff);
       
// hack // hack // hack //
    reg68k_regs[3]=0;    
// hack // hack // hack //
    
 if  (reg68k_regs[3]) reg68k_sr.sr_struct.c=1;
 else                 {reg68k_sr.sr_struct.c=0; reg68k_sr.sr_struct.n=0;}

 reg68k_regs[0]=loops | 0xffff;

  //ALERT_LOG(0,"Checksum verify done.  Result:%08x prev:%08x",reg68k_regs[3],reg68k_regs[2]);
}

extern int get_vianum_from_addr(long addr);

void romless_proread(void)
{
  ProFileType *P;

  // default to motherboard parallel port
  if (reg68k_regs[8+0]==0) reg68k_regs[8+0]=0xfcd901;

  uint32 addr=reg68k_regs[8+0];

  int vianum=get_vianum_from_addr(addr);
  ALERT_LOG(0,"address is %08x vianum:%d",addr,vianum);
  if (vianum<2 || vianum>8) vianum=2;

  P=via[vianum].ProFile;

  int i;
  uint8 *blk=NULL;
  uint32 sectornumber=reg68k_regs[1];
  uint8 *sectordata=&P->DataBlock[4]; 

  if (P==NULL)        {ALERT_LOG(0,"ProFile Pointer for via:%d is null",vianum); return;}
  if (&P->DC42==NULL) {ALERT_LOG(0,"ProFile DC42 for via:%d Pointer is null",vianum); return;}

//  #ifdef DEBUG
//    if (sectornumber==2055 && running_lisa_os==LISA_OFFICE_RUNNING) debug_on("los-hle");   //2021.03.31
//  #endif

  DEBUG_LOG(0,"about to read sector:%d (pre-deinterleave) on via:%d",sectornumber,vianum);

  P->DataBlock[512+4]=0xff; // premark non aaaa
  P->DataBlock[512+5]=0xff;

  //if (P->DC42.fd<3 || P->DC42.fh==NULL) {ALERT_LOG(0,"Profile file handle or descriptor does not exist"); return;}

//  if (sectornumber<50)   ALERT_LOG(0,"Slot 1 ID:%04x, Slot 2 ID:%04x, Slot 3 ID:%04x",
//           lisa_ram_safe_getword(1,0x298), lisa_ram_safe_getword(1,0x29a), lisa_ram_safe_getword(1,0x29c) ) 

  if (sectornumber<0x00f00000)   sectornumber=deinterleave5(sectornumber);

  if (sectornumber>0x00fffff0)  
    {
       if (sectornumber==0x00ffffff)  // get_structure_identity_table(P); // 20191107 - disabling until widget code works
                                      get_profile_spare_table(P);
  //    ALERT_LOG(0,"Slot 1 ID:%04x, Slot 2 ID:%04x, Slot 3 ID:%04x",
  //           lisa_ram_safe_getword(1,0x298), lisa_ram_safe_getword(1,0x29a), lisa_ram_safe_getword(1,0x29c) ) 

    }
  else
    {
       ALERT_LOG(0,"reading sector.");
       blk=dc42_read_sector_data(&P->DC42,sectornumber);
       if (!blk)  {
            ALERT_LOG(0,"Read sector from blk#%d failed with error:%d %s",sectornumber,P->DC42.retval,P->DC42.errormsg);
            reg68k_regs[0]=0xffffff; reg68k_sr.sr_struct.c=1;  
            return;
          }
       memcpy(sectordata,blk,512);

       blk=dc42_read_sector_tags(&P->DC42,sectornumber);
       if (!blk)  {
            ALERT_LOG(0,"Read tags from blk#%d failed with error:%d %s",sectornumber,P->DC42.retval,P->DC42.errormsg);
            reg68k_regs[0]=0xffffff; reg68k_sr.sr_struct.c=1;  
            return;
          }
       memcpy((void *)&sectordata[512],blk,20);
    }
  
  // for loop is needed to prevent MMU failure when crossing mmu page boundaries
  for (i=0; i< 20; i++) storebyte(reg68k_regs[ 9]+i,    sectordata[i+512]); // a1
  for (i=0; i<512; i++) storebyte(reg68k_regs[10]+i,    sectordata[i]);     // a2

  ALERT_LOG(0,"sector:%d fileid:%02x %02x",sectornumber,sectordata[512+4],sectordata[512+5]);

  P->DataBlock[0]=0;
  P->DataBlock[1]=0;
  P->DataBlock[2]=0;
  P->DataBlock[3]=0;

  via[2].ProFile->Command=-2;
  via[2].ProFile->StateMachineStep=0x00;
  via[2].ProFile->indexread=0x0218;
  via[2].ProFile->indexwrite=0x0004;
  via[2].ProFile->CMDLine=0x00;
  via[2].ProFile->BSYLine=0x00;
  via[2].ProFile->DENLine=0x01;
  via[2].ProFile->RRWLine=0x00;
  via[2].ProFile->VIA_PA=0x08;
  via[2].ProFile->last_a_accs=0x00;

  reg68k_regs[0]=0;  
  reg68k_regs[1]=0;  
  reg68k_sr.sr_struct.c=0;

//  storebyte(0x01B4,0);
//  storebyte(0x01B5,0);
//  storebyte(0x01B6,0);
//  storebyte(0x01B7,0);

//    lisa_ram_safe_setbyte(context,0x22e,0);
}


void romless_twgread(void)
{
 DC42ImageType *F;

 int drive,side,sector,track, sectornumber;
 uint8 *ptr;
 uint32 i;
 drive =( reg68k_regs[1]>>24 ) & 0xff;
 side  =( reg68k_regs[1]>>16 ) & 0xff;
 sector=( reg68k_regs[1]>>8  ) & 0xff;
 track =( reg68k_regs[1]     ) & 0xff;


 F=( (drive & 0x80) ? &Floppy_u : &Floppy_l );
 if (!F) {reg68k_regs[0]=FLOP_STAT_INVSEC; reg68k_sr.sr_struct.c=1; return;} 

 sectornumber=getsectornum(F, side, track, sector);

 if (sectornumber<0) {ALERT_LOG(0,"failed."); reg68k_regs[0]=FLOP_STAT_INVSEC; reg68k_sr.sr_struct.c=1; return;}
 reg68k_regs[8+3]=0x00FCDD81;
  

 ptr=dc42_read_sector_tags(F,sectornumber);
 if (!ptr) { ALERT_LOG(0,"failed"); reg68k_regs[0]=FLOP_STAT_NOREAD; reg68k_sr.sr_struct.c=1; return; }
 for (i=0; i<12; i++) storebyte(reg68k_regs[9]+i,ptr[i]);

 ptr=dc42_read_sector_data(F,sectornumber);  if (!ptr) {DEBUG_LOG(0,"Could not read sector #%ld",sectornumber); return;}
 if (!ptr) { ALERT_LOG(0,"failed!"); reg68k_regs[0]=FLOP_STAT_NOREAD; reg68k_sr.sr_struct.c=1; return; }
 for (i=0; i<512; i++) storebyte(reg68k_regs[10]+i,ptr[i]);


 reg68k_regs[0]=0; reg68k_sr.sr_struct.c=0;
 floppy_motor_sounds(track);
}




int romless_entry(void)
{
  int taken=0;

  if ((reg68k_pc & 0x00ffffff)==rom_profile_read_entry)
  {
     DEBUG_LOG(0,"HLE Intercept reading ProFile block 0x%08x (%ld)",reg68k_regs[1],reg68k_regs[1]);
     romless_proread();
     taken=1;
     cpu68k_clocks+=5000; // not exactly accurate. :-)
     pc24=reg68k_pc=fetchlong(reg68k_regs[15]); reg68k_regs[15]+=4;   /* Simulate RTS */
     reg68k_regs[0]=0; reg68k_regs[1]=0;
     ALERT_LOG(0,"reg68k_regs[0]=%d reg68k_regs[1]=%d",reg68k_regs[0],reg68k_regs[1]);
     return 1;  // abort F-Line handler
  }

if ((reg68k_pc & 0x00ff0000)==0x00fe0000)
   switch((reg68k_pc & 0x00ffffff))
   {   
   
    case 0x00fe0090: romless_proread();   taken=1; break;
    
    case 0x00fe0094: romless_twgread();   taken=1; break;
    
    case 0x00fe00bc: romless_vfychksum(); taken=1; break;
    
    case 0x00fe0084: {
                      char str[1024];
                            snprintf(str,1024,"OS Returned error code:%d.\n%s\n\nSee:\n"
                                          "http://lisafaq.sunder.net/lisafaq-hw-rom_error_codes.html or\n"
                                         "http://lisafaq.sunder.net/lisafaq-sw-los_error_codes.html\n\nfor details.\n%s\n",
                                         (int16)(reg68k_regs[0] & 0x0000ffff),
                                       los_error_code((signed long int)(reg68k_regs[0] & 0x0000ffff)),
                                             reg68k_regs[8+3] ? (char *)(&lisaram[CHK_MMU_TRANS(reg68k_regs[8+3])]) : "" );                 
                       messagebox(str, "OS BOOT Aborted");
                         cpu68k_clocks=cpu68k_clocks_stop; regs.stop=1;
                       lisa_powered_off();
                       return 1;
                     } 

    case 0x00fe0080: {
                       messagebox("OS has reset the Lisa", "OS BOOT Aborted");
                       lisa_powered_off();
                         cpu68k_clocks=cpu68k_clocks_stop; regs.stop=1;
                       return 1;
                     } 
    case 0x00fe0736: {
                          cpu68k_clocks=cpu68k_clocks_stop; regs.stop=1;
                       lisa_powered_off();
                       return 1;
                     }

    default:     
                EXITR(1,0,"Entered UNHANDLED ROM code while ROMLESS from operating system at %08x from %08x\n"
                          "D 0:%08x 1:%08x 2:%08x 3:%08x 4:%08x 5:%08x 6:%08x 7:%08x\n"
                          "A 0:%08x 1:%08x 2:%08x 3:%08x 4:%08x 5:%08x 6:%08x 7:%08x\n",
                          reg68k_pc,pc24,
                            reg68k_regs[0], reg68k_regs[1], reg68k_regs[2], reg68k_regs[3], reg68k_regs[4], reg68k_regs[5], reg68k_regs[6], reg68k_regs[7],
                            reg68k_regs[8], reg68k_regs[9], reg68k_regs[10],reg68k_regs[11],reg68k_regs[12],reg68k_regs[13],reg68k_regs[14],reg68k_regs[15]
                    );
  }

  if (taken)
   {
    cpu68k_clocks+=5000; // not exactly acurate. :-)
    pc24=reg68k_pc=fetchlong(reg68k_regs[15]); reg68k_regs[15]+=4;   /* Simulate RTS */
    return 1;  // abort F-Line handler
   }

 return 0;     // allow F-Line handler
}

extern uint16 chksum_a_rom_range(uint8 *rom, uint32 a0, uint32 a1);

int romless_dualparallel(void)
{
 uint16 newchks;

// ALERT_LOG(0,"Using fake dual parallel ROM.")
 memset(dualparallelrom,0xff,2048);
 dualparallelrom[0]=0xe0;
 dualparallelrom[1]=0x02;

 dualparallelrom[2]=0x03;
 dualparallelrom[3]=0xfd;

 dualparallelrom[4]=0x60;
 dualparallelrom[5]=0x02;

 dualparallelrom[6]=0x60;
 dualparallelrom[7]=0x02;

 uint16 cardid= (dualparallelrom[0]<<8)|(dualparallelrom[1] );
 uint16 words=( (dualparallelrom[2]<<8)|(dualparallelrom[3]) )*2  +4;

//E002 03FD 6008 6002 

 dualparallelrom[4+ 0x0a]=0x42; dualparallelrom[4+ 0x0b]=0x40;  // CLR.W D0
 dualparallelrom[4+ 0x0c]=0x4e; dualparallelrom[4+ 0x0d]=0x75;  // RTS

 words=0x7fe;
 newchks=chksum_a_rom_range(dualparallelrom,0,words );
 newchks=(~newchks)+1;
 dualparallelrom[words]=(newchks>>8); dualparallelrom[words+1]=(newchks & 0xff);
 return 0;
}
