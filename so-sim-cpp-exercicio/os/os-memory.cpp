#include "os-memory.h"
#include "os-lib.h"
#include "process.h"
#include "../arch/cpu.h"
#include "../arch/arch.h"
#include "../arch/terminal.h"
#include "../config.h"

#include <cstdint>


namespace OS {

extern Arch::Cpu *cpu;


// meus frames da memoria fissica
const int NUM_FRAMES = Config::phys_mem_size_words / Config::page_size;

bool frame_esta_ocupado[NUM_FRAMES];


int pmm_alocar_frame (){

	for (int i = 0; i < NUM_FRAMES; i++) {

		if (frame_esta_ocupado[i] == false) {
			
			frame_esta_ocupado[i] = true;
			return i;
		}
	}

	return -1;
}


void pmm_liberar_frame(int frame) {
	frame_esta_ocupado[frame] = false;
}


void carregar_processo_na_memoria (Process *proc){
    
	int tamanho_do_programa = proc->image.size();

	int qtd_paginas = (tamanho_do_programa + Config::page_size - 1) >> Config::page_size_bits;

	// repete uma vez PRA CADA PAGINA que o programa precisa
	for (int numero_da_pagina = 0; numero_da_pagina < qtd_paginas; numero_da_pagina++) {

		int inicio_da_pagina = numero_da_pagina << Config::page_size_bits;

		int frame = pmm_alocar_frame();

		if (frame == -1) {
			terminal_println(cpu, Terminal::Kernel, "acabou a memoria fisica ao carregar o processo!");
			cpu->turn_off();
			return;
		}

		proc->frames_alocados.push_back(frame);

		//passsa pro frame fisico
			for (int i = 0; i < Config::page_size; i++) {

				int endereco_virtual_da_palavra = inicio_da_pagina + i;

				uint16_t valor;

				if (endereco_virtual_da_palavra < tamanho_do_programa) {

					valor = proc->image[endereco_virtual_da_palavra];
				}
				else {
					//preenche com 0 pra n deixar lixo na memoria
					valor = 0;
				}

				// aquela mesma tecnica da rua + o n da casa
				int endereco_fisico = (frame << Config::page_size_bits) + i;

				// ele vai lá no endereco_fisico e grava
				cpu->pmem_write(endereco_fisico, valor);
			}

		// agora atualiza a entrada da tabela de paginas dessa pagina
		PageTableEntry &pte = proc->page_table[numero_da_pagina];

		//QUAL FRAME FISSICO MINHA PAG ESTA
		pte.set(Arch::Cpu::PteField::PhyFrameID, frame);
		
		//MARCA 1 PRADIZER QUE ELA EXISTE
		pte.set(Arch::Cpu::PteField::Present,1);

		// diz O QUE pode ser feito com ela: ler, escrever, executar
		pte.set(Arch::Cpu::PteField::Readable,1);
		pte.set(Arch::Cpu::PteField::Writable,1);
		pte.set(Arch::Cpu::PteField::Executable,1);
	}

}

void liberar_memoria_processo (Process *proc) {
	for (int frame : proc->frames_alocados) {
		pmm_liberar_frame(frame);
	}

	proc->frames_alocados.clear();
}

}