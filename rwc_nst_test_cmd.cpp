/*

Module: rwc_nst_test_cmd.cpp

Function:
    Process the various test commands.

Copyright:
    See accompanying LICENSE file for copyright and license information.

Author:
    Terry Moore, MCCI Corporation   November 2019

*/

#include "rwc_nst_test_cmd.h"

#include "rwc_nst_test.h"
#include "rwc_nst_test_lmiclog.h"
#include <strings.h>

McciCatena::cCommandStream::CommandFn cmdTxTest;
McciCatena::cCommandStream::CommandFn cmdRxTest;
McciCatena::cCommandStream::CommandFn cmdRxWindowTest;
McciCatena::cCommandStream::CommandFn cmdRxCount;
McciCatena::cCommandStream::CommandFn cmdParam;
McciCatena::cCommandStream::CommandFn cmdLog;
McciCatena::cCommandStream::CommandFn cmdQuit;
McciCatena::cCommandStream::CommandFn cmdTxWindowTest;

using namespace McciCatena;

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
        { "rw", cmdRxWindowTest },
        { "tw", cmdTxWindowTest },
        { "count", cmdRxCount },
        { "param", cmdParam },
        { "log", cmdLog },
        { "q", cmdQuit },
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

/*

Name:   ::setup_commands()

Function:
    setup()-time routine to register commands.

Definition:
    void ::setup_commands(void);

Description:
    The command table is registered.

Returns:
    No explicit result.

*/

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
    McciCatena::cCommandStream::CommandFn cmdRxTest;

    McciCatena::cCommandStream::CommandStatus cmdRxTest(
        cCommandStream *pThis,
        void *pContext,
        int argc,
        char **argv
        );

Description:
    The "rx" command takes no arguments. It starts a receive
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

Name:   ::cmdRxWindowTest()

Function:
    Command dispatcher for "rw" command.

Definition:
    McciCatena::cCommandStream::CommandFn cmdRxWindowTest;

    McciCatena::cCommandStream::CommandStatus cmdRxWindowTest(
        cCommandStream *pThis,
        void *pContext,
        int argc,
        char **argv
        );

Description:
    The "rw" command takes no arguments. It starts a receive window
    test. The receive window test waits for a rising edge on a specified
    digital line (param RxDigIn), and captures the os_getTime() value.
    It then starts a single receive scheduled at `param RxWindow`, using
    RxSyms and ClockError to simulate the LMIC's window. 

    This process repeats (controlled by param RxCount), and counts of pulses
    and successful receives are accumulated.

Returns:
    cCommandStream::CommandStatus::kSuccess if successful.
    Some other value for failure.

*/

// argv[0] is the matched command name.

cCommandStream::CommandStatus cmdRxWindowTest(
    cCommandStream *pThis,
    void *pContext,
    int argc,
    char **argv
    )
    {

    if (argc != 1)
        return cCommandStream::CommandStatus::kInvalidParameter;

    if (! gTest.evSendStartRxWindow())
        {
        pThis->printf("busy\n");
        return cCommandStream::CommandStatus::kError;
        }

    return cCommandStream::CommandStatus::kSuccess;
    }

/*

Name:   ::cmdTxWindowTest()

Function:
    Command dispatcher for "tw" command.

Definition:
    McciCatena::cCommandStream::CommandFn cmdTxWindowTest;

    McciCatena::cCommandStream::CommandStatus cmdTxWindowTest(
        cCommandStream *pThis,
        void *pContext,
        int argc,
        char **argv
        );

Description:
    The "tw" command takes no arguments. It starts the transmit part of
    a receive window test. The transmit window test pulses a specific digital
    output high, then waits a specified period of time and transmits a test
    packet. It does this forever, or until canceled by the 'q' command.
    This is normally coupled with a second Catena running the rw test.

Returns:
    cCommandStream::CommandStatus::kSuccess if successfully started.
    Some other value for failure.

*/

// argv[0] is the matched command name.

cCommandStream::CommandStatus cmdTxWindowTest(
    cCommandStream *pThis,
    void *pContext,
    int argc,
    char **argv
    )
    {

    if (argc != 1)
        return cCommandStream::CommandStatus::kInvalidParameter;

    if (! gTest.evSendStartTxWindow())
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
            if (gTest.getParam(p.getName(), buf, sizeof(buf)))
                pThis->printf("%s: %s\n", p.getName(), buf);
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
                pThis->printf("%s: %s\n", p.getName(), p.getHelp());
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

/*

Name:   ::cmdLog()

Function:
    Command dispatcher for "log" command.

Definition:
    McciCatena::cCommandStream::CommandFn cmdLog;

    McciCatena::cCommandStream::CommandStatus cmdLog(
        cCommandStream *pThis,
        void *pContext,
        int argc,
        char **argv
        );

Description:
    The "log" command has one form

    1. "log" by itself dumps the log.
    2. "log registers" displays the current radio registers.

Returns:
    cCommandStream::CommandStatus::kSuccess if successful.
    Some other value for failure.

*/

// argv[0] is the matched command name.
cCommandStream::CommandStatus cmdLog(
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
        eventQueue.printAll();
        return cCommandStream::CommandStatus::kSuccess;

    case 2:
        if (strcasecmp(argv[1], "registers") == 0)
            {
            eventQueue.printAllRegisters();
            pThis->printf("\n");
            return cCommandStream::CommandStatus::kSuccess;
            }
        return cCommandStream::CommandStatus::kInvalidParameter;
        }
    }

/*

Name:   ::cmdQuit()

Function:
    Command dispatcher for "q" command.

Definition:
    McciCatena::cCommandStream::CommandFn cmdQuit;

    McciCatena::cCommandStream::CommandStatus cmdQuit(
        cCommandStream *pThis,
        void *pContext,
        int argc,
        char **argv
        );

Description:
    The "q" command takes no arguments. It just stops the current test.

Returns:
    cCommandStream::CommandStatus::kSuccess if successful.
    Some other value for failure.

*/

// argv[0] is the matched command name.

cCommandStream::CommandStatus cmdQuit(
    cCommandStream *pThis,
    void *pContext,
    int argc,
    char **argv
    )
    {

    if (argc != 1)
        return cCommandStream::CommandStatus::kInvalidParameter;

    gTest.evStopTest();

    return cCommandStream::CommandStatus::kSuccess;
    }
