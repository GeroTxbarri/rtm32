# Pruebas del Instruction Set RTM32 (STX4)

Todas las pruebas se ejecutaron contra el emulador real `rtm32`, conectado por telnet
al debugger (`telnet localhost 4444`), usando los comandos `set`, `step`, `registers`
y `examine`. Cada instrucción se codificó a mano a 32 bits según los formatos R/I/L/J
del manual, se cargó en memoria con `set [addr] 0xPALABRA` y se ejecutó con `step`.
El resultado se comparó contra la Tabla A.1/A.2 del manual.

Convención de registros: $t0=$10, $t1=$11, $t2=$12 (fuente1, fuente2, destino), salvo
que se indique otra cosa.

Nota: el profesor publicó una versión nueva de la máquina que corrige ADDI y LUI.
Los Casos 7 y 8 fueron re-testeados contra esa versión.

Cobertura: 58 de 58 instrucciones probadas. 54 correctas. No funcionan: LHU, CFS, CTS,
TRAP, RFT (Casos 19, 22, 23).

---

# Caso 1
## Descripción
ADD y SUB, verificando `$rd = $rs + $rt` y `$rd = $rs - $rt`.

## Instrucciones
ADD $12,$10,$11 ; SUB $13,$10,$11

## Precondiciones
- $10 = 15
- $11 = 4
- [0x0] = ADD ; [0x4] = SUB

## Code
ADD: rs=10, rt=11, rd=12, func=0x1C -> `0x0296C01C`
SUB: rs=10, rt=11, rd=13, func=0x1D -> `0x0296D01D`
```
set [0x0] 0x0296C01C
set [0x4] 0x0296D01D
set r10 15
set r11 4
step
registers
step
registers
```

## Postcondiciones
- Tras step 1: R12 = 0x13 (19 = 15+4)
- Tras step 2: R13 = 0x0B (11 = 15-4)

## Conclusiones
Anduvo. Ambos resultados coinciden con la Tabla A.2.

---

# Caso 2
## Descripción
AND, OR, XOR, NOR con el mismo par de operandos.

## Instrucciones
AND $12,$10,$11 ; OR $13,$10,$11 ; XOR $14,$10,$11 ; NOR $15,$10,$11

## Precondiciones
- $10 = 0xC (1100b)
- $11 = 0xA (1010b)
- [0x0..0xC] = las 4 instrucciones

## Code
func: AND=0x08, OR=0x09, XOR=0x0A, NOR=0x0B
```
set [0x0] 0x0296C008
set [0x4] 0x0296D009
set [0x8] 0x0296E00A
set [0xC] 0x0296F00B
set r10 12
set r11 10
step
registers
step
registers
step
registers
step
registers
```

## Postcondiciones
- R12 = 0x8 (AND)
- R13 = 0xE (OR)
- R14 = 0x6 (XOR)
- R15 = 0xFFFFFFF1 (NOR)

## Conclusiones
Anduvo. Los 4 resultados coinciden con la tabla de verdad esperada.

---

# Caso 3
## Descripción
SLT (con signo) vs SLTU (sin signo), mismo bit pattern, para ver la diferencia.

## Instrucciones
SLT $12,$10,$11 ; SLTU $13,$10,$11

## Precondiciones
- $10 = 0xFFFFFFFF (con signo = -1)
- $11 = 1

## Code
func: SLT=0x0C, SLTU=0x0D
```
set [0x0] 0x0296C00C
set [0x4] 0x0296D00D
set r10 0xFFFFFFFF
set r11 1
step
registers
step
registers
```

## Postcondiciones
- R12 = 1 (SLT: -1 < 1 con signo = verdadero)
- R13 = 0 (SLTU: 0xFFFFFFFF < 1 sin signo = falso)

## Conclusiones
Anduvo. Confirma que SLT interpreta con signo y SLTU sin signo.

---

# Caso 4
## Descripción
SLL, SRL, SRA con cantidad de shift inmediata (aux). SRA se prueba para confirmar
que preserva el signo.

## Instrucciones
SLL $12,$11,4 ; SRL $13,$11,1 ; SRA $14,$11,1

## Precondiciones
- Para SLL: $11 = 1
- Para SRL/SRA: $11 = 0x80000000

## Code
func: SLL=0x00, SRL=0x01, SRA=0x02 (rs no se usa, aux = cantidad de shift)
```
set [0x0] 0x0016C200
set [0x4] 0x0016D081
set [0x8] 0x0016E082
set r11 1
step
registers
set r11 0x80000000
step
registers
step
registers
```

## Postcondiciones
- R12 = 0x10 (SLL: 1<<4)
- R13 = 0x40000000 (SRL: entra un 0)
- R14 = 0xC0000000 (SRA: entra el bit de signo)

## Conclusiones
Anduvo. SRL rellena con ceros y SRA replica el signo, como especifica la tabla.

---

# Caso 5
## Descripción
Igual que el Caso 4, pero con la cantidad de shift tomada de un registro: SLLR,
SRLR, SRAR ($rs[4:0]).

## Instrucciones
SLLR $12,$10,$11 ; SRLR $13,$10,$11 ; SRAR $14,$10,$11

## Precondiciones
- Para SLLR: $10 = 3 (cantidad), $11 = 1
- Para SRLR/SRAR: $10 = 1, $11 = 0x80000000

## Code
func: SLLR=0x03, SRLR=0x04, SRAR=0x05
```
set [0x0] 0x0296C003
set [0x4] 0x0296D004
set [0x8] 0x0296E005
set r10 3
set r11 1
step
registers
set r10 1
set r11 0x80000000
step
registers
step
registers
```

## Postcondiciones
- R12 = 0x8 (SLLR: 1<<3)
- R13 = 0x40000000 (SRLR)
- R14 = 0xC0000000 (SRAR)

## Conclusiones
Anduvo. Mismo comportamiento que la versión por inmediato (Caso 4).

---

# Caso 6
## Descripción
MUL, DIV, REST (resto) con operandos positivos chicos.

## Instrucciones
MUL $12,$10,$11 ; DIV $13,$10,$11 ; REST $14,$10,$11

## Precondiciones
- $10 = 7
- $11 = 3

## Code
func: MUL=0x15, DIV=0x18, REST=0x1A
```
set [0x0] 0x0296C015
set [0x4] 0x0296D018
set [0x8] 0x0296E01A
set r10 7
set r11 3
step
registers
step
registers
step
registers
```

## Postcondiciones
- R12 = 21 (7*3)
- R13 = 2 (7/3)
- R14 = 1 (7%3)

## Conclusiones
Anduvo. Los tres resultados son correctos.

---

# Caso 7
## Descripción
ADDI (suma con inmediato de 17 bits), probada con y sin `$rs=$zero`. Se reportó en el
grupo que esta instrucción podía estar fallando.

## Instrucciones
ADDI $11,$10,10 ; ADDI $13,$0,100

## Precondiciones
- Test A: $10 = 5 (se espera $11 = 15)
- Test B: $rs=$0 (se espera $13 = 100)
- Cada test en un proceso nuevo del emulador

## Code
ADDI $11,$10,10: rs=10, rt=11, imm=10 -> `0x0A96000A`
ADDI $13,$0,100: rs=0, rt=13, imm=100 -> `0x081A0064`
```
set [0x0] 0x0A96000A
set r10 5
step
registers

set [0x0] 0x081A0064
step
registers
```

## Postcondiciones (versión vieja de la máquina)
- Test A: R11 quedó en 0 (no se actualizó). CAUSE pasó de 0 a 0x00000003, VBR de
  0xF0000000 a 0x00000002.
- Test B: mismo patrón, R13 quedó en 0.
- Una vez producido el fallo, cualquier instrucción posterior en la misma sesión
  también muestra CAUSE=3 y PC congelado, aunque se reposicione el PC con `set pc`.

## Postcondiciones (versión nueva de la máquina)
- Test A: R11 = 15. CAUSE = 0, VBR = 0xF0000000 (sin corrupción).
- Test B: R13 = 100. Mismo resultado limpio.

## Conclusiones
En la versión vieja, no andaba: no actualizaba el destino y corrompía CAUSE/VBR con
una excepción espuria. En la versión nueva publicada por el profesor, ya anduvo:
mismos tests, resultado correcto y sin corrupción de registros especiales.

---

# Caso 8
## Descripción
ANDI/ANDIH, ORI/ORIH, XORI/XORIH (h=0 y h=1) y LUI.

## Instrucciones
ANDI $11,$10,0xFF ; ANDIH $12,$10,0xFF ; ORI $13,$10,0x1234 ; ORIH $16,$10,0x1234 ;
XORI $14,$10,0xFF ; XORIH $17,$10,0xFF ; LUI $15,0xABCD

## Precondiciones
- ANDI/ANDIH: $10 = 0xFFFFFFFF
- ORI/ORIH: $10 = 0
- XORI: $10 = 0x0000FFFF
- XORIH: $10 = 0xFFFF0000
- LUI: sin precondición

## Code
Formato L: `opcode rs rt h imm(16)`.
- ANDI (h=0) -> `0x229600FF` ; ANDIH (h=1) -> `0x229900FF`
- ORI -> `0x2A9A1234` ; ORIH -> `0x2AA11234`
- XORI -> `0x329C00FF` ; XORIH -> `0x32A300FF`
- LUI -> `0x381EABCD`
```
set [0x0] 0x229600FF
set [0x4] 0x229900FF
set [0x8] 0x2A9A1234
set [0xC] 0x329C00FF
set [0x10] 0x381EABCD
set r10 0xFFFFFFFF
step
registers
step
registers
set r10 0
step
registers
set r10 0x0000FFFF
step
registers
step
registers

(ORIH/XORIH, proceso aparte)
set [0x0] 0x2AA11234
set [0x4] 0x32A300FF
set r10 0
step
registers
set r10 0xFFFF0000
step
registers
```

## Postcondiciones
- ANDI: R11 = 0xFF
- ANDIH: R12 = 0x00FF0000
- ORI: R13 = 0x1234
- ORIH: R16 = 0x12340000
- XORI: R14 = 0x0000FF00
- XORIH: R17 = 0xFF000000
- LUI (versión vieja): R15 quedó en 0, CAUSE=0x00000003, VBR=0x00000002
- LUI (versión nueva, mismo test): R15 = 0xABCD0000, CAUSE=0, VBR sin cambios

## Conclusiones
ANDI, ANDIH, ORI, ORIH, XORI y XORIH anduvieron en ambas versiones (el bug de ANDI
que menciona el manual no se manifestó con estos valores). LUI no andaba en la
versión vieja (mismo patrón de falla que ADDI) y ya está confirmada arreglada en la
versión nueva.

---

# Caso 9
## Descripción
SW y LW, guardando y releyendo una palabra de 32 bits.

## Instrucciones
SW $11,0($10) ; LW $12,0($10)

## Precondiciones
- $10 = 0x100
- $11 = 0xDEADBEEF
- mem[0x100] = 0 antes de empezar

## Code
```
set [0x0] 0x4A960000
set [0x4] 0x42980000
set r10 0x100
set r11 0xDEADBEEF
examine 0x100
step
examine 0x100
registers
step
registers
```

## Postcondiciones
- examine 0x100 antes: 0
- examine 0x100 después de SW: 0xDEADBEEF
- R12 tras LW: 0xDEADBEEF

## Conclusiones
Anduvo. SW escribe la palabra completa y LW la recupera intacta.

---

# Caso 10
## Descripción
SB, LB y LBU, para comparar extensión de signo vs. extensión con ceros.

## Instrucciones
SB $11,0($10) ; LB $12,0($10) ; LBU $13,0($10)

## Precondiciones
- $10 = 0x200
- $11 = 0xFFFFFF80 (byte bajo = 0x80, signo activo)

## Code
```
set [0x0] 0x5A960000
set [0x4] 0x72980000
set [0x8] 0x7A9A0000
set r10 0x200
set r11 0xFFFFFF80
step
examine 0x200
registers
step
registers
step
registers
```

## Postcondiciones
- examine 0x200 tras SB: 0x00000080 (solo el byte bajo)
- R12 (LB): 0xFFFFFF80 (extendido con signo)
- R13 (LBU): 0x00000080 (extendido con ceros)

## Conclusiones
Anduvo. SB guarda solo el byte bajo y LB/LBU difieren correctamente en la extensión.

---

# Caso 11
## Descripción
SH y LH, análogo al Caso 10 con 16 bits.

## Instrucciones
SH $11,0($10) ; LH $12,0($10)

## Precondiciones
- $10 = 0x300
- $11 = 0xFFFF8000 (halfword bajo = 0x8000, signo activo)

## Code
```
set [0x0] 0x52960000
set [0x4] 0x62980000
set r10 0x300
set r11 0xFFFF8000
step
examine 0x300
registers
step
registers
```

## Postcondiciones
- examine 0x300 tras SH: 0x00008000
- R12 (LH): 0xFFFF8000 (extendido con signo)

## Conclusiones
Anduvo. Comportamiento correcto y consistente con el Caso 10.

---

# Caso 12
## Descripción
SLTI, casos verdadero y falso.

## Instrucciones
SLTI $11,$10,10 ; SLTI $12,$10,3

## Precondiciones
- $10 = 5

## Code
```
set [0x0] 0xB296000A
set [0x4] 0xB2980003
set r10 5
step
registers
step
registers
```

## Postcondiciones
- R11 = 1 (5 < 10)
- R12 = 0 (5 < 3 es falso)

## Conclusiones
Anduvo en ambos casos.

---

# Caso 13
## Descripción
BEQ y BNE, rama tomada y no tomada, verificando `PC = PC+4+4*imm`.

## Instrucciones
BEQ $10,$11,2 ; BNE $10,$11,2

## Precondiciones
- Sub-test A (BEQ tomada): $10=5, $11=5. [0x4] y [0x8] tienen instrucciones "veneno"
  (no deberían ejecutarse), [0xC] tiene un marcador.
- Sub-test B (BNE no tomada): $10=5, $11=5
- Sub-test C (BNE tomada): $10=5, $11=9

## Code
Los marcadores usan ORI (no ADDI, para no depender del Caso 7).
```
Sub-test A:
set [0x0] 0x82960002
set [0x4] 0x282803E7
set [0x8] 0x282A0378
set [0xC] 0x2828006F
set r10 5
set r11 5
step
registers
step
registers

Sub-test B:
set [0x0] 0x8A960002
set r10 5
set r11 5
step
registers

Sub-test C:
set [0x0] 0x8A960002
set r10 5
set r11 9
step
registers
step
registers
```

## Postcondiciones
- Sub-test A: tras step 1, PC = 0xC (saltó, se saltó el veneno). Tras step 2, R20 = 111.
- Sub-test B: tras step 1, PC = 0x4 (no saltó).
- Sub-test C: tras step 1, PC = 0xC. Tras step 2, R20 = 111.

## Conclusiones
Anduvo en los tres escenarios.

---

# Caso 14
## Descripción
J y JAL.

## Instrucciones
J 0x20 ; JAL 0x20

## Precondiciones
- [0x20] tiene un marcador
- Destino byte 0x20 = palabra 8

## Code
```
set [0x0] 0x10000008
set [0x20] 0x282C00DE
step
registers
step
registers

(luego, J reemplazado por JAL en 0x0)
set [0x0] 0x18000008
step
registers
step
registers
```

## Postcondiciones
- J: tras step 1, PC = 0x20. Tras step 2, R22 = 222 (aterrizó).
- JAL: tras step 1, PC = 0x20 y R31 = 0x4 (PC+4). Tras step 2, R22 = 222.

## Conclusiones
Anduvo. J salta correctamente y JAL además guarda la dirección de retorno en $ra.

---

# Caso 15
## Descripción
JR y JALR.

## Instrucciones
JR $10 ; JALR $11,$0

## Precondiciones
- JR: $10 = 0x30, marcador en 0x30
- JALR: $11 = 0x40, marcador en 0x40

## Code
```
JR:
set [0x0] 0x0280000E
set [0x30] 0x282E014D
set r10 0x30
step
registers
step
registers

JALR:
set [0x0] 0x02C0000F
set [0x40] 0x283001BC
set r11 0x40
step
registers
step
registers
```

## Postcondiciones
- JR: tras step 1, PC = 0x30. Tras step 2, R23 = 333.
- JALR: tras step 1, PC = 0x40 y R31 = 0x4. Tras step 2, R24 = 444.

## Conclusiones
Anduvo. Igual que J/JAL pero con dirección en registro.

---

# Caso 16
## Descripción
MULH y MULHU, con operandos donde el resultado con y sin signo difiere.

## Instrucciones
MULH $12,$10,$11 ; MULHU $13,$10,$11

## Precondiciones
- $10 = 0xFFFFFFFF
- $11 = 2

## Code
func: MULH=0x16, MULHU=0x17
```
set [0x0] 0x0296C016
set [0x4] 0x0296D017
set r10 0xFFFFFFFF
set r11 2
step
registers
step
registers
```

## Postcondiciones
- R12 (MULH) = 0xFFFFFFFF (mitad alta de -1*2 con signo)
- R13 (MULHU) = 0x1 (mitad alta de 4294967295*2 sin signo)

## Conclusiones
Anduvo. Ambas calculan bien la mitad alta según su interpretación de signo.

---

# Caso 17
## Descripción
DIVU y RESTU con un operando cuyo bit alto está activo.

## Instrucciones
DIVU $12,$10,$11 ; RESTU $13,$10,$11

## Precondiciones
- $10 = 0x80000000
- $11 = 3

## Code
func: DIVU=0x19, RESTU=0x1B
```
set [0x0] 0x0296C019
set [0x4] 0x0296D01B
set r10 0x80000000
set r11 3
step
registers
step
registers
```

## Postcondiciones
- R12 = 0x2AAAAAAA (2147483648 / 3)
- R13 = 2 (resto)

## Conclusiones
Anduvo. Trata el operando como número sin signo grande, no como negativo.

---

# Caso 18
## Descripción
Loads indexados por dos registros: LWX, LHX, LHUX, LBX, LBUX. La dirección efectiva
es `$rs + $rd` (no `$rs + imm`).

## Instrucciones
LWX $13,$10,$12 ; LHX $14,$10,$12 ; LHUX $15,$10,$12 ; LBX $16,$10,$12 ; LBUX $17,$10,$12

## Precondiciones
- $10 = 0x100, $12 = 0x10 (dirección efectiva = 0x110)
- Antes de LWX: mem[0x110] = 0xCAFEBABE
- Antes de LHX/LHUX: mem[0x110] = 0x00008000
- Antes de LBX/LBUX: mem[0x110] = 0x00000080

## Code
func: LWX=0x14, LHX=0x10, LHUX=0x11, LBX=0x12, LBUX=0x13
```
set [0x0] 0x029AC014
set [0x4] 0x029CC010
set [0x8] 0x029EC011
set [0xC] 0x02A0C012
set [0x10] 0x02A2C013
set r10 0x100
set r12 0x10
set [0x110] 0xCAFEBABE
step
registers
set [0x110] 0x00008000
step
registers
step
registers
set [0x110] 0x00000080
step
registers
step
registers
```

## Postcondiciones
- R13 (LWX) = 0xCAFEBABE
- R14 (LHX) = 0xFFFF8000
- R15 (LHUX) = 0x00008000
- R16 (LBX) = 0xFFFFFF80
- R17 (LBUX) = 0x00000080

## Conclusiones
Anduvieron las 5. El direccionamiento indexado calcula bien la dirección y cada
variante aplica la extensión que corresponde.

---

# Caso 19
## Descripción
LHU (load halfword sin signo), aislada en proceso limpio.

## Instrucciones
LHU $13,0($10)

## Precondiciones
- $10 = 0x300
- mem[0x300] = 0x0000ABCD

## Code
LHU: rs=10, rt=13, opcode=13 -> `0x6A9A0000`
```
set [0x300] 0x0000ABCD
set [0x0] 0x6A9A0000
set r10 0x300
step
registers
```

## Postcondiciones
- R13 = 0xFFFFFFCD (se esperaba 0x0000ABCD)
- Last Memory Operation: Size = 1 (se esperaba 2)

## Conclusiones
No anduvo. LHU lee 1 solo byte y lo extiende con signo, en vez de leer un halfword de
2 bytes y extenderlo con ceros: se comporta como LB. Confirmado con dos valores de
memoria distintos.

---

# Caso 20
## Descripción
BLT, BGT, BLE, BGE, incluyendo los bordes de igualdad en BLE/BGE.

## Instrucciones
BLT $10,$11,1 ; BGT $10,$11,1 ; BLE $10,$11,1 ; BGE $10,$11,1

## Precondiciones
- BLT: $10=3, $11=7
- BGT: $10=7, $11=3
- BLE: $10=3, $11=3 (igualdad)
- BGE: $10=3, $11=3 (igualdad)
- [0x4] veneno, [0x8] marcador, en cada caso

## Code
opcode: BLT=18, BGT=19, BLE=20, BGE=21
```
set [0x0] <word> 
set [0x4] 0x282803E7
set [0x8] 0x2828006F
set r10 <valor>
set r11 <valor>
step
registers
step
registers
```

## Postcondiciones
- Los 4 casos saltan a 0x8 (se saltean el veneno) y el marcador confirma R20 = 111.

## Conclusiones
Anduvieron los 4, incluidos los bordes de igualdad de BLE y BGE.

---

# Caso 21
## Descripción
SLTIU, comparación sin signo con inmediato.

## Instrucciones
SLTIU $11,$10,10 ; SLTIU $12,$10,10

## Precondiciones
- Test A: $10 = 0xFFFFFFFF
- Test B: $10 = 5

## Code
```
set [0x0] 0xBA96000A
set [0x4] 0xBA98000A
set r10 0xFFFFFFFF
step
registers
set r10 5
step
registers
```

## Postcondiciones
- R11 = 0 (4294967295 no es menor a 10, sin signo)
- R12 = 1 (5 < 10)

## Conclusiones
Anduvo. Mismo bit pattern que en SLTI (Caso 12) da resultado opuesto, confirmando
que la comparación es sin signo.

---

# Caso 22
## Descripción
CFS (leer registro especial) y CTS (escribir registro especial).

## Instrucciones
CFS $10,4 ; CTS $11,4

## Precondiciones
- Índice de registro especial usado (según el orden del manual): 0=psw, 1=ecr,
  2=epc, 3=bva, 4=vbr
- $11 = 0x12345678 para el test de CTS
- Se probaron los 5 índices posibles (0 a 4) para descartar un índice equivocado

## Code
func: CFS=0x06, CTS=0x07
```
set [0x0] 0x02800206   (CFS aux=4)
set [0x4] 0x02800186   (CFS aux=3)
set [0x8] 0x02800106   (CFS aux=2)
set [0xC] 0x02800086   (CFS aux=1)
set [0x10] 0x02800006  (CFS aux=0)
step / registers (x5)

set [0x0] 0x02C00207   (CTS aux=4)
set [0x4] 0x02C00187   (CTS aux=3)
set [0x8] 0x02C00107   (CTS aux=2)
set [0xC] 0x02C00087   (CTS aux=1)
set [0x10] 0x02C00007  (CTS aux=0)
set r11 0x99999999
step / registers (x5)
```

## Postcondiciones
- CFS: R10 quedó en 0 en los 5 índices. PC avanza normal, CAUSE = 0.
- CTS: ningún registro especial (PC, CAUSE, EPC, BADVADR, VBR, Mode) cambió en
  ningún índice.

## Conclusiones
No anduvieron. A diferencia de ADDI/LUI, no generan excepción: ejecutan pero no
tienen ningún efecto observable, para ningún índice probado. Parecen no
implementadas.

---

# Caso 23
## Descripción
TRAP (guarda PC+4 en $epc y salta a M[aux<<2]) y RFT (PC = $epc).

## Instrucciones
TRAP 2 ; RFT

## Precondiciones
- [0x0] = TRAP aux=2
- [0x4] = marcador de retorno
- [0x8] = dato 0x40 (dirección destino que debería leer TRAP)
- [0x40] = marcador de handler
- [0x44] = RFT

## Code
func: TRAP=0x20, RFT=0x21
```
set [0x0] 0x00000120
set [0x4] 0x282A00DE
set [0x8] 0x00000040
set [0x40] 0x2828006F
set [0x44] 0x00000021
step
registers
step
registers
step
registers
step
registers
```

## Postcondiciones
- Tras step 1 (TRAP): PC = 0x4 (no saltó a 0x40). CAUSE = 0x00000003, EPC = 0
  (no se guardó). Mismo patrón que ADDI/LUI antes de arreglarse.
- Pasos siguientes: PC queda congelado en 0x4.
- RFT sola, aislada: mismo resultado, CAUSE=0x00000003 apenas se ejecuta.
- Prueba adicional: una excepción real (LW a dirección 0x2, desalineada) sí produce
  CAUSE=0x00000005 y BADVADR=0x2 correctos, pero tampoco actualiza EPC.

## Conclusiones
No anduvieron. Disparan el mismo código de causa (3) que tenían ADDI/LUI cuando no
estaban implementadas. El manejo de $epc parece incompleto en general (tampoco se
actualiza en una excepción real de acceso desalineado).

---

# Resumen final

| # | Instrucciones | Resultado |
|---|---|---|
| 1 | ADD, SUB | OK |
| 2 | AND, OR, XOR, NOR | OK |
| 3 | SLT, SLTU | OK |
| 4 | SLL, SRL, SRA | OK |
| 5 | SLLR, SRLR, SRAR | OK |
| 6 | MUL, DIV, REST | OK |
| 7 | ADDI | Estaba rota, arreglada en la versión nueva |
| 8 | ANDI, ANDIH, ORI, ORIH, XORI, XORIH | OK |
| 8 | LUI | Estaba rota, arreglada en la versión nueva |
| 9 | SW, LW | OK |
| 10 | SB, LB, LBU | OK |
| 11 | SH, LH | OK |
| 12 | SLTI | OK |
| 13 | BEQ, BNE | OK |
| 14 | J, JAL | OK |
| 15 | JR, JALR | OK |
| 16 | MULH, MULHU | OK |
| 17 | DIVU, RESTU | OK |
| 18 | LWX, LHX, LHUX, LBX, LBUX | OK |
| 19 | LHU | No anduvo — se comporta como LB |
| 20 | BLT, BGT, BLE, BGE | OK |
| 21 | SLTIU | OK |
| 22 | CFS, CTS | No implementadas |
| 23 | TRAP, RFT | No implementadas |

Total: 58 de 58 instrucciones probadas. 54 correctas.
