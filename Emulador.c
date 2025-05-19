#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tipos
typedef unsigned char byte; // 8 bits
typedef unsigned int palavra; // 32 bits
typedef unsigned long int microinstrucao; // 64 bits (usa 36 bits efetivamente)

// Registradores
palavra MAR = 0, MDR = 0, PC = 0; // Acesso da Memoria
byte MBR = 0;                    // Acesso da Memoria

palavra SP = 0, LV = 0, TOS = 0, // Operação da ULA
        OPC = 0, CPP = 0, H = 0; // Operação da ULA

microinstrucao MIR; // Microinstrução Atual
palavra MPC = 0;    // Próximo endereço da Microinstrução

// Barramentos
palavra Barramento_B, Barramento_C;

// Flip-Flops
byte N, Z;

// Auxiliares para decodificar Microinstrução
byte MIR_B, MIR_Operacao, MIR_Deslocador, MIR_MEM, MIR_pulo;
palavra MIR_C;

// Armazenamento de Controle
microinstrucao Armazenamento[512];

// Memória Principal
byte Memoria[100000000];

// Protótipos das Funções
void carregar_microprogram_de_controle();
void carregar_programa(const char *programa);
void exibir_processos();
void decodificar_microinstrucao();
void atribuir_barramento_B();
void realizar_operacao_ALU();
void atribuir_barramento_C();
void operar_memoria();
void pular();

void binario(void* valor, int tipo);


// Laço Principal
int main(int argc, const char *argv[]) {
    if (argc < 2) {
        printf("Erro: Arquivo do programa nao especificado.\n");
        return 1;
    }

    carregar_microprogram_de_controle();
    carregar_programa(argv[1]);

    while (1) {
        exibir_processos();
        MIR = Armazenamento[MPC];

        decodificar_microinstrucao();
        atribuir_barramento_B();
        realizar_operacao_ALU();
        atribuir_barramento_C();
        operar_memoria();
        pular();
    }

    return 0;
}

// Implementação das Funções
void carregar_microprogram_de_controle() {
    FILE* MicroPrograma = fopen("microprog.rom", "rb");
    if (MicroPrograma == NULL) {
        printf("Erro ao abrir o arquivo microprog.rom\n");
        exit(1);
    }

    fread(Armazenamento, sizeof(microinstrucao), 512, MicroPrograma);
    fclose(MicroPrograma);
}

void carregar_programa(const char* prog) {
    FILE* Programa = fopen(prog, "rb");
    if (Programa == NULL) {
        printf("Erro ao abrir o arquivo do programa %s\n", prog);
        exit(1);
    }

    palavra tamanho;
    byte tamanho_temp[4];

    fread(tamanho_temp, sizeof(byte), 4, Programa); // Lendo tamanho
    memcpy(&tamanho, tamanho_temp, 4);

    fread(Memoria, sizeof(byte), 20, Programa); // Inicializacao

    fread(&Memoria[0x0401], sizeof(byte), tamanho - 20, Programa); // Programa

    fclose(Programa);
}

void decodificar_microinstrucao() {
    MIR_B = (MIR) & 0b1111;
    MIR_MEM = (MIR >> 4) & 0b111;
    MIR_C = (MIR >> 7) & 0b111111111;
    MIR_Operacao = (MIR >> 16) & 0b111111;
    MIR_Deslocador = (MIR >> 22) & 0b11;
    MIR_pulo = (MIR >> 24) & 0b111;
    MPC = (MIR >> 27) & 0b111111111;
}

void atribuir_barramento_B() {
    switch (MIR_B) {
        case 0: Barramento_B = MDR; break;
        case 1: Barramento_B = PC; break;
        case 2:
            Barramento_B = MBR;
            if (MBR & 0b10000000)
                Barramento_B |= 0xFFFFFF00; // Extensão de sinal para 32 bits
            break;
        case 3: Barramento_B = MBR; break;
        case 4: Barramento_B = SP; break;
        case 5: Barramento_B = LV; break;
        case 6: Barramento_B = CPP; break;
        case 7: Barramento_B = TOS; break;
        case 8: Barramento_B = OPC; break;
        default: Barramento_B = 0; break; 
    }
}

void realizar_operacao_ALU() {
    switch (MIR_Operacao) {
        case 12: Barramento_C = H & Barramento_B; break;
        case 17: Barramento_C = 1; break;
        case 18: Barramento_C = -1; break;
        case 20: Barramento_C = Barramento_B; break;
        case 24: Barramento_C = H; break;
        case 26: Barramento_C = ~H; break;
        case 28: Barramento_C = H | Barramento_B; break;
        case 44: Barramento_C = ~Barramento_B; break;
        case 53: Barramento_C = Barramento_B + 1; break;
        case 54: Barramento_C = Barramento_B - 1; break;
        case 57: Barramento_C = H + 1; break;
        case 59: Barramento_C = -H; break;
        case 60: Barramento_C = H + Barramento_B; break;
        case 61: Barramento_C = H + Barramento_B + 1; break;
        case 63: Barramento_C = Barramento_B - H; break;
        default:
            printf("Operação inválida da ULA: %d\n", MIR_Operacao);
            Barramento_C = 0;
            break;
    }

    // Corrigindo as flags N e Z: N = 1 se negativo, Z = 1 se zero
    if ((int)Barramento_C < 0)
        N = 1;
    else
        N = 0;

    if (Barramento_C == 0)
        Z = 1;
    else
        Z = 0;

    switch (MIR_Deslocador) {
        case 1: Barramento_C <<= 8; break;
        case 2: Barramento_C >>= 1; break;
    }
}

void atribuir_barramento_C() {
    if (MIR_C & 0b000000001) MAR = Barramento_C;
    if (MIR_C & 0b000000010) MDR = Barramento_C;
    if (MIR_C & 0b000000100) PC = Barramento_C;
    if (MIR_C & 0b000001000) SP = Barramento_C;
    if (MIR_C & 0b000010000) LV = Barramento_C;
    if (MIR_C & 0b000100000) CPP = Barramento_C;
    if (MIR_C & 0b001000000) TOS = Barramento_C;
    if (MIR_C & 0b010000000) OPC = Barramento_C;
    if (MIR_C & 0b100000000) H = Barramento_C;
}

void operar_memoria() {
    // Multiplicação por 4 pois MAR aponta para palavras
    if (MIR_MEM & 0b001) MBR = Memoria[PC];
    if (MIR_MEM & 0b010) memcpy(&MDR, &Memoria[MAR * 4], 4);
    if (MIR_MEM & 0b100) memcpy(&Memoria[MAR * 4], &MDR, 4);
}

void pular() {
    if (MIR_pulo & 0b001) MPC = MPC | (N << 8);
    if (MIR_pulo & 0b010) MPC = MPC | (Z << 8);
    if (MIR_pulo & 0b100) MPC = MPC | (MBR);
}

void exibir_processos() {
    if (LV && SP) {
        printf("\t\t  PILHA DE OPERANDOS\n");
        printf("========================================\n");
        printf("     END");
        printf("\t   BINARIO DO VALOR");
        printf(" \t\tVALOR\n");
        for (int i = SP; i >= LV; i--) {
            palavra valor;
            memcpy(&valor, &Memoria[i * 4], 4);

            if (i == SP)
                printf("SP ->");
            else if (i == LV)
                printf("LV ->");
            else
                printf("     ");

            printf("%X ", i);
            binario(&valor, 1);
            printf(" ");
            printf("%d\n", valor);
        }

        printf("========================================\n");
    }

    if (PC >= 0x0401) {
        printf("\n\t\t\tArea do Programa\n");
        printf("========================================\n");
        printf("\t\tBinario");
        printf("\t HEX");
        printf("  END DE BYTE\n");
        for (int i = PC - 2; i <= PC + 3; i++) {
            if (i == PC)
                printf("Em execucao >>  ");
            else
                printf("\t\t");
            binario(&Memoria[i], 2);
            printf(" 0x%02X ", Memoria[i]);
            printf("\t%X\n", i);
        }
        printf("========================================\n\n");
    }

    printf("\t\tREGISTRADORES\n");
    printf("\tBINARIO");
    printf("\t\t\t\tHEX\n");
   
// Função para imprimir registrador (32 bits ou 8 bits) em binário e hexadecimal
void imprimir_registrador(const char* nome, void* valor, int tipo) {
    printf("%-4s = ", nome);
    if (tipo == 1) { // palavra 32 bits
        unsigned int* num = (unsigned int*)valor;
        for (int i = 31; i >= 0; i--) {
            printf("%d", ((*num >> i) & 1));
            if (i % 8 == 0 && i != 0) printf(" ");
        }
        printf("\t0x%08X\n", *num);
    } else if (tipo == 2) { // byte 8 bits
        unsigned char* num = (unsigned char*)valor;
        for (int i = 7; i >= 0; i--) {
            printf("%d", ((*num >> i) & 1));
        }
        printf("\t\t0x%02X\n", *num);
    }
}

// Exemplo de chamada para imprimir todos os registradores
void exibir_registradores() {
    imprimir_registrador("PC", &PC, 1);
    imprimir_registrador("MDR", &MDR, 1);
    imprimir_registrador("MAR", &MAR, 1);
    imprimir_registrador("MBR", &MBR, 2);
    imprimir_registrador("SP", &SP, 1);
    imprimir_registrador("LV", &LV, 1);
    imprimir_registrador("CPP", &CPP, 1);
    imprimir_registrador("TOS", &TOS, 1);
    imprimir_registrador("OPC", &OPC, 1);
    imprimir_registrador("H", &H, 1);
}
