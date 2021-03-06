# Generated by OpenQL 0.9.0 for program diamond_api
version 1.2

pragma @ql.name("diamond_api")


.kernel
    prep_z q[0]
    prep_x q[0]
    prep_y q[0]
    initialize q[0]
    measure q[0]
    measure q[0]
    mprep_x q[0]
    measure q[0]
    mprep_y q[0]
    measure q[0]
    x q[0]
    y q[0]
    z q[0]
    s q[0]
    t q[0]
    cnot q[0], q[1]
    cz q[0], q[1]
    h q[2]
    cnot q[1], q[2]
    tdag q[2]
    cnot q[0], q[2]
    t q[2]
    cnot q[1], q[2]
    tdag q[2]
    cnot q[0], q[2]
    t q[1]
    t q[2]
    cnot q[0], q[1]
    h q[2]
    t q[0]
    tdag q[1]
    cnot q[0], q[1]
    cal_measure q[0]
    cal_pi q[0]
    cal_halfpi q[0]
    decouple q[0]
    rx q[0], 1.57
    ry q[0], 1.57
    crk q[0], q[1], 0
    cr q[0], q[1], 3.14
    crc q[0], 30, 5
    rabi_check q[0], 100, 2, 3
    excite_mw q[0], 1, 100, 200, 0, 60
    qentangle q[0], 15
    nventangle q[0], q[1]
    memswap q[0], 1
    sweep_bias q[0], 10, 0, 0, 10, 100, 0
    wait 10, q[0], q[1], q[2]
    qnop q[0]
    calculate_current q[0]
    calculate_voltage q[0]
    sweep_bias q[0], 10, 0, 0, 10, 100, 0
    calculate_voltage q[0]
    skip 13
