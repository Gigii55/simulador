#include "Keyboard.h"
#include "os.h"

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