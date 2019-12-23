/*

Module:  rwc_nst_test_lmiclog.h

Function:
    LMIC logging for non-signalling test app.

Copyright notice and License:
    See LICENSE file accompanying this project.

Author:
    Terry Moore, MCCI Corporation	2019

*/

#ifndef _rwc_nst_test_lmiclog_h_
# define _rwc_nst_test_lmiclog_h_

#pragma once

#include <arduino_lmic.h>
#include <Catena_CommandStream.h>

#if LMIC_ENABLE_event_logging
extern "C" {
    void LMICOS_logEvent(const char *pMessage);
    void LMICOS_logEventUint32(const char *pMessage, uint32_t datum);
}
#endif // LMIC_ENABLE_event_logging

class cEventQueue {
public:
    cEventQueue() {};
    ~cEventQueue() {};
    // neither copyable nor movable
    cEventQueue(const cEventQueue&) = delete;
    cEventQueue& operator=(const cEventQueue&) = delete;
    cEventQueue(const cEventQueue&&) = delete;
    cEventQueue& operator=(const cEventQueue&&) = delete;

#if LMIC_ENABLE_event_logging
    static constexpr bool kLmicLoggingEnabled = true;
#else
    static constexpr bool kLmicLoggingEnabled = false;
#endif

    struct eventnode_t {
        osjob_t     job;
        ev_t        event;
        const char *pMessage;
        uint32_t    datum;
        ostime_t    time;
        ostime_t    txend;
        ostime_t    globalDutyAvail;
        u4_t        freq;
        u2_t        opmode;
        u2_t        fcntDn;
        u2_t        fcntUp;
        u2_t        rxsyms;
        rps_t       rps;
        u1_t        txChnl;
        u1_t        datarate;
        u1_t        txrxFlags;
        u1_t        saveIrqFlags;

        void print() const;
        const char *getSfName() const;
        const char *getBwName() const;
        const char *getCrName() const;
        const char *getCrcName() const;
    private:
        void printFreq() const;
        void printRps() const;
        void printOpmode(char sep = ',') const;
        void printTxend() const;
        void printTxChnl() const;
        void printDatarate() const;
        void printTxrxflags() const;
        void printSaveIrqFlags() const;
        void printFcnts() const;
        static void printAllRegisters()
            {
            cEventQueue::printAllRegisters();
            }
    };

    bool getEvent(eventnode_t &node) {
        if (m_head == m_tail) {
            return false;
        }
        node = m_queue[m_head];
        if (++m_head == sizeof(m_queue) / sizeof(m_queue[0])) {
            m_head = 0;
        }
        return true;
    }

    bool putEvent(ev_t event, const char *pMessage = nullptr, uint32_t datum = 0) {
        auto i = m_tail + 1;
        if (i == sizeof(m_queue) / sizeof(m_queue[0])) {
            i = 0;
        }
        if (i != m_head) {
            auto const pn = &m_queue[m_tail];
            pn->job = LMIC.osjob;
            pn->time = os_getTime();
            pn->txend = LMIC.txend;
            pn->globalDutyAvail = LMIC.globalDutyAvail;
            pn->event = event;
            pn->pMessage = pMessage;
            pn->datum = datum;
            pn->freq = LMIC.freq;
            pn->opmode = LMIC.opmode;
            pn->fcntDn = (u2_t) LMIC.seqnoDn;
            pn->fcntUp = (u2_t) LMIC.seqnoUp;
            pn->rxsyms = LMIC.rxsyms;
            pn->rps = LMIC.rps;
            pn->txChnl = LMIC.txChnl;
            pn->datarate = LMIC.datarate;
            pn->txrxFlags = LMIC.txrxFlags;
            pn->saveIrqFlags = LMIC.saveIrqFlags;
            m_tail = i;
            return true;
        } else {
            return false;
        }
    }

    // print all entries.
    void printAll()
        {
        while (this->printOne())
            ;
        }

    // print one entry and advance. Return false if no more to print.
    bool printOne()
        {
        eventnode_t e;
        if (this->getEvent(e))
            {
            e.print();
            return true;
            }
        else
            {
            return false;
            }
        }

    // print all registers.
    static void printAllRegisters();

    // set things up.
    void begin();

private:
    unsigned m_head, m_tail;
    eventnode_t m_queue[32];
    osjob_t m_job;
};

extern cEventQueue eventQueue;

#endif // _rwc_nst_test_lmiclog_h_
