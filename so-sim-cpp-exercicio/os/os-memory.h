#ifndef OS_MEMORY_H
#define OS_MEMORY_H

#include "process.h"

namespace OS {

	int pmm_alocar_frame();

	void pmm_liberar_frame(int frame);

	void carregar_processo_na_memoria(Process *proc);
	
	void liberar_memoria_processo(Process *proc);

}

#endif