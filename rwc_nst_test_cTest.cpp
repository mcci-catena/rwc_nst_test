/*

Module:  rwc_nst_test_cTest.cpp

Function:
    cTest class implementation

Copyright notice and License:
    See LICENSE file accompanying this project.

Author:
    Terry Moore, MCCI Corporation	2019

*/

#include "rwc_nst_test.h"

#include <strings.h>
#include <mcciadk_baselib.h>

void cTest::begin()
    {
    if (! this->m_fRegistered)
        {
        this->m_fRegistered = true;
        gCatena.registerObject(this);
        }

    // start (or restart) the FSM.
    if (! this->m_fRunning)
        {
        this->m_fExit = false;
        this->m_fsm.init(*this, &cTest::fsmDispatch);
        }
    }

void cTest::end()
    {
    if (this->m_fRunning)
        {
        this->m_fExit = true;
        this->m_fsm.eval();
        }
    }

// virtual void poll() override
void cTest::poll()
    {
    this->m_fsm.eval();
    }

cTest::State cTest::fsmDispatch(
    cTest::State currentState,
    bool fEntry
    )
    {
    State newState = State::stNoChange;

    if (fEntry && this->isTraceEnabled(this->DebugFlags::kTrace))
        {
        gCatena.SafePrintf("cTest::fsmDispatch: enter %s\n",
                this->getStateName(currentState)
                );
        }

    switch(currentState)
        {
    case State::stInitial:
        {
        static const std::uint8_t defaultData[] =
            { 0xCA, 0xFE, 0xF0, 0x0D };

        newState = State::stIdle;
        this->m_params = kDefaultParams();

        memcpy(this->m_Tx.Data, defaultData, sizeof(defaultData));
        this->m_Tx.nData = sizeof(defaultData);
        }
        break;

    case State::stIdle:
        {
        if (fEntry)
            {
            gCatena.SafePrintf("Idle\n");
            }

        auto const cmd = this->m_pendingCmd;
        if (cmd != Command::None)
            {
            this->m_pendingCmd = Command::None;
            switch (cmd)
                {
            case Command::StartTx:
                newState = State::stTxTest;
                break;
            case Command::StartRx:
                newState = State::stRxTest;
                break;
            case Command::StartRxWindow:
                newState = State::stRxWindowTest;
                break;
            case Command::StartTxWindow:
                newState = State::stTxWindowTest;
                break;

            default:
                // ignore
                break;
                }
            }
        }
        break;

    case State::stTxTest:
        if (this->txTest(fEntry))
            newState = State::stIdle;
        break;

    case State::stRxTest:
        if (this->rxTest(fEntry))
            newState = State::stIdle;
        break;

    case State::stRxWindowTest:
        if (this->rxWindowTest(fEntry))
            newState = State::stIdle;
        break;

    case State::stTxWindowTest:
        if (this->txWindowTest(fEntry))
            newState = State::stIdle;
        break;

    case State::stFinal:
        break;

    default:
        break;
        }

    return newState;
    }

// set up LMIC
void cTest::setupLMIC(const cTest::Params &params)
    {
    LMIC_reset();
    LMIC.freq = params.Freq;
    LMIC.rps = makeRps(params.SpreadingFactor, params.Bandwidth, params.CodingRate, 0, 0);
    LMIC.noRXIQinversion = true;
    LMIC.lbt_ticks = us2osticks(params.RxRssiIntervalUs);
    LMIC.lbt_dbmax = params.RxRssiDbMax;
    LMIC.radio_txpow = params.TxPower;
    using rxsym_t = decltype(LMIC.rxsyms);
    if (sizeof(rxsym_t) == sizeof(std::uint8_t) && params.RxSyms > 0xFFu)
        LMIC.rxsyms = 0xFFu;
    else
        LMIC.rxsyms = rxsym_t(params.RxSyms);

    float clockError = (std::abs)(params.ClockError * MAX_CLOCK_ERROR / 100.0) + 0.5;
    LMIC_setClockError(clockError >= UINT16_MAX ? UINT16_MAX : u2_t(clockError));

    gCatena.SafePrintf("Freq=%u Hz, ", LMIC.freq);
    if (getSf(LMIC.rps) == FSK)
        gCatena.SafePrintf("FSK");
    else
        {
        gCatena.SafePrintf(
            "LoRa SF%u, BW%u",
            getSf(LMIC.rps) + 6,
            125 << getBw(LMIC.rps)
            );
        }

    u2_t ceppk = LMIC.client.clockError * 1000 / MAX_CLOCK_ERROR;

    gCatena.SafePrintf(
        ", TxPwr=%d dB, CR 4/%u, CRC=%u, LBT=%u us/%d dB, clockError=%u.%u (0x%x), RxSyms=%u\n",
        LMIC.radio_txpow,
        getCr(LMIC.rps) + 5 - CR_4_5,
        ! getNocrc(LMIC.rps),
        osticks2us(LMIC.lbt_ticks),
        LMIC.lbt_dbmax,
        ceppk / 10, ceppk % 10, LMIC.client.clockError,
        LMIC.rxsyms
        );
    }

void cTest::txTestDone(osjob_t *job)
    {
    gTest.m_Tx.fIdle = true;
    gTest.m_fsm.eval();
    }

// transmit test driver
bool cTest::txTest(
    bool fEntry
    )
    {
    if (fEntry)
        {
        this->m_fStopTest = false;
        this->m_Tx.Count = this->m_params.TxTestCount;
        this->m_Tx.fIdle = true;
        this->m_Tx.fContinuous = this->m_Tx.Count == 0;
        this->m_Tx.Tnext = os_getTime();
        this->m_TxDigOut.setOutput(this->m_params.TxDigOut, true);

        gCatena.SafePrintf("Start TX test: %u bytes, ", this->m_Tx.nData);

        if (m_Tx.fContinuous)
            gCatena.SafePrintf("continous");
        else
            gCatena.SafePrintf("%u packets", this->m_Tx.Count);

        if (this->m_TxDigOut.isEnabled())
            {
            gCatena.SafePrintf(" pulsing digital I/O %d", this->m_params.TxDigOut);
            }

        // setup LMIC and print settings
        gCatena.SafePrintf(". ");
        this->setupLMIC(this->m_params);
        }

    os_runloop_once();

    if (! this->m_Tx.fIdle)
        return false;
    else if (this->m_fStopTest)
        {
        gCatena.SafePrintf("\nTX test stopped.\n");
        return true;
        }
    else if (this->m_Tx.Count == 0 && ! this->m_Tx.fContinuous)
        {
        // all done.
        gCatena.SafePrintf("\nTx test complete.\n");
        return true;
        }
    else if ((os_getTime() - this->m_Tx.Tnext) < 0)
        {
        // waiting for time to go.
        return false;
        }
    else
        {
        // advance time
        this->m_Tx.Tnext += ms2osticks(this->m_params.TxInterval);

        // reset the radio.
        os_radio(RADIO_RST);

        // print a dot
        gCatena.SafePrintf(".");

        if (! this->m_Tx.fContinuous)
            --this->m_Tx.Count;

        // give radio time to responsd to reset
        digitalWrite(LED_BUILTIN, 1);
        delay(1);
        digitalWrite(LED_BUILTIN, 0);

        // load up the buffer.
        memcpy(LMIC.frame, this->m_Tx.Data, this->m_Tx.nData);
        LMIC.dataLen = this->m_Tx.nData;

        // set the done function
        LMIC.osjob.func = cTest::txTestDone;

        this->m_Tx.fIdle = false;
        os_radio(RADIO_TX);
        return false;
        }
    }

void cTest::evStopTest()
    {
    this->m_fStopTest = true;
    this->m_fsm.eval();
    }

// receive window test driver
// fEntry is true to start a test, false subequently.
// The receive window test waits for a rising edge on a specified
// digital line (param RxDigIn), and captures the os_getTime() value.
// It then starts a single receive scheduled at `param RxWindow`, using
// RxSyms and ClockError to simulate the LMIC's window.
// This process repeats (controlled by param RxCount), and counts of pulses
// and successful receives are accumulated.
bool cTest::rxWindowTest(
    bool fEntry
    )
    {
    if (fEntry)
        {
        this->m_fStopTest = false;
        if (! this->m_RwTest.begin(*this))
            return true;

        gCatena.SafePrintf(
            "Start RX Window test: vary window from %ld to %ld us in %ld us steps, %u tries each step\n",
            (long) osticks2us(this->m_RwTest.WindowStart),
            (long) osticks2us(this->m_RwTest.WindowStop),
            (long) osticks2us(this->m_RwTest.WindowStep),
            this->m_RwTest.Count
            );

        gCatena.SafePrintf(
            "Rx triggered by digital input %d",
            this->m_params.RxDigIn
            );
        if (this->m_RxDigOut.isEnabled())
            {
            gCatena.SafePrintf(", pulsing digital I/O %d", this->m_params.RxDigOut);
            }

        gCatena.SafePrintf(
            ".\n"
            "Set up second Catena and start tx loop. Use 'count' or 'q' to quit\n"
            );
        }

    // now, evaluate state
    return this->m_RwTest.poll();
    }

bool cTest::RwTest_t::begin(cTest &Test)
    {
    this->pTest = &Test;
    this->resetStats();
    this->Count = Test.m_params.RxCount;
    this->WindowStart = us2osticks(Test.m_params.WindowStart);
    this->WindowStop = us2osticks(Test.m_params.WindowStop);
    this->WindowStep = us2osticks(Test.m_params.WindowStep);
    if (this->WindowStart <= 0 || this->WindowStop <= 0)
        {
        gCatena.SafePrintf("** please specify positive, non-zero param Window.Start and Window.Stop **\n");
        return false;
        }
    if (this->WindowStep == 0)
        {
        gCatena.SafePrintf("** please specify a non-zero param Window.Step **\n");
        return false;
        }
    this->DigIn.setInput(Test.m_params.RxDigIn, true);
    if (! this->DigIn.isEnabled())
        {
        gCatena.SafePrintf("** please set param Rx.DigIn to rx trigger input **\n");
        return false;
        }
    Test.m_RxDigOut.setOutput(Test.m_params.RxDigOut, true);

    this->Fsm.init(*this, &RwTest_t::fsmDispatch);
    this->Fsm.eval();

    return true;
    }

bool cTest::RwTest_t::poll()
    {
    if (! this->fRunning)
        return true;

    os_runloop_once();
    this->Fsm.eval();
    return false;
    }

cTest::RwTest_t::State cTest::RwTest_t::fsmDispatch(
    cTest::RwTest_t::State curState,
    bool fEntry
    )
    {
    State newState = State::stNoChange;

    switch (curState)
        {
    case State::stInitial:
        this->fRunning = true;
        this->Window = this->WindowStart;
        this->pTest->setupLMIC(this->pTest->m_params);

        newState = State::stInitWindow;
        break;

    case State::stInitWindow:
        if (fEntry)
            {
            // calculate the time we'll use for the window. We have to
            // first calculate the half-symbol time.  If bandwidth is 125*2^bw,
            // and sf is 7..12, then hs (usec) is 128 << (sf-5-bw).
            auto const sf = getSf(LMIC.rps);
            ostime_t hsym;

            if (sf == FSK)
                hsym = us2osticksRound(80);
            else
                hsym = us2osticksRound(
                                128 << ((sf - SF7 + 7) -
                                        this->pTest->m_params.Bandwidth - 5)
                                );

            this->WindowAdjust =
                LMICcore_adjustForDrift(
                    this->Window +
                        LMICcore_RxWindowOffset(hsym, LMICbandplan_MINRX_SYMS_LoRa_ClassA),
                    hsym
                    );

            gCatena.SafePrintf(
                "Window %ld us: adjusted %ld us, hsym %ld (%ld us) rxsyms %u (%ld us)\n",
                (long)osticks2us(this->Window),
                (long)osticks2us(this->WindowAdjust),
                (long)hsym,
                (long)osticks2us(hsym),
                LMIC.rxsyms,
                (long)osticks2us(LMIC.rxsyms * hsym * 2)
                );
            }

        newState = State::stWaitForTrigger;
        break;

    case State::stWaitForTrigger:
        if (fEntry)
            {
            // nothing.
            }

        if (this->pTest->m_fStopTest)
            newState = State::stFinal;
        else if (this->DigIn.poll(this->tEdge))
            newState = State::stWaitForWindow;
        break;

    case State::stWaitForWindow:
        if (fEntry)
            {
            LMIC.rxtime = this->tEdge + this->WindowAdjust;
            }

        if (this->pTest->m_fStopTest)
            newState = State::stFinal;
        else if (os_getTime() - (LMIC.rxtime - RX_RAMPUP) > 0)
            newState = State::stRxWindow;
        break;

    case State::stRxWindow:
        if (fEntry)
            {
            // set the callback function.
            LMIC.osjob.func =
                [](osjob_t *job) -> void
                    {
                    gTest.m_RwTest.fRxComplete = true;
                    };

            // start the transmit
            this->fRxComplete = false;
            os_radio(RADIO_RX);
            }

        if (this->pTest->m_fStopTest)
            {
            os_clearCallback(&LMIC.osjob);
            os_radio(RADIO_RST);
            newState = State::stFinal;
            }
        else if (this->fRxComplete)
            newState = State::stRxEval;
        break;

    case State::stRxEval:
        if (fEntry)
            {
            bool fDone;

            // accumulate stats.
            ++this->nTries;
            if (LMIC.dataLen != 0)
                {
                ++this->nGood;
                gCatena.SafePrintf("+");
                }
            else
                {
                gCatena.SafePrintf("-");
                }
            fDone = false;
            if (this->nTries >= this->Count)
                {
                // print
                gCatena.SafePrintf("\nwindow %6u: received %u/%u\n",
                    osticks2us(this->Window),
                    this->nGood,
                    this->nTries
                    );

                // accumulate stats
                this->nGoodTotal += this->nGood;
                this->nTriesTotal += this->nTries;

                this->nGood = this->nTries = 0;

                // go to next window value.
                this->Window += this->WindowStep;

                // check whether we're done.
                if (this->WindowStep >= 0)
                    fDone = (this->Window > this->WindowStop);
                else
                    fDone = (this->Window < this->WindowStop);

                // Do the appropriate state transition.
                if (fDone)
                    newState = State::stFinal;
                else
                    newState = State::stInitWindow;
                }

            // otherwise, start next receive using current window.
            else
                newState = State::stWaitForTrigger;
            }
        break;

    default:
        newState = State::stFinal;
        break;

    case State::stFinal:
        if (fEntry)
            {
            this->fRunning = false;
            this->nGoodTotal += this->nGood;
            this->nTriesTotal += this->nTries;
            gCatena.SafePrintf("total: received %u/%u\n",
                this->nGoodTotal,
                this->nTriesTotal
                );
            }
        break;
        }

    return newState;
    }

bool cTest::handleLmicEvent(const char *pMessage)
    {
    if (pMessage == nullptr)
        return false;

    if (pMessage[0] == '*')
        {
        // turn off GPIOs
        this->m_RxDigOut.off();
        this->m_TxDigOut.off();
        return true;
        }
    else if (pMessage[0] == '+')
        {
        if (pMessage[1] == 'R')
            this->m_RxDigOut.on();
        else if (pMessage[1] == 'T')
            this->m_TxDigOut.on();

        return true;
        }
    else
        return false;
    }
