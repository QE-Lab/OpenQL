# Generated by OpenQL 0.9.0 for program test_7_ALAP
version 1.2

pragma @ql.name("test_7_ALAP")


.kernel_7_ALAP
    prepz q[0]
    skip 1
    h q[0]
    skip 1
    { # start at cycle 4
        prepz q[2]
        t q[0]
    }
    skip 1
    { # start at cycle 6
        prepz q[5]
        h q[0]
        h q[2]
    }
    skip 1
    { # start at cycle 8
        prepz q[1]
        prepz q[3]
        prepz q[4]
        prepz q[6]
        t q[0]
        t q[2]
        h q[5]
    }
    skip 1
    { # start at cycle 10
        measure q[0]
        measure q[1]
        measure q[2]
        measure q[3]
        measure q[4]
        measure q[5]
        measure q[6]
    }
    skip 1
