/** \file
 * Defines the BundleInfo class/structure.
 */

#pragma once

#include "types.h"
#include "options.h"
#include "settings.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

class BundleInfo {
public: // funcs
    BundleInfo() = default;

public: // vars
    // output gates
    Str signalValue;
    UInt durationInCycles = 0;
#if OPT_SUPPORT_STATIC_CODEWORDS
    Int staticCodewordOverride = Settings::NO_STATIC_CODEWORD_OVERRIDE;
#endif
#if OPT_FEEDBACK
    // readout feedback
    Bool isMeasFeedback = false;
    Vec<UInt> operands;                         // NB: also used by OPT_PRAGMA
    Vec<UInt> creg_operands;
    Vec<UInt> breg_operands;

    // conditional gates
    ir::compat::ConditionType condition = ir::compat::ConditionType::ALWAYS;        // FIXME
    Vec<UInt> cond_operands;
#endif
#if OPT_PRAGMA
    // pragma 'gates'
    RawPtr<const Json> pragma;
#endif
}; // information for an instrument group (of channels), for a single instruction
// FIXME: rename tInstrInfo, store gate as annotation, move to class cc:IR, together with customGate(), bundleStart(), bundleFinish()?

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
