/*

Module:  rwc_nst_test.h

Function:
  Auto-configured raw test example, for Adafruit Feather M0 LoRa

Copyright notice and License:
  See LICENSE file accompanying this project.

Author:
    Terry Moore, MCCI Corporation	2019

*/

#ifndef _rwc_nst_test_h_
# define _rwc_nst_test_h_

#include <Catena.h>
#include <Catena_CommandStream.h>
#include <Catena_FSM.h>
#include <Catena_Mx25v8035f.h>
#include "rwc_nst_test_cTest.h"
#include "rwc_nst_test_version.h"
#include <SPI.h>
#include <arduino_lmic.h>

/****************************************************************************\
|
|   kAppVersion: the application version.
|
\****************************************************************************/

constexpr std::uint32_t kAppVersion = ::makeVersion(0,6,0,0);

/****************************************************************************\
|
|   handy constexpr to extract the base name of a file
|
\****************************************************************************/

static constexpr const char *filebasename(const char *s)
    {
    const char *pName = s;

    for (auto p = s; *p != '\0'; ++p)
        {
        if (*p == '/' || *p == '\\')
            pName = p + 1;
        }
    return pName;
    }

/****************************************************************************\
|
|   Globals
|
\****************************************************************************/

extern McciCatena::Catena gCatena;
extern McciCatena::Catena::LoRaWAN gLoRaWAN;
extern SPIClass gSPI2;
extern McciCatena::Catena_Mx25v8035f gFlash;
extern cTest gTest;

#endif // _rwc_nst_test_h_
