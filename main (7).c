#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_SIZE 256
#define INSTR_SIZE 16
#define REG_COUNT 8  

// ================= ENUM =================
enum classe_inst {
    tipo_I,
    tipo_J,
    tipo_R
};

// ================= INSTRUÇÃO =================
struct instrucao {
    enum classe_inst tipo_inst;
    char inst_char[INSTR_SIZE + 1];
    int opcode;
    int rs, rt, rd;
    int funct;
    int imm;
    int addr;
};

// ================= MEMORIA =================
struct memoria_dados {
    int dados[DATA_SIZE];
};

// ================= PC =================
struct pc {
    int pc;
    int prev_pc;
};

// ================= ULA =================
struct ULA {
    int entrada1;
    int entrada2;
    int resultado;
};

// ================= CONTROLE =================
struct controle {
    int alu_op;
    int mem_read;
    int mem_write;
    int reg_write;
};

// ================= SIMULADOR =================
struct simulador {
    struct memoria_dados dmem;
    struct pc pc;
    int reg[REG_COUNT];
    struct instrucao *programa;
    int prog_size;

    struct ULA ula;
    struct controle ctrl;

    // 🔥 MULTICICLO
    struct instrucao ir;
    int alu_out;
    int mdr;
};

// ================= PRINT =================
void mostrar_instrucao(struct instrucao *inst) {
    if (inst->tipo_inst == tipo_R) {
        if (inst->funct == 0) printf("ADD\n");
        else if (inst->funct == 1) printf("SUB\n");
        else if (inst->funct == 2) printf("AND\n");
        else if (inst->funct == 3) printf("OR\n");
    }
    else if (inst->tipo_inst == tipo_I) {
        if (inst->opcode == 8) printf("ADDI\n");
        else if (inst->opcode == 4) printf("LW\n");
        else if (inst->opcode == 5) printf("SW\n");
        else if (inst->opcode == 9) printf("BEQ\n");
    }
    else printf("JUMP\n");
}

// ================= ULA =================
int executar_ula(struct ULA *ula, int op) {
    switch(op) {
        case 0: ula->resultado = ula->entrada1 + ula->entrada2; break;
        case 1: ula->resultado = ula->entrada1 - ula->entrada2; break;
        case 2: ula->resultado = ula->entrada1 & ula->entrada2; break;
        case 3: ula->resultado = ula->entrada1 | ula->entrada2; break;
        default: ula->resultado = 0;
    }
    return ula->resultado;
}

// ================= CONTROLE =================
void unidade_controle(struct instrucao *inst, struct controle *ctrl) {

    ctrl->alu_op = 0;
    ctrl->mem_read = 0;
    ctrl->mem_write = 0;
    ctrl->reg_write = 0;

    if (inst->tipo_inst == tipo_R) {
        ctrl->reg_write = 1;
        ctrl->alu_op = inst->funct;
    }
    else if (inst->tipo_inst == tipo_I) {

        if (inst->opcode == 8) { // ADDI
            ctrl->reg_write = 1;
            ctrl->alu_op = 0;
        }

        else if (inst->opcode == 4) { // LW
            ctrl->mem_read = 1;
            ctrl->reg_write = 1;
        }

        else if (inst->opcode == 5) { // SW
            ctrl->mem_write = 1;
        }
    }
}

// ================= DECODIFICADOR =================
void decodificador(struct instrucao *inst) {

    int i;
    inst->inst_char[INSTR_SIZE] = '\0';

    inst->opcode = 0;
    for (i = 0; i < 4; i++)
        inst->opcode = (inst->opcode << 1) | (inst->inst_char[i] - '0');

    if (inst->opcode == 0) inst->tipo_inst = tipo_R;
    else if (inst->opcode == 2) inst->tipo_inst = tipo_J;
    else inst->tipo_inst = tipo_I;

    if (inst->tipo_inst == tipo_R) {

        inst->rs = inst->rt = inst->rd = inst->funct = 0;

        for (i = 4; i < 7; i++)
            inst->rs = (inst->rs << 1) | (inst->inst_char[i] - '0');

        for (i = 7; i < 10; i++)
            inst->rt = (inst->rt << 1) | (inst->inst_char[i] - '0');

        for (i = 10; i < 13; i++)
            inst->rd = (inst->rd << 1) | (inst->inst_char[i] - '0');

        for (i = 13; i < 16; i++)
            inst->funct = (inst->funct << 1) | (inst->inst_char[i] - '0');
    }

    else if (inst->tipo_inst == tipo_I) {

        inst->rs = inst->rt = inst->imm = 0;

        for (i = 4; i < 7; i++)
            inst->rs = (inst->rs << 1) | (inst->inst_char[i] - '0');

        for (i = 7; i < 10; i++)
            inst->rt = (inst->rt << 1) | (inst->inst_char[i] - '0');

        for (i = 10; i < 16; i++)
            inst->imm = (inst->imm << 1) | (inst->inst_char[i] - '0');
    }

    else {
        inst->addr = 0;
        for (i = 4; i < 16; i++)
            inst->addr = (inst->addr << 1) | (inst->inst_char[i] - '0');
    }
}

// ================= MULTICICLO =================

// BUSCA
void busca(struct simulador *sim) {
    sim->ir = sim->programa[sim->pc.pc];
    sim->pc.prev_pc = sim->pc.pc;
    sim->pc.pc++;

    printf("\n[BUSCA] %s\n", sim->ir.inst_char);
}

// DECODE
void decodificacao(struct simulador *sim) {
    decodificador(&sim->ir);
    unidade_controle(&sim->ir, &sim->ctrl);

    printf("[DECODE] ");
    mostrar_instrucao(&sim->ir);
}

// EXECUÇÃO
void execucao(struct simulador *sim) {

    struct instrucao *inst = &sim->ir;

    if (inst->tipo_inst == tipo_R) {
        sim->ula.entrada1 = sim->reg[inst->rs];
        sim->ula.entrada2 = sim->reg[inst->rt];
        sim->alu_out = executar_ula(&sim->ula, sim->ctrl.alu_op);
    }

    else if (inst->tipo_inst == tipo_I) {

        if (inst->opcode == 8) // ADDI
            sim->alu_out = sim->reg[inst->rs] + inst->imm;

        else if (inst->opcode == 4 || inst->opcode == 5) { // LW/SW
            sim->alu_out = sim->reg[inst->rs] + inst->imm;
            printf("[EXEC] Endereco = %d\n", sim->alu_out);
        }

        else if (inst->opcode == 9) { // BEQ
            if (sim->reg[inst->rs] == sim->reg[inst->rt])
                sim->pc.pc += inst->imm;
        }
    }

    else if (inst->tipo_inst == tipo_J)
        sim->pc.pc = inst->addr;
}

// MEMÓRIA
void memoria(struct simulador *sim) {

    struct instrucao *inst = &sim->ir;

    if (sim->ctrl.mem_read) { // LW
        sim->mdr = sim->dmem.dados[sim->alu_out];
        printf("[MEM] LW -> Mem[%d] = %d\n", sim->alu_out, sim->mdr);
    }

    else if (sim->ctrl.mem_write) { // SW
        sim->dmem.dados[sim->alu_out] = sim->reg[inst->rt];
        printf("[MEM] SW -> Mem[%d] = %d\n", sim->alu_out, sim->reg[inst->rt]);
    }
}

// WRITE BACK
void write_back(struct simulador *sim) {

    struct instrucao *inst = &sim->ir;

    if (inst->tipo_inst == tipo_R && sim->ctrl.reg_write)
        sim->reg[inst->rd] = sim->alu_out;

    else if (inst->opcode == 8)
        sim->reg[inst->rt] = sim->alu_out;

    else if (sim->ctrl.mem_read)
        sim->reg[inst->rt] = sim->mdr;
}

// STEP
void step_simulation(struct simulador *sim) {

    if (sim->pc.pc >= sim->prog_size) {
        printf("Fim do programa\n");
        return;
    }

    busca(sim);
    decodificacao(sim);
    execucao(sim);
    memoria(sim);
    write_back(sim);
}

// RUN
void run_simulation(struct simulador *sim) {
    while (sim->pc.pc < sim->prog_size)
        step_simulation(sim);
}

// MENU
void mostrar_registradores(int reg[]) {
    for (int i = 0; i < REG_COUNT; i++)
        printf("R%d = %d\n", i, reg[i]);
}

void imprimir_memoria(struct memoria_dados *mem) {
    for (int i = 0; i < DATA_SIZE; i++)
        if (mem->dados[i] != 0)
            printf("Mem[%d] = %d\n", i, mem->dados[i]);
}

void menu(struct simulador *sim) {

    int op;

    do {
        printf("\n1-Memoria  2-Regs  3-RUN  4-STEP  5-PC  0-Sair\n");
        scanf("%d", &op);

        switch(op) {
            case 1: imprimir_memoria(&sim->dmem); break;
            case 2: mostrar_registradores(sim->reg); break;
            case 3: run_simulation(sim); break;
            case 4: step_simulation(sim); break;
            case 5: printf("PC = %d\n", sim->pc.pc); break;
        }

    } while(op != 0);
}

// MAIN
int main() {

    FILE *arq = fopen("teste.mem", "r");

    if (!arq) {
        printf("Erro ao abrir teste.mem\n");
        return 1;
    }

    struct instrucao programa[100];
    int i = 0;

    while (fscanf(arq, "%s", programa[i].inst_char) != EOF)
        i++;

    fclose(arq);

    struct simulador sim = {
        .dmem = {0},
        .pc = {0, -1},
        .reg = {0},
        .programa = programa,
        .prog_size = i
    };

    menu(&sim);

    return 0;
}