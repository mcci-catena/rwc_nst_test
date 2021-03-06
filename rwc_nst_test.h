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

#ifdef ARDUINO_ARCH_STM32
# include <Catena_Mx25v8035f.h>
#endif

#include "rwc_nst_test_cTest.h"
#include "rwc_nst_test_version.h"
#include <SPI.h>
#include <arduino_lmic.h>

/****************************************************************************\
|
|   kAppVersion: the application version.
|
\****************************************************************************/

constexpr std::uint32_t kAppVersion = ::makeVersion(0,8,0,0);

/****************************************************************************\
|
|   handy constexpr to extract the base name of a file
|
\****************************************************************************/

// two-argument version: first arg is what to return if we don't find
// a directory separator in the second part.
static constexpr const char *filebasename(const char *s, const char *p)
    {
    return p[0] == '\0'                     ? s                            :
           (p[0] == '/' || p[0] == '\\')    ? filebasename(p + 1, p + 1)   :
                                              filebasename(s, p + 1)       ;
    }

static constexpr const char *filebasename(const char *s)
    {
    return filebasename(s, s);
    }

/****************************************************************************\
|
|   Globals
|
\****************************************************************************/

extern McciCatena::Catena gCatena;
extern McciCatena::Catena::LoRaWAN gLoRaWAN;
extern SPIClass gSPI2;
#if defined(ARDUINO_ARCH_STM32)
extern McciCatena::Catena_Mx25v8035f gFlash;
#endif
extern cTest gTest;

#endif // _rwc_nst_test_h_
