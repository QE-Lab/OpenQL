/**
 * @file   qsoverlay.h
 * @date   11/2019
 * @author Diogo Valada
 * @brief  Prepares the quantumsim circuits using the qsoverlay format
 */

#include <vector>
#include <string>
#include <kernel.h>
#include <gate.h>
#include <sstream>


//Only support for DiCarlo setup atm
void write_qsoverlay_program( std::string prog_name, size_t num_qubits,
        std::vector<ql::quantum_kernel>& kernels, const ql::quantum_platform & platform, std::string suffix, size_t ns_per_cycle, bool compiled)
    {

		//TODO remove the next line. Using this because qsoverlay has some bugs when time is explicited
		// compiled = false;



        IOUT("Writing scheduled QSoverlay program");
        // std::ofstream fout;
        std::stringstream fout;
        string qfname( ql::options::get("output_dir") + "/" + prog_name + "_quantumsim_" + suffix + ".py");
        DOUT("Writing scheduled QSoverlay program " << qfname);
        IOUT("Writing scheduled QSoverlay program " << qfname);
        // fout.open( qfname, ios::binary);
        // if ( fout.fail() )
        // {
        //     EOUT("opening file " << qfname << std::endl
        //              << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
        //     return;
        // }

        fout << "# Quantumsim (via Qsoverlay) program generated by OpenQL\n"
             << "# Please modify at your will to obtain extra information from Quantumsim\n\n";

		fout << "#==== WARNING !!!! ======\n"
			 << "#DO NOT USE PREPZ's!! These cause a bug in qsoverlay, making it return a wrong circuit.\n\n";

        fout << "import numpy as np\n"
			 << "from qsoverlay import DiCarlo_setup\n"
			 << "from qsoverlay.circuit_builder import Builder\n";
			
		fout << "from quantumsim.sparsedm import SparseDM\n"
             << "\n"
             << "# print('GPU is used:', sparsedm.using_gpu)\n"
             << "\n"
             << "\n";

				
				
		//Gate correspondence
		std::map <std::string, std::string> gate_map = {
			{"prepz", "prepz"},
			{"x", "X"},
			{"x45", "RX"},
			{"x90", "RX"},
			{"xm45", "RX"},
			{"xm90", "RX"},
			{"y", "Y"},
			{"y45", "RY"},
			{"y90", "RY"},
			{"ym45", "RY"},
			{"ym90", "RY"},
			{"h", "H"},
			{"cz", "CZ"},
			{"measure", "Measure"},
		};

		std::map <std::string, std::string> angles = {
			{"x45", "np.pi/4"},
			{"x90", "np.pi/2"},
			{"xm45", "-np.pi/4"},
			{"xm90", "-np.pi/2"},
			{"y45", "np.pi/4"},
			{"y90", "np.pi/2"},
			{"ym45", "-np.pi/4"},
			{"ym90", "-np.pi/2"},
		};

		if (not compiled)
		{
			gate_map["cnot"] = "CNOT";
			gate_map["t"] = "RZ";
			angles["t"] = "np.pi/4";
			gate_map["tdag"] = "RZ";
			angles["t"] = "-np.pi/4";

		}

		//Create qubit list
        std::vector<size_t> check_usecount;
        check_usecount.resize(num_qubits, 0);
		for (auto & gp: kernels.front().c)
        {
            switch(gp->type())
            {
            // case __classical_gate__:
            // case __wait_gate__:
                // break;
            default:    // quantum gate
                for (auto v: gp->operands)
                {
                    check_usecount[v]++;
                }
                break;
            }
        }

		std::string qubit_list = "";
		// for (size_t qubit=0; qubit < num_qubits; qubit++)
		// {
		// 	qubit_list += "'";
		// 	qubit_list += (qubit != num_qubits-1) ? (std::to_string(qubit) + "', ") : (std::to_string(qubit) + "'");
		// }
		bool loop_helper = false;
		for (size_t qubit=0; qubit < num_qubits; qubit++)
		{
			if (check_usecount[qubit] == 0)
				continue;
			if (loop_helper)
				qubit_list += ", ";
			if (check_usecount[qubit] != 0)
				loop_helper = true;

			qubit_list += "'";
			qubit_list += std::to_string(qubit) + "'";
		}
		std::vector<size_t> qubit_end_times(num_qubits, 1); //The only use is to correct the Circuit Builder times later (due to unwanted behaviour in qsoverlay)

		//Circuit creation
		fout << "\n#Now the circuit is created\n"

			 << "\ndef circuit_generated(noise_flag, setup_name = 'DiCarlo_setup'):\n"
			 << "	qubit_list = [" + qubit_list + "]\n"
			 << "	if setup_name == 'DiCarlo_setup':\n"
			 << "		setup = DiCarlo_setup.quick_setup(qubit_list, noise_flag = noise_flag)\n"
			 << "	b = Builder(setup)\n"
			 << "	b.new_circuit(circuit_title = '" << ( (compiled) ? kernels.front().name + "_mapped" : kernels.front().name ) << "')\n";


		//Circuit creation: Add gates
		for (auto & gate: kernels.front().c)
		{
			std::string qs_name;
			try
			{
  				qs_name = gate_map.at(gate->name);
			}
  			catch (exception& e)
  			{
				// WOUT("Next gate: " + gate->name + " .... WRONG");
				EOUT("Qsoverlay: unknown gate detected!: " + gate->name);
    			throw ql::exception("Qsoverlay: unknown gate detected!:"  + gate->name, false);
				
  			}

			// IOUT(gate->name);
			if (gate->operands.size() == 1)
            {
				// IOUT("Gate operands: " + std::to_string(gate->operands[0]));
				qubit_end_times.at(gate->operands[0]) = (gate->cycle - 1)*ns_per_cycle + gate->duration;
            }
			else if (gate->operands.size() == 2)
            {
				// IOUT("Gate operands: " + std::to_string(gate->operands[0]) + ", " + std::to_string(gate->operands[1]));
				qubit_end_times.at(gate->operands[0]) = (gate->cycle - 1)*ns_per_cycle + gate->duration;
				qubit_end_times.at(gate->operands[1]) = (gate->cycle - 1)*ns_per_cycle + gate->duration;
            }
			else
			{
				IOUT("GATE OPERANDS: Problem encountered");
			}
			

			fout << "	b.add_gate('" << qs_name  << "', " << "['" 
				 << std::to_string(gate->operands[0])
				 << (( gate->operands.size() == 1 ) ? "']" : ("', '" + std::to_string(gate->operands[1]) + "']"));
			
			
			//Add angles for the gates that require it
			if (qs_name == "RX" or qs_name == "RY" or qs_name == "t" or qs_name == "tdag")
				fout << ", angle = " << angles[gate->name];

			
			//Add gate timing, if circuit was compiled.
			if (qs_name == "prepz")
			{
				if (compiled)
					fout << ", time = " << std::to_string((gate->cycle-1)*ns_per_cycle + gate->duration );
			}

			else if (qs_name == "Measure")
			{
				fout << ", output_bit = " << "'" << gate->operands[0] << "_out'";
				if (compiled)
					fout << ", time = " << std::to_string((gate->cycle-1)*ns_per_cycle + gate->duration/4);
			}
			else
			{
				if (compiled)
					fout << ", time = " << std::to_string((gate->cycle-1)*ns_per_cycle + gate->duration/2);
			}
			fout << ")\n";
		}

		//Now we use the qubit_end_cycle values to correct the circuit builder list
		if (compiled)
		{
			fout << "\n";
			fout << "	b.times = {";
			for (size_t i=0; i < num_qubits; i++)
			{
				fout << "'" << i << "' : " << (qubit_end_times.at(i)-1)*ns_per_cycle ;
				if ( i != num_qubits-1)
					fout << ", ";
			}
			fout << "}\n";
		}

		//Now we finalize the circuit
		fout << "\n"
			 << "	b.finalize()\n"
			 << "	return b.circuit\n";


		//Write contents to file
        std::ofstream file_out;
        file_out.open( qfname, ios::binary);
        if ( file_out.fail() )
        {
            EOUT("opening file " << qfname << std::endl
                     << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
            return;
        }
		file_out << fout.str();
        file_out.close();
        IOUT("Writing scheduled QSoverlay program [Done]");
    }
