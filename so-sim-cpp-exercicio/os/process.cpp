#include "os-memory.h"
#include "process.h"
#include "os-lib.h"
#include "../arch/cpu.h"
#include "../arch/terminal.h"
#include "../lib.h"
#include "../config.h"
#include <fstream>

namespace OS {

extern Arch::Cpu *cpu;
Process *alocar_entrada_processo();


void listar_processos() {

    terminal_println(cpu, Terminal::Kernel, "--- processos ---");

    terminal_println(cpu, Terminal::Kernel, "pid 0 (idle) - memoria: ", processo_idle.frames_alocados.size() * Config::page_size,
    " palavras - estado: ", processo_atual == &processo_idle ? "Executando" : "Pronto");

    for (Process &proc : tabela_processos) {

        std::string estado;

        if (proc.estado == EstadoProcesso::Livre)
            estado = "Livre";
        else if (proc.estado == EstadoProcesso::Pronto)
            estado = "Pronto";
        else if (proc.estado == EstadoProcesso::Executando)
            estado = "Executando";
        else if (proc.estado == EstadoProcesso::Dormindo)
            estado = "Dormindo";

        terminal_println(cpu, Terminal::Kernel,"pid ", proc.process_id," (", proc.nome_binario,") - memoria: ",
            proc.frames_alocados.size() * Config::page_size," palavras - estado: ", estado);
    }
}


void matar_processo_atual() {

    if (processo_atual == &processo_idle) {
    terminal_println(cpu, Terminal::Kernel,"ERRO: tentativa de matar o idle!");
        return;
    }

    Process *morto = processo_atual;

    terminal_println(cpu, Terminal::Kernel,"processo ", morto->process_id," finalizado");

    liberar_memoria_processo(morto);

    morto->estado = EstadoProcesso::Livre;

    trocar_processo(&processo_idle);

    for (auto it = tabela_processos.begin();
         it != tabela_processos.end(); ++it) {

        if (&(*it) == morto) {
            tabela_processos.erase(it);
            break;
        }
    }
}

void matar_processo(int pid) {

    if (pid == 0) {
        terminal_println(cpu, Terminal::Kernel, "ERRO: nao pode matar o idle!");
        return;
    }

    for (auto it = tabela_processos.begin();
         it != tabela_processos.end(); ++it) {

        if (it->process_id == pid) {

            if (&(*it) == processo_atual) {
                matar_processo_atual();
                return;
            }

            liberar_memoria_processo(&(*it));
            terminal_println(cpu, Terminal::Kernel,"processo ", pid, " finalizado");
            tabela_processos.erase(it);
            return;
        }
    }

    terminal_println(cpu, Terminal::Kernel,
        "ERRO: processo ", pid, " nao encontrado!");
}

Process *carregar_programa(std::string nome) {

    if (nome.empty()) {
        terminal_println(cpu, Terminal::Kernel,"ERRO: informe o nome do arquivo!");
        return nullptr;
    }

    std::string caminho = "archieves/" + nome;

    std::ifstream arquivo(caminho);

    if (!arquivo.is_open()) {
        terminal_println( cpu,Terminal::Kernel,"ERRO: arquivo nao encontrado: ", nome);
        return nullptr;
    }

    arquivo.close();

   Process *novo = alocar_entrada_processo();

    novo->process_id = proximo_pid++;
    auto image = Lib::load_from_disk_to_16bit_buffer(caminho);
    novo->nome_binario = nome;
    novo->estado = EstadoProcesso::Pronto;

    novo->pc_salvo = 1;
    novo->gprs_salvos = {};
    novo->page_table = {};

   if (!carregar_processo_na_memoria(novo, image)) {

    for (auto it = tabela_processos.begin();
         it != tabela_processos.end(); ++it) {

        if (&(*it) == novo) {
            tabela_processos.erase(it);
            break;
        }
    }

    return nullptr;
}

trocar_processo(novo);
    
    terminal_println(cpu, Terminal::Kernel, "processo ", novo->process_id, " (", novo->nome_binario, ") carregado, usando ",
    novo->frames_alocados.size() * Config::page_size, " palavras de memoria");

    return novo;    
}

void escalonar() {

    if (tabela_processos.empty())
        return;

    auto atual = tabela_processos.end();

    for (auto it = tabela_processos.begin();
         it != tabela_processos.end(); ++it) {

        if (&(*it) == processo_atual) {
            atual = it;
            break;
        }
    }

    auto proximo = atual;

    if (proximo == tabela_processos.end())
        proximo = tabela_processos.begin();
    else
        ++proximo;

    for (size_t passo = 0;
         passo < tabela_processos.size(); ++passo) {

        if (proximo == tabela_processos.end())
            proximo = tabela_processos.begin();

        if (proximo->estado == EstadoProcesso::Pronto) {
            trocar_processo(&(*proximo));
            return;
        }

        ++proximo;
    }
}

void trocar_processo(Process *processo) {
    if (processo_atual != nullptr && (processo_atual->estado == EstadoProcesso::Executando || processo_atual->estado == EstadoProcesso::Dormindo)) {
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