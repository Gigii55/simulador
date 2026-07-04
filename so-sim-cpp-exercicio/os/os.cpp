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

namespace OS {
	
Arch::Cpu *cpu;

// memoria RAM simulada (memoria fisica)
// meus frames da memoria fissica
const int NUM_FRAMES = Config::phys_mem_size_words / Config::page_size;

bool frame_esta_ocupado[NUM_FRAMES]; //verifico se o frame está ocupado true e false

// procura um frame livre, marca como ocupado e devolve o numero dele
// devolve -1 se nao achou nenhum livre (memoria acabou)
int pmm_alocar_frame (){

	for (int i = 0; i < NUM_FRAMES; i++) {

		if (frame_esta_ocupado[i] == false) {
			
			frame_esta_ocupado[i] = true;
			return i;
		}
	}
	
	return -1;
}

Process processo_principal;
Process *processo_atual;

void tratar_page_fault (Process *proc, uint16_t endereco_virtual) {

	// qual pagina contem esse endereco?
	int numero_da_pagina = endereco_virtual >> Config::page_size_bits;

	// endereco virtual do INICIO dessa pagina
	int inicio_da_pagina = numero_da_pagina << Config::page_size_bits;

	// pega um frame fisico livre
	int frame = pmm_alocar_frame();

	if (frame == -1) {
		terminal_println(cpu, Terminal::Kernel, "acabou a memoria fisica! processo");
		return;
	}

	//inicia um loop que vai rodar exatamente o número de vezes igual ao tamanho de uma página
	for (int i = 0; i < Config::page_size; i++) {

		//somando 'i' (o deslocamento/offset), é o endereço virtual exato do que ta processando agora
		int endereco_virtual_da_palavra = inicio_da_pagina + i;

		uint16_t valor; //vai guardar os valoress

		//o endereço virtual atual é menor que o tamanho total do programa?
		if (endereco_virtual_da_palavra < (int) proc->image.size()) {

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
	//sem ele, a CPU acha que a pagina
	//ainda nao existe e gera page fault de novo, pra sempre

	// diz O QUE pode ser feito com ela: ler, escrever, executar
	pte.set(Arch::Cpu::PteField::Readable,1);
	pte.set(Arch::Cpu::PteField::Writable,1);
	pte.set(Arch::Cpu::PteField::Executable,1);
}

void boot (Arch::Cpu *cpu)
{
	OS::cpu = cpu;

	processo_principal.process_id = 1;
	processo_principal.image = Lib::load_from_disk_to_16bit_buffer("foto.bin"); //carrega o programa(nao tenho o arquivo ainda)

	processo_atual = &processo_principal; // ponteiro aponta para o processo de cima

	cpu->set_page_table(&processo_atual->page_table); //seta minha tabela de pagins
	cpu->set_vmem_mode(VmemMode::Paging); //liga a paginação

	terminal_println(cpu, Terminal::Command, "Type commands here");
	terminal_println(cpu, Terminal::App, "Apps output here");
	terminal_println(cpu, Terminal::Kernel, "Kernel output here");
}


void interrupt (const InterruptCode interrupt_code)
{
	if (interrupt_code == InterruptCode::CpuException) {

		CpuException excecao = cpu->get_ref_cpu_exception(); // é uma execao da CPU?

		if (excecao.type == CpuException::Type::VmemPageFault) { // se for problema de paginação
			tratar_page_fault(processo_atual, excecao.vaddr);
		}
		else {
			terminal_println(cpu, Terminal::Kernel, "erro grave (", excecao.type, ") no endereco ", excecao.vaddr); // outro erro
		}
	}
	else if (interrupt_code == InterruptCode::Keyboard) {
	
	}
	
}

void syscall ()
{

}


}