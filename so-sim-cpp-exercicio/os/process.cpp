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

extern std::array<Process, 4> tabela_processos;
extern int proximo_pid;

extern Arch::Cpu *cpu;
Process *alocar_entrada_processo();

void listar_processos () {

    terminal_println(cpu, Terminal::Kernel, "--- processos ---");

    // idle primeiro, ele nao esta na tabela
    terminal_println(cpu, Terminal::Kernel, "pid 0 (idle) - memoria: ",
        processo_idle.frames_alocados.size() * Config::page_size, " palavras");

    for (int i = 0; i < MAX_PROCESSOS; i++) {

        if (tabela_processos[i].estado == EstadoProcesso::Livre)
            continue; // pula vaga vazia

        terminal_println(cpu, Terminal::Kernel,
            "pid ", tabela_processos[i].process_id,
            " (", tabela_processos[i].nome_binario, ") - memoria: ",
            tabela_processos[i].frames_alocados.size() * Config::page_size, " palavras");
    }
}

void matar_processo_atual() {

    if (processo_atual == &processo_idle) {
        terminal_println(cpu, Terminal::Kernel,"ERRO CRITICO: tentativa de finalizar o processo idle!");
        return;
    }

    terminal_println(cpu,Terminal::Kernel,"processo ",processo_atual->process_id," (", processo_atual->nome_binario, ") morto, liberando ",
    processo_atual->frames_alocados.size() * Config::page_size, " palavras de memoria");

    liberar_memoria_processo(processo_atual);
    processo_atual->estado = EstadoProcesso::Livre;
    trocar_processo(&processo_idle);
}

void carregar_programa(std::string nome) {

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

   Process *novo = alocar_entrada_processo();

    if (novo == nullptr) {
    terminal_println(cpu, Terminal::Kernel, "ERRO: tabela de processos cheia!");
    return;
    }

    novo->process_id = proximo_pid++;
    novo->image = Lib::load_from_disk_to_16bit_buffer(caminho);
    novo->nome_binario = nome;
    novo->estado = EstadoProcesso::Pronto;

    novo->pc_salvo = 1;
    novo->gprs_salvos = {};
    novo->page_table = {};

    carregar_processo_na_memoria(novo);
    trocar_processo(novo);
    
terminal_println(cpu, Terminal::Kernel, "processo ", novo->process_id, " (", novo->nome_binario, ") carregado, usando ",
    novo->frames_alocados.size() * Config::page_size, " palavras de memoria");
}

void trocar_processo(Process *processo) {
    if (processo_atual != nullptr) {//mandar aviso
        processo_atual->pc_salvo = cpu->get_pc();
        for (uint32_t i = 0; i < Config::nregs; i++)
            processo_atual->gprs_salvos[i] = cpu->get_gpr(i);

        if (processo_atual->estado == EstadoProcesso::Executando)
            processo_atual->estado = EstadoProcesso::Pronto;
    }
//prefirir fazer multitarefa do que alocação dinamica
    processo_atual = processo;
    processo->estado = EstadoProcesso::Executando;
    cpu->set_page_table(&processo->page_table);
    cpu->set_pc(processo->pc_salvo);
    for (uint32_t i = 0; i < Config::nregs; i++)
        cpu->set_gpr(i, processo->gprs_salvos[i]);
}
}