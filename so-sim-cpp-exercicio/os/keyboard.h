#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <string>
#include "../arch/arch.h"

class Keyboard {
private:
    Arch::Cpu* cpu;
    std::string buffer_teclado;

public:

    Keyboard(Arch::Cpu* ponteiro_cpu);

    void processar_tecla();
};

#endif