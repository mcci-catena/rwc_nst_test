/*

Module:  rwc_nst_test_lmiclog.cpp

Function:
    LMIC logging for non-signalling test app.

Copyright notice and License:
    See LICENSE file accompanying this project.

Author:
    Terry Moore, MCCI Corporation	2019

*/

#include "rwc_nst_test_lmiclog.h"

#include "rwc_nst_test.h"
#include <mcciadk_baselib.h>

#if LMIC_ENABLE_event_logging

void LMICOS_logEvent(const char *pMessage)
    {
    gTest.handleLmicEvent(pMessage);
    eventQueue.putEvent(ev_t(-1), pMessage);
    }

void LMICOS_logEventUint32(const char *pMessage, uint32_t datum)
    {
    gTest.handleLmicEvent(pMessage);
    eventQueue.putEvent(ev_t(-2), pMessage, datum);
    }

#endif // LMIC_ENABLE_event_logging

/*
|| for convenience, handle assertion failures.
*/
static hal_failure_handler_t log_assertion;

static void log_assertion(const char *pMessage, uint16_t line) {
    eventQueue.putEvent(ev_t(-3), pMessage, line);
    eventQueue.printAll();
    gCatena.SafePrintf("***HALTED BY ASSERT***\n");
    while (true)
        yield();
}

void cEventQueue::begin()
    {
    hal_set_failure_handler(log_assertion);
    }

const char *cEventQueue::eventnode_t::getSfName() const
    {
    const char * const t[] = { "FSK", "SF7", "SF8", "SF9", "SF10", "SF11", "SF12", "SFrfu" };
    return t[getSf(this->rps)];
    }

const char *cEventQueue::eventnode_t::getBwName() const
    {
    const char * const t[] = { "BW125", "BW250", "BW500", "BWrfu" };
    return t[getBw(this->rps)];
    }

const char *cEventQueue::eventnode_t::getCrName() const
    {
    const char * const t[] = { "CR 4/5", "CR 4/6", "CR 4/7", "CR 4/8" };
    return t[getCr(this->rps)];
    }

const char *cEventQueue::eventnode_t::getCrcName() const
    {
    return getNocrc(this->rps) ? "NoCrc" : "Crc";
    }

void cEventQueue::eventnode_t::printFreq() const
    {
    gCatena.SafePrintf(": freq=%u.%u", freq / 1000000, freq % 1000000 / 100000);
    }

void cEventQueue::eventnode_t::printRps() const
    {
    gCatena.SafePrintf(" rps=0x%02x (%s %s %s %s IH=%u)",
        rps,
        this->getSfName(),
        this->getBwName(),
        this->getCrName(),
        this->getCrcName(),
        unsigned(getIh(this->rps))
        );
    }

void cEventQueue::eventnode_t::printOpmode(char sep) const
    {
    char s[2] = { sep, '\0' };
    gCatena.SafePrintf("%s opmode=%04x", s, this->opmode);
    }

void cEventQueue::eventnode_t::printTxend() const
    {
    gCatena.SafePrintf(", txend=%u, avail=%d", this->txend, this->globalDutyAvail);
    }

void cEventQueue::eventnode_t::printTxChnl() const
    {
    gCatena.SafePrintf(": ch=%u", this->txChnl);
    }

void cEventQueue::eventnode_t::printDatarate() const
    {
    gCatena.SafePrintf(", datarate=%u", unsigned(datarate));
    }

void cEventQueue::eventnode_t::printTxrxflags() const
    {
    gCatena.SafePrintf(
        ", txrxFlags=0x%02x%s",
        this->txrxFlags,
        (this->txrxFlags & TXRX_ACK ? "; Received ack" : "")
        );
    }

void cEventQueue::eventnode_t::printSaveIrqFlags() const
    {
    gCatena.SafePrintf(", saveIrqFlags 0x%02x", this->saveIrqFlags);
    }

void cEventQueue::eventnode_t::printFcnts() const
    {
    gCatena.SafePrintf(", FcntUp=%04x, FcntDn=%04x", this->fcntUp, this->fcntDn);
    }

// dump all the registers.
void cEventQueue::printAllRegisters(void)
    {
    uint8_t regbuf[0x80];
    regbuf[0] = 0;
    hal_spi_read(1, regbuf + 1, sizeof(regbuf) - 1);

    for (unsigned i = 0; i < sizeof(regbuf); ++i)
        {
        if (i % 16 == 0)
            {
            gCatena.SafePrintf("\n%02x", i);
            }
        gCatena.SafePrintf(
            "%s%02x",
            (((i % 8) == 0) ? " - " : " "),
            regbuf[i]
            );
        }

    // reset the radio, just in case the register dump caused issues.
    hal_pin_rst(0);
    delay(2);
    hal_pin_rst(2);
    delay(6);

    // restore the radio to idle.
    const uint8_t opmode = 0x88;    // LoRa and sleep.
    hal_spi_write(0x81, &opmode, 1);
    }

void cEventQueue::eventnode_t::print() const
    {
    ev_t ev = this->event;

    gCatena.SafePrintf("%ld (%ld ms): ",
        long(this->time),
        long(osticks2ms(this->time))
        );

    if (ev == ev_t(-1) || ev == ev_t(-2))
        {
        gCatena.SafePrintf("%s", this->pMessage);
        if (ev == ev_t(-2))
            {
            gCatena.SafePrintf(", datum=0x%lx", (unsigned long)(this->datum));
            }
        this->printOpmode('.');
        }
    else if (ev == ev_t(-3))
        {
        gCatena.SafePrintf("%s, line %lu", this->pMessage, (unsigned long)(this->datum));
        this->printFreq();
        this->printTxend();
        this->printTxChnl();
        this->printRps();
        this->printOpmode(',');
        this->printTxrxflags();
        this->printSaveIrqFlags();
        this->printAllRegisters();
        }
    else
        {
        static const char evNames[] = LMIC_EVENT_NAME_MULTISZ__INIT;

        auto const pName = McciAdkLib_MultiSzIndex(evNames, ev);

        if (pName[0] != '\0')
            {
            gCatena.SafePrintf("%s", pName);
            }
        else
            {
            gCatena.SafePrintf("Unknown event: %u", unsigned(ev));
            }

        switch(ev)
            {
        case EV_SCAN_TIMEOUT:
            break;
        case EV_BEACON_FOUND:
            break;
        case EV_BEACON_MISSED:
            break;
        case EV_BEACON_TRACKED:
            break;
        case EV_JOINING:
            break;

        case EV_JOINED:
            printTxChnl();
            gCatena.SafePrintf("\n");
            do  {
                u4_t netid = 0;
                devaddr_t devaddr = 0;
                u1_t nwkSKey[16];
                u1_t appSKey[16];
                LMIC_getSessionKeys(&netid, &devaddr, nwkSKey, appSKey);
                gCatena.SafePrintf("netid: %u devaddr %08lx\nnwkSKey: ",
                    unsigned(netid), (unsigned long) devaddr
                    );
                for (size_t i=0; i<sizeof(nwkSKey); ++i)
                    {
                    gCatena.SafePrintf("%s%02x",
                        i != 0 ? "-" : "",
                        nwkSKey[i]
                        );
                    }
                gCatena.SafePrintf("\nappSKey: ");
                for (size_t i=0; i<sizeof(appSKey); ++i)
                    {
                    gCatena.SafePrintf("%s%02x",
                        i != 0 ? "-" : "",
                        appSKey[i]
                        );
                    }
                } while (0);
            break;

        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
        case EV_JOIN_FAILED:
            // print out rx info
            this->printFreq();
            this->printRps();
            this->printOpmode();
            this->printAllRegisters();
            break;

        case EV_REJOIN_FAILED:
            // this event means that someone tried a rejoin, and it failed.
            // it doesn't really mean anything bad, it's just advisory.
            break;

        case EV_TXCOMPLETE:
            this->printTxChnl();
            this->printRps();
            this->printTxrxflags();
            this->printFcnts();
            this->printTxend();
            break;
        case EV_LOST_TSYNC:
            break;
        case EV_RESET:
            break;
        case EV_RXCOMPLETE:
            break;
        case EV_LINK_DEAD:
            break;
        case EV_LINK_ALIVE:
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            // this event tells us that a transmit is about to start.
            // but printing here is bad for timing.
            this->printTxChnl();
            this->printRps();
            this->printDatarate();
            this->printOpmode();
            this->printTxend();
            break;

        case EV_RXSTART:
            this->printFreq();
            this->printRps();
            this->printDatarate();
            this->printOpmode();
            this->printTxend();
            gCatena.SafePrintf(", delta ms %ld, rxsyms=%u",
                (long)(osticks2ms(this->time - this->txend)),
                unsigned(this->rxsyms)
                );
            break;

        case EV_JOIN_TXCOMPLETE:
            this->printSaveIrqFlags();
            break;

        default:
            break;
            } // end case
        }

    gCatena.SafePrintf("\n");
    }
