#include "os-memory.h"
#include "process.h"
#include "os-lib.h"
#include "../arch/cpu.h"
#include "../arch/terminal.h"
#include "../lib.h"
#include "../config.h"

namespace OS {

extern Process processo_idle;
extern Process *processo_atual;
extern Process processo_programa;

extern Arch::Cpu *cpu;


void matar_processo_atual() {

    if (processo_atual == &processo_idle) {
        terminal_println(cpu, Terminal::Kernel,"ERRO CRITICO: tentativa de finalizar o processo idle!");
        return;
    }

    int tamanho_do_programa = processo_atual->image.size();

    // minha "divisao"
    int qtd_paginas = (tamanho_do_programa + Config::page_size - 1)>> Config::page_size_bits;

    // libera os frames usados pelo processo
    for (int pagina = 0; pagina < qtd_paginas; pagina++) {

        // pego a entrada da tabela de pag
        PageTableEntry &pte = processo_atual->page_table[pagina];

        // descubro o frame
        uint16_t frame = pte[Arch::Cpu::PteField::PhyFrameID];

        pmm_liberar_frame(frame);

        pte.set(Arch::Cpu::PteField::Present,0);
    }

    terminal_println(cpu,Terminal::Kernel,"processo ",processo_atual->process_id," morto");

    trocar_processo(&processo_idle);
} 


void carregar_programa(std::string nome) {

    if (processo_atual != &processo_idle) {
        terminal_println(cpu, Terminal::Kernel,"ja tem um programa rodando");
        return;
    }

    if (nome.empty()) {
        terminal_println(cpu, Terminal::Kernel,"ERRO: informe o nome do arquivo!");
        return;
    }

    std::string caminho = "archieves/" + nome;

    std::ifstream arquivo(caminho);

    if (!arquivo.is_open()) {
        terminal_println( cpu,Terminal::Kernel,"ERRO: arquivo nao encontrado: ", nome);
        return;
    }

    arquivo.close();

    processo_programa.process_id = 1;

    processo_programa.image = Lib::load_from_disk_to_16bit_buffer(caminho);

    carregar_processo_na_memoria(&processo_programa);

    trocar_processo(&processo_programa);
}


void trocar_processo(Process *processo) {

    processo_atual = processo;

    cpu->set_page_table(&processo->page_table);

    cpu->set_pc(1);
}

}