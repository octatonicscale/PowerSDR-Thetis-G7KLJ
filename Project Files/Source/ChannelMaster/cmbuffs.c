// This is an independent project of an individual developer. Dear PVS-Studio,
// please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// http://www.viva64.com
/*  cmbuffs.c

This file is part of a program that implements a Software-Defined Radio.

Copyright (C) 2014 Warren Pratt, NR0V

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

#include "cmcomm.h"

void start_cmthread(int id) {
    _beginthread(cm_main, 0, (void*)id);
}

void create_cmbuffs(
    int id, int accept, int max_insize, int max_outsize, int outsize) {
    CMB a = (CMB)malloc0(sizeof(cmb));
    pcm->pcbuff[id] = pcm->pdbuff[id] = pcm->pebuff[id] = pcm->pfbuff[id] = a;
    a->id = id;
    a->accept = accept;
    a->run = 1;
    a->max_in_size = max_insize;
    a->max_outsize = max_outsize;
    a->r1_outsize = outsize;
    if (a->max_outsize > a->max_in_size)
        a->r1_size = a->max_outsize;
    else
        a->r1_size = a->max_in_size;
    a->r1_active_buffsize = CMB_MULT * a->r1_size;
    a->r1_baseptr
        = (double*)malloc0(a->r1_active_buffsize * sizeof(WDSP_COMPLEX));
    a->r1_inidx = 0;
    a->r1_outidx = 0;
    a->r1_unqueuedsamps = 0;
    a->Sem_BuffReady = CreateSemaphore(0, 0, 1000, 0);
    InitializeCriticalSectionAndSpinCount(&a->csIN, 2500);
    InitializeCriticalSectionAndSpinCount(&a->csOUT, 2500);
    start_cmthread(id);
}

void destroy_cmbuffs(int id) {
    CMB a = pcm->pcbuff[id];
    InterlockedBitTestAndReset(
        &a->accept, 0); // shut the Inbound() gate to prevent new infusions
    EnterCriticalSection(
        &a->csIN); // wait until the current Inbound() infusion is finished
    EnterCriticalSection(&a->csOUT); // block the CM thread before cmdata()
    Sleep(25); // wait for the thread to arrive at the top of the cm_main() loop
    InterlockedBitTestAndReset(&a->run, 0); // set a trap for the CM thread
    ReleaseSemaphore(a->Sem_BuffReady, 1,
        0); // be sure the CM thread can pass WaitForSingleObject in cm_main()
            // //
    LeaveCriticalSection(
        &a->csOUT); // let the thread pass to the trap in cmdata()
    Sleep(2); // wait for the CM thread to die
    DeleteCriticalSection(&a->csOUT);
    DeleteCriticalSection(&a->csIN);
    CloseHandle(a->Sem_BuffReady);
    _aligned_free(a->r1_baseptr);
    _aligned_free(a);
}

void flush_cmbuffs(int id) {
    CMB a = pcm->pfbuff[id];
    memset(a->r1_baseptr, 0, a->r1_active_buffsize * sizeof(WDSP_COMPLEX));
    a->r1_inidx = 0;
    a->r1_outidx = 0;
    a->r1_unqueuedsamps = 0;
    while (!WaitForSingleObject(a->Sem_BuffReady, 1))
        ;
}

PORT void Inbound(int id, int nsamples, double* in) {
    int n;
    int first, second;
    CMB a = pcm->pebuff[id];

    if (_InterlockedAnd(&a->accept, 1)) {
        EnterCriticalSection(&a->csIN);
        if (nsamples > (a->r1_active_buffsize - a->r1_inidx)) {
            first = a->r1_active_buffsize - a->r1_inidx;
            second = nsamples - first;
        } else {
            first = nsamples;
            second = 0;
        }
        memcpy(
            a->r1_baseptr + 2 * a->r1_inidx, in, first * sizeof(WDSP_COMPLEX));
        memcpy(a->r1_baseptr, in + 2 * first, second * sizeof(WDSP_COMPLEX));

        if ((a->r1_unqueuedsamps += nsamples) >= a->r1_outsize) {
            n = a->r1_unqueuedsamps / a->r1_outsize;
            a->when_sembuffready = timeGetTime();
            ReleaseSemaphore(a->Sem_BuffReady, n, 0);
            a->r1_unqueuedsamps -= n * a->r1_outsize;
        }
        if ((a->r1_inidx += nsamples) >= a->r1_active_buffsize)
            a->r1_inidx -= a->r1_active_buffsize;
        LeaveCriticalSection(&a->csIN);
    }
}

void cmdata(int id, double* out) {
    int first, second;
    CMB a = pcm->pdbuff[id];
    EnterCriticalSection(&a->csOUT);
    if (!_InterlockedAnd(&a->run, 1)) {
        LeaveCriticalSection(&a->csOUT);
        _endthread();
    }
    if (a->r1_outsize > (a->r1_active_buffsize - a->r1_outidx)) {
        first = a->r1_active_buffsize - a->r1_outidx;
        second = a->r1_outsize - first;
    } else {
        first = a->r1_outsize;
        second = 0;
    }
    memcpy(out, a->r1_baseptr + 2 * a->r1_outidx, first * sizeof(WDSP_COMPLEX));
    memcpy(out + 2 * first, a->r1_baseptr, second * sizeof(WDSP_COMPLEX));
    if ((a->r1_outidx += a->r1_outsize) >= a->r1_active_buffsize)
        a->r1_outidx -= a->r1_active_buffsize;
    LeaveCriticalSection(&a->csOUT);
}
#ifdef DEBUG_TIMINGS
static unsigned int times_ctr = 0;
#endif

void cm_main(void* pargs) {
    HANDLE hpri = prioritise_thread_max();

#pragma warning(disable : 4311)
    int id = (int)pargs;
#pragma warning(default : 4311)
    CMB a = pcm->pdbuff[id];

#ifdef DEBUG_TIMINGS
    if (times_ctr == 0) {
        a->when_sembuffready = 0;
    }
#endif

    while (_InterlockedAnd(&a->run, 1)) {
#ifdef DEBUG_TIMINGS
        DWORD dwstartwait = timeGetTime();
#endif
        DWORD dwWait = WaitForSingleObject(a->Sem_BuffReady, 500);
        if (dwWait == WAIT_TIMEOUT) {
            continue;
        }

#ifdef DEBUG_TIMINGS
        DWORD dwendwait = timeGetTime();
        DWORD took = dwendwait - dwstartwait;
        if (took > 10 && times_ctr++ > 50) {
            DWORD how_long_ready = dwendwait - a->when_sembuffready;
            fprintf(stderr,
                "cm_main: long time between calls: %ld ms, for id %ld\n",
                (int)took, id);
            fprintf(stderr,
                "cm_main: time since Sem_BuffReady signalled: %ld ms.\n",
                (int)how_long_ready);
            fflush(stderr);
        }
#endif

        cmdata(id, pcm->in[id]);
        xcmaster(id);
    }

    if (hpri) prioritise_thread_cleanup(hpri);
}

void SetCMRingOutsize(int id, int size) {
    CMB a = pcm->pcbuff[id];
    InterlockedBitTestAndReset(
        &a->accept, 0); // shut the Inbound() gate to prevent new infusions
    EnterCriticalSection(
        &a->csIN); // wait until the current Inbound() infusion is finished
    EnterCriticalSection(&a->csOUT); // block the CM thread before cmdata()
    Sleep(25); // wait for the thread to arrive at the top of the cm_main() loop
    InterlockedBitTestAndReset(&a->run, 0); // set a trap for the CM thread
    ReleaseSemaphore(a->Sem_BuffReady, 1,
        0); // be sure the CM thread can pass WaitForSingleObject in cm_main()
            // //
    LeaveCriticalSection(
        &a->csOUT); // let the thread pass to the trap in cmdata()
    Sleep(2); // wait for the CM thread to die
    flush_cmbuffs(id); // restore ring to pristine condition
    a->r1_outsize = size; // set its new outsize
    InterlockedBitTestAndSet(&a->run, 0); // remove the CM thread trap
    start_cmthread(id); // start the CM thread
    LeaveCriticalSection(&a->csIN); // enable Inbound() processing
    InterlockedBitTestAndSet(&a->accept, 0); // open the Inbound() gate
}
