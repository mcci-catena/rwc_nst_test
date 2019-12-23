/*

Module:  rwc_nst_test_cTest.h

Function:
  Auto-configured raw test example, for Adafruit Feather M0 LoRa

Copyright notice and License:
  See LICENSE file accompanying this project.

Author:
    Terry Moore, MCCI Corporation	2019

*/

#ifndef _rwc_nst_test_cTest_h_
# define _rwc_nst_test_cTest_h_

#pragma once

#include <Catena_PollableInterface.h>
#include <Catena_FSM.h>
#include <arduino_lmic.h>

/****************************************************************************\
|
|   cTest: the test object.
|
\****************************************************************************/

// the test class object
class cTest : public McciCatena::cPollableObject
    {
    // constructor
private:
    static constexpr std::uint32_t kRxTimeoutMsDefault      = 5000;
    static constexpr std::uint32_t kTxIntervalMsDefault     = 2000;
    static constexpr std::uint32_t kRxRssiIntervalUsDefault = 0;    // LBT disabled
    static constexpr unsigned kTxTestCountDefault           = 3;
    static constexpr std::uint32_t kDefaultFreq             = 902300000;
    static constexpr cr_t kDefaultCodingRate                = CR_4_5;
    static constexpr sf_t kDefaultSpreadingFactor           = SF7;
    static constexpr bw_t kDefaultBandwidth                 = BW125;
    static constexpr std::int8_t kDefaultRxRssiDbMax        = -80;
    static constexpr float kDefaultClockError               = 0.0;  // 0 percent, no error
    static constexpr std::int8_t kDefaultTxPower            = 0;
    static constexpr std::uint16_t kRxSymsDefault           = 6;
    static constexpr std::int8_t kRxDigOutDefault           = -1;
    static constexpr std::int8_t kTxDigOutDefault           = -1;

public:
    struct Params
        {
        std::uint32_t   RxTimeout;
        std::uint32_t   TxInterval;
        std::uint32_t   RxRssiIntervalUs;
        std::uint32_t   TxTestCount;
        std::uint32_t   Freq;
        float           ClockError;
        std::uint16_t   RxSyms;
        cr_t            CodingRate;
        sf_t            SpreadingFactor;
        bw_t            Bandwidth;
        std::int8_t     RxRssiDbMax;
        std::int8_t     TxPower;
        std::int8_t     RxDigOut;
        std::int8_t     TxDigOut;
        };

    enum class ParamKey : std::uint8_t
        {
        RxTimeout,
        TxInterval,
        RxRssiIntervalUs,
        TxTestCount,
        Freq,
        ClockError,
        RxSyms,
        CodingRate,
        SpreadingFactor,
        Bandwidth,
        RxRssiDbMax,
        TxPower,
        RxDigOut,
        TxDigOut,
        Max
        };

    class ParamInfo_t
        {
    private:
        ParamKey        m_key;
        const char *    m_name;
        const char *    m_help;

    public:
        ParamInfo_t(ParamKey key, const char *name, const char *help)
            : m_key(key)
            , m_name(name)
            , m_help(help)
            {}
        ParamKey getKey() const     { return this->m_key; }
        const char *getName() const { return this->m_name; }
        const char *getHelp() const { return this->m_help; }
        };
        
    static const ParamInfo_t ParamInfo[unsigned(ParamKey::Max)];

    enum DebugFlags : std::uint32_t
        {
        kError      = 1 << 0,
        kWarning    = 1 << 1,
        kTrace      = 1 << 2,
        kInfo       = 1 << 3,
        };

private:
    static constexpr Params kDefaultParams() 
        {
        return Params 
            {
            .RxTimeout = kRxTimeoutMsDefault,
            .TxInterval = kTxIntervalMsDefault,
            .RxRssiIntervalUs = kRxRssiIntervalUsDefault,
            .TxTestCount = kTxTestCountDefault,
            .Freq = kDefaultFreq,
            .ClockError = kDefaultClockError,
            .RxSyms = kRxSymsDefault,
            .CodingRate = kDefaultCodingRate,
            .SpreadingFactor = kDefaultSpreadingFactor,
            .Bandwidth = kDefaultBandwidth,
            .RxRssiDbMax = kDefaultRxRssiDbMax,
            .TxPower = kDefaultTxPower,
            .RxDigOut = kRxDigOutDefault,
            .TxDigOut = kTxDigOutDefault,
            };
        };

public:
    cTest() {};

    // neither copyable nor movable
    cTest(const cTest&) = delete;
    cTest& operator=(const cTest&) = delete;
    cTest(const cTest&&) = delete;
    cTest& operator=(const cTest&&) = delete;

    // FSM
    enum class State : std::uint8_t
        {
        stNoChange = 0, // this name must be present: indicates "no change of state"
        stInitial,      // this name must be present: it's the starting state.
        stIdle,         // parked; not doing anything.
        stTxTest,       // running the Tx Test
        stRxTest,       // running the Rx test

        stFinal,        // this name must be present, it's the terminal state.
        };

    enum class Command : std::uint8_t
        {
        None = 0,    // no command has been entered
        StartTx,     // request to start TX test
        StartRx,     // request to start RX test
        };

    static constexpr const char *getStateName(State s)
        {
        switch (s)
            {
        case State::stNoChange: return "stNoChange";
        case State::stInitial:  return "stInitial";
        case State::stIdle:     return "stIdle";
        case State::stTxTest:   return "stTxTest";
        case State::stRxTest:   return "stRxTest";
        case State::stFinal:    return "stFinal";
        default:                return "<<unknown>>";
            }
        }

    //-----------------
    // public methods
    //-----------------
public:
    void begin();
    void end();
    virtual void poll() override;

    // report an event to stop current test.
    void evStopTest();

    bool evSendStartRx() { return this->evSendCommand(Command::StartRx); }
    bool evSendStartTx() { return this->evSendCommand(Command::StartTx); }

    // request an operation
    bool evSendCommand(Command cmd)
        {
        if (this->m_fsm.getState() != State::stIdle ||
            this->m_pendingCmd != Command::None)
            return false;

        this->m_pendingCmd = cmd;
        this->m_fsm.eval();
        return true;
        }

    // get the current RX Count
    unsigned getRxCount() const
        {
        return this->m_Rx.Count;
        }

    // get parameters in text form based on text key
    bool getParam(const char *pKey, char *pBuf, size_t nBuf) const;
    bool getParamByKey(ParamKey key, char *pBuf, size_t nBuf) const;
    bool setParam(const char *pKey, const char *pValue);
    bool setParamByKey(ParamKey key, const char *pValue);

    // return true if a given debug mask is enabled.
    bool isTraceEnabled(DebugFlags mask) const
        {
        return this->m_DebugFlags & mask;
        }

    // process LMIC trace messages for GPIO control
    bool handleLmicEvent(const char *pMessage);

    //-----------------
    // Output handling
    //-----------------
    class cDigOut
        {
    public:
        cDigOut() {};

        void setOutput(int pin, bool fActive)
            {
            if (pin < 0)
                {
                this->off();
                if (this->m_pin >= 0)
                    pinMode(uint32_t(this->m_pin), INPUT);
                }
            else
                {
                if (this->m_pin != pin)
                    {
                    this->off();
                    pinMode(uint32_t(this->m_pin), INPUT);
                    }
                pinMode(pin, OUTPUT);
                digitalWrite(pin, !fActive);
                }
            this->m_pin = pin;
            this->m_fActive = fActive;
            }

        void off() const
            {
            if (this->m_pin >= 0)
                digitalWrite(uint32_t(this->m_pin), !this->m_fActive);
            }
        void on() const
            {
            if (this->m_pin >= 0)
                digitalWrite(this->m_pin, this->m_fActive);
            }
        bool isEnabled() const
            {
            return this->m_pin >= 0;
            }
        bool isConfigured() const
            {
#if LMIC_ENABLE_event_logging
            return true;
#else
            return false;
#endif            
            }

    private:
        int m_pin = -1;
        bool m_fActive = true;
        };

    //-----------------
    // the FSM
    //-----------------
private:
    McciCatena::cFSM<cTest, State> m_fsm;
    // evaluate the control FSM.
    State fsmDispatch(State currentState, bool fEntry);

    //-----------------
    // private methods
    //-----------------
private:
    // run transmit test; return true when done.
    bool txTest(bool fEntry);
    // run receive test; return true when done.
    bool rxTest(bool fEntry);
    // stop the rx test.
    void rxTestStop();
    // set up LMIC from Params
    void setupLMIC(const Params &params);

    static osjobcbfn_t txTestDone;

    //------------------------------------
    // the various operating properties
    //-------------------------------------
private:
    Params      m_params;
    Command     m_pendingCmd;
    // debug flags
    DebugFlags                      m_DebugFlags;

    // true if registered with polling engine
    bool        m_fRegistered: 1;
    bool        m_fRunning: 1;
    bool        m_fExit: 1;
    bool        m_fStopTest: 1;

    struct Tx_t
        {
        // transmission down-counter.
        std::uint32_t Count;
        // time of last transmission
        ostime_t    Tnext;
        bool        fContinuous: 1;
        bool        fIdle: 1;
        std::uint8_t nData;
        std::uint8_t Data[255];
        cDigOut     DigOut;
        };

    Tx_t        m_Tx;

    struct Rx_t
        {
        ostime_t    Timeout;
        unsigned    Count;
        bool        fContinuous: 1;
        bool        fTimedOut: 1;
        bool        fReceiving: 1;
        cDigOut     DigOut;
        osjob_t     TimeoutJob;
        };

    Rx_t        m_Rx;
    };

#endif // _rwc_nst_test_cTest_h_
