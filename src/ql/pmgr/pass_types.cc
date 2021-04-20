/** \file
 * Defines the base classes for passes. These are all abstract, to be
 * implemented by actual passes; only common functionality is provided.
 */

#include "ql/pmgr/pass_types.h"

#include <cctype>
#include <regex>
#include "ql/pmgr/pass_manager.h"

namespace ql {
namespace pmgr {
namespace pass_types {

/**
 * Returns whether the given user-specified name is a valid pass name.
 */
void Base::check_pass_name(
    const utils::Str &name,
    const utils::Map<utils::Str, Ref> &existing_pass_names
) {
    static const std::regex name_re{"[a-zA-Z0-9_\\-]+"};
    if (!std::regex_match(name, name_re)) {
        throw utils::Exception("pass name \"" + name + "\" is invalid");
    }
    if (existing_pass_names.count(name)) {
        throw utils::Exception("duplicate pass name \"" + name + "\"");
    }
}

/**
 * Returns a unique name generated from the given type name.
 */
utils::Str Base::generate_valid_pass_name(const utils::Str &type_name) const {

    // The type name will have type hierarchy separators (periods), and the
    // final entry will be TitleCase. However, instance names are normally
    // lower_case. So, we replace periods with underscores, and insert an
    // underscore before groups of uppercase characters.
    auto reference_name = type_name;
    if (type_name.empty()) {
        reference_name = "group";
    }
    utils::Str sub_instance_name;
    char prev = '_';
    for (char cur : reference_name) {
        if (cur == '.') {
            cur = '_';
        }
        if (std::isupper(cur) && !std::isupper(prev) && prev != '_') {
            sub_instance_name += '_';
        }
        sub_instance_name += std::tolower(cur);
        prev = cur;
    }

    // If the generated name doesn't exist yet, use it as is.
    if (sub_pass_names.find(sub_instance_name) == sub_pass_names.end()) {
        return sub_instance_name;
    }

    // Append numbers until we find a name that we haven't used yet.
    utils::UInt uniquifier = 1;
    while (true) {
        utils::Str uniquified_sub_instance_name = sub_instance_name + "_" + utils::to_string(uniquifier);
        if (sub_pass_names.find(uniquified_sub_instance_name) == sub_pass_names.end()) {
            return uniquified_sub_instance_name;
        }
        uniquifier++;
    }
}

/**
 * Makes a new pass. Used by the various functions that add passes.
 */
Ref Base::make_pass(
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) const {

    // Generate a name if no name is provided.
    auto gen_instance_name = instance_name;
    if (instance_name.empty()) {
        gen_instance_name = generate_valid_pass_name(type_name);
    }

    // Check the name.
    check_pass_name(gen_instance_name, sub_pass_names);

    // Construct the pass.
    Ref pass = PassFactory::build_pass(pass_factory, type_name, gen_instance_name);

    // Set initial options.
    for (const auto &opt : options) {
        pass->set_option(opt.first, opt.second);
    }

    return pass;
}

/**
 * Returns an iterator for the sub_pass_order list corresponding with the
 * given target instance name, or throws an exception if no such pass is
 * found.
 */
utils::List<Ref>::iterator Base::find_pass(const utils::Str &target) {
    for (auto it = sub_pass_order.begin(); it != sub_pass_order.end(); ++it) {
        if ((*it)->get_name() == target) {
            return it;
        }
    }
    throw utils::Exception("pass with name \"" + target + "\" not found");
}

/**
 * Checks whether access to the sub-pass list is allowed. Throws an
 * exception if not.
 */
void Base::check_group_access_allowed() const {
    if (!is_group()) {
        throw utils::Exception("cannot access sub-pass list before construct() or for normal pass");
    }
}

/**
 * Checks whether access to the condition is allowed. Throws an exception
 * if not.
 */
void Base::check_condition_access_allowed() const {
    if (!is_conditional()) {
        throw utils::Exception("cannot access condition for non-conditional pass");
    }
}

/**
 * Constructs the abstract pass. No error checking here; this is up to the
 * parent pass group.
 */
Base::Base(
    const utils::Ptr<const PassFactory> &pass_factory,
    const utils::Str &type_name,
    const utils::Str &instance_name
) :
    pass_factory(pass_factory),
    type_name(type_name),
    instance_name(instance_name)
{
    // TODO common pass options
    options.add_str(
        "output_prefix",
        "Format string for the prefix used for all output products. "
        "%n is substituted with the user-specified name of the program. "
        "%N is substituted with the optionally uniquified name of the program. "
        "%p is substituted with the local name of the pass within its group. "
        "%P is substituted with the fully-qualified name of the pass, using "
        "periods as hierarchy separators (guaranteed unique). "
        "%U is substituted with the fully-qualified name of the pass, using "
        "underscores as hierarchy separators. This may not be completely unique,"
        "%D is substituted with the fully-qualified name of the pass, using "
        "slashes as hierarchy separators. "
        "Any directories that don't exist will be created as soon as an output "
        "file is written.",
        "%N.%P"
    );
}

/**
 * Returns the full, desugared type name that this pass was constructed
 * with.
 */
const utils::Str &Base::get_type() const {
    return type_name;
}

/**
 * Returns the full, desugared type name that this pass was constructed
 * with.
 */
const utils::Str &Base::get_name() const {
    return instance_name;
}

/**
 * Dumps the documentation for this pass to the given stream.
 */
void Base::dump_help(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    dump_docs(os, line_prefix);
    os << line_prefix << "\n";
    os << line_prefix << "Available options:\n";
    os << line_prefix << "\n";
    options.help(os, line_prefix + "  ");
}

/**
 * Dumps the current state of the options to the given stream. If only_set
 * is set to true, only the options that were explicitly configured are
 * dumped.
 */
void Base::dump_options(
    utils::Bool only_set,
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    options.dump(only_set, os, line_prefix);
}

/**
 * Dumps the entire compilation strategy including configured options of
 * this pass and all sub-passes.
 */
void Base::dump_strategy(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::Str indent;
    if (!is_root()) {
        os << line_prefix << "- " << instance_name;
        if (!type_name.empty()) {
            os << ": " << type_name << "\n";
        }
        os << "\n";
        indent = line_prefix + "  ";
    } else {
        indent = line_prefix;
    }
    options.dump(true, os, line_prefix + "  ");
    if (is_group()) {
        switch (node_type) {
            case NodeType::GROUP_IF:
                os << indent << "if " << condition->to_string() << ":\n";
                break;
            case NodeType::GROUP_WHILE:
                os << indent << "while " << condition->to_string() << ":\n";
                break;
            case NodeType::GROUP_REPEAT_UNTIL_NOT:
                os << indent << "repeat:\n";
                break;
            default:
                os << indent << "passes:\n";
                break;
        }
        for (const auto &pass : sub_pass_order) {
            pass->dump_strategy(os, line_prefix + "   |");
        }
        if (node_type == NodeType::GROUP_REPEAT_UNTIL_NOT) {
            os << line_prefix << "until " << condition->to_string() << "\n";
        }
    }
    os << std::endl;
}

/**
 * Sets an option. Periods may be used as hierarchy separators to set
 * options for sub-passes. Furthermore, each period-separated element
 * (except the last one, which is the option name) may be a single
 * asterisk, to select all sub-passes. The return value is the number of
 * passes that were affected. If must_exist is set, an exception will be
 * thrown if none of the passes were affected.
 */
utils::UInt Base::set_option(
    const utils::Str &option,
    const utils::Str &value,
    utils::Bool must_exist
) {
    auto period = option.find('.');

    // Handle setting an option on this pass.
    if (period == utils::Str::npos) {
        if (must_exist && is_constructed()) {
            throw utils::Exception(
                "cannot modify pass option after pass construction"
            );
        }
        if (must_exist && !options.has_option(option)) {
            throw utils::Exception(
                "option " + option + " does not exist for pass " + instance_name
            );
        }
        if (!is_constructed() && options.has_option(option)) {
            options[option] = value;
            return 1;
        } else {
            return 0;
        }
    }

    // Handle setting options on sub-passes.
    if (!is_constructed()) {
        throw utils::Exception(
            "cannot set sub-pass options before parent pass ("
            + instance_name + ") is constructed"
        );
    }
    if (!is_group()) {
        throw utils::Exception(
            "cannot set sub-pass options for non-group pass " + instance_name
        );
    }
    utils::Str sub_pass = option.substr(0, period);
    utils::Str sub_option = option.substr(period + 1);

    // Handle setting an option on all sub-passes.
    if (sub_pass == "*") {
        utils::UInt passes_affected = 0;
        for (auto &pass : sub_pass_order) {
            passes_affected += pass->set_option(sub_option, value, false);
        }
        if (must_exist && !passes_affected) {
            throw utils::Exception(
                "option " + sub_option + " could not be set on any sub-pass of "
                + instance_name
            );
        }
        return passes_affected;
    }

    // Handle setting an option on a single sub-pass.
    static const std::regex name_re{"[a-zA-Z0-9_\\-]+"};
    if (!std::regex_match(sub_pass, name_re)) {
        throw utils::Exception(
            "\"" + sub_pass + "\" is not a valid pass name or supported pattern"
        );
    }
    auto sub_pass_it = sub_pass_names.find(sub_pass);
    if (sub_pass_it != sub_pass_names.end()) {
        return sub_pass_it->second->set_option(sub_option, value, must_exist);
    } else {
        if (must_exist) {
            throw utils::Exception(
                "no sub-pass with name \"" + sub_pass + "\" in pass "
                + instance_name
            );
        }
        return 0;
    }

}

/**
 * Returns the current value of an option. Periods may be used as hierarchy
 * separators to get options from sub-passes (if any).
 */
const utils::Option &Base::get_option(const utils::Str &option) const {
    auto period = option.find('.');

    // Handle getting an option from this pass.
    if (period == utils::Str::npos) {
        return options[option];
    }

    // Handle getting an option from a sub-pass.
    utils::Str sub_pass = option.substr(0, period);
    utils::Str sub_option = option.substr(period + 1);
    auto sub_pass_it = sub_pass_names.find(sub_pass);
    if (sub_pass_it == sub_pass_names.end()) {
        throw utils::Exception(
            "no sub-pass with name \"" + sub_pass + "\" in pass " + instance_name
        );
    }
    return sub_pass_it->second->get_option(sub_option);

}

/**
 * Returns mutable access to the embedded options object. This is allowed only
 * until construct() is called.
 */
utils::Options &Base::get_options() {
    if (is_constructed()) {
        throw utils::Exception("cannot modify pass option after pass construction");
    }
    return options;
}

/**
 * Returns read access to the embedded options object.
 */
const utils::Options &Base::get_options() const {
    return options;
}

/**
 * Constructs this pass. During construction, the pass implementation may
 * decide, based on its options, to become a group of passes or a normal
 * pass. If it decides to become a group, the group may be introspected or
 * modified by the user. The options are frozen after this, so set_option()
 * will start throwing exceptions when called. construct() may be called any
 * number of times, but becomes no-op after the first call.
 */
void Base::construct() {

    // If we've already been constructed, don't do it again.
    if (is_constructed()) {
        return;
    }

    // Run the construction implementation.
    utils::List<Ref> constructed_pass_order;
    condition::Ref constructed_condition;
    auto constructed_node_type = on_construct(
        pass_factory,
        constructed_pass_order,
        constructed_condition
    );

    // Check basic postconditions.
    switch (constructed_node_type) {
        case NodeType::NORMAL:
            QL_ASSERT(constructed_pass_order.empty());
            QL_ASSERT(!constructed_condition.has_value());
            break;
        case NodeType::GROUP:
            QL_ASSERT(!constructed_condition.has_value());
            break;
        case NodeType::GROUP_IF:
        case NodeType::GROUP_WHILE:
        case NodeType::GROUP_REPEAT_UNTIL_NOT:
            QL_ASSERT(constructed_condition.has_value());
            break;
        default:
            QL_ASSERT(false);
    }

    // Check validity and uniqueness of the names, and build the name to pass
    // map.
    utils::Map<utils::Str, Ref> constructed_pass_names;
    std::regex name_re{"[a-zA-Z0-9_\\-]+"};
    for (const auto &pass : constructed_pass_order) {
        check_pass_name(pass->get_name(), constructed_pass_names);
        constructed_pass_names.set(pass->get_name()) = pass;
    }

    // Commit the results.
    node_type = constructed_node_type;
    sub_pass_order = std::move(constructed_pass_order);
    sub_pass_names = std::move(constructed_pass_names);
    condition = std::move(constructed_condition);

}

/**
 * Recursively constructs this pass and all its sub-passes (if it constructs
 * or previously constructed into a group). Also checks that all platform
 * preprocessing passes come before regular passes.
 */
void Base::construct_recursive(
    utils::Bool &still_preprocessing_platform,
    const utils::Str &pass_name_prefix
) {

    // Construct ourself.
    auto full_name = pass_name_prefix + instance_name;
    construct();

    // Check platform preprocessing order.
    if (!run_transforms_platform()) {
        if (!is_collapsible()) {

            // Not a trivial/unprocessed run() function, and run() does not
            // modify the platform (and thus is allowed to operate on the
            // program, making use of the platform). After such a pass, we
            // shouldn't modify the platform anymore.
            still_preprocessing_platform = false;

        }
    } else if (!still_preprocessing_platform) {

        // Our run() function modifies the platform, but previous passes have
        // already used it under the assumption that it's frozen. Thus, the
        // strategy is invalid.
        throw utils::Exception(
            "pass \"" + full_name + "\" of type \"" + type_name + "\" modifies "
            "the platform, but regular passes are scheduled to happen before it!"
        );

    }

    // If we constructed into a group, construct the sub-passes recursively.
    if (is_group()) {
        for (const auto &pass : sub_pass_order) {
            pass->construct_recursive(
                still_preprocessing_platform,
                full_name + "."
            );
        }
    }
}

/**
 * Returns whether this pass has been constructed yet.
 */
utils::Bool Base::is_constructed() const {
    return node_type != NodeType::UNKNOWN;
}

/**
 * Returns whether this pass has configurable sub-passes.
 */
utils::Bool Base::is_group() const {
    return node_type == NodeType::GROUP
        || node_type == NodeType::GROUP_IF
        || node_type == NodeType::GROUP_WHILE
        || node_type == NodeType::GROUP_REPEAT_UNTIL_NOT;
}

/**
 * Returns whether this pass is a simple group of which the sub-passes can
 * be collapsed into the parent pass group without affecting the strategy.
 */
utils::Bool Base::is_collapsible() const {
    return node_type == NodeType::GROUP;
}

/**
 * Returns whether this is the root pass group in a pass manager.
 */
utils::Bool Base::is_root() const {
    return instance_name.empty();
}

/**
 * Returns whether this pass transforms the platform tree.
 */
utils::Bool Base::is_platform_transformer() const {
    if (run_transforms_platform()) {
        return true;
    }
    if (is_group()) {
        for (const auto &pass : sub_pass_order) {
            if (pass->run_transforms_platform()) return true;
        }
    }
    return false;
}

/**
 * Returns whether this pass contains a conditionally-executed group.
 */
utils::Bool Base::is_conditional() const {
    return node_type == NodeType::GROUP_IF
        || node_type == NodeType::GROUP_WHILE
        || node_type == NodeType::GROUP_REPEAT_UNTIL_NOT;
}

/**
 * If this pass constructed into a group of passes, appends a pass to the
 * end of its pass list. Otherwise, an exception is thrown. If type_name is
 * empty or unspecified, a generic subgroup is added. Returns a reference to
 * the constructed pass.
 */
Ref Base::append_sub_pass(
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    check_group_access_allowed();
    Ref pass = make_pass(type_name, instance_name, options);
    sub_pass_order.push_back(pass);
    sub_pass_names.set(pass->instance_name) = pass;
    return pass;
}

/**
 * If this pass constructed into a group of passes, appends a pass to the
 * beginning of its pass list. Otherwise, an exception is thrown. If
 * type_name is empty or unspecified, a generic subgroup is added. Returns a
 * reference to the constructed pass.
 */
Ref Base::prefix_sub_pass(
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    check_group_access_allowed();
    Ref pass = make_pass(type_name, instance_name, options);
    sub_pass_order.push_front(pass);
    sub_pass_names.set(pass->instance_name) = pass;
    return pass;
}

/**
 * If this pass constructed into a group of passes, inserts a pass
 * immediately after the target pass (named by instance). If target does not
 * exist or this pass is not a group of sub-passes, an exception is thrown.
 * If type_name is empty or unspecified, a generic subgroup is added.
 * Returns a reference to the constructed pass.
 */
Ref Base::insert_sub_pass_after(
    const utils::Str &target,
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    check_group_access_allowed();
    auto it = find_pass(target);
    Ref pass = make_pass(type_name, instance_name, options);
    sub_pass_order.insert(std::next(it), pass);
    sub_pass_names.set(pass->instance_name) = pass;
    return pass;
}

/**
 * If this pass constructed into a group of passes, inserts a pass
 * immediately before the target pass (named by instance). If target does
 * not exist or this pass is not a group of sub-passes, an exception is
 * thrown. If type_name is empty or unspecified, a generic subgroup is
 * added. Returns a reference to the constructed pass.
 */
Ref Base::insert_sub_pass_before(
    const utils::Str &target,
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Map<utils::Str, utils::Str> &options
) {
    check_group_access_allowed();
    auto it = find_pass(target);
    Ref pass = make_pass(type_name, instance_name, options);
    sub_pass_order.insert(it, pass);
    sub_pass_names.set(pass->instance_name) = pass;
    return pass;
}

/**
 * If this pass constructed into a group of passes, looks for the pass with
 * the target instance name, and embeds it into a newly generated group. The
 * group will assume the name of the original pass, while the original pass
 * will be renamed as specified by sub_name. Note that this ultimately does
 * not modify the pass order. If target does not exist or this pass is not a
 * group of sub-passes, an exception is thrown. Returns a reference to the
 * constructed group.
 */
Ref Base::group_sub_pass(
    const utils::Str &target,
    const utils::Str &sub_name
) {
    check_group_access_allowed();

    // Find and remove the target pass from the pass list.
    auto it = find_pass(target);
    Ref pass = *it;
    it = sub_pass_order.erase(it);
    sub_pass_names.erase(pass->instance_name);

    // Create a group with the same name as the target pass.
    Ref group = make_pass("", pass->instance_name, {});
    QL_ASSERT(group->is_group());
    QL_ASSERT(group->is_constructed());
    QL_ASSERT(group->sub_pass_order.empty());
    sub_pass_order.insert(it, group);
    sub_pass_names.set(group->instance_name) = group;

    // Rename the child pass and add it to the group.
    pass->instance_name = sub_name;
    group->sub_pass_order.push_back(pass);
    group->sub_pass_names.set(pass->instance_name) = pass;

    return group;
}

/**
 * Like group_sub_pass(), but groups an inclusive range of passes into a
 * group with the given name, leaving the original pass names unchanged.
 */
Ref Base::group_sub_passes(
    const utils::Str &from,
    const utils::Str &to,
    const utils::Str &group_name
) {
    check_group_access_allowed();

    // Get the pass range as iterators.
    auto begin = find_pass(from);
    auto end = std::next(find_pass(to));

    // Create a group for the sub-passes.
    Ref group = make_pass("", group_name, {});
    QL_ASSERT(group->is_group());
    QL_ASSERT(group->is_constructed());
    QL_ASSERT(group->sub_pass_order.empty());

    // Copy the passes in the range into the group, and remove the passes from
    // our name lookup
    for (auto it = begin; it != end; ++it) {
        auto pass = *it;
        group->sub_pass_order.push_back(pass);
        group->sub_pass_names.set(pass->instance_name) = pass;
        sub_pass_names.erase(pass->instance_name);
    }

    // Erase the passes from our pass list.
    auto it = sub_pass_order.erase(begin, end);

    // Add the group where we just removed the pass range.
    sub_pass_order.insert(it, group);
    sub_pass_names.set(group->instance_name) = group;

    return group;
}

/**
 * If this pass constructed into a group of passes, looks for the pass with
 * the target instance name, treats it as a generic group, and flattens its
 * contained passes into the list of sub-passes of its parent. The names of
 * the passes found in the collapsed subgroup are prefixed with name_prefix
 * before they are added to the parent group. Note that this ultimately does
 * not modify the pass order. If target does not exist, does not construct
 * into a group of passes (construct() is called automatically), or this
 * pass is not a group of sub-passes, an exception is thrown.
 */
void Base::flatten_subgroup(
    const utils::Str &target,
    const utils::Str &name_prefix
) {
    check_group_access_allowed();

    // Find the target, ensure that it's a simple group, and then remove it.
    auto it = find_pass(target);
    auto group = *it;
    if (!group->is_constructed()) {
        throw utils::Exception("cannot collapse pass that isn't constructed yet");
    }
    if (!group->is_collapsible()) {
        throw utils::Exception("cannot collapse pass that isn't a simple group");
    }
    it = sub_pass_order.erase(it);
    sub_pass_names.erase(group->instance_name);

    // Rename and insert the passes from the group where we found the group.
    for (const auto &pass : group->sub_pass_order) {
        pass->instance_name = name_prefix + pass->instance_name;
        check_pass_name(pass->instance_name, sub_pass_names);
        sub_pass_order.insert(it, pass);
        sub_pass_names.set(pass->instance_name) = pass;
        ++it;
    }

}

/**
 * If this pass constructed into a group of passes, returns a reference to
 * the pass with the given instance name. If target does not exist or this
 * pass is not a group of sub-passes, an exception is thrown.
 */
Ref Base::get_sub_pass(const utils::Str &target) const {
    check_group_access_allowed();
    return sub_pass_names.get(target);
}

/**
 * If this pass constructed into a group of passes, returns whether a
 * sub-pass with the target instance name exists. Otherwise, an exception is
 * thrown.
 */
utils::Bool Base::does_sub_pass_exist(const utils::Str &target) const {
    check_group_access_allowed();
    return sub_pass_names.count(target) > 0;
}

/**
 * If this pass constructed into a group of passes, returns the total number
 * of sub-passes. Otherwise, an exception is thrown.
 */
utils::UInt Base::get_num_sub_passes() const {
    check_group_access_allowed();
    return sub_pass_order.size();
}

/**
 * If this pass constructed into a group of passes, returns a reference to
 * the list containing all the sub-passes. Otherwise, an exception is
 * thrown.
 */
const utils::List<Ref> &Base::get_sub_passes() const {
    check_group_access_allowed();
    return sub_pass_order;
}

/**
 * If this pass constructed into a group of passes, returns an indexable
 * list of references to all passes with the given type. Otherwise, an
 * exception is thrown.
 */
utils::Vec<Ref> Base::get_sub_passes_by_type(const utils::Str &target) const {
    check_group_access_allowed();
    utils::Vec<Ref> retval;
    for (const auto &pass : sub_pass_order) {
        if (pass->type_name == target) {
            retval.push_back(pass);
        }
    }
    return retval;
}

/**
 * If this pass constructed into a group of passes, removes the sub-pass
 * with the target instance name. If target does not exist or this pass is
 * not a group of sub-passes, an exception is thrown.
 */
void Base::remove_sub_pass(const utils::Str &target) {
    check_group_access_allowed();
    auto it = find_pass(target);
    Ref pass = *it;
    sub_pass_order.erase(it);
    sub_pass_names.erase(pass->instance_name);
}

/**
 * If this pass constructed into a group of passes, removes all sub-passes.
 * Otherwise, an exception is thrown.
 */
void Base::clear_sub_passes() {
    check_group_access_allowed();
    sub_pass_names.clear();
    sub_pass_order.clear();
}

/**
 * If this pass constructed into a conditional pass group, returns a const
 * reference to the configured condition. Otherwise, an exception is thrown.
 */
condition::CRef Base::get_condition() const {
    check_condition_access_allowed();
    return condition.as_const();
}

/**
 * If this pass constructed into a conditional pass group, returns a mutable
 * reference to the configured condition. Otherwise, an exception is thrown.
 */
condition::Ref Base::get_condition() {
    check_condition_access_allowed();
    return condition;
}

/**
 * Wrapper around running the main pass implementation for this pass, taking
 * care of logging, profiling, etc.
 */
utils::Int Base::run_main_pass(
    const ir::ProgramRef &program,
    const utils::Str &pass_name_prefix
) const {

    // Construct pass context.
    Context context;
    context.full_pass_name = pass_name_prefix + instance_name;

    // Apply substitution rules for the output prefix option.
    utils::Bool special = false;
    for (auto c : options["output_prefix"].as_str()) {
        if (special) {
            switch (c) {
                case '%':
                    context.output_prefix += '%';
                    break;
                case 'n':
                    context.output_prefix += program->name;
                    break;
                case 'N':
                    context.output_prefix += program->unique_name;
                    break;
                case 'p':
                    context.output_prefix += instance_name;
                    break;
                case 'P':
                    context.output_prefix += context.full_pass_name;
                    break;
                case 'U':
                    context.output_prefix += utils::replace_all(context.full_pass_name, ".", "_");
                    break;
                case 'D':
                    context.output_prefix += utils::replace_all(context.full_pass_name, ".", "/");
                    break;
                default:
                    throw utils::Exception(
                        "undefined substitution sequence in output_prefix option "
                        "for pass " + context.full_pass_name + ": %" + c
                    );
            }
            special = false;
        } else if (c == '%') {
            special = true;
        } else {
            context.output_prefix += c;
        }
    }
    if (special) {
        throw utils::Exception(
            "unterminated substitution sequence in output_prefix option "
            "for pass " + context.full_pass_name
        );
    }

    // Run the pass.
    QL_IOUT("starting pass \"" << context.full_pass_name << "\" of type \"" << type_name << "\"...");
    auto retval = run_internal(program, context);
    QL_IOUT("completed pass \"" << context.full_pass_name << "\"; return value is " << retval);
    return retval;
}

/**
 * Wrapper around running the sub-passes for this pass, taking care of logging,
 * profiling, etc.
 */
void Base::run_sub_passes(
    const ir::ProgramRef &program,
    const utils::Str &pass_name_prefix
) const {
    utils::Str sub_prefix = pass_name_prefix + instance_name + ".";
    for (const auto &pass : sub_pass_order) {
        pass->compile(program, sub_prefix);
    }
}

/**
 * Executes this pass or pass group on the given platform and program.
 */
void Base::compile(
    const ir::ProgramRef &program,
    const utils::Str &pass_name_prefix
) {

    // The passes should already have been constructed by the pass manager.
    QL_ASSERT(is_constructed());

    // Traverse our level of the pass tree based on our node type.
    switch (node_type) {
        case NodeType::NORMAL: {
            run_main_pass(program, pass_name_prefix);
            break;
        }

        case NodeType::GROUP: {
            run_sub_passes(program, pass_name_prefix);
            break;
        }

        case NodeType::GROUP_IF: {
            auto retval = run_main_pass(program, pass_name_prefix);
            if (condition->evaluate(retval)) {
                QL_IOUT("pass condition returned true, running sub-passes...");
                run_sub_passes(program, pass_name_prefix);
            } else {
                QL_IOUT("pass condition returned false, skipping " << sub_pass_order.size() << " sub-pass(es)");
            }
            break;
        }

        case NodeType::GROUP_WHILE: {
            QL_IOUT("entering loop pass loop...");
            while (true) {
                auto retval = run_main_pass(program, pass_name_prefix);
                if (!condition->evaluate(retval)) {
                    QL_IOUT("pass condition returned false, exiting loop");
                    break;
                } else {
                    QL_IOUT("pass condition returned true, continuing loop...");
                }
                run_sub_passes(program, pass_name_prefix);
            }
            break;
        }

        case NodeType::GROUP_REPEAT_UNTIL_NOT: {
            QL_IOUT("entering loop pass loop...");
            while (true) {
                run_sub_passes(program, pass_name_prefix);
                auto retval = run_main_pass(program, pass_name_prefix);
                if (!condition->evaluate(retval)) {
                    QL_IOUT("pass condition returned false, exiting loop");
                    break;
                } else {
                    QL_IOUT("pass condition returned true, continuing loop...");
                }
            }
            break;
        }

        default: QL_ASSERT(false);
    }
}

/**
 * Constructs the abstract pass group. No error checking here; this is up to
 * the parent pass group.
 */
Group::Group(
    const utils::Ptr<const PassFactory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Base(pass_factory, type_name, instance_name) {
    construct();
}

/**
 * Simple implementation for on_construct() that always returns true and
 * defers to get_passes() for the initial pass list.
 */
NodeType Group::on_construct(
    const utils::Ptr<const PassFactory> &factory,
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
    const utils::Ptr<const PassFactory> &pass_factory,
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
    const utils::Ptr<const PassFactory> &factory,
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
    const utils::Ptr<const PassFactory> &pass_factory,
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
    const utils::Ptr<const PassFactory> &pass_factory,
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
    const utils::Ptr<const PassFactory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Normal(pass_factory, instance_name, type_name) {
}

/**
 * Implementation for on_compile() that calls run() appropriately.
 */
utils::Int KernelTransformation::run_internal(
    const ir::ProgramRef &program,
    const Context &context
) const {
    for (const auto &kernel : program->kernels) {
        run(program, kernel, context);
    }
    return 0;
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
    const utils::Ptr<const PassFactory> &pass_factory,
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
    const utils::Ptr<const PassFactory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Normal(pass_factory, instance_name, type_name) {
}

/**
 * Implementation for on_compile() that calls run() appropriately.
 */
utils::Int KernelAnalysis::run_internal(
    const ir::ProgramRef &program,
        const Context &context
) const {
    for (const auto &kernel : program->kernels) {
        run(program, kernel, context);
    }
    return 0;
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