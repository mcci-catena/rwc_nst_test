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

const cTest::ParamInfo_t cTest::ParamInfo[] =
    {
    { ParamKey::Bandwidth,          "Bandwidth",          "125, 250, or 500 (kHz)" },
    { ParamKey::ClockError,         "ClockError",         "clock error (%)" },
    { ParamKey::CodingRate,         "CodingRate",         "coding rate (4/8, 5/8, 6/8, 7/8)" },
    { ParamKey::Freq,               "Frequency",          "test frequency (Hz)" },
    { ParamKey::RxRssiDbMax,        "LBT.dB",             "listen-before-talk maximum signal (dB)" },
    { ParamKey::RxRssiIntervalUs,   "LBT.time",           "listen-before-talk measurement time (us)" },
    { ParamKey::RxTimeout,          "RxTimeout",          "receive timeout (ms)" },
    { ParamKey::SpreadingFactor,    "SpreadingFactor",    "7-12 or FSK" },
    { ParamKey::TxInterval,         "TxInterval",         "transmit interval (ms)" },
    { ParamKey::TxPower,            "TxPower",            "transmit power (dB)" },
    { ParamKey::TxTestCount,        "TxTestCount",        "transmit test repeat count" },
    };

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

    float clockError = std::abs(params.ClockError * MAX_CLOCK_ERROR / 100.0) + 0.5;
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
        ", TxPwr %d dB, CR 4/%u, CRC=%u, LBT=%u us/%d dB, clockError=%u.%u (0x%x)\n",
        LMIC.radio_txpow,
        getCr(LMIC.rps) + 5 - CR_4_5,
        ! getNocrc(LMIC.rps),
        osticks2us(LMIC.lbt_ticks),
        LMIC.lbt_dbmax,
        ceppk / 10, ceppk % 10, LMIC.client.clockError
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

        gCatena.SafePrintf("Start TX test: %u bytes, ", this->m_Tx.nData);

        if (m_Tx.fContinuous)
            gCatena.SafePrintf("continous");
        else
            gCatena.SafePrintf("%u packets", this->m_Tx.Count);

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
    else
        {
        // reset the radio.
        os_radio(RADIO_RST);

        // print a dot
        gCatena.SafePrintf("<tx>\n");

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

        gCatena.SafePrintf("Start RX test: capturing raw downlink ");
        if (m_Rx.fContinuous)
            gCatena.SafePrintf("until canceled");
        else
            gCatena.SafePrintf("for %u milliseconds", this->m_params.RxTimeout);

        gCatena.SafePrintf(
            ".\n"
            "At RWC5020, select NST>Signal Generator, then Run.\n"
            );

        // setup LMIC and print settings
        this->setupLMIC(this->m_params);

        // if there's a timeout, start it.
        if (! m_Rx.fContinuous)
            {
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

void cTest::evStopTest()
    {
    this->m_fStopTest = true;
    this->m_fsm.eval();
    }

bool cTest::getParam(const char *pKey, char *pBuf, size_t nBuf) const
    {
    for (auto &p : cTest::ParamInfo)
        {
        if (strcasecmp(pKey, p.getName()) == 0)
            return cTest::getParamByKey(p.getKey(), pBuf, nBuf);
        }
    
    return false;
    }

bool cTest::getParamByKey(cTest::ParamKey key, char *pBuf, size_t nBuf) const
    {
    bool fResult = true;

    switch (key)
        {
    case ParamKey::RxTimeout:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", this->m_params.RxTimeout);
        break;

    case ParamKey::TxInterval:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", this->m_params.TxInterval);
        break;

    case ParamKey::RxRssiIntervalUs:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", this->m_params.RxRssiIntervalUs);
        break;

    case ParamKey::TxTestCount:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", this->m_params.TxTestCount);
        break;

    case ParamKey::Freq:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", this->m_params.Freq);
        break;

    case ParamKey::ClockError:
        {
        unsigned ceppk = std::abs(this->m_params.ClockError * 10) + 0.5;
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u.%u%%", ceppk / 10, ceppk % 10);
        }
        break;

    case ParamKey::CodingRate:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "4/%u", this->m_params.CodingRate + 5 - CR_4_5);
        break;

    case ParamKey::SpreadingFactor:
        if (this->m_params.SpreadingFactor == FSK)
            McciAdkLib_Snprintf(pBuf, nBuf, 0, "FSK");
        else
            McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", this->m_params.SpreadingFactor + 7 - SF7);
        break;

    case ParamKey::Bandwidth:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", 125 << this->m_params.Bandwidth);
        break;

    case ParamKey::RxRssiDbMax:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%d", this->m_params.RxRssiDbMax);
        break;

    case ParamKey::TxPower:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%d", this->m_params.TxPower);
        break;

    default:
        fResult = false;
        break;
        }

    return fResult;
    }

bool cTest::setParam(const char *pKey, const char *pValue) 
    {
    for (auto &p : cTest::ParamInfo)
        {
        if (strcasecmp(pKey, p.getName()) == 0)
            return cTest::setParamByKey(p.getKey(), pValue);
        }
    
    return false;
    }

static bool parseUnsigned(const char *pValue, size_t nValue, std::uint32_t &result)
    {
    if (nValue == 0)
        return false;

    bool fOverflow = false;
    std::uint32_t nonce;
    bool fResult = McciAdkLib_BufferToUint32(pValue, nValue, 10, &nonce, &fOverflow) == nValue && fOverflow == false;
    if (fResult)
        result = nonce;
    return fResult;
    }

static bool parseUnsignedPartial(
    const char *pValue, 
    size_t nValue, 
    std::uint32_t &result, 
    size_t &nParsed
    )
    {
    if (nValue == 0)
        return false;

    bool fOverflow = false;
    nParsed = McciAdkLib_BufferToUint32(pValue, nValue, 10, &result, &fOverflow);
    return ! fOverflow;
    }

static bool parse_int8(
    const char *pValue, 
    size_t nValue, 
    std::int8_t &result
    )
    {
    std::uint32_t nonce;
    bool fMinus;
    
    fMinus = false;
    if (nValue > 0 && pValue[0] == '-')
        {
        ++pValue, --nValue;
        fMinus = true;
        }
    if (! (nValue > 0 && '0' <= pValue[0] && pValue[0] <= '9'))
        return false;

    bool fResult = parseUnsigned(pValue, nValue, nonce);
    if (fResult && fMinus && nonce <= 128)
        result = std::int8_t(-(int)nonce);
    else if (fResult && !fMinus && nonce <= 127)
        {
        result = std::int8_t(nonce);
        }
    else
        {
        fResult = false;
        }

    return fResult;
    }

bool cTest::setParamByKey(cTest::ParamKey key, const char *pValue)
    {
    bool fResult = true;
    size_t nValue = strlen(pValue);

    switch (key)
        {
    case ParamKey::RxTimeout:
        fResult = parseUnsigned(pValue, nValue, this->m_params.RxTimeout);
        break;

    case ParamKey::TxInterval:
        fResult = parseUnsigned(pValue, nValue, this->m_params.TxInterval);
        break;

    case ParamKey::RxRssiIntervalUs:
        fResult = parseUnsigned(pValue, nValue, this->m_params.RxRssiIntervalUs);
        break;

    case ParamKey::TxTestCount:
        fResult = parseUnsigned(pValue, nValue, this->m_params.TxTestCount);
        break;

    case ParamKey::Freq:
        fResult = parseUnsigned(pValue, nValue, this->m_params.Freq);
        break;

    case ParamKey::ClockError:
        {
        size_t nParsed;
        std::uint32_t nonce;
        float clockError;

        if (nValue > 0 && pValue[nValue-1] == '%')
            --nValue;

        if (nValue > 0 && pValue[0] == '.')
            {
            // only have a fraction
            nParsed = 0;
            nonce = 0;
            fResult = true;
            }
        else
            {
            fResult = parseUnsignedPartial(pValue, nValue, nonce, nParsed);
            }

        if (fResult)
            {
            clockError = nonce;
            }
    
        if (fResult && nParsed < nValue && pValue[nParsed] == '.')
            {
            // scan the fraction.
            pValue += nParsed + 1;
            nValue -= nParsed + 1;

            if (nValue > 0)
                {
                fResult = parseUnsignedPartial(pValue, nValue, nonce, nParsed);
                if (nParsed != nValue || nParsed > 6)
                    fResult = false;
                else
                    {
                    unsigned adjust = 10;
                    for (auto i = nParsed - 1; i > 0; --i)
                        adjust *= 10;
                    clockError = (clockError * adjust + nonce) / adjust;
                    }
                }
            }

        if (fResult && ! (0.0f <= clockError && clockError <= 100.0))
            fResult = false;

        this->m_params.ClockError = clockError;
        break;
        }

    case ParamKey::CodingRate:
        {
        std::uint32_t nonce;
        if (! (nValue >= 3 && pValue[0] == '4' && pValue[1] == '/'))
            {
            fResult == false;
            break;
            }

        fResult = parseUnsigned(pValue + 2, nValue - 2, nonce);
        if (! (fResult && 5 <= nonce && nonce <= 8))
            {
            fResult = false;
            break;
            }
        this->m_params.CodingRate = cr_t(nonce - 5 + CR_4_5);
        }
        break;

    case ParamKey::SpreadingFactor:
        if (strcasecmp(pValue, "fsk") == 0)
            this->m_params.SpreadingFactor = FSK;
        else
            {
            std::uint32_t nonce;
            fResult = parseUnsigned(pValue, nValue, nonce);
            if (! (fResult && 7 <= nonce && nonce <= 12))
                {
                fResult = false;
                break;
                }

            this->m_params.SpreadingFactor = sf_t(nonce + (SF7 - 7));
            }
        break;

    case ParamKey::Bandwidth:
        {
        std::uint32_t nonce;

        fResult = parseUnsigned(pValue, nValue, nonce);
        if (! fResult)
            break;
        switch (nonce)
            {
        case 125:
            this->m_params.Bandwidth = BW125;
            break;
        case 250:
            this->m_params.Bandwidth = BW250;
            break;
        case 500:
            this->m_params.Bandwidth = BW500;
            break;
        default:
            fResult = false;
            break;
            }
        }
        break;

    case ParamKey::RxRssiDbMax:
        fResult = parse_int8(pValue, nValue, this->m_params.RxRssiDbMax);
        break;

    case ParamKey::TxPower:
        fResult = parse_int8(pValue, nValue, this->m_params.TxPower);
        break;

    default:
        fResult = false;
        break;
        }

    return fResult;
    }
