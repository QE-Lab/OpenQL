/** \file
 * Defines the resource manager.
 */

#include "ql/rmgr/manager.h"

#include <regex>
#include "ql/arch/architecture.h"

namespace ql {
namespace rmgr {

/**
 * Dumps the documentation for the resource JSON configuration structure.
 */
void Manager::dump_docs(std::ostream &os, const utils::Str &line_prefix) {
    utils::dump_str(os, line_prefix, R"(
    Two JSON structures are supported: one for compatibility with older
    platform configuration files, and one extended structure. The extended
    structure has the following syntax:

    ```
    {
        "architecture": <optional string, default "">,
        "dnu": <optional list of strings, default []>,
        "resources": {
            "<name>": {
                "type": "<type>",
                "config": {
                    <optional configuration>
                }
            }
            ...
        }
    }
    ```

    The optional `"architecture"` key may be used to make shorthands for
    architecture- specific resources, normally prefixed with
    `"arch.<architecture>."`. If it's not specified or an empty string,
    the architecture is derived from the compiler configuration
    (either `"eqasm_compiler"` or as overridden by the compiler configuration
    file passed to the platform constructor).

    The optional `"dnu"` key may be used to specify a list of do-not-use
    resource types (experimental, deprecated, or any other resource that's
    considered unfit for "production" use) that you explicitly want to use,
    including the `"dnu"` namespace they are defined in. Once specified, you'll
    be able to use the resource type without the `"dnu"` namespace element. For
    example, if you would include `"dnu.whatever"` in the list, the resource
    type `"whatever"` may be used to add the resource.

    The `"resources"` key specifies the actual resource list. This consists of
    a map from unique resource names matching `[a-zA-Z0-9_\-]+` to a resource
    configuration. The configuration object must have a "type" key, which
    must identify a resource type that OpenQL knows about. The "config" key
    is optional, and is used to pass type-specific configuration data to the
    resource. If not specified, an empty JSON object will be passed to the
    resource instead.

    If the `"resources"` key is not present, the old structure is used instead.
    This has the following simplified form:

    ```
    {
        "<type>": {
            <configuration>
        },
        ...
    }
    ```
    )");
}

/**
 * Returns whether the given user-specified name is a valid resource name.
 */
void Manager::check_resource_name(
    const utils::Str &name
) const {
    static const std::regex name_re{"[a-zA-Z0-9_\\-]+"};
    if (!std::regex_match(name, name_re)) {
        throw utils::Exception("resource name \"" + name + "\" is invalid");
    }
    if (resources.count(name)) {
        throw utils::Exception("duplicate resource name \"" + name + "\"");
    }
}

/**
 * Returns a unique name generated from the given type name.
 */
utils::Str Manager::generate_valid_resource_name(const utils::Str &type_name) const {

    // The type name will have type hierarchy separators (periods), and the
    // final entry will be TitleCase. However, instance names are normally
    // lower_case. So, we replace periods with underscores, and insert an
    // underscore before groups of uppercase characters.
    auto reference_name = type_name;
    utils::Str instance_name;
    char prev = '_';
    for (char cur : reference_name) {
        if (cur == '.') {
            cur = '_';
        }
        if (std::isupper(cur) && !std::isupper(prev) && prev != '_') {
            instance_name += '_';
        }
        instance_name += std::tolower(cur);
        prev = cur;
    }

    // If the generated name doesn't exist yet, use it as is.
    if (resources.find(instance_name) == resources.end()) {
        return instance_name;
    }

    // Append numbers until we find a name that we haven't used yet.
    utils::UInt uniquifier = 1;
    while (true) {
        utils::Str uniquified_sub_instance_name = instance_name + "_" + utils::to_string(uniquifier);
        if (resources.find(uniquified_sub_instance_name) == resources.end()) {
            return uniquified_sub_instance_name;
        }
        uniquifier++;
    }
}

/**
 * Constructs a new, empty resource manager.
 */
Manager::Manager(
    const ir::compat::PlatformRef &platform,
    const utils::Str &architecture,
    const utils::Set<utils::Str> &dnu,
    const Factory &factory,
    const ir::Ref &ir
) :
    factory(factory.configure(architecture, dnu)),
    platform(platform),
    ir(ir),
    resources()
{
}

/**
 * Constructs a resource manager based on the given JSON configuration.
 * Refer to dump_docs() for more information.
 */
Manager Manager::from_json(
    const ir::compat::PlatformRef &platform,
    const utils::Json &json,
    const Factory &factory,
    const ir::Ref &ir
) {
    // Shorthand.
    using JsonType = utils::Json::value_t;

    // Check toplevel type.
    if (json.type() != JsonType::object) {
        throw utils::Exception("resource manager configuration must be an object");
    }

    // Infer (default) architecture from the rest of the platform.
    utils::Str architecture = platform->architecture->family->get_namespace_name();

    // If a "resources" key exists, this is a new-style resource configuration
    // structure. Otherwise it's an old-style structure.
    if (json.find("resources") == json.end()) {

        // Old-style structure. Create the manager using defaults.
        Manager manager{platform, architecture, {}, factory, ir};

        // Add resources to it using the old JSON syntax.
        for (auto it = json.begin(); it != json.end(); ++it) {
            if (it.value().type() != JsonType::object) {
                throw utils::Exception("resource configuration must be an object");
            }
            manager.add_resource(it.key(), "", it.value());
        }

        return manager;
    }

    // New-style structure.
    // Read the strategy structure.
    utils::Set<utils::Str> dnu = {};
    const utils::Json *resources = nullptr;
    for (auto it = json.begin(); it != json.end(); ++it) {
        if (it.key() == "architecture") {
            if (it.value().type() == JsonType::string) {
                architecture = it.value().get<utils::Str>();
            } else {
                throw utils::Exception("resource architecture must be a string if specified");
            }
        } else if (it.key() == "dnu") {
            if (it.value().type() == JsonType::string) {
                dnu.insert(it.value().get<utils::Str>());
            } else if (it.value().type() == JsonType::array) {
                for (const auto &val : it.value()) {
                    if (val.type() == JsonType::string) {
                        dnu.insert(it.value().get<utils::Str>());
                    } else {
                        throw utils::Exception("resource dnu.* must be a string");
                    }
                }
            } else {
                throw utils::Exception("resource dnu must be a string or array of strings if specified");
            }
        } else if (it.key() == "resources") {
            if (it.value().type() == JsonType::object) {
                resources = &it.value();
            } else {
                throw utils::Exception("resources must be an object");
            }
        } else {
            throw utils::Exception("unknown key in resource configuration: " + it.key());
        }
    }
    if (!resources) {
        throw utils::Exception("missing resources key");
    }

    // Create the manager.
    Manager manager{platform, architecture, dnu, factory, ir};

    // Add resources to it.
    for (auto it = resources->begin(); it != resources->end(); ++it) {
        if (it.value().type() != JsonType::object) {
            throw utils::Exception("resource description must be an object");
        }

        // Read the resource description structure.
        utils::Str type = {};
        const utils::Json *config = nullptr;
        for (auto it2 = it.value().begin(); it2 != it.value().end(); ++it2) {
            if (it2.key() == "type") {
                if (it2.value().type() == JsonType::string) {
                    type = it2.value().get<utils::Str>();
                } else {
                    throw utils::Exception("resource type must be a string");
                }
            } else if (it2.key() == "config") {
                if (it2.value().type() == JsonType::object) {
                    config = &it2.value();
                } else {
                    throw utils::Exception("resource configuration must be an object if specified");
                }
            } else {
                throw utils::Exception("unknown key in resource description: " + it2.key());
            }
        }
        if (type.empty()) {
            throw utils::Exception("missing resource type key");
        }

        // Construct the resource.
        if (config) {
            manager.add_resource(type, it.key(), *config);
        } else {
            manager.add_resource(type, it.key(), "{}"_json);
        }

    }

    return manager;
}

/**
 * Builds the default resource manager for the platform. The JSON data is
 * taken from platform.resources.
 */
Manager Manager::from_defaults(
    const ir::compat::PlatformRef &platform,
    const Factory &factory,
    const ir::Ref &ir
) {
    return from_json(platform, platform->resources, factory, ir);
}

/**
 * Writes documentation for the available resource types to the given output
 * stream.
 */
void Manager::dump_resource_types(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    factory.dump_resource_types(os, line_prefix);
}

/**
 * Writes information about the current configuration of this set of
 * resources to the given output stream.
 */
void Manager::dump_config(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    for (const auto &it: resources) {
        const auto &resource = it.second;
        os << line_prefix << "Resource " << resource->get_name();
        os << " of type " << resource->get_type() << ":\n";
        resource->dump_config(os, line_prefix + "    ");
        os << "\n";
    }
    os.flush();
}

/**
 * Adds a resource.
 */
void Manager::add_resource(
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const utils::Json &configuration
) {

    // Generate/check the instance name.
    utils::Str name = instance_name;
    if (name.empty()) {
        name = generate_valid_resource_name(type_name);
    }
    check_resource_name(name);

    // Build the resource.
    auto resource = factory.build_resource(type_name, name, platform, ir, configuration);

    // Add it to our map. This is intentionally on a separate line; otherwise
    // an exception raised while building the resource would leave an empty
    // pointer behind in the resources map.
    resources.set(name) = resource;

}

/**
 * Returns whether a resource with the target instance name exists.
 */
utils::Bool Manager::does_resource_exist(
    const utils::Str &target
) {
    return resources.find(target) != resources.end();
}

/**
 * Removes the resource with the given target instance name, or throws an
 * exception if no such resource exists.
 */
void Manager::remove_resource(
    const utils::Str &target
) {
    if (!does_resource_exist(target)) {
        throw utils::Exception(
            "no resource with name " + target + " exists; cannot remove"
        );
    }
    resources.erase(target);
}

/**
 * Builds a state tracker from the configured list of resources.
 */
State Manager::build(Direction direction) const {
    State state;
    state.resources.reserve(resources.size());
    for (const auto &it : resources) {
        state.resources.emplace_back(it.second.clone());
        state.resources.back()->initialize(direction);
    }
    return state;
}

} // namespace rmgr
} // namespace ql
