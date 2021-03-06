/*

Module:  rwc_nst_test.ino

Function:
    Non-signaling-mode test sketch.

Copyright notice and License:
    See LICENSE file accompanying this project.

Author:
    Terry Moore, MCCI Corporation	2019

*/

#include "rwc_nst_test.h"
#include "rwc_nst_test_cmd.h"
#include "rwc_nst_test_lmiclog.h"
#include <lmic.h>
#include <hal/hal.h>
#include <arduino_lmic_hal_boards.h>

using namespace McciCatena;

#if ARDUINO_LMIC_VERSION < ARDUINO_LMIC_VERSION_CALC(3,0,99,10)
# error "This sketch requires v3.99.0.10 or later of the LMIC"
#endif

/****************************************************************************\
|
|   Manifest constants
|
\****************************************************************************/


/****************************************************************************\
|
|   Variables
|
\****************************************************************************/

Catena gCatena;
Catena::LoRaWAN gLoRaWAN;
#if defined(ARDUINO_ARCH_STM32)
SPIClass gSPI2(
    Catena::PIN_SPI2_MOSI,
    Catena::PIN_SPI2_MISO,
    Catena::PIN_SPI2_SCK
    );
Catena_Mx25v8035f gFlash;
#endif
cTest gTest;
cEventQueue eventQueue;

/****************************************************************************\
|
|   Setup
|
\****************************************************************************/

void setup()
    {
    setup_platform();
    setup_printSignOn();

    setup_flash();

    setup_lmic();
    setup_commands();
    setup_test();
    }

void setup_platform()
    {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, 1);

    gCatena.begin();

    digitalWrite(LED_BUILTIN, 0);

    while (! Serial)
        {
        /* wait for USB attach */
        yield();
        digitalWrite(LED_BUILTIN, 1);
        delay(100);
        digitalWrite(LED_BUILTIN, 0);
        delay(900);
        }
    }

void setup_printSignOn()
    {
    static const char dashes[] = "--------------------------------------";

    gCatena.SafePrintf("\n%s%s\n", dashes, dashes);

    gCatena.SafePrintf("This is %s v%d.%d.%d.%d.\n",
        filebasename(__FILE__),
        getMajor(kAppVersion), getMinor(kAppVersion), getPatch(kAppVersion), getLocal(kAppVersion)
        );

    do
        {
        char sRegion[16];
        gCatena.SafePrintf("Target network: %s / %s\n",
                        gLoRaWAN.GetNetworkName(),
                        gLoRaWAN.GetRegionString(sRegion, sizeof(sRegion))
                        );
        } while (0);

    gCatena.SafePrintf("System clock rate is %u.%03u MHz\n",
        ((unsigned)gCatena.GetSystemClockRate() / (1000*1000)),
        ((unsigned)gCatena.GetSystemClockRate() / 1000 % 1000)
        );
    gCatena.SafePrintf("Enter 'help' for a list of commands.\n");
    gCatena.SafePrintf("Please select 'Line Ending: Newline' in the IDE monitor window.\n");
    gCatena.SafePrintf("If using a terminal emulator, please turn off local echo.\n");

    gCatena.SafePrintf("%s%s\n" "\n", dashes, dashes);
    }

void setup_flash(void)
    {
#ifdef ARDUINO_ARCH_STM32
    if (gFlash.begin(&gSPI2, Catena::PIN_SPI2_FLASH_SS))
        {
        gFlash.powerDown();
        gCatena.SafePrintf("FLASH found, put power down\n");
        }
    else
        {
        gFlash.end();
        gSPI2.end();
        gCatena.SafePrintf("No FLASH found: check hardware\n");
        }
#endif
    }

void setup_lmic()
    {
    // initialize runtime env
    // don't die mysteriously; die noisily.
    const lmic_pinmap *pPinMap = Arduino_LMIC::GetPinmap_ThisBoard();

    if (pPinMap == nullptr) 
        {
        for (;;) 
            {
            // flash lights, sleep.
            for (int i = 0; i < 5; ++i) 
                {
                digitalWrite(LED_BUILTIN, 1);
                delay(100);
                digitalWrite(LED_BUILTIN, 0);
                delay(900);
                }
            Serial.println(F("board not known to library; add pinmap or update getconfig_thisboard.cpp"));
            }
        }

    if (! eventQueue.kLmicLoggingEnabled)
        Serial.println(F("**Warning: LMIC logging not enabled: GPIO toggling ineffective. Add LMIC_ENABLE_event_logging=1 to config.**"));

    eventQueue.begin();
    os_init_ex(pPinMap);
    }

void setup_test()
    {
    gTest.begin();
    }

/****************************************************************************\
|
|   Loop
|
\****************************************************************************/

void loop()
    {
    gCatena.poll();
    }
