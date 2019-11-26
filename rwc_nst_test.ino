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
#include <lmic.h>
#include <hal/hal.h>
#include <arduino_lmic_hal_boards.h>

using namespace McciCatena;

/****************************************************************************\
|
|   Manifest constants
|
\****************************************************************************/

/****************************************************************************\
|
|   User commands
|
\****************************************************************************/

// the individual commmands are put in this table
static const cCommandStream::cEntry sMyExtraCommmands[] =
        {
        { "tx", cmdTxTest },
        { "rx", cmdRxTest },
        { "count", cmdRxCount },
        { "param", cmdParam },
        // { "debugmask", cmdDebugMask },
        // other commands go here....
        };

/* a top-level structure wraps the above and connects to the system table */
/* it optionally includes a "first word" so you can for sure avoid name clashes */
static cCommandStream::cDispatch
sMyExtraCommands_top(
        sMyExtraCommmands,          /* this is the pointer to the table */
        sizeof(sMyExtraCommmands),  /* this is the size of the table */
        nullptr                     /* this is no "first word" for all the commands in this table */
        );


/****************************************************************************\
|
|   Variables
|
\****************************************************************************/

Catena gCatena;
Catena::LoRaWAN gLoRaWAN;
SPIClass gSPI2(
    Catena::PIN_SPI2_MOSI,
    Catena::PIN_SPI2_MISO,
    Catena::PIN_SPI2_SCK
    );
Catena_Mx25v8035f gFlash;
cTest gTest;

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

    setup_commands();
    setup_test();
    }

void setup_platform()
    {
    gCatena.begin();
    while (! Serial)
        /* wait for USB attach */
        yield();
    }

void setup_printSignOn()
    {
    static const char dashes[] = "------------------------------------";

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
    gCatena.SafePrintf("(remember to select 'Line Ending: Newline' at the bottom of the monitor window.)\n");

    gCatena.SafePrintf("%s%s\n" "\n", dashes, dashes);
    }

void setup_flash(void)
    {
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
    }

void setup_commands()
    {
    /* add our application-specific commands */
    gCatena.addCommands(
        /* name of app dispatch table, passed by reference */
        sMyExtraCommands_top,
        /*
        || optionally a context pointer using static_cast<void *>().
        || normally only libraries (needing to be reentrant) need
        || to use the context pointer.
        */
        nullptr
        );
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
