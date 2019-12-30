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
#include <cmath>    // for float fabs().

#if __cplusplus < 201703L
 static constexpr float std_fabsf(float f) { return fabs(f); }
#else
 using std_fabsf = std::fabsf;
#endif

const cTest::ParamInfo_t cTest::ParamInfo[] =
    {
    { ParamKey::Bandwidth,          "Bandwidth",          "125, 250, or 500 (kHz)" },
    { ParamKey::ClockError,         "ClockError",         "clock error (%)" },
    { ParamKey::CodingRate,         "CodingRate",         "coding rate (4/8, 5/8, 6/8, 7/8)" },
    { ParamKey::Freq,               "Frequency",          "test frequency (Hz)" },
    { ParamKey::RxRssiDbMax,        "LBT.dB",             "listen-before-talk maximum signal (dB)" },
    { ParamKey::RxRssiIntervalUs,   "LBT.time",           "listen-before-talk measurement time (us)" },
    { ParamKey::RxCount,            "RxCount",            "receive window repeat count" },
    { ParamKey::RxDigIn,            "RxDigIn",            "digital input for rx window test" },
    { ParamKey::RxDigOut,           "RxDigOut",           "digital output to pulse during RX (pin)" },
    { ParamKey::RxSyms,             "RxSyms",             "packet preamble timeout (symbols)" },
    { ParamKey::RxTimeout,          "RxTimeout",          "receive timeout (ms)" },
    { ParamKey::SpreadingFactor,    "SpreadingFactor",    "7-12 or FSK" },
    { ParamKey::TxDigOut,           "TxDigOut",           "digital output to pulse during TX (pin)" },
    { ParamKey::TxGuardUs,          "TxGuardUs",          "transmit window guard time (usec)" },
    { ParamKey::TxInterval,         "TxInterval",         "transmit interval (ms)" },
    { ParamKey::TxPower,            "TxPower",            "transmit power (dB)" },
    { ParamKey::TxPulseMs,          "TxPulseMs",          "transimt window pulse width (ms)" },
    { ParamKey::TxPulseOut,         "TxPulseOut",         "digital output to pulse for timing of TX window (pin)" },
    { ParamKey::TxStartUs,          "TxStartUs",          "transmit window startup calibration time (usec)" },
    { ParamKey::TxTestCount,        "TxTestCount",        "transmit test repeat count" },
    { ParamKey::WindowStart,        "Window.Start",       "receive window start (us)" },
    { ParamKey::WindowStep,         "Window.Step",        "receive window step (us)" },
    { ParamKey::WindowStop,         "Window.Stop",        "receive window stop (us)" },
    };

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
    case ParamKey::RxSyms:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", this->m_params.RxSyms);
        break;

    case ParamKey::RxTimeout:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", this->m_params.RxTimeout);
        break;

    case ParamKey::TxInterval:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", this->m_params.TxInterval);
        break;

    case ParamKey::TxPulseMs:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", this->m_params.TxPulseMs);
        break;

    case ParamKey::TxGuardUs:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", this->m_params.TxGuardUs);
        break;

    case ParamKey::TxStartUs:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", this->m_params.TxStartUs);
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
        unsigned ceppk = std_fabsf(this->m_params.ClockError * 10.0f) + 0.5f;
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u.%u%%", ceppk / 10, ceppk % 10);
        }
        break;

    case ParamKey::RxCount:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%u", this->m_params.RxCount);
        break;

    case ParamKey::WindowStart:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%ld", (long)this->m_params.WindowStart);
        break;

    case ParamKey::WindowStop:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%ld", (long)this->m_params.WindowStop);
        break;

    case ParamKey::WindowStep:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%ld", (long)this->m_params.WindowStep);
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

    case ParamKey::RxDigIn:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%d", this->m_params.RxDigIn);
        break;

    case ParamKey::RxDigOut:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%d", this->m_params.RxDigOut);
        break;

    case ParamKey::TxDigOut:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%d", this->m_params.TxDigOut);
        break;

    case ParamKey::TxPulseOut:
        McciAdkLib_Snprintf(pBuf, nBuf, 0, "%d", this->m_params.TxPulseOut);
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

static bool parseUnsigned16(const char *pValue, size_t nValue, std::uint16_t &result)
    {
    if (nValue == 0)
        return false;

    bool fOverflow = false;
    unsigned long nonce;
    bool fResult = McciAdkLib_BufferToUlong(pValue, nValue, 10, &nonce, &fOverflow) == nValue && fOverflow == false;
    if (fResult)
        {
        if (nonce > UINT16_MAX)
            {
            nonce = UINT16_MAX;
            fOverflow = true;
            }
        result = std::uint16_t(nonce);
        }
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

static bool parse_int32(
    const char *pValue,
    size_t nValue,
    std::int32_t &result
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
    if (fResult && fMinus && nonce <= std::uint32_t(INT32_MAX) + 1)
        result = std::int32_t(-(std::int32_t)nonce);
    else if (fResult && !fMinus && nonce <= INT32_MAX)
        {
        result = std::int32_t(nonce);
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
    case ParamKey::RxSyms:
        fResult = parseUnsigned16(pValue, nValue, this->m_params.RxSyms);
        break;

    case ParamKey::RxTimeout:
        fResult = parseUnsigned(pValue, nValue, this->m_params.RxTimeout);
        break;

    case ParamKey::TxInterval:
        fResult = parseUnsigned(pValue, nValue, this->m_params.TxInterval);
        break;

    case ParamKey::TxPulseMs:
        fResult = parseUnsigned(pValue, nValue, this->m_params.TxPulseMs);
        break;

    case ParamKey::TxGuardUs:
        fResult = parseUnsigned(pValue, nValue, this->m_params.TxGuardUs);
        break;

    case ParamKey::TxStartUs:
        fResult = parseUnsigned(pValue, nValue, this->m_params.TxStartUs);
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

    case ParamKey::RxCount:
        fResult = parseUnsigned(pValue, nValue, this->m_params.RxCount);
        break;

    case ParamKey::WindowStart:
        fResult = parse_int32(pValue, nValue, this->m_params.WindowStart);
        break;

    case ParamKey::WindowStop:
        fResult = parse_int32(pValue, nValue, this->m_params.WindowStop);
        break;

    case ParamKey::WindowStep:
        fResult = parse_int32(pValue, nValue, this->m_params.WindowStep);
        break;

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

     case ParamKey::RxDigIn:
        fResult = parse_int8(pValue, nValue, this->m_params.RxDigIn);
        break;

   case ParamKey::RxDigOut:
        fResult = parse_int8(pValue, nValue, this->m_params.RxDigOut);
        break;

    case ParamKey::TxDigOut:
        fResult = parse_int8(pValue, nValue, this->m_params.TxDigOut);
        break;

    case ParamKey::TxPulseOut:
        fResult = parse_int8(pValue, nValue, this->m_params.TxPulseOut);
        break;

    default:
        fResult = false;
        break;
        }

    return fResult;
    }
