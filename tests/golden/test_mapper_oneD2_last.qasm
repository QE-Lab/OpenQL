# Generated by OpenQL 0.9.0 for program test_mapper_oneD2
version 1.2

pragma @ql.name("test_mapper_oneD2")


.kernel_oneD2
    { # start at cycle 0
        x q[2]
        ym90 q[5]
    }
    cz q[2], q[5]
    skip 1
    { # start at cycle 3
        y90 q[5]
        ym90 q[2]
    }
    cz q[5], q[2]
    skip 1
    { # start at cycle 6
        y90 q[2]
        ym90 q[5]
    }
    cz q[2], q[5]
    skip 1
    { # start at cycle 9
        y90 q[5]
        y90 q[3]
    }
    cz q[5], q[3]
    skip 1
    { # start at cycle 12
        ym90 q[3]
        x q[5]
    }
