/** \file
 * Defines the cQASM writer pass.
 */

#include "ql/pass/io/cqasm/report.h"

#include "ql/utils/filesystem.h"
#include "ql/pass/ana/statistics/report.h"

namespace ql {
namespace pass {
namespace io {
namespace cqasm {
namespace report {

/**
 * Dumps cQASM code for the given program to the given output stream.
 * Optionally, the after_kernel callback function may be used to dump additional
 * information at the end of each kernel. The third argument specifies the
 * appropriate line prefix for correctly-indented comments.
 */
void dump(
    const ir::ProgramRef &program,
    std::ostream &os,
    std::function<void(const ir::KernelRef&, std::ostream&, const utils::Str&)> after_kernel
) {
    os << "version 1.0\n";
    os << "# This file has been automatically generated by the OpenQL compiler. Please do not modify it manually.\n";
    os << "qubits " << program->qubit_count << "\n";

    // FIXME: this is all kinds of broken. get_prologue()/get_epilogue() don't
    //  return cQASM code (after all, how could they; cQASM doesn't support
    //  control-flow), there's the wait vs skip problem, the qasm output for the
    //  gates is inconsistent (because the IR is inconsistent about gate types)
    //  etc.
    for (auto &kernel : program->kernels) {
        if (kernel->cycles_valid) {
            os << kernel->get_prologue();
            ir::Bundles bundles = ir::bundler(kernel->c, program->platform->cycle_time);
            os << ir::qasm(bundles);
            os << kernel->get_epilogue();
        } else {
            os << kernel->qasm();
        }
        after_kernel(kernel, os, "    # ");
    }
}

/**
 * Specialization of dump() that includes statistics per kernel and program in
 * comments.
 */
void dump_with_statistics(
    const ir::ProgramRef &program,
    std::ostream &os
) {
    dump(
        program, os, [](
            const ir::KernelRef &k, std::ostream &os, const utils::Str &lp
        ) {
            ana::statistics::report::dump(k, os, lp);
        }
    );
    ana::statistics::report::dump(program, os, "# ");
}


/**
 * Dumps docs for the cQASM writer.
 */
void ReportCQasmPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass writes the current program out as a cQASM file.
    )");
}

/**
 * Constructs a cQASM writer.
 */
ReportCQasmPass::ReportCQasmPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::ProgramAnalysis(pass_factory, instance_name, type_name) {
    options.add_str(
        "output_suffix",
        "Suffix to use for the output filename.",
        ".cq"
    );
    options.add_bool(
        "with_statistics",
        "Whether to include the current statistics for each kernel and the "
        "complete program in the generated comments.",
        false
    );
}

/**
 * Runs the cQASM writer.
 */
utils::Int ReportCQasmPass::run(
    const ir::ProgramRef &program,
    const pmgr::pass_types::Context &context
) const {
    utils::OutFile file{context.output_prefix + options["output_suffix"].as_str()};
    if (options["with_statistics"].as_bool()) {
        dump_with_statistics(program, file.unwrap());
    } else {
        dump(program, file.unwrap());
    }
    return 0;
}

} // namespace report
} // namespace cqasm
} // namespace io
} // namespace pass
} // namespace ql
