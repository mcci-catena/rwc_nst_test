/*

Module: rwc_nst_test.cpp

Function:
    Process the various test commands.

Copyright:
    See accompanying LICENSE file for copyright and license information.

Author:
    Terry Moore, MCCI Corporation   November 2019

*/

#include "rwc_nst_test_cmd.h"

#include "rwc_nst_test.h"
#include <strings.h>

using namespace McciCatena;

/*

Name:   ::cmdTxTest()

Function:
    Command dispatcher for "tx" command.

Definition:
    McciCatena::cCommandStream::CommandFn cmdTxTest;

    McciCatena::cCommandStream::CommandStatus cmdTxTest(
        cCommandStream *pThis,
        void *pContext,
        int argc,
        char **argv
        );

Description:
    The "tx" command takes no arguments. It starts a transmit
    test.

Returns:
    cCommandStream::CommandStatus::kSuccess if successful.
    Some other value for failure.

*/

// argv[0] is the matched command name.

cCommandStream::CommandStatus cmdTxTest(
    cCommandStream *pThis,
    void *pContext,
    int argc,
    char **argv
    )
    {

    if (argc != 1)
        return cCommandStream::CommandStatus::kInvalidParameter;

    if (! gTest.evSendStartTx())
        {
        pThis->printf("busy\n");
        return cCommandStream::CommandStatus::kError;
        }

    return cCommandStream::CommandStatus::kSuccess;
    }
    
/*

Name:   ::cmdRxTest()

Function:
    Command dispatcher for "rx" command.

Definition:
    McciCatena::cCommandStream::CommandFn cmdTxTest;

    McciCatena::cCommandStream::CommandStatus cmdTxTest(
        cCommandStream *pThis,
        void *pContext,
        int argc,
        char **argv
        );

Description:
    The "rx" command takes no arguments. It starts a transmit
    test.

Returns:
    cCommandStream::CommandStatus::kSuccess if successful.
    Some other value for failure.

*/

// argv[0] is the matched command name.

cCommandStream::CommandStatus cmdRxTest(
    cCommandStream *pThis,
    void *pContext,
    int argc,
    char **argv
    )
    {

    if (argc != 1)
        return cCommandStream::CommandStatus::kInvalidParameter;

    if (! gTest.evSendStartRx())
        {
        pThis->printf("busy\n");
        return cCommandStream::CommandStatus::kError;
        }

    return cCommandStream::CommandStatus::kSuccess;
    }

/*

Name:   ::cmdRxCount()

Function:
    Command dispatcher for "count" command.

Definition:
    McciCatena::cCommandStream::CommandFn cmdRxCount;

    McciCatena::cCommandStream::CommandStatus cmdRxCount(
        cCommandStream *pThis,
        void *pContext,
        int argc,
        char **argv
        );

Description:
    The "count" command takes no arguments. It prints out the current
    received-packet count.

Returns:
    cCommandStream::CommandStatus::kSuccess if successful.
    Some other value for failure.

*/

// argv[0] is the matched command name.

cCommandStream::CommandStatus cmdRxCount(
    cCommandStream *pThis,
    void *pContext,
    int argc,
    char **argv
    )
    {

    if (argc != 1)
        return cCommandStream::CommandStatus::kInvalidParameter;

    gTest.evStopTest();

    pThis->printf("RxCount: %u\n", gTest.getRxCount());

    return cCommandStream::CommandStatus::kSuccess;
    }

/*

Name:   ::cmdParam()

Function:
    Command dispatcher for "param" command.

Definition:
    McciCatena::cCommandStream::CommandFn cmdParam;

    McciCatena::cCommandStream::CommandStatus cmdParam(
        cCommandStream *pThis,
        void *pContext,
        int argc,
        char **argv
        );

Description:
    The "param" command has three forms:

    1. "param" by itself displays all the parameters
    2. "param x" displays parameter x (only)
    3. "param x v" sets x to v.

Returns:
    cCommandStream::CommandStatus::kSuccess if successful.
    Some other value for failure.

*/

// argv[0] is the matched command name.
cCommandStream::CommandStatus cmdParam(
    cCommandStream *pThis,
    void *pContext,
    int argc,
    char **argv
    )
    {
    switch (argc)
        {
    default:
        return cCommandStream::CommandStatus::kInvalidParameter;

    case 1:
        for (auto & p : cTest::ParamInfo)
            {
            char buf[64];
            if (gTest.getParam(p.name, buf, sizeof(buf)))
                pThis->printf("%s: %s\n", p.name, buf);
            }
        break;
    
    case 2:
        {
        char buf[64];
        if (gTest.getParam(argv[1], buf, sizeof(buf)))
            pThis->printf("%s\n", buf);
        else if (strcasecmp(argv[1], "help") == 0|| argv[1][0] == '?')
            {
            for (auto & p : cTest::ParamInfo)
                {
                pThis->printf("%s: %s\n", p.name, p.help);
                }
            }
        else
            return cCommandStream::CommandStatus::kInvalidParameter;
        }
        break;

    case 3:
        {
        if (! gTest.setParam(argv[1], argv[2]))
            return cCommandStream::CommandStatus::kInvalidParameter;
        }
        break;
        }

    return cCommandStream::CommandStatus::kSuccess;
    }
