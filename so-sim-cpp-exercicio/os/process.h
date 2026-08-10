    #ifndef PROCESS_H
    #define PROCESS_H

    #include <vector>
    #include <cstdint>
    #include "../arch/cpu.h"
    #include "../arch/arch.h"

    struct Process {
        int process_id;
        Arch::Cpu::PageTable page_table;
        std::vector<uint16_t> image; //programa que vou passar

    };

    namespace OS {

    void matar_processo_atual();

    void carregar_programa(std::string nome);

    void trocar_processo(Process *processo);
    
    }

    #endif