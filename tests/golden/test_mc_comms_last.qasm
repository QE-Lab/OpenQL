version 1.0
# this file has been automatically generated by the OpenQL compiler please do not modify it manually.
qubits 16

.kernel_comms
    x q[4]
    premove q[4]
    wait 4
    teleportmove q[4],q[2]
    wait 24
    x q[0]
    move q[0],q[1]
    wait 3
    postmove q[2]
    wait 4
    cnot q[1],q[2]
    wait 4