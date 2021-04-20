/** \file
 * Resources copied from the CC-light platform.
 *
 * FIXME: needs cleanup, generalization, and conversion to new resource types,
 *  including support for the "undefined" direction (since the mapper was
 *  apparently already using it)
 */

#include "ql/resource/todo.h"

namespace ql {
namespace resource {
namespace todo {

using namespace utils;
using namespace rmgr::resource_types;

// in configuration file, duration is in nanoseconds, while here we prefer it to have it in cycles
// it is needed to define the extend of the resource occupation in case of multi-cycle operations
static UInt ccl_get_operation_duration(const ir::GateRef &ins, const plat::PlatformRef &platform) {
    return ceil( static_cast<Real>(ins->duration) / platform->cycle_time);
}

// operation type is "mw" (for microwave), "flux", "readout", or "extern" (used for inter-core)
// it reflects the different resources used to implement the various gates and that resource management must distinguish
static Str ccl_get_operation_type(const ir::GateRef &ins, const plat::PlatformRef &platform) {
    Str operation_type("cc_light_type");
    QL_JSON_ASSERT(platform->instruction_settings, ins->name, ins->name);
    if (!platform->instruction_settings[ins->name]["type"].is_null()) {
        operation_type = platform->instruction_settings[ins->name]["type"].get<Str>();
    }
    return operation_type;
}

// operation name is used to know which operations are the same when one qwg steers several qubits using the vsm
static Str ccl_get_operation_name(const ir::GateRef &ins, const plat::PlatformRef &platform) {
    Str operation_name(ins->name);
    QL_JSON_ASSERT(platform->instruction_settings, ins->name, ins->name);
    if (!platform->instruction_settings[ins->name]["cc_light_instr"].is_null()) {
        operation_name = platform->instruction_settings[ins->name]["cc_light_instr"].get<Str>();
    }
    return operation_name;
}

ccl_qubit_resource_t::ccl_qubit_resource_t(
    const plat::PlatformRef &platform,
    rmgr::Direction dir
) :
    OldResource("qubits", dir)
{
    // QL_DOUT("... creating " << name << " resource");
    count = platform->resources[name]["count"];
    state.resize(count);
    for (UInt q = 0; q < count; q++) {
        state[q] = (dir == rmgr::Direction::FORWARD ? 0 : ir::MAX_CYCLE);
    }
}

Bool ccl_qubit_resource_t::available(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) const {
    UInt operation_duration = ccl_get_operation_duration(ins, platform);
    Str operation_type = ccl_get_operation_type(ins, platform);

    for (auto q : ins->operands) {
        if (direction == rmgr::Direction::FORWARD) {
            QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  qubit: " << q << " is busy till cycle : " << state[q]);
            if (op_start_cycle < state[q]) {
                QL_DOUT("    " << name << " resource busy ...");
                return false;
            }
        } else {
            QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  qubit: " << q << " is busy from cycle : " << state[q]);
            if (op_start_cycle + operation_duration > state[q]) {
                QL_DOUT("    " << name << " resource busy ...");
                return false;
            }
        }
    }
    QL_DOUT("    " << name << " resource available ...");
    return true;
}

void ccl_qubit_resource_t::reserve(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) {
    Str operation_type = ccl_get_operation_type(ins, platform);
    UInt operation_duration = ccl_get_operation_duration(ins, platform);

    for (auto q : ins->operands) {
        state[q] = (direction == rmgr::Direction::FORWARD ? op_start_cycle + operation_duration : op_start_cycle);
        QL_DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " qubit: " << q << " reserved till/from cycle: " << state[q]);
    }
}

ccl_qwg_resource_t::ccl_qwg_resource_t(
    const plat::PlatformRef &platform,
    rmgr::Direction dir
) :
    OldResource("qwgs", dir)
{
    // QL_DOUT("... creating " << name << " resource");
    count = platform->resources[name]["count"];
    fromcycle.resize(count);
    tocycle.resize(count);
    operations.resize(count);

    for (UInt i = 0; i < count; i++) {
        fromcycle[i] = (dir == rmgr::Direction::FORWARD ? 0 : ir::MAX_CYCLE);
        tocycle[i] = (dir == rmgr::Direction::FORWARD ? 0 : ir::MAX_CYCLE);
        operations[i] = "";
    }
    auto & constraints = platform->resources[name]["connection_map"];
    for (auto it = constraints.cbegin(); it != constraints.cend(); ++it) {
        // COUT(it.key() << " : " << it.value() );
        UInt qwgNo = stoi( it.key() );
        auto & connected_qubits = it.value();
        for (auto &q : connected_qubits) {
            qubit2qwg.set(q) = qwgNo;
        }
    }
}

Bool ccl_qwg_resource_t::available(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) const {
    Str operation_type = ccl_get_operation_type(ins, platform);
    Str operation_name = ccl_get_operation_name(ins, platform);
    UInt      operation_duration = ccl_get_operation_duration(ins, platform);

    Bool is_mw = (operation_type == "mw");
    if (is_mw) {
        for (auto q : ins->operands) {
            QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  qwg: " << qubit2qwg.at(q) << " is busy from cycle: " << fromcycle[qubit2qwg.at(q)] << " to cycle: " << tocycle[qubit2qwg.at(q)] << " for operation: " << operations[qubit2qwg.at(q)]);
            if (direction == rmgr::Direction::FORWARD) {
                if (
                    op_start_cycle < fromcycle[qubit2qwg.at(q)]
                    || (op_start_cycle < tocycle[qubit2qwg.at(q)] && operations[qubit2qwg.at(q)] != operation_name)
                ) {
                    QL_DOUT("    " << name << " resource busy ...");
                    return false;
                }
            } else {
                if (
                    op_start_cycle + operation_duration > tocycle[qubit2qwg.at(q)]
                    || ( op_start_cycle + operation_duration > fromcycle[qubit2qwg.at(q)] && operations[qubit2qwg.at(q)] != operation_name)
                ) {
                    QL_DOUT("    " << name << " resource busy ...");
                    return false;
                }
            }
        }
        QL_DOUT("    " << name << " resource available ...");
    }
    return true;
}

void ccl_qwg_resource_t::reserve(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) {
    Str operation_type = ccl_get_operation_type(ins, platform);
    Str operation_name = ccl_get_operation_name(ins, platform);
    UInt      operation_duration = ccl_get_operation_duration(ins, platform);

    Bool is_mw = operation_type == "mw";
    if (is_mw) {
        for (auto q : ins->operands) {
            if (direction == rmgr::Direction::FORWARD) {
                if (operations[qubit2qwg.at(q)] == operation_name) {
                    tocycle[qubit2qwg.at(q)] = max(tocycle[qubit2qwg.at(q)], op_start_cycle + operation_duration);
                } else {
                    fromcycle[qubit2qwg.at(q)] = op_start_cycle;
                    tocycle[qubit2qwg.at(q)] = op_start_cycle + operation_duration;
                    operations[qubit2qwg.at(q)] = operation_name;
                }
            } else {
                if (operations[qubit2qwg.at(q)] == operation_name) {
                    fromcycle[qubit2qwg.at(q)] = min(fromcycle[qubit2qwg.at(q)], op_start_cycle);
                } else {
                    fromcycle[qubit2qwg.at(q)] = op_start_cycle;
                    tocycle[qubit2qwg.at(q)] = op_start_cycle + operation_duration;
                    operations[qubit2qwg.at(q)] = operation_name;
                }
            }
            QL_DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " qwg: " << qubit2qwg.at(q) << " reserved from cycle: " << fromcycle[qubit2qwg.at(q)] << " to cycle: " << tocycle[qubit2qwg.at(q)] << " for operation: " << operations[qubit2qwg.at(q)]);
        }
    }
}

ccl_meas_resource_t::ccl_meas_resource_t(
    const plat::PlatformRef &platform,
    rmgr::Direction dir
) :
    OldResource("meas_units", dir)
{
    // QL_DOUT("... creating " << name << " resource");
    count = platform->resources[name]["count"];
    fromcycle.resize(count);
    tocycle.resize(count);

    for (UInt i=0; i<count; i++) {
        fromcycle[i] = (dir == rmgr::Direction::FORWARD ? 0 : ir::MAX_CYCLE);
        tocycle[i] = (dir == rmgr::Direction::FORWARD ? 0 : ir::MAX_CYCLE);
    }
    auto &constraints = platform->resources[name]["connection_map"];
    for (auto it = constraints.begin(); it != constraints.end(); ++it) {
        // COUT(it.key() << " : " << it.value());
        UInt measUnitNo = stoi( it.key() );
        auto & connected_qubits = it.value();
        for (auto &q : connected_qubits) {
            qubit2meas.set(q) = measUnitNo;
        }
    }
}

Bool ccl_meas_resource_t::available(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) const {
    Str operation_type = ccl_get_operation_type(ins, platform);
    UInt operation_duration = ccl_get_operation_duration(ins, platform);

    Bool is_measure = (operation_type == "readout");
    if (is_measure) {
        for (auto q : ins->operands) {
            QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << "  meas: " << qubit2meas.at(q) << " is busy from cycle: " << fromcycle[qubit2meas.at(q)] << " to cycle: " << tocycle[qubit2meas.at(q)] );
            if (direction == rmgr::Direction::FORWARD) {
                if (op_start_cycle != fromcycle[qubit2meas.at(q)]) {
                    // If current measurement on same measurement-unit does not start in the
                    // same cycle, then it should wait for current measurement to finish
                    if (op_start_cycle < tocycle[qubit2meas.at(q)]) {
                        QL_DOUT("    " << name << " resource busy ...");
                        return false;
                    }
                }
            } else {
                if (op_start_cycle != fromcycle[qubit2meas.at(q)]) {
                    // If current measurement on same measurement-unit does not start in the
                    // same cycle, then it should wait until it would finish at start of or earlier than current measurement
                    if (op_start_cycle + operation_duration > fromcycle[qubit2meas.at(q)]) {
                        QL_DOUT("    " << name << " resource busy ...");
                        return false;
                    }
                }
            }
        }
        QL_DOUT("    " << name << " resource available ...");
    }
    return true;
}

void ccl_meas_resource_t::reserve(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) {
    Str operation_type = ccl_get_operation_type(ins, platform);
    UInt      operation_duration = ccl_get_operation_duration(ins, platform);

    Bool is_measure = (operation_type == "readout");
    if (is_measure) {
        for (auto q : ins->operands) {
            fromcycle[qubit2meas.at(q)] = op_start_cycle;
            tocycle[qubit2meas.at(q)] = op_start_cycle + operation_duration;
            QL_DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " meas: " << qubit2meas.at(q) << " reserved from cycle: " << fromcycle[qubit2meas.at(q)] << " to cycle: " << tocycle[qubit2meas.at(q)]);
        }
    }
}

ccl_edge_resource_t::ccl_edge_resource_t(
    const plat::PlatformRef &platform,
    rmgr::Direction dir
) :
    OldResource("edges", dir)
{
    // QL_DOUT("... creating " << name << " resource");
    count = platform->resources[name]["count"];
    state.resize(count);

    for (UInt i = 0; i < count; i++) {
        state[i] = (dir == rmgr::Direction::FORWARD ? 0 : ir::MAX_CYCLE);
    }

    if (platform->topology.count("edges") <= 0) {
        QL_FATAL("topology[\"edges\"] not defined in configuration file");
    }
    for (auto &anedge : platform->topology["edges"]) {
        UInt s = anedge["src"];
        UInt d = anedge["dst"];
        UInt e = anedge["id"];

        qubits_pair_t aqpair(s,d);
        auto it = qubits2edge.find(aqpair);
        if (it != qubits2edge.end()) {
            QL_EOUT("re-defining edge " << s << "->" << d << " !");
            throw Exception("[x] Error : re-defining edge !", false);
        } else {
            qubits2edge.set(aqpair) = e;
        }
    }

    if (platform->resources[name].count("connection_map") <= 0) {
        QL_FATAL("resources[[\"edges\"][\"connection_map\"] not defined in configuration file");
    }
    auto &constraints = platform->resources[name]["connection_map"];
    for (auto it = constraints.cbegin(); it != constraints.cend(); ++it) {
        // COUT(it.key() << " : " << it.value() << "\n");
        UInt edgeNo = stoi( it.key() );
        auto &connected_edges = it.value();
        for (auto &e : connected_edges) {
            edge2edges.set(e).push_back(edgeNo);
        }
    }
}

Bool ccl_edge_resource_t::available(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) const {
    Str operation_type = ccl_get_operation_type(ins, platform);
    UInt operation_duration = ccl_get_operation_duration(ins, platform);

    auto gname = ins->name;
    Bool is_flux = (operation_type == "flux");
    if (is_flux) {
        auto nopers = ins->operands.size();
        if (nopers == 1) {
            // single qubit flux operation does not reserve an edge resource
            QL_DOUT(" available for single qubit flux operation: " << name);
        } else if (nopers == 2) {
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto it = qubits2edge.find(aqpair);
            if (it != qubits2edge.end()) {
                auto edge_no = qubits2edge.at(aqpair);

                QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << ", edge: " << edge_no << " is busy till/from cycle : " << state[edge_no] << " for operation: " << ins->name);

                Vec<UInt> edges2check(edge2edges.get(edge_no));
                edges2check.push_back(edge_no);
                for (auto &e : edges2check) {
                    if (direction == rmgr::Direction::FORWARD) {
                        if (op_start_cycle < state[e]) {
                            QL_DOUT("    " << name << " resource busy ...");
                            return false;
                        }
                    } else {
                        if (op_start_cycle + operation_duration > state[e]) {
                            QL_DOUT("    " << name << " resource busy ...");
                            return false;
                        }
                    }
                }
                QL_DOUT("    " << name << " resource available ...");
            } else {
                QL_FATAL("Use of illegal edge: " << q0 << "->" << q1 << " in operation: " << ins->name << " !");
            }
        } else {
            QL_FATAL("Incorrect number of operands used in operation: " << ins->name << " !");
        }
    }
    return true;
}

void ccl_edge_resource_t::reserve(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) {
    Str operation_type = ccl_get_operation_type(ins, platform);
    UInt operation_duration = ccl_get_operation_duration(ins, platform);

    auto gname = ins->name;
    Bool is_flux = (operation_type == "flux");
    if (is_flux) {
        auto nopers = ins->operands.size();
        if (nopers == 1) {
            // single qubit flux operation does not reserve an edge resource
        } else if (nopers == 2) {
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto edge_no = qubits2edge.at(aqpair);
            if (direction == rmgr::Direction::FORWARD) {
                state[edge_no] = op_start_cycle + operation_duration;
                for (auto &e : edge2edges.get(edge_no)) {
                    state[e] = op_start_cycle + operation_duration;
                }
            } else {
                state[edge_no] = op_start_cycle;
                for (auto &e : edge2edges.get(edge_no)) {
                    state[e] = op_start_cycle;
                }
            }
            QL_DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " edge: " << edge_no << " reserved till cycle: " << state[ edge_no ] << " for operation: " << ins->name);
        } else {
            QL_FATAL("Incorrect number of operands used in operation: " << ins->name << " !");
        }
    }
}

ccl_detuned_qubits_resource_t::ccl_detuned_qubits_resource_t(
    const plat::PlatformRef &platform,
    rmgr::Direction dir
) :
    OldResource("detuned_qubits", dir)
{
    // QL_DOUT("... creating " << name << " resource");
    count = platform->resources[name]["count"];
    fromcycle.resize(count);
    tocycle.resize(count);
    operations.resize(count);

    // initialize resource state machine to be free for all qubits
    for (UInt i = 0; i < count; i++) {
        fromcycle[i] = (dir == rmgr::Direction::FORWARD ? 0 : ir::MAX_CYCLE);
        tocycle[i] = (dir == rmgr::Direction::FORWARD ? 0 : ir::MAX_CYCLE);
        operations[i] = "";
    }

    // initialize qubitpair2edge map from json description; this is a constant map
    if (platform->topology.count("edges") <= 0) {
        QL_FATAL("topology[\"edges\"] not defined in configuration file");
    }
    for (auto &anedge : platform->topology["edges"]) {
        UInt s = anedge["src"];
        UInt d = anedge["dst"];
        UInt e = anedge["id"];

        qubits_pair_t aqpair(s,d);
        auto it = qubitpair2edge.find(aqpair);
        if (it != qubitpair2edge.end()) {
            QL_EOUT("re-defining edge " << s << "->" << d << " !");
            throw Exception("[x] Error : re-defining edge !", false);
        } else {
            qubitpair2edge.set(aqpair) = e;
        }
    }

    // initialize edge_detunes_qubits map from json description; this is a constant map
    if (platform->resources[name].count("connection_map") <= 0) {
        QL_FATAL("resources[[\"detuned_qubits\"][\"connection_map\"] not defined in configuration file");
    }
    auto &constraints = platform->resources[name]["connection_map"];
    for (auto it = constraints.cbegin(); it != constraints.cend(); ++it) {
        // COUT(it.key() << " : " << it.value() << "\n");
        UInt edgeNo = stoi( it.key() );
        auto & detuned_qubits = it.value();
        for (auto & q : detuned_qubits) {
            edge_detunes_qubits.set(edgeNo).push_back(q);
        }
    }
}

// When a two-qubit flux gate, check whether the qubits it would detune are not busy with a rotation.
// When a one-qubit rotation, check whether the qubit is not detuned (busy with a flux gate).
Bool ccl_detuned_qubits_resource_t::available(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) const {
    Str operation_type = ccl_get_operation_type(ins, platform);
    UInt      operation_duration = ccl_get_operation_duration(ins, platform);

    auto gname = ins->name;
    Bool is_flux = operation_type == "flux";
    if (is_flux) {
        auto nopers = ins->operands.size();
        if (nopers == 1) {
            // single qubit flux operation does not reserve a detuned qubits resource
            QL_DOUT(" available for single qubit flux operation: " << name);
        } else if (nopers == 2) {
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto it = qubitpair2edge.find(aqpair);
            if (it != qubitpair2edge.end()) {
                auto edge_no = qubitpair2edge.at(aqpair);

                for (auto &q : edge_detunes_qubits.get(edge_no)) {
                    QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << ", edge: " << edge_no << " detuning qubit: " << q << " for operation: " << ins->name << " busy from: " << fromcycle[q] << " till: " << tocycle[q] << " with operation_type: " << operation_type);
                    if (direction == rmgr::Direction::FORWARD) {
                        if (
                            op_start_cycle < fromcycle[q]
                            || ( op_start_cycle < tocycle[q] && operations[q] != operation_type)
                        ) {
                            QL_DOUT("    " << name << " resource busy for a two-qubit gate...");
                            return false;
                        }
                    } else {
                        if (
                            op_start_cycle + operation_duration > tocycle[q]
                            || ( op_start_cycle + operation_duration > fromcycle[q] && operations[q] != operation_type)
                        ) {
                            QL_DOUT("    " << name << " resource busy for a two-qubit gate...");
                            return false;
                        }
                    }
                }	// for over edges
            } else { // no edge found
                QL_EOUT("Use of illegal edge: " << q0 << "->" << q1 << " in operation: " << ins->name << " !");
                throw Exception("[x] Error : Use of illegal edge" + to_string(q0) + "->" + to_string(q1) + "in operation:" + ins->name + " !", false);
            }
        } else { // nopers != 1 or 2
            QL_FATAL("Incorrect number of operands used in operation: " << ins->name << " !");
        }
    }

    Bool is_mw = operation_type == "mw";
    if (is_mw) {
        for (auto q : ins->operands) {
            QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle << ", qubit: " << q << " for operation: " << ins->name << " busy from: " << fromcycle[q] << " till: " << tocycle[q] << " with operation_type: " << operation_type);
            if (direction == rmgr::Direction::FORWARD) {
                if (op_start_cycle < fromcycle[q]) {
                    QL_DOUT("    " << name << " busy for rotation: op_start cycle " << op_start_cycle << " < fromcycle[" << q << "] " << fromcycle[q] );
                    return false;
                }
                if (op_start_cycle < tocycle[q] && operations[q] != operation_type) {
                    QL_DOUT("    " << name << " busy for rotation with flux: op_start cycle " << op_start_cycle << " < tocycle[" << q << "] " << tocycle[q] );
                    return false;
                }
            } else {
                if (op_start_cycle + operation_duration > tocycle[q]) {
                    QL_DOUT("    " << name << " busy for rotation: op_start cycle " << op_start_cycle << " + duration > tocycle[" << q << "] " << tocycle[q] );
                    return false;
                }
                if (op_start_cycle + operation_duration > fromcycle[q] && operations[q] != operation_type) {
                    QL_DOUT("    " << name << " busy for rotation with flux: op_start cycle " << op_start_cycle << " + duration > fromcycle[" << q << "] " << fromcycle[q] );
                    return false;
                }
            }
        }
    }
    if (is_flux || is_mw) QL_DOUT("    " << name << " resource available ...");
    return true;
}

// A two-qubit flux gate must set the qubits it would detune to detuned, busy with a flux gate.
// A one-qubit rotation gate must set its operand qubit to busy, busy with a rotation.
void ccl_detuned_qubits_resource_t::reserve(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) {
    Str operation_type = ccl_get_operation_type(ins, platform);
    UInt operation_duration = ccl_get_operation_duration(ins, platform);

    auto gname = ins->name;
    Bool is_flux = operation_type == "flux";
    if (is_flux) {
        auto nopers = ins->operands.size();
        if (nopers == 1) {
            // single qubit flux operation does not reserve a detuned qubits resource
        } else if (nopers == 2) {
            auto q0 = ins->operands[0];
            auto q1 = ins->operands[1];
            qubits_pair_t aqpair(q0, q1);
            auto edge_no = qubitpair2edge.at(aqpair);

            for (auto &q : edge_detunes_qubits.get(edge_no)) {
                if (direction == rmgr::Direction::FORWARD) {
                    if (operations[q] == operation_type) {
                        tocycle[q] = max(tocycle[q], op_start_cycle + operation_duration);
                        QL_DOUT("reserving " << name << ". for qubit: " << q << " reusing cycle: " << fromcycle[q] << " to extending tocycle: " << tocycle[q] << " for old operation: " << ins->name);
                    } else {
                        fromcycle[q] = op_start_cycle;
                        tocycle[q] = op_start_cycle + operation_duration;
                        operations[q] = operation_type;
                        QL_DOUT("reserving " << name << ". for qubit: " << q << " from fromcycle: " << fromcycle[q] << " to new tocycle: " << tocycle[q] << " for new operation: " << ins->name);
                    }
                } else {
                    if (operations[q] == operation_type) {
                        fromcycle[q] = min(fromcycle[q], op_start_cycle);
                        QL_DOUT("reserving " << name << ". for qubit: " << q << " from extended cycle: " << fromcycle[q] << " reusing tocycle: " << tocycle[q] << " for old operation: " << ins->name);
                    } else {
                        fromcycle[q] = op_start_cycle;
                        tocycle[q] = op_start_cycle + operation_duration;
                        operations[q] = operation_type;
                        QL_DOUT("reserving " << name << ". for qubit: " << q << " from new cycle: " << fromcycle[q] << " to tocycle: " << tocycle[q] << " for new operation: " << ins->name);
                    }
                }
                QL_DOUT("reserved " << name << ". op_start_cycle: " << op_start_cycle << " edge: " << edge_no << " detunes qubit: " << q << " reserved from cycle: " << fromcycle[q] << " till cycle: " << tocycle[q] << " for operation: " << ins->name);
            }
        } else {
            QL_FATAL("Incorrect number of operands used in operation: " << ins->name << " !");
        }
    }
    Bool is_mw = operation_type == "mw";
    if (is_mw) {
        for (auto q : ins->operands) {
            if (direction == rmgr::Direction::FORWARD) {
                if (operations[q] == operation_type) {
                    tocycle[q] = max(tocycle[q], op_start_cycle + operation_duration);
                    QL_DOUT("reserving " << name << ". for qubit: " << q << " reusing cycle: " << fromcycle[q] << " to extending tocycle: " << tocycle[q] << " for old operation: " << ins->name);
                } else {
                    fromcycle[q] = op_start_cycle;
                    tocycle[q] = op_start_cycle + operation_duration;
                    operations[q] = operation_type;
                    QL_DOUT("reserving " << name << ". for qubit: " << q << " from fromcycle: " << fromcycle[q] << " to new tocycle: " << tocycle[q] << " for new operation: " << ins->name);
                }
            } else {
                if (operations[q] == operation_type) {
                    fromcycle[q] = min(fromcycle[q], op_start_cycle);
                    QL_DOUT("reserving " << name << ". for qubit: " << q << " from extended cycle: " << fromcycle[q] << " reusing tocycle: " << tocycle[q] << " for old operation: " << ins->name);
                } else {
                    fromcycle[q] = op_start_cycle;
                    tocycle[q] = op_start_cycle + operation_duration;
                    operations[q] = operation_type;
                    QL_DOUT("reserving " << name << ". for qubit: " << q << " from new cycle: " << fromcycle[q] << " to tocycle: " << tocycle[q] << " for new operation: " << ins->name);
                }
            }
            QL_DOUT("... reserved " << name << ". op_start_cycle: " << op_start_cycle << " for qubit: " << q << " reserved from cycle: " << fromcycle[q] << " till cycle: " << tocycle[q] << " for operation: " << ins->name);
        }
    }
}

ccl_channel_resource_t::ccl_channel_resource_t(
    const plat::PlatformRef &platform,
    rmgr::Direction dir
) :
    OldResource("channels", dir)
{
    QL_DOUT("... creating " << name << " resource");

    // ncores = topology.number_of_cores: total number of cores
    if (platform->topology.count("number_of_cores") <= 0) {
        ncores = 1;
        QL_DOUT("Number of cores (topology[\"number_of_cores\"] not defined; assuming: " << ncores);
    } else {
        ncores = platform->topology["number_of_cores"];
        if (ncores <= 0) {
            QL_FATAL("Number of cores (topology[\"number_of_cores\"]) is not a positive value: " << ncores);
        }
    }
    QL_DOUT("Number of cores = " << ncores);

    // nchannels = resources.channels.count: number of channels in each core
    if (platform->resources[name].count("count") <= 0) {
        nchannels = platform->qubit_count/ncores;   // i.e. as many as there are qubits in a core
        QL_DOUT("Number of channels per core (resources[\"channels\"][\"count\"]) not defined; assuming: " << nchannels);
    } else {
        nchannels = platform->resources[name]["count"];
        if (nchannels <= 0) {
            QL_DOUT("Number of channels per core (resources[\"channels\"][\"count\"]) is not a positive value: " << nchannels);
            nchannels = platform->qubit_count/ncores;   // i.e. as many as there are qubits in a core
        }
        if (nchannels > platform->qubit_count/ncores) {
            QL_FATAL("Number of channels per core (resources[\"channels\"][\"count\"]) is larger than number of qubits per core: " << nchannels);
        }
    }
    QL_DOUT("Number of channels per core= " << nchannels);

    state.resize(ncores);
    for (UInt i=0; i<ncores; i++) state[i].resize(nchannels, (dir == rmgr::Direction::FORWARD ? 0 : ir::MAX_CYCLE));
}

Bool ccl_channel_resource_t::available(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) const {
    Str operation_type = ccl_get_operation_type(ins, platform);
    UInt operation_duration = ccl_get_operation_duration(ins, platform);

    Bool is_ic = (operation_type == "extern");
    if (is_ic) {
        QL_DOUT(" available " << name << "? op_start_cycle: " << op_start_cycle  << " for: " << ins->qasm());
        if (direction == rmgr::Direction::FORWARD) {
            for (auto q : ins->operands) {
                UInt core = q/(platform->qubit_count/ncores);
                Bool is_avail = false;
                // fwd: channel c is busy till cycle=state[core][c],
                // when reserving state[core][c] = start_cycle + duration
                // i.e. all cycles < state[core][c] it is busy, i.e. available when start_cycle >= state[core][c]
                QL_DOUT(" available " << name << "? ... q=" << q << " core=" << core);
                for (UInt c=0; c<nchannels; c++) {
                    QL_DOUT(" available " << name << "? ... c=" << c);
                    if (
                        op_start_cycle >= state[core][c]
                    ) {
                        QL_DOUT(" available " << name << "! for qubit: " << q << " in core: " << core << " channel: " << c << " available");
                        is_avail = true;
                        break;
                    }
                }
                if (!is_avail) {
                     QL_DOUT(" busy " << name << "! for qubit: " << q << " in core: " << core << " all channels busy");
                     return false;
                }
            }
        } else {
            for (auto q : ins->operands) {
                UInt core = q/(platform->qubit_count/ncores);
                Bool is_avail = false;
                // bwd: channel c is busy from cycle=state[core][c],
                // when reserving state[core][c] = start_cycle
                // i.e. all cycles >= state[core][c] it is busy, i.e. available when start_cycle + duration <= state[core][c]
                QL_DOUT(" available " << name << "? ... q=" << q << " core=" << core);
                for (UInt c=0; c<nchannels; c++) {
                    QL_DOUT(" available " << name << "? ... c=" << c);
                    if (
                        op_start_cycle + operation_duration <= state[core][c]
                    ) {
                        QL_DOUT(" available " << name << "! for qubit: " << q << " in core: " << core << " channel: " << c << " available");
                        is_avail = true;
                        break;
                    }
                }
                if (!is_avail) {
                    QL_DOUT(" busy " << name << "! for qubit: " << q << " in core: " << core << " all channels busy");
                    return false;
                }
            }
        }
        QL_DOUT(" available " << name << " resource available for: " << ins->qasm());
    }
    return true;
}

void ccl_channel_resource_t::reserve(
    UInt op_start_cycle,
    const ir::GateRef &ins,
    const plat::PlatformRef &platform
) {
    // for each operand:
    //     find a free channel c and then do
    //     state[core][c] = (forward_scheduling == direction ?  op_start_cycle + operation_duration : op_start_cycle );
    Str operation_type = ccl_get_operation_type(ins, platform);
    UInt      operation_duration = ccl_get_operation_duration(ins, platform);

    Bool is_ic = (operation_type == "extern");
    if (is_ic) {
        QL_DOUT(" reserve " << name << "? op_start_cycle: " << op_start_cycle  << " for: " << ins->qasm());
        if (direction == rmgr::Direction::FORWARD) {
            for (auto q : ins->operands) {
                UInt core = q/(platform->qubit_count/ncores);
                Bool is_avail = false;
                // fwd: channel c is busy till cycle=state[core][c],
                // when reserving state[core][c] = start_cycle + duration
                // i.e. all cycles < state[core][c] it is busy, i.e. available when start_cycle >= state[core][c]
                for (UInt c=0; c<nchannels; c++) {
                    if (
                        op_start_cycle >= state[core][c]
                    ) {
                        state[core][c] = op_start_cycle + operation_duration;
                        QL_DOUT(" reserved " << name << "? for qubit: " << q << " in core: " << core << " channel: " << c << "till cycle: " << state[core][c]);
                        is_avail = true;
                        break;
                    }
                }
                QL_ASSERT(is_avail);
            }
        } else {
            // bwd: channel c is busy from cycle=state[core][c],
            // i.e. all cycles >= state[core][c] it is busy, i.e. available when start_cycle <= state[core][c]
            for (auto q : ins->operands) {
                UInt core = q/(platform->qubit_count/ncores);
                Bool is_avail = false;
                // bwd: channel c is busy from cycle=state[core][c],
                // when reserving state[core][c] = start_cycle
                // i.e. all cycles >= state[core][c] it is busy, i.e. available when start_cycle <= state[core][c]
                for (UInt c=0; c<nchannels; c++) {
                    if (
                        op_start_cycle + operation_duration <= state[core][c]
                    ) {
                        state[core][c] = op_start_cycle;
                        QL_DOUT(" reserved " << name << "? for qubit: " << q << " in core: " << core << " channel: " << c << " from cycle: " << state[core][c]);
                        is_avail = true;
                        break;
                    }
                }
                QL_ASSERT(is_avail);
            }
        }
    }
}

} // namespace todo



} // namespace resource
} // namespace ql