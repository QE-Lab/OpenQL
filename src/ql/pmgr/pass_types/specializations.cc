/** \file
 * Defines specialized abstract classes for passes. These are all abstract, to
 * be implemented by actual passes; only common functionality is provided.
 */

#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pmgr {
namespace pass_types {

/**
 * Constructs the abstract pass group. No error checking here; this is up to
 * the parent pass group.
 */
Group::Group(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Base(pass_factory, type_name, instance_name) {
}

/**
 * Simple implementation for on_construct() that always returns true and
 * defers to get_passes() for the initial pass list.
 */
NodeType Group::on_construct(
    const utils::Ptr<const Factory> &factory,
    utils::List<Ref> &passes,
    condition::Ref &condition
) {
    get_passes(factory, passes);
    return NodeType::GROUP;
}

/**
 * Dummy implementation for compilation. Should never be called, as this
 * pass always behaves as an unconditional group. Thus, it just throws an
 * exception.
 */
utils::Int Group::run_internal(
    const ir::ProgramRef &program,
    const Context &context
) const {
    QL_ASSERT(false);
}

/**
 * Group passes don't call run, so run_internal() method doesn't affect the
 * platform tree, so this always returns false.
 */
utils::Bool Group::run_transforms_platform() const {
    return false;
}

/**
 * Constructs the normal pass. No error checking here; this is up to the
 * parent pass group.
 */
Normal::Normal(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Base(pass_factory, instance_name, type_name) {
}

/**
 * Default implementation for on_construct() that makes this a normal pass.
 * May be overridden to allow the pass to generate into a group as well,
 * based on its options.
 */
NodeType Normal::on_construct(
    const utils::Ptr<const Factory> &factory,
    utils::List<Ref> &passes,
    condition::Ref &condition
) {
    return NodeType::NORMAL;
}

/**
 * Constructs the pass. No error checking here; this is up to the parent
 * pass group.
 */
PlatformTransformation::PlatformTransformation(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Normal(pass_factory, instance_name, type_name) {
}

/**
 * Implementation for on_compile() that calls run() appropriately.
 */
utils::Int PlatformTransformation::run_internal(
    const ir::ProgramRef &program,
    const Context &context
) const {
    return run(program->platform, context);
}

/**
 * Returns true, as this is a platform transformation.
 */
utils::Bool PlatformTransformation::run_transforms_platform() const {
    return true;
}

/**
 * Constructs the pass. No error checking here; this is up to the parent
 * pass group.
 */
ProgramTransformation::ProgramTransformation(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Normal(pass_factory, instance_name, type_name) {
}

/**
 * Implementation for on_compile() that calls run() appropriately.
 */
utils::Int ProgramTransformation::run_internal(
    const ir::ProgramRef &program,
    const Context &context
) const {
    return run(program, context);
}

/**
 * Returns false, as this is not a platform transformation.
 */
utils::Bool ProgramTransformation::run_transforms_platform() const {
    return false;
}

/**
 * Constructs the pass. No error checking here; this is up to the parent
 * pass group.
 */
KernelTransformation::KernelTransformation(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Normal(pass_factory, instance_name, type_name) {
}

/**
 * Initial accumulator value for the return value. Defaults to zero.
 */
utils::Int KernelTransformation::retval_initialize() const {
    return 0;
}

/**
 * Return value reduction operator. Defaults to addition.
 */
utils::Int KernelTransformation::retval_accumulate(
    utils::Int state,
    utils::Int kernel
) const {
    return state + kernel;
}

/**
 * Implementation for on_compile() that calls run() appropriately.
 */
utils::Int KernelTransformation::run_internal(
    const ir::ProgramRef &program,
    const Context &context
) const {
    utils::Int accumulator = retval_initialize();
    for (const auto &kernel : program->kernels) {
        accumulator = retval_accumulate(accumulator, run(program, kernel, context));
    }
    return accumulator;
}

/**
 * Returns false, as this is not a platform transformation.
 */
utils::Bool KernelTransformation::run_transforms_platform() const {
    return false;
}

/**
 * Constructs the pass. No error checking here; this is up to the parent
 * pass group.
 */
ProgramAnalysis::ProgramAnalysis(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Normal(pass_factory, instance_name, type_name) {
}

/**
 * Implementation for on_compile() that calls run() appropriately.
 */
utils::Int ProgramAnalysis::run_internal(
    const ir::ProgramRef &program,
    const Context &context
) const {
    return run(program, context);
}

/**
 * Returns false, as this is not a platform transformation.
 */
utils::Bool ProgramAnalysis::run_transforms_platform() const {
    return false;
}

/**
 * Constructs the pass. No error checking here; this is up to the parent
 * pass group.
 */
KernelAnalysis::KernelAnalysis(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Normal(pass_factory, instance_name, type_name) {
}

/**
 * Initial accumulator value for the return value. Defaults to zero.
 */
utils::Int KernelAnalysis::retval_initialize() const {
    return 0;
}

/**
 * Return value reduction operator. Defaults to addition.
 */
utils::Int KernelAnalysis::retval_accumulate(
    utils::Int state,
    utils::Int kernel
) const {
    return state + kernel;
}

/**
 * Implementation for on_compile() that calls run() appropriately.
 */
utils::Int KernelAnalysis::run_internal(
    const ir::ProgramRef &program,
        const Context &context
) const {
    utils::Int accumulator = retval_initialize();
    for (const auto &kernel : program->kernels) {
        accumulator = retval_accumulate(accumulator, run(program, kernel, context));
    }
    return accumulator;
}

/**
 * Returns false, as this is not a platform transformation.
 */
utils::Bool KernelAnalysis::run_transforms_platform() const {
    return false;
}

} // namespace pass_types
} // namespace pmgr
} // namespace ql
