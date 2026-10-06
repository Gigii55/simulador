#ifndef PROCESS_H
#define PROCESS_H

#include <vector>
#include <array>
#include <cstdint>
#include "../arch/cpu.h"
#include "../arch/arch.h"
#include "../config.h"

enum class EstadoProcesso {
    Livre,
    Pronto,
    Executando
};
  constexpr int MAX_PROCESSOS = 4;
  
    struct Process {
    int process_id;
    Arch::Cpu::PageTable page_table;
    std::vector<uint16_t> image;

    uint16_t pc_salvo = 1; // em qual instrução parou?
    std::array<uint16_t, Config::nregs> gprs_salvos{}; //estado da execucao do processo (oq tava rodando?)
    EstadoProcesso estado = EstadoProcesso::Livre;
    std::string nome_binario;
    std::vector<int> frames_alocados; //em que ponto exato da execução esse processo estava, com quais valores nos registradores?
  
 

};

    namespace OS {

    extern Process processo_idle;
    extern Process *processo_atual;
    extern Arch::Cpu *cpu;

    extern std::array<Process, MAX_PROCESSOS> tabela_processos;
    extern int proximo_pid;

    Process *alocar_entrada_processo();
    void matar_processo_atual();
    void carregar_programa(std::string nome);
    void trocar_processo(Process *processo);
    void listar_processos();


    void matar_processo_atual();

    void carregar_programa(std::string nome);

    void trocar_processo(Process *processo);
    
    }

    #endif