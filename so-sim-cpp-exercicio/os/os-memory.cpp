
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


bool carregar_processo_na_memoria(Process *proc, const std::vector<uint16_t>& image) {

    int tamanho_do_programa = image.size();

    int qtd_paginas = (tamanho_do_programa + Config::page_size - 1) >> Config::page_size_bits;

    for (int numero_da_pagina = 0; numero_da_pagina < qtd_paginas; numero_da_pagina++) {

        int inicio_da_pagina = numero_da_pagina << Config::page_size_bits;

        int frame = pmm_alocar_frame();

        if (frame == -1) {
            terminal_println(cpu, Terminal::Kernel, "ERRO: memoria fisica insuficiente!");

            liberar_memoria_processo(proc);
            proc->page_table = {};

            return false;
        }

        proc->frames_alocados.push_back(frame);

        // Copia as palavras do programa para a RAM
        for (int i = 0; i < Config::page_size; i++) {

            int endereco_virtual_da_palavra = inicio_da_pagina + i;

            uint16_t valor;

            if (endereco_virtual_da_palavra < tamanho_do_programa) {
                valor = image[endereco_virtual_da_palavra];
            }
            else {
                valor = 0;
            }

            int endereco_fisico = (frame << Config::page_size_bits) + i;

            cpu->pmem_write(endereco_fisico, valor);
        }

        // Configura a tabela de paginas
        PageTableEntry &pte = proc->page_table[numero_da_pagina];

        pte.set(Arch::Cpu::PteField::PhyFrameID, frame);
        pte.set(Arch::Cpu::PteField::Present, 1);
        pte.set(Arch::Cpu::PteField::Readable, 1);
        pte.set(Arch::Cpu::PteField::Writable, 1);
        pte.set(Arch::Cpu::PteField::Executable, 1);
    }

    return true;
}


void liberar_memoria_processo(Process *proc) {
    for (int frame : proc->frames_alocados) {
        pmm_liberar_frame(frame);
    }

    proc->frames_alocados.clear();
}

}
