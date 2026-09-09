#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// --- Definiciones de la Arquitectura ---
#define MEM_SIZE 1024  // Tamaño arbitrario para la memoria (en palabras de 16 bits)
#define NUM_REGS 16

// Opcodes extraídos del pizarrón
#define OP_ADD 0x0
#define OP_LW  0x2
#define OP_SW  0x3
#define OP_BEQ 0x4
#define OP_J   0xE     // 1110 en binario

// Estructura del estado del CPU
typedef struct {
    uint16_t pc;
    uint16_t psw;
    uint16_t regs[NUM_REGS];
    
    // Arquitectura Harvard: Memorias separadas
    uint16_t data_memory[MEM_SIZE];
    uint16_t program_memory[MEM_SIZE];
} CPU;

// --- Funciones del Simulador ---

// Inicializa el procesador a un estado limpio
void init_cpu(CPU *cpu) {
    cpu->pc = 0;
    cpu->psw = 0;
    for (int i = 0; i < NUM_REGS; i++) cpu->regs[i] = 0;
    for (int i = 0; i < MEM_SIZE; i++) {
        cpu->data_memory[i] = 0;
        cpu->program_memory[i] = 0;
    }
}

// Ejecuta un ciclo de reloj (Fetch, Decode, Execute)
void clock_cycle(CPU *cpu) {
    // 1. FETCH
    // Obtenemos la instrucción apuntada por el PC
    uint16_t instruction = cpu->program_memory[cpu->pc];
    cpu->pc++; // Avanzamos el PC al siguiente word

    // 2. DECODE
    // Usamos máscaras de bits para aislar los campos de 4 bits
    uint16_t opcode = (instruction >> 12) & 0x000F;
    uint16_t rd     = (instruction >> 8)  & 0x000F;
    uint16_t rs     = (instruction >> 4)  & 0x000F;
    
    // El último bloque de 4 bits depende del tipo de instrucción
    uint16_t rt     = instruction & 0x000F;       // Para Tipo-R
    uint16_t imm    = instruction & 0x000F;       // Para Tipo-I (4 bits)
    uint16_t addr   = instruction & 0x0FFF;       // Para Tipo-J (12 bits)

    // Extensión de signo manual para el inmediato de 4 bits (útil para saltos hacia atrás)
    int8_t signed_imm = (imm & 0x8) ? (imm | 0xF0) : imm;

    // 3. EXECUTE
    switch (opcode) {
        case OP_ADD:
            // add $rd, $rs, $rt -> rd = rs + rt
            cpu->regs[rd] = cpu->regs[rs] + cpu->regs[rt];
            printf("Ejecutado: ADD r%d, r%d, r%d\n", rd, rs, rt);
            break;

        case OP_LW:
            // lw $rd, $rs, imm -> Carga en rd el valor de memoria en (rs + offset)
            // Asumimos direccionamiento base + offset
            uint16_t lw_addr = (cpu->regs[rs] + signed_imm) % MEM_SIZE;
            cpu->regs[rd] = cpu->data_memory[lw_addr];
            printf("Ejecutado: LW r%d, %d(r%d)\n", rd, signed_imm, rs);
            break;

        case OP_SW:
            // sw $rd, $rs, imm -> Guarda el valor de rd en memoria en (rs + offset)
            uint16_t sw_addr = (cpu->regs[rs] + signed_imm) % MEM_SIZE;
            cpu->data_memory[sw_addr] = cpu->regs[rd];
            printf("Ejecutado: SW r%d, %d(r%d)\n", rd, signed_imm, rs);
            break;

        case OP_BEQ:
            // beq $rd, $rs, imm -> Si rd == rs, saltar relativo al PC
            if (cpu->regs[rd] == cpu->regs[rs]) {
                cpu->pc += signed_imm; // Salto relativo
                printf("Ejecutado: BEQ (Branch Taken) a PC=%d\n", cpu->pc);
            } else {
                printf("Ejecutado: BEQ (Branch Not Taken)\n");
            }
            break;

        case OP_J:
            // j addr -> Salto incondicional absoluto (limitado a 12 bits)
            cpu->pc = addr;
            printf("Ejecutado: J %d\n", addr);
            break;

        default:
            // Si el procesador lee memoria vacía u opcodes no definidos
            printf("Opcode desconocido (0x%X) en PC %d. Deteniendo.\n", opcode, cpu->pc - 1);
            break;
    }
}

// --- Programa de Prueba ---
int main() {
    CPU my_cpu;
    init_cpu(&my_cpu);

    // Preparando un escenario de prueba en memoria
    my_cpu.regs[1] = 10; // rs (r1) = 10
    my_cpu.regs[2] = 5;  // rt (r2) = 5
    my_cpu.data_memory[15] = 99; // Dato a cargar en LW

    // Ensamblando instrucciones manualmente (programando la program_memory)
    // 1. ADD r3, r1, r2  (0x0312) -> Op=0, rd=3, rs=1, rt=2
    my_cpu.program_memory[0] = (OP_ADD << 12) | (3 << 8) | (1 << 4) | 2;
    
    // 2. SW r3, r0, 5    (0x3305) -> Op=3, rd=3, rs=0, imm=5 (guarda r3 en mem[0+5])
    my_cpu.program_memory[1] = (OP_SW << 12) | (3 << 8) | (0 << 4) | 5;

    // 3. J 10            (0xE00A) -> Op=14, addr=10
    my_cpu.program_memory[2] = (OP_J << 12) | 10;

    printf("--- Iniciando Simulación ---\n");
    for (int i = 0; i < 3; i++) {
        clock_cycle(&my_cpu);
    }
    
    printf("\n--- Estado Final ---\n");
    printf("Registro r3 (Resultado del ADD): %d\n", my_cpu.regs[3]);
    printf("Memoria de datos en 5 (Resultado del SW): %d\n", my_cpu.data_memory[5]);
    printf("PC actual (Después del Jump): %d\n", my_cpu.pc);

    return 0;
}