/** \file
 * Defines the statistics reporting pass.
 */

#pragma once

#include "ql/pmgr/pass_types.h"

namespace ql {
namespace pass {
namespace ana {
namespace statistics {
namespace report {

/**
 * Statistics reporting pass.
 */
class ReportStatisticsPass : public pmgr::pass_types::ProgramAnalysis {
protected:

    /**
     * Dumps docs for the statistics reporter.
     */
    void dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

public:

    /**
     * Constructs a statistics reporter.
     */
    ReportStatisticsPass(
        const utils::Ptr<const pmgr::PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the statistics reporter.
     */
    utils::Int run(
        const ir::ProgramRef &program,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = ReportStatisticsPass;

} // namespace report
} // namespace statistics
} // namespace ana
} // namespace pass
} // namespace ql