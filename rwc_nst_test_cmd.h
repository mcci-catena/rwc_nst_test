/*

Module:  rwc_nst_test_cmd.h

Function:
  Auto-configured raw test example, for Adafruit Feather M0 LoRa

Copyright notice and License:
  See LICENSE file accompanying this project.

Author:
    Terry Moore, MCCI Corporation	2019

*/

#pragma once

#include <Catena_CommandStream.h>

McciCatena::cCommandStream::CommandFn cmdTxTest;
McciCatena::cCommandStream::CommandFn cmdRxTest;
McciCatena::cCommandStream::CommandFn cmdRxCount;
McciCatena::cCommandStream::CommandFn cmdParam;
