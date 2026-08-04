#include <stdexcept>
#include <string>
#include <string_view>
#include "../arch/cpu.h"
#include <cstdint>
#include <cstdlib>
#include "../arch/terminal.h"
#include "../config.h"
#include "../lib.h"
#include "../arch/arch.h"
#include "os.h"
#include "os-lib.h"
#include "process.h"
#include "keyboard.h"

namespace OS {
	
Arch::Cpu *cpu;
Keyboard* meu_teclado;

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

Process processo_idle;
Process *processo_atual;

void carregar_processo_na_memoria (Process *proc)
{
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

void boot (Arch::Cpu *cpu)

{
	OS::cpu = cpu;

	processo_idle.process_id = 0;

	//processo_idle.image = Lib::load_from_disk_to_16bit_buffer("archieves/gpf_test.bin"); //estoura memoria
	 processo_idle.image = Lib::load_from_disk_to_16bit_buffer("archieves/idle.bin"); 
	//processo_idle.image = Lib::load_from_disk_to_16bit_buffer("archieves/print2.bin"); //imprime nome
	//processo_idle.image = Lib::load_from_disk_to_16bit_buffer("archieves/syscall_test.bin"); //imprime numero
	processo_atual = &processo_idle;

	// carrega o programa inteiro na memoria fisica, ANTES de comecar a rodar
	carregar_processo_na_memoria(processo_atual);

	cpu->set_page_table(&processo_atual->page_table); //seta minha tabela de pagins
	cpu->set_vmem_mode(VmemMode::Paging); //liga a paginação
	
	meu_teclado = new Keyboard(cpu);

	terminal_println(cpu, Terminal::Command, "Type commands here");
	terminal_println(cpu, Terminal::App, "Apps output here");
	terminal_println(cpu, Terminal::Kernel, "Kernel output here");
}

void interrupt (InterruptCode interrupt_code)
{
	if (interrupt_code == InterruptCode::CpuException) {

		CpuException excecao = cpu->get_ref_cpu_exception();

		terminal_println(cpu, Terminal::Kernel, "EXCESAO (", excecao.type, ") no endereco ", excecao.vaddr, "processo ", processo_atual->process_id, " morto");

		// ""mato"" o processo
		cpu->set_pc(0);
	}

	else if (interrupt_code == InterruptCode::Keyboard) {
		meu_teclado->processar_tecla();
	}
	
}

void syscall () {

	uint16_t codigo = cpu->get_gpr(0);

	if (codigo == 0) {

		terminal_println(cpu, Terminal::Kernel, "processo ", processo_atual->process_id, " fechado");
		cpu->set_pc(0);
		
		}

	else if (codigo == 1) {
		
		uint16_t endereco_virtual = cpu->get_gpr(1);
		std::string texto = "";
		uint16_t caractere;

	do {
		//   37 ÷ 16 = 2
		  int pagina = endereco_virtual >> Config::page_size_bits;
		//    2 × 16 = 32
		  int inicio_pagina = pagina << Config::page_size_bits;
		//     37 - 32 = 5
		  int offset = endereco_virtual - inicio_pagina;

		  //descobrir em qual frame a página está
			PageTableEntry &pte = processo_atual->page_table[pagina];

			uint16_t frame = pte[Arch::Cpu::PteField::PhyFrameID];

			int endereco_fisico = (frame << Config::page_size_bits) + offset;

			caractere = cpu->pmem_read(endereco_fisico);

			if (caractere != 0)
				texto += (char) caractere;

			endereco_virtual++;
		} 
		
		while (caractere != 0);

		terminal_print_str(cpu, Terminal::App, texto);

		}	
			//quebra de linha
			else if (codigo == 2) {
				terminal_print_str(cpu, Terminal::App, "\n");
			}
			// imprime n inteiro
				else if (codigo == 3) {
					uint16_t numero = cpu->get_gpr(1);
					terminal_print(cpu, Terminal::App, numero);
				}
	}

}