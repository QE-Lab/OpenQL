/**
 * @file    options_cc.h
 * @date    20201007
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   options for Central Controller backend
 * @note
 */

#pragma once

// constants
#define CC_BACKEND_VERSION_STRING       "0.2.6"

// options
#define OPT_CC_SCHEDULE_RC              0   // 1=use resource constraint scheduler
#define OPT_RUN_ONCE                    0   // 0=loop indefinitely (CC-light emulation), 1=run once (preferred, but breaks compatibility)
#define OPT_SUPPORT_STATIC_CODEWORDS    1   // support (currently: require) static codewords, instead of allocating them on demand
#define OPT_STATIC_CODEWORDS_ARRAYS     1   // JSON field static_codeword_override is an array with one element per qubit parameter
#define OPT_VECTOR_MODE                 0   // 1=generate single code word for all output groups together (requires codewords allocated by backend)
#define OPT_OLD_SEQBAR_SEMANTICS        0   // support old semantics of seqbar instruction. Will be deprecated
#define OPT_FEEDBACK                    1   // feedback support. New feature in being developed
#define OPT_CROSSCHECK_INSTRUMENT_DEF   1   // check signal dimensions against instrument definition. Will be always on after testing against configuration files of hardware setup
