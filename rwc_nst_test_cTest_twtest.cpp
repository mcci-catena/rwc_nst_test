/*

Module:  rwc_nst_test_cTest.cpp

Function:
    cTest class implementation

Copyright notice and License:
    See LICENSE file accompanying this project.

Author:
    Terry Moore, MCCI Corporation	2019

*/

#include "rwc_nst_test_cTest.h"

#include "rwc_nst_test.h"
#include <strings.h>
#include <mcciadk_baselib.h>

// transmit window test driver
// fEntry is true to start a test, false subequently.
// The transmit window test generates a rising edge on a specified
// digital line (param RxDigOut), and captures the os_getTime() value.
// It then starts a single transmit scheduled at `param TxInterval`.
// It iterates `param TxCount` times, or until stopped with the `q`
// command.
bool cTest::txWindowTest(
    bool fEntry
    )
    {
    if (fEntry)
        {
        this->m_fStopTest = false;
        if (! this->m_TwTest.begin(*this))
            return true;

        gCatena.SafePrintf(
            "Start TX Window test: pulse output %d, then transmit %ld ms after rising edge",
            this->m_params.TxPulseOut,
            (long) this->m_params.TxInterval
            );

        if (this->m_TwTest.isContinuous())
            gCatena.SafePrintf(", continuous.\n");
        else
            gCatena.SafePrintf(
                ", for %lu messages\n",
                this->m_params.TxTestCount
                );
        }

    // now, evaluate state
    return this->m_TwTest.poll();
    }

bool cTest::TwTest_t::begin(cTest &Test)
    {
    this->pTest = &Test;
    this->Count = Test.m_params.TxTestCount;
    this->fRunning = false;
    if (this->Count == 0)
        this->fContinuous = true;
    else
        this->fContinuous = false;

    this->tDelay = ms2osticks(Test.m_params.TxInterval);
    this->tPulse = ms2osticks(Test.m_params.TxPulseMs);
    this->tGuard = us2osticksRound(Test.m_params.TxGuardUs);
    this->tStartup = us2osticksRound(Test.m_params.TxStartUs);

    if (this->tDelay <= 0 || this->tPulse <= 0)
        {
        gCatena.SafePrintf("** please specify positive, non-zero param TxInterval and TxPulseMs **\n");
        return false;
        }
    this->PulseOut.setOutput(Test.m_params.TxPulseOut, true);
    if (! this->PulseOut.isEnabled())
        {
        gCatena.SafePrintf("** please set param TxPulseOut to tx pulse output **\n");
        return false;
        }
    Test.m_TxDigOut.setOutput(Test.m_params.TxDigOut, true);

    this->Fsm.init(*this, &TwTest_t::fsmDispatch);
    this->Fsm.eval();

    return true;
    }

bool cTest::TwTest_t::poll()
    {
    if (! this->fRunning)
        return true;

    os_runloop_once();
    this->Fsm.eval();
    return false;
    }

cTest::TwTest_t::State cTest::TwTest_t::fsmDispatch(
    cTest::TwTest_t::State curState,
    bool fEntry
    )
    {
    State newState = State::stNoChange;

    switch (curState)
        {
    case State::stInitial:
        this->fRunning = true;
        this->pTest->setupLMIC(this->pTest->m_params);

        newState = State::stPulse;
        break;

    case State::stPulse:
        if (fEntry)
            {
            this->PulseOut.on();
            this->tEdge = os_getTime();

            // start the radio reset.
            os_radio(RADIO_RST);
            }

        if (this->pTest->m_fStopTest)
            newState = State::stFinal;
        else if (os_getTime() - this->tEdge >= this->tPulse)
            {
            this->PulseOut.off();
            newState = State::stDelay;
            }
        break;

    case State::stDelay:
        if (fEntry)
            {
            // load up the buffer.
            memcpy(LMIC.frame, this->pTest->m_Tx.Data, this->pTest->m_Tx.nData);
            LMIC.dataLen = this->pTest->m_Tx.nData;
            }

        if (this->pTest->m_fStopTest)
            newState = State::stFinal;
        else if (os_getTime() - this->tEdge >= this->tDelay - this->tGuard)
            newState = State::stTx;
        break;

    case State::stTx:
        if (fEntry)
            {
            // set the done function
            LMIC.osjob.func =
                [](osjob_t *job) -> void
                    {
                    gTest.m_TwTest.fTxComplete = true;
                    };

            this->fTxComplete = false;
            LMIC.txend = this->tEdge + this->tDelay - this->tStartup;
            os_radio(RADIO_TX_AT);
            }

        if (this->pTest->m_fStopTest)
            {
            os_clearCallback(&LMIC.osjob);
            os_radio(RADIO_RST);
            newState = State::stFinal;
            }
        else if (this->fTxComplete)
            {
            if (! this->fContinuous && --this->Count == 0)
                newState = State::stFinal;
            else
                newState = State::stPostTx;
            }
        break;

    case State::stPostTx:
        if (fEntry)
            {
            this->tEdge = os_getTime();
            }

        if (this->pTest->m_fStopTest)
            newState = State::stFinal;
        else if (os_getTime() - this->tEdge >= this->tPulse)
            newState = State::stPulse;
        break;

    default:
        newState = State::stFinal;
        break;

    case State::stFinal:
        if (fEntry)
            {
            this->fRunning = false;
            gCatena.SafePrintf("End Tx Window Test\n");
            }
        break;
        }

    return newState;
    }
