//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//

//----------------------------------------------------------
// CommandLine.cpp - tiny very specific command line parser
//----------------------------------------------------------

#include "standardpch.h"
#include "commandline.h"
#include "superpmi.h"
#include "mclist.h"
#include "methodcontext.h"
#include "logging.h"

// NOTE: this is parsed by parallelsuperpmi.cpp::ProcessChildStdOut() to determine if an incorrect
// argument usage error has occurred.
const char* const g_SuperPMIUsageFirstLine      = "SuperPMI is a JIT compiler testing tool.";

void CommandLine::DumpHelp(const char* program)
{
    printf("%s\n", g_SuperPMIUsageFirstLine);
    printf("\n");
    printf("Usage: %s [options] jitname [jitname2] filename.mc\n", program);
    printf(" jitname" PLATFORM_SHARED_LIB_SUFFIX_A " - path of jit to be tested\n");
    printf(" jitname2" PLATFORM_SHARED_LIB_SUFFIX_A " - optional path of second jit to be tested\n");
    printf(" filename.mc - load method contexts from filename.mc\n");
    printf(" -j[it] Name - optionally -jit can be used to specify jits\n");
    printf(" -l[oad] filename - optionally -load can be used to specify method contexts\n");
    printf("\n");
    printf("Options:\n");
    printf("\n");
    printf(" -boe\n");
    printf("     Break on error return from compileMethod\n");
    printf("\n");
    printf(" -boa\n");
    printf("     Break on assert from the JIT\n");
    printf("\n");
    printf(" -v[erbosity] messagetypes\n");
    printf("     Controls which types of messages SuperPMI logs. Specify a string of\n");
    printf("     characters representing message categories to enable, where:\n");
    printf("         e - errors (internal fatal errors that are non-recoverable)\n");
    printf("         w - warnings (internal conditions that are unusual, but not serious)\n");
    printf("         m - missing (failures due to missing JIT-EE interface details)\n");
    printf("         i - issues (issues found with the JIT, e.g. asm diffs, asserts)\n");
    printf("         n - information (notifications/summaries, e.g. 'Loaded 5  Jitted 4  FailedCompile 1')\n");
    printf("         v - verbose (status messages, e.g. 'Jit startup took '151.12ms')\n");
    printf("         d - debug (lots of detailed output)\n");
    printf("         a - all (enable all message types; overrides other enable message types)\n");
    printf("         q - quiet (disable all output; overrides all others)\n");
    printf("     e.g. '-v ew' only writes error and warning messages to the console.\n");
    printf("     'q' takes precedence over any other message type specified.\n");
    printf("     Default set of messages enabled is 'ewminv'.\n");
    printf("\n");
    printf(" -w[riteLogFile] logfile\n");
    printf("     Write log messages to the specified file.\n");
    printf("\n");
    printf(" -c[ompile] <indices>\n");
    printf("     Compile only those method contexts whose indices are specified.\n");
    printf("     Indices can be either a single index, comma separated values,\n");
    printf("     a range, or the name of a .MCL file with newline delimited indices.\n");
    printf("     e.g. -compile 20\n");
    printf("     e.g. -compile 20,25,30,32\n");
    printf("     e.g. -compile 10-99\n");
    printf("     e.g. -compile 5,10-99,101,201-300\n");
    printf("     e.g. -compile failed.mcl\n");
    printf("\n");
    printf(" -m[atchHash] <MD5 Hash>\n");
    printf("     Compile only method context with specific MD5 hash\n");
    printf("\n");
    printf(" -e[mitMethodStats] <stats-types>\n");
    printf("     Emit method statistics in CSV format to filename.mc.stats.\n");
    printf("     Specify a string of characters representing statistics to emit, where:\n");
    printf("         i - method IL code size\n");
    printf("         a - method compiled ASM code size\n");
    printf("         h - method hash to uniquely identify a method across MCH files\n");
    printf("         n - method number inside the source MCH\n");
    printf("         t - method throughput time\n");
    printf("         * - all available method stats\n");
    printf("\n");
    printf(" -a[pplyDiff]\n");
    printf("     Compare the compile result generated from the provided JIT with the\n");
    printf("     compile result stored with the MC. If two JITs are provided, this\n");
    printf("     compares the compile results generated by the two JITs.\n");
    printf("\n");
    printf(" -r[eproName] prefix\n");
    printf("     Write out failing methods to prefix-n.mc\n");
    printf("\n");
    printf(" -f[ailingMCList] mclfilename\n");
    printf("     Write out failing methods to mclfilename.\n");
    printf("     If using -applyDiff and no -diffMCList is specified,\n");
    printf("     comparison failures also get written to mclfilename.\n");
    printf("\n");
    printf(" -diffMCList diffMCLfilename\n");
    printf("     Write out methods that differ between compilations to diffMCLfilename.\n");
    printf("     This only works with -applyDiff.\n");
    printf("\n");
    printf(" -p[arallel] [workerCount]\n");
    printf("     Run in parallel mode by spawning 'workerCount' processes to do processing.\n");
    printf("     If 'workerCount' is not specified, the number of workers used is\n");
    printf("     the number of processors on the machine.\n");
    printf("\n");
    printf(" -skipCleanup\n");
    printf("     Skip deletion of temporary files created by child SuperPMI processes with -parallel.\n");
    printf("\n");
    printf(" -target <target>\n");
    printf("     Used by the assembly differences calculator. This specifies the target\n");
    printf("     architecture for cross-compilation. Currently allowed <target> value: arm64\n");
    printf("\n");
#ifdef USE_COREDISTOOLS
    printf(" -coredistools\n");
    printf("     Use disassembly tools from the CoreDisTools library\n");
    printf("\n");
#endif // USE_COREDISTOOLS
    printf("Inputs are case sensitive.\n");
    printf("\n");
    printf("SuperPMI method contexts are stored in files with extension .MC, implying\n");
    printf("a single method context, or .MCH, implying a set of method contexts. Either\n");
    printf("extension works equivalently.\n");
    printf("\n");
    printf("Exit codes:\n");
    printf("0  : success\n");
    printf("-1 : general fatal error (e.g., failed to initialize, failed to read files)\n");
    printf("-2 : JIT failed to initialize\n");
    printf("1  : there were compilation failures\n");
    printf("2  : there were assembly diffs\n");
    printf("\n");
    printf("Examples:\n");
    printf(" %s " MAKEDLLNAME_A("clrjit") " test.mch\n", program);
    printf("     ; compile all functions in test.mch using " MAKEDLLNAME_A("clrjit") "\n");
    printf(" %s -p " MAKEDLLNAME_A("clrjit") " test.mch\n", program);
    printf("     ; same as above, but use all available processors to compile in parallel\n");
    printf(" %s -f fail.mcl " MAKEDLLNAME_A("clrjit") " test.mch\n", program);
    printf("     ; if there are any failures, record their MC numbers in the file fail.mcl\n");
}

//Assumption: All inputs are initialized to default or real value.  we'll just set the stuff we see on the command line.
//Assumption: Single byte names are passed in.. mb stuff doesnt cause an obvious problem... but it might have issues...
//Assumption: Values larger than 2^31 aren't expressible from the commandline.... (atoi) Unless you pass in negatives.. :-|
bool CommandLine::Parse(int argc, char* argv[], /* OUT */ Options* o)
{
    size_t argLen = 0;
    size_t tempLen = 0;
    bool foundJit = false;
    bool foundFile = false;

    if (argc == 1) //Print help when no args are passed
    {
        DumpHelp(argv[0]);
        return false;
    }

    for (int i = 1; i<argc; i++)
    {
        bool isASwitch = (argv[i][0] == '-');
#ifndef FEATURE_PAL
        if (argv[i][0] == '/') // Also accept "/" on Windows
        {
            isASwitch = true;
        }
#endif // !FEATURE_PAL

        //Process a switch
        if (isASwitch)
        {
            argLen = strlen(argv[i]);

            if (argLen >1)
                argLen--; //adjust for leading switch
            else
            {
                DumpHelp(argv[0]);
                return false;
            }

            if ((_strnicmp(&argv[i][1], "help", argLen) == 0) ||
                (_strnicmp(&argv[i][1], "HELP", argLen) == 0) ||
                (_strnicmp(&argv[i][1], "?", argLen) == 0))
            {
                DumpHelp(argv[0]);
                return false;
            }
            else if ((_strnicmp(&argv[i][1], "load", argLen) == 0))
            {
                if (++i >= argc)
                {
                    DumpHelp(argv[0]);
                    return false;
                }

            processMethodContext:

                tempLen = strlen(argv[i]);
                if (tempLen == 0)
                {
                    LogError("Arg '%s' is invalid, name of file missing.", argv[i]);
                    DumpHelp(argv[0]);
                    return false;
                }
                o->nameOfInputMethodContextFile = new char[tempLen + 1];
                strcpy_s(o->nameOfInputMethodContextFile, tempLen + 1, argv[i]);
                foundFile = true;
            }
            else if ((_strnicmp(&argv[i][1], "jit", argLen) == 0))
            {
                if (++i >= argc)
                {
                    DumpHelp(argv[0]);
                    return false;
                }

            processJit:

                tempLen = strlen(argv[i]);
                if (tempLen == 0)
                {
                    LogError("Arg '%s' is invalid, name of jit missing.", argv[i]);
                    DumpHelp(argv[0]);
                    return false;
                }
                char *tempStr = new char[tempLen + 1];
                strcpy_s(tempStr, tempLen + 1, argv[i]);
                if (!foundJit)
                {
                    o->nameOfJit = tempStr;
                    foundJit = true;
                }
                else
                {
                    o->nameOfJit2 = tempStr;
                }
            }
            else if ((_strnicmp(&argv[i][1], "reproName", argLen) == 0))
            {
                if (++i >= argc)
                {
                    DumpHelp(argv[0]);
                    return false;
                }

                tempLen = strlen(argv[i]);
                if (tempLen == 0)
                {
                    LogError("Arg '%s' is invalid, name of prefix missing.", argv[i]);
                    DumpHelp(argv[0]);
                    return false;
                }
                char *tempStr = new char[tempLen + 1];
                strcpy_s(tempStr, tempLen + 1, argv[i]);
                o->reproName = tempStr;
            }
            else if ((_strnicmp(&argv[i][1], "failingMCList", argLen) == 0))
            {
                if (++i >= argc)
                {
                    DumpHelp(argv[0]);
                    return false;
                }

                o->mclFilename = argv[i];
            }
            else if ((_strnicmp(&argv[i][1], "diffMCList", 10) == 0))
            {
                if (++i >= argc)
                {
                    DumpHelp(argv[0]);
                    return false;
                }

                o->diffMCLFilename = argv[i];
            }
            else if ((_strnicmp(&argv[i][1], "target", 6) == 0))
            {
                if (++i >= argc)
                {
                    DumpHelp(argv[0]);
                    return false;
                }

                o->targetArchitecture = argv[i];
            }
            else if ((_strnicmp(&argv[i][1], "boe", 3) == 0))
            {
                o->breakOnError = true;
            }
            else if ((_strnicmp(&argv[i][1], "boa", 3) == 0))
            {
                o->breakOnAssert = true;
            }
            else if ((_strnicmp(&argv[i][1], "verbosity", argLen) == 0))
            {
                if (++i >= argc)
                {
                    DumpHelp(argv[0]);
                    return false;
                }

                Logger::SetLogLevel(Logger::ParseLogLevelString(argv[i]));
            }
            else if ((_strnicmp(&argv[i][1], "writeLogFile", argLen) == 0))
            {
                if (++i >= argc)
                {
                    DumpHelp(argv[0]);
                    return false;
                }

                o->writeLogFile = argv[i];
                Logger::OpenLogFile(argv[i]);
            }
            else if ((_strnicmp(&argv[i][1], "emitMethodStats", argLen) == 0))
            {
                if (++i >= argc)
                {
                    DumpHelp(argv[0]);
                    return false;
                }

                o->methodStatsTypes = argv[i];
            }
            else if ((_strnicmp(&argv[i][1], "applyDiff", argLen) == 0))
            {
                o->applyDiff = true;
            }
            else if ((_strnicmp(&argv[i][1], "compile", argLen) == 0))
            {
                if (++i >= argc)
                {
                    DumpHelp(argv[0]);
                    return false;
                }

                bool isValidList = MCList::processArgAsMCL(argv[i], &o->indexCount, &o->indexes);
                if (!isValidList)
                {
                    LogError("Arg '%s' is invalid, needed method context list.", argv[i]);
                    DumpHelp(argv[0]);
                    return false;
                }
                if (o->hash != nullptr)
                {
                    LogError("Cannot use both method context list and method context hash.");
                    DumpHelp(argv[0]);
                    return false;
                }
                if (o->offset > 0 && o->increment > 0)
                {
                    LogError("Cannot use method context list in parallel mode.");
                    DumpHelp(argv[0]);
                    return false;
                }

                o->compileList = argv[i]; // Save this in case we need it for -parallel.
            }
#ifdef USE_COREDISTOOLS
            else if ((_strnicmp(&argv[i][1], "coredistools", argLen) == 0)) {
                o->useCoreDisTools = true;
            }
#endif // USE_COREDISTOOLS
            else if ((_strnicmp(&argv[i][1], "matchHash", argLen) == 0))
            {
                if (++i >= argc)
                {
                    DumpHelp(argv[0]);
                    return false;
                }

                if (strlen(argv[i]) != (MD5_HASH_BUFFER_SIZE - 1))
                {
                    LogError("Arg '%s' is invalid, needed a valid method context hash.", argv[i]);
                    DumpHelp(argv[0]);
                    return false;
                }
                if (o->indexCount > 0)
                {
                    LogError("Cannot use both method context list and method context hash.");
                    DumpHelp(argv[0]);
                    return false;
                }
                if (o->offset > 0 && o->increment > 0)
                {
                    LogError("Cannot use method context hash in parallel mode.");
                    DumpHelp(argv[0]);
                    return false;
                }
                o->hash = argv[i];
            }
            else if ((_strnicmp(&argv[i][1], "parallel", argLen) == 0))
            {
                o->parallel = true;

                // Is there another argument?
                if (i + 1 < argc)
                {
                    // If so, does it look like a worker count?
                    bool isWorkerCount = true;
                    size_t nextlen = strlen(argv[i + 1]);
                    for (size_t j = 0; j < nextlen; j++)
                    {
                        if (!isdigit(argv[i + 1][j]))
                        {
                            isWorkerCount = false; // Doesn't look like a worker count; bail out and let someone else handle it.
                            break;
                        }
                    }
                    if (isWorkerCount)
                    {
                        ++i;
                        o->workerCount = atoi(argv[i]);

                        if (o->workerCount < 1)
                        {
                            LogError("Invalid workers count specified, workers count must be at least 1.");
                            DumpHelp(argv[0]);
                            return false;
                        }
                        if (o->workerCount > MAXIMUM_WAIT_OBJECTS)
                        {
                            LogError("Invalid workers count specified, workers count cannot be more than %d.", MAXIMUM_WAIT_OBJECTS);
                            DumpHelp(argv[0]);
                            return false;
                        }
                    }
                }
            }
            else if ((_stricmp(&argv[i][1], "skipCleanup") == 0))
            {
                o->skipCleanup = true;
            }
            else if ((_strnicmp(&argv[i][1], "stride", argLen) == 0))
            {
                // "-stride" is an internal switch used by -parallel. Usage is:
                //
                // -stride offset increment
                //
                // It compiles methods in this series until end-of-file:
                //      offset, offset+increment, offset+2*increment, offset+3*increment, ...

                if (++i >= argc)
                {
                    DumpHelp(argv[0]);
                    return false;
                }

                o->offset = atoi(argv[i]);

                if (++i >= argc)
                {
                    DumpHelp(argv[0]);
                    return false;
                }

                o->increment = atoi(argv[i]);

                if (o->offset < 1 || o->increment < 1)
                {
                    LogError("Incorrect offset/increment specified for -stride. Offset and increment both must be > 0.");
                    DumpHelp(argv[0]);
                    return false;
                }
                if (o->indexCount > 0)
                {
                    LogError("Cannot use method context list in parallel mode.");
                    DumpHelp(argv[0]);
                    return false;
                }
                if (o->hash != nullptr)
                {
                    LogError("Cannot use method context hash in parallel mode.");
                    DumpHelp(argv[0]);
                    return false;
                }
            }
            else
            {
                LogError("Unknown switch '%s' passed as argument.", argv[i]);
                DumpHelp(argv[0]);
                return false;
            }
        }
        //Process an input filename
        //String comparisons on file extensions must be case-insensitive since we run on Windows
        else
        {
            char *lastdot = strrchr(argv[i], '.');
            if (lastdot == nullptr)
            {
                DumpHelp(argv[0]);
                return false;
            }

            if (_stricmp(lastdot, PLATFORM_SHARED_LIB_SUFFIX_A) == 0)
                goto processJit;
            else if (_stricmp(lastdot, ".mc") == 0)
                goto processMethodContext;
            else if (_stricmp(lastdot, ".mch") == 0)
                goto processMethodContext;
            else if (_stricmp(lastdot, ".mct") == 0)
                goto processMethodContext;
            else
            {
                LogError("Unknown file type passed as argument, '%s'.", argv[i]);
                DumpHelp(argv[0]);
                return false;
            }
        }
    }

    // Do some argument validation.

    if (o->nameOfJit == nullptr)
    {
        LogError("Missing name of a Jit.");
        DumpHelp(argv[0]);
        return false;
    }
    if (o->nameOfInputMethodContextFile == nullptr)
    {
        LogError("Missing name of an input file.");
        DumpHelp(argv[0]);
        return false;
    }
    if (o->diffMCLFilename != nullptr && !o->applyDiff)
    {
        LogError("-diffMCList specified without -applyDiff.");
        DumpHelp(argv[0]);
        return false;
    }
    if (o->targetArchitecture != nullptr && (0 != _stricmp(o->targetArchitecture, "arm64")))
    {
        LogError("Illegal target architecture specified with -target (only arm64 is supported).");
        DumpHelp(argv[0]);
        return false;
    }
    if (o->skipCleanup && !o->parallel)
    {
        LogError("-skipCleanup requires -parallel.");
        DumpHelp(argv[0]);
        return false;
    }
    return true;
}
