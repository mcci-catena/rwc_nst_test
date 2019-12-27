/*

Module:  rwc_nst_test_cTest_rxTest.cpp

Function:
    cTest::rxTest() implementation

Copyright notice and License:
    See LICENSE file accompanying this project.

Author:
    Terry Moore, MCCI Corporation	2019

*/

#include "rwc_nst_test_cTest.h"

#include "rwc_nst_test.h"

// receive test driver
bool cTest::rxTest(
    bool fEntry
    )
    {
    if (fEntry)
        {
        this->m_fStopTest = false;
        this->m_Rx.Timeout = ms2osticks(this->m_params.RxTimeout);
        this->m_Rx.fContinuous = this->m_Rx.Timeout == 0;
        this->m_Rx.fTimedOut = false;
        this->m_Rx.Count = 0;
        this->m_Rx.fReceiving = false;
        this->m_RxDigOut.setOutput(this->m_params.RxDigOut, true);

        gCatena.SafePrintf("Start RX test: capturing raw downlink ");
        if (m_Rx.fContinuous)
            gCatena.SafePrintf("until canceled by `count` command");
        else
            gCatena.SafePrintf("for %u milliseconds", this->m_params.RxTimeout);

        if (this->m_RxDigOut.isEnabled())
            {
            gCatena.SafePrintf(" pulsing digital I/O %d", this->m_params.RxDigOut);
            }

        gCatena.SafePrintf(
            ".\n"
            "At RWC5020, select NST>Signal Generator, then Run.\n"
            );

        // setup LMIC and print settings
        this->setupLMIC(this->m_params);

        // if there's a timeout, start it.
        if (! m_Rx.fContinuous)
            {
            // when a timeout happens, this function is called.
            auto const timeoutFn = [](osjob_t *job)
                {
                os_radio(RADIO_RST);
                delay(1);
                os_clearCallback(&LMIC.osjob);
                gTest.m_Rx.fTimedOut = true;
                };

            os_setTimedCallback(
                &m_Rx.TimeoutJob,
                os_getTime() + this->m_Rx.Timeout,
                timeoutFn
                );
            }
        }

    // now, evaluate state
    os_runloop_once();

    if (this->m_fStopTest)
        {
        this->rxTestStop();
        gCatena.SafePrintf(
            "\nRX test stopped: received messages: %u.\n",
            this->m_Rx.Count
            );
        return true;
        }
    else if (this->m_Rx.fTimedOut)
        {
        this->rxTestStop();
        gCatena.SafePrintf(
            "\nRX test complete: received messages: %u.\n",
            this->m_Rx.Count
            );
        return true;
        }
    else
        {
        if (! this->m_Rx.fReceiving)
            {
            // set the coallback processor.
            LMIC.osjob.func = [](osjob_t *job)
                {
                if (LMIC.dataLen > 0)
                    ++gTest.m_Rx.Count;

                gCatena.SafePrintf(".");
                gTest.m_Rx.fReceiving = false;
                gTest.m_fsm.eval();
                };

            LMIC.rxtime = os_getTime();
            this->m_Rx.fReceiving = true;
            os_radio(RADIO_RXON);
            }
        return false;
        }
    }

void cTest::rxTestStop()
    {
    os_radio(RADIO_RST);
    os_clearCallback(&LMIC.osjob);
    os_clearCallback(&this->m_Rx.TimeoutJob);
    }

