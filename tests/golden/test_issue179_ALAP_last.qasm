version 1.0
# this file has been automatically generated by the OpenQL compiler please do not modify it manually.
qubits 7

.kernel_issue179_ALAP
    y q[3]
    wait 1
    { x q[2] | x q[4] }
    wait 1