#include "Keyboard.h"

Keyboard::Keyboard(Arch::Cpu* ponteiro_cpu) {
    cpu = ponteiro_cpu;
    buffer_teclado = "";
}

void Keyboard::processar_tecla() {

    char c = (char) cpu->read_io(2);
    
    cpu->write_io(0, 2); 
    cpu->write_io(1, c); 

    if (c == '\n') {
        
        if (buffer_teclado == "fechar") {
            cpu->turn_off(); 
        }
    
        buffer_teclado = ""; 
    } 
    else {
        
        buffer_teclado += c;
    }
   
}