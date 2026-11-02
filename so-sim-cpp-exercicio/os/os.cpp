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
#include "os-memory.h"
#include <fstream>
#include <array>

namespace OS {

Process processo_idle;
Process *processo_atual;

Arch::Cpu *cpu;
Keyboard* meu_teclado;

std::list<Process> tabela_processos;
int proximo_pid = 1;

Process *alocar_entrada_processo() {
    tabela_processos.emplace_back();
    return &tabela_processos.back();
}
//-----------------------------~


void boot(Arch::Cpu *cpu)
{
    OS::cpu = cpu;

    //prepara o processo idle
    processo_idle.process_id = 0;

    auto image_idle =
        Lib::load_from_disk_to_16bit_buffer("archieves/idle.bin");

    //carrega o idle na memoria uma unica vez
    if (!carregar_processo_na_memoria(&processo_idle, image_idle)) {
        cpu->turn_off();
        return;
    }

    //astiva a paginacao
    cpu->set_vmem_mode(VmemMode::Paging);

    //coloca o idle para executar
    trocar_processo(&processo_idle);

    meu_teclado = new Keyboard(cpu);

    terminal_println(cpu, Terminal::Command, "Type commands here");
    terminal_println(cpu, Terminal::App, "Apps output here");
    terminal_println(cpu, Terminal::Kernel, "Kernel output here");
}

void configurar_timer(int ciclos) {

    if (ciclos < 1 || ciclos > 65535) {
        terminal_println(cpu, Terminal::Kernel, "ERRO: numero de ciclos invalido!");
        return;
    }

    cpu->write_io(IO_Port::TimerInterruptCycles,static_cast<uint16_t>(ciclos));

    terminal_println(cpu, Terminal::Kernel,"Timer configurado para ", ciclos, " ciclos");
}


void interrupt (InterruptCode interrupt_code) {
	if (interrupt_code == InterruptCode::CpuException) {

		CpuException excecao = cpu->get_ref_cpu_exception();

		terminal_println(cpu, Terminal::Kernel, "EXCESAO (", excecao.type, ") no endereco ", excecao.vaddr);

		matar_processo_atual();
	}

	else if (interrupt_code == InterruptCode::Keyboard) {
		meu_teclado->processar_tecla();
	}

	else if (interrupt_code == InterruptCode::Timer) {
    escalonar();
}
	
}

void syscall () {

	uint16_t codigo = cpu->get_gpr(0);

	if (codigo == 0) {
		matar_processo_atual();
		
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

				// checagem do endereço virtual
				if (pte[Arch::Cpu::PteField::Present] == 0 || pte[Arch::Cpu::PteField::Readable] == 0) {
					terminal_println(cpu,Terminal::Kernel,"ERRO: endereco virtual invalido");
					matar_processo_atual();
					return;
				}

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
				//reloogio do simulador (ver tempo decorrido)
				else if (codigo == 7) {
    				uint16_t tempo = cpu->read_io(IO_Port::TimerGetTimeSeconds);
    					cpu->set_gpr(0, tempo);
}
	}

}
