# Generated by OpenQL 0.9.0 for program RAR
version 1.2

pragma @ql.name("RAR")


.aKernel
    { # start at cycle 0
        prepz q[0]
        prepz q[1]
        prepz q[2]
        prepz q[3]
    }
    skip 7
    cz q[0], q[1]
    skip 7
    { # start at cycle 16
        cz q[0], q[2]
        measure q[1]
    }
    skip 15
    measure q[0]
    skip 59
