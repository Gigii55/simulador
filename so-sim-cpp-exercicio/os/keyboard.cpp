#include "Keyboard.h"
#include "os.h"
#include "process.h"
#include "os-lib.h"
#include "../arch/terminal.h"
#include <sstream>

Keyboard::Keyboard(Arch::Cpu* ponteiro_cpu) {
	cpu = ponteiro_cpu;
	buffer_teclado = "";
}

void Keyboard::processar_tecla() {

	char c = (char) cpu->read_io(2);
	
	if (c == 8 || c == 127) {

        if (!buffer_teclado.empty()) {
            buffer_teclado.pop_back();
        }

        // apaga a linha atual
        cpu->write_io(0, 2);
        cpu->write_io(1, '\r');

        // escreve novamente o que sobrou
        for (char letra : buffer_teclado) {
            cpu->write_io(1, letra);
        }

        return;
    }

	cpu->write_io(0, 2); 
	cpu->write_io(1, c); 

	if (c == '\n') {
		
		if (buffer_teclado == "fechar") {
			cpu->turn_off(); 
		}

        else if (buffer_teclado == "matar") {
            OS::matar_processo_atual();
        }

		else if (buffer_teclado.substr(0, 6) == "matar ") {

		std::string texto_pid = buffer_teclado.substr(6);

		std::istringstream entrada(texto_pid);

		int pid;
		char sobra;

		if (entrada >> pid && !(entrada >> sobra)) {
				OS::matar_processo(pid);
			}

		else {
			OS::terminal_println(cpu, OS::Terminal::Kernel,"ERRO: PID invalido!");
			}
		}

		else if (buffer_teclado.substr(0, 6) == "timer ") {

		std::string texto_ciclos = buffer_teclado.substr(6);

		std::istringstream entrada(texto_ciclos);

		int ciclos;
		char sobra;

		if (entrada >> ciclos && !(entrada >> sobra)) {
			OS::configurar_timer(ciclos);
		}
		else {
			OS::terminal_println(cpu, OS::Terminal::Kernel,"ERRO: numero de ciclos invalido!");
		}
	}
		else if (buffer_teclado == "listar") {
    	OS::listar_processos();
		}

		else if (buffer_teclado.substr(0, 9) == "carregar ") {

			std::string nome = buffer_teclado.substr(9);

			OS::carregar_programa(nome);
		}
		

		buffer_teclado = ""; 
	} 
	else {
		
		buffer_teclado += c;
	}
}