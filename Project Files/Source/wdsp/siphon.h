// This is an independent project of an individual developer. Dear PVS-Studio,
// please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// http://www.viva64.com
/*  siphon.h

This file is part of a program that implements a Software-Defined Radio.

Copyright (C) 2013 Warren Pratt, NR0V

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

The author can be reached by email at

warren@wpratt.com

*/

// 'siphon' collects samples in a buffer.  These samples can then be PULLED from
// the buffer
//	in either raw or FFT'd form.

#ifndef _siphon_h
#define _siphon_h

typedef struct _siphon {
    int run;
    int position;
    int mode;
    int disp;
    int insize;
    double* in;
    int sipsize; // NOTE:  sipsize MUST BE A POWER OF TWO!!
    double* sipbuff;
    int outsize;
    int idx;
    double* sipout;
    int fftsize;
    double* specout;
    volatile long specmode;
    fftw_plan sipplan;
    double* window;
    CRITICAL_SECTION update;
} siphon, *SIPHON;

extern SIPHON create_siphon(int run, int position, int mode, int disp,
    int insize, double* in, int sipsize, int fftsize, int specmode);

extern void destroy_siphon(SIPHON a);

extern void flush_siphon(SIPHON a);

extern void xsiphon(SIPHON a, int pos);

extern void setBuffers_siphon(SIPHON a, double* in);

extern void setSamplerate_siphon(SIPHON a, int rate);

extern void setSize_siphon(SIPHON a, int size);

// RXA Properties

extern __declspec(dllexport) void RXAGetaSipF(
    int channel, float* out, int size);

extern __declspec(dllexport) void RXAGetaSipF1(
    int channel, float* out, int size);

// TXA Properties

extern __declspec(dllexport) void TXAGetaSipF(
    int channel, float* out, int size);

extern __declspec(dllexport) void TXAGetaSipF1(
    int channel, float* out, int size);

extern __declspec(dllexport) void TXAGetSpecF1(int channel, float* out);

// Calls for External Use

extern __declspec(dllexport) void create_siphonEXT(
    int id, int run, int insize, int sipsize, int fftsize, int specmode);

extern __declspec(dllexport) void destroy_siphonEXT(int id);

extern __declspec(dllexport) void xsiphonEXT(int id, double* buff);

extern __declspec(dllexport) void SetSiphonInsize(int id, int size);

#endif
