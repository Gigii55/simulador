#include "os-memory.h"
#include "process.h"
#include "os-lib.h"
#include "../arch/cpu.h"
#include "../arch/terminal.h"
#include "../lib.h"
#include "../config.h"
#include <fstream>

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

    terminal_println(cpu,Terminal::Kernel,"processo ",processo_atual->process_id," (", processo_atual->nome_binario, ") morto, liberando ",
    processo_atual->frames_alocados.size() * Config::page_size, " palavras de memoria");

    liberar_memoria_processo(processo_atual);
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
    processo_programa.nome_binario = nome; 
    processo_programa.estado = EstadoProcesso::Pronto; 

    processo_programa.pc_salvo = 1;
    processo_programa.gprs_salvos = {};
    processo_programa.page_table = {};

    carregar_processo_na_memoria(&processo_programa);

    trocar_processo(&processo_programa);
    
    terminal_println(cpu, Terminal::Kernel, "processo ", processo_programa.process_id, " (", processo_programa.nome_binario, ") carregado, usando ",
    processo_programa.frames_alocados.size() * Config::page_size, " palavras de memoria");
}

void trocar_processo(Process *processo) {
    if (processo_atual != nullptr) {
        processo_atual->pc_salvo = cpu->get_pc();
        for (uint32_t i = 0; i < Config::nregs; i++)
            processo_atual->gprs_salvos[i] = cpu->get_gpr(i);

        if (processo_atual->estado == EstadoProcesso::Executando)
            processo_atual->estado = EstadoProcesso::Pronto;
    }

    processo_atual = processo;
    processo->estado = EstadoProcesso::Executando;
    cpu->set_page_table(&processo->page_table);
    cpu->set_pc(processo->pc_salvo);
    for (uint32_t i = 0; i < Config::nregs; i++)
        cpu->set_gpr(i, processo->gprs_salvos[i]);
}
}