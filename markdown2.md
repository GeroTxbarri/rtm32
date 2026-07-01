# Pruebas del Instruction Set RTM32 (STX4)

Todas las pruebas fueron ejecutadas contra el emulador real `rtm32` (RTM32 Computer v0.4)
mediante el debugger remoto (`telnet localhost 4444`), usando los comandos `set`, `step`
y `registers`/`examine` del propio depurador. Cada instrucción fue codificada a mano a
32 bits siguiendo los formatos R/I/L/J descriptos en el manual (rtm32.pdf), cargada en
memoria con `set [addr] 0xPALABRA`, y ejecutada con `step`. El resultado se comparó con
lo que la Tabla A.1/A.2 del manual dice que debería pasar.

**Convención de registros usada:** `$t0..$t2` = `$10,$11,$12` (fuente1, fuente2, destino)
salvo que se indique lo contrario. Todas las direcciones de memoria/instrucción están en
hexadecimal de 32 bits.

**Resumen de resultados:** 26 comportamientos de instrucción confirmados correctos,
2 bugs confirmados (**ADDI** y **LUI** — ver Casos 7 y 8). Esto cubre 28 de ~55-58
variantes del set (~50%), cumpliendo la meta pedida.

---

# Caso 1
## Descripción
Testeo las instrucciones aritméticas básicas tipo R: `ADD` y `SUB`, verificando que
`$rd = $rs + $rt` y `$rd = $rs - $rt` respectivamente.

## Instrucciones
`ADD $t2, $t0, $t1` ; `SUB $t3, $t0, $t1`

## Precondiciones
- `$t0 ($10) = 15`
- `$t1 ($11) = 4`
- Memoria: `[0x00] = ADD $12,$10,$11` ; `[0x04] = SUB $13,$10,$11`

## Code
Codificación manual (formato R: `opcode(0) rs rt rd aux(0) X(0) func`):
- `ADD`: rs=10, rt=11, rd=12, func=0x1C (011100) → `0x0296C01C`
- `SUB`: rs=10, rt=11, rd=13, func=0x1D (011101) → `0x0296D01D`

Comandos enviados al debugger:
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
Salida real del debugger tras cada `step`:
```
Stepped instructions. Target PC: 0x00000004
R[10]: 0x0000000F   R[11]: 0x00000004   R[12]: 0x00000013   ...

Stepped instructions. Target PC: 0x00000008
R[12]: 0x00000013   R[13]: 0x0000000B   ...
```
`$12 = 0x13 = 19 = 15+4` ✓ ; `$13 = 0x0B = 11 = 15-4` ✓

## Conclusiones
**Anduvo.** Tanto `ADD` como `SUB` se comportan exactamente como especifica la Tabla A.2,
tanto en el resultado aritmético como en el avance normal del PC (+4).

---

# Caso 2
## Descripción
Testeo las 4 operaciones lógicas bit a bit tipo R: `AND`, `OR`, `XOR`, `NOR`.

## Instrucciones
`AND $t2,$t0,$t1` ; `OR $t3,$t0,$t1` ; `XOR $t4,$t0,$t1` ; `NOR $t5,$t0,$t1`

## Precondiciones
- `$t0 = 0xC` (1100b), `$t1 = 0xA` (1010b)
- Memoria `0x00..0x0C` con las 4 instrucciones (func 0x08,0x09,0x0A,0x0B respectivamente)

## Code
- `AND` rd=12 → `0x0296C008`
- `OR`  rd=13 → `0x0296D009`
- `XOR` rd=14 → `0x0296E00A`
- `NOR` rd=15 → `0x0296F00B`
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
```
R[12]: 0x00000008   (AND: 0xC & 0xA = 0x8)
R[13]: 0x0000000E   (OR:  0xC | 0xA = 0xE)
R[14]: 0x00000006   (XOR: 0xC ^ 0xA = 0x6)
R[15]: 0xFFFFFFF1   (NOR: ~(0xC|0xA) = ~0xE = 0xFFFFFFF1)
```
Los 4 resultados coinciden exactamente con la tabla de verdad esperada.

## Conclusiones
**Anduvo.** `AND`, `OR`, `XOR` y `NOR` funcionan correctamente, incluyendo el
complemento a 32 bits completo en `NOR`.

---

# Caso 3
## Descripción
Testeo `SLT` (set-less-than con signo) vs `SLTU` (sin signo) usando el mismo par de
registros, para exponer la diferencia entre comparación signada y no signada.

## Instrucciones
`SLT $t2,$t0,$t1` ; `SLTU $t3,$t0,$t1`

## Precondiciones
- `$t0 = 0xFFFFFFFF` (interpretado con signo = -1)
- `$t1 = 0x00000001` (= 1)

## Code
`SLT` func=0x0C rd=12 → `0x0296C00C` ; `SLTU` func=0x0D rd=13 → `0x0296D00D`
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
```
R[12]: 0x00000001   (SLT:  -1 < 1  con signo → true → 1)
R[13]: 0x00000000   (SLTU: 0xFFFFFFFF < 1 sin signo → false → 0)
```

## Conclusiones
**Anduvo.** Se confirma que `SLT` interpreta los operandos con signo y `SLTU` sin signo;
el mismo bit pattern da resultados opuestos según la instrucción, tal como especifica la
tabla.

---

# Caso 4
## Descripción
Testeo los desplazamientos con cantidad inmediata: `SLL` (lógico izq.), `SRL` (lógico
der.) y `SRA` (aritmético der.), este último para confirmar que preserva el signo.

## Instrucciones
`SLL $t2,$t1,4` ; `SRL $t3,$t1,1` ; `SRA $t4,$t1,1`

## Precondiciones
- Para `SLL`: `$t1 = 1`
- Para `SRL`/`SRA`: `$t1 = 0x80000000` (bit de signo activo)

## Code
Formato R con `aux` = cantidad de shift: rs=0(no usado), rt=11, aux=shift, func.
- `SLL` rd=12 aux=4 func=0x00 → `0x0016C200`
- `SRL` rd=13 aux=1 func=0x01 → `0x0016D081`
- `SRA` rd=14 aux=1 func=0x02 → `0x0016E082`
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
```
R[12]: 0x00000010   (SLL: 1<<4 = 16)
R[13]: 0x40000000   (SRL: 0x80000000>>1 lógico, entra un 0)
R[14]: 0xC0000000   (SRA: 0x80000000>>1 aritmético, entra el bit de signo=1)
```

## Conclusiones
**Anduvo.** El resultado de `SRL` vs `SRA` sobre el mismo valor negativo confirma que
`SRL` rellena con ceros y `SRA` replica el bit de signo, exactamente como indica la tabla
(`≫` vs `⋙`).

---

# Caso 5
## Descripción
Igual que el Caso 4 pero con la cantidad de shift tomada de un registro:
`SLLR`, `SRLR`, `SRAR` (usan `$rs[4:0]` como cantidad).

## Instrucciones
`SLLR $t2,$t0,$t1` ; `SRLR $t3,$t0,$t1` ; `SRAR $t4,$t0,$t1`
(convención: `$t0`=registro con la cantidad de shift, `$t1`=valor a desplazar)

## Precondiciones
- Para `SLLR`: `$t0=3` (shift amount), `$t1=1`
- Para `SRLR`/`SRAR`: `$t0=1`, `$t1=0x80000000`

## Code
- `SLLR` func=0x03 rd=12 → `0x0296C003`
- `SRLR` func=0x04 rd=13 → `0x0296D004`
- `SRAR` func=0x05 rd=14 → `0x0296E005`
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
```
R[12]: 0x00000008   (SLLR: 1<<3 = 8)
R[13]: 0x40000000   (SRLR: 0x80000000>>1 lógico)
R[14]: 0xC0000000   (SRAR: 0x80000000>>1 aritmético, conserva signo)
```

## Conclusiones
**Anduvo.** Comportamiento idéntico y consistente con la versión por inmediato (Caso 4),
usando ahora la cantidad de shift desde un registro.

---

# Caso 6
## Descripción
Testeo las operaciones de multiplicación y división entera: `MUL`, `DIV`, `REST` (resto).

## Instrucciones
`MUL $t2,$t0,$t1` ; `DIV $t3,$t0,$t1` ; `REST $t4,$t0,$t1`

## Precondiciones
- `$t0 = 7`, `$t1 = 3`

## Code
- `MUL` func=0x15 rd=12 → `0x0296C015`
- `DIV` func=0x18 rd=13 → `0x0296D018`
- `REST` func=0x1A rd=14 → `0x0296E01A`
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
```
R[12]: 0x00000015   (MUL:  7*3 = 21)
R[13]: 0x00000002   (DIV:  7/3 = 2, división entera)
R[14]: 0x00000001   (REST: 7%3 = 1)
```

## Conclusiones
**Anduvo.** Las tres operaciones dan el resultado entero correcto.

---

# Caso 7 — ⚠️ BUG CONFIRMADO
## Descripción
Testeo `ADDI` (suma con inmediato de 17 bits), la instrucción que varios compañeros
reportaron como sospechosa. Se probó en dos configuraciones distintas y aisladas
(procesos separados del emulador, para no arrastrar estado de otros tests).

## Instrucciones
`ADDI $t1,$t0,10` (con `$rs≠$zero`) y `ADDI $t3,$zero,100` (idiom clásico de "cargar
constante", con `$rs=$zero`)

## Precondiciones
- Test A: `$t0 ($10) = 5`, memoria `[0x0] = ADDI $11,$10,10` (se espera `$11 = 15`)
- Test C: memoria `[0x0] = ADDI $13,$0,100` (se espera `$13 = 100`)
- Ambos tests se corrieron en **procesos nuevos** del emulador (recién iniciado, todos
  los registros en 0, `CAUSE=0`).

## Code
Codificación (formato I: `opcode(1) rs rt imm`):
- `ADDI $11,$10,10`: rs=10,rt=11,imm=10 → `0x0A96000A`
- `ADDI $13,$0,100`: rs=0,rt=13,imm=100 → `0x081A0064`

Decodifiqué el word `0x0A96000A` bit a bit para descartar error de codificación propio:
opcode=00001(1,ADDI) ✓, rs=01010(10) ✓, rt=01011(11) ✓, imm=0000000000001010(10) ✓ — la
codificación es correcta.

```
Test A:
set [0x0] 0x0A96000A
set r10 5
registers
examine 0x0
step
registers

Test C:
set [0x0] 0x081A0064
registers
step
registers
```

## Postcondiciones
Test A — antes del step: `R[10]=5, R[11]=0, CAUSE=0, VBR=0xF0000000`.
`examine 0x0` confirma que la memoria tiene el word correcto: `0x00000000: 0x0A96000A`.

Después del `step`:
```
Stepped instructions. Target PC: 0x00000004
R[10]: 0x00000005   R[11]: 0x00000000   <-- debería ser 0x0000000F (15)
PC      : 0x00000004  CAUSE   : 0x00000003  EPC     : 0x00000000
BADVADR : 0x00000000  VBR     : 0x00000002   <-- debería seguir en 0xF0000000
```

Test C (idéntico patrón de falla):
```
Stepped instructions. Target PC: 0x00000004
R[13]: 0x00000000   <-- debería ser 0x00000064 (100)
CAUSE   : 0x00000003   VBR     : 0x00000002
```

## Conclusiones
**No anduvo.** `ADDI` está rota, y de forma 100% reproducible en ambos escenarios
(con y sin `$rs=$zero`, con inmediato positivo). En vez de sumar y guardar el resultado
en `$rt`:
1. El registro destino **nunca se actualiza** (queda en su valor previo).
2. El PC sí avanza normalmente (+4), así que no es un problema de fetch.
3. Los registros especiales `CAUSE` (pasa de 0 a `0x00000003`) y `VBR` (pasa de
   `0xF0000000` a `0x00000002`) cambian **sin que la instrucción tenga nada que ver con
   excepciones ni con `CTS`/interrupciones**. Esto sugiere que el decodificador interno
   de `ADDI` está mal enrutado y termina disparando una excepción espuria (código de
   causa 3) y/o escribiendo por error en los registros especiales `$ecr`/`$vbr` en vez de
   en `$rt`.
4. Una vez que esto ocurre, **el estado de fallo contamina toda ejecución posterior** en
   la misma sesión (ver nota abajo) — otra instrucción cualquiera que se ejecute después
   también muestra `PC` congelado y `CAUSE=3`, aun hacíendo `set pc 0x0` de nuevo. Solo se
   resuelve reiniciando el proceso del emulador.

**Esto confirma lo que se comentó en el grupo: `ADDI` no anda.** Recomiendo avisar al
profesor con esta evidencia concreta.

---

# Caso 8 — ⚠️ Un bug más (LUI), el resto OK
## Descripción
Testeo las 4 operaciones lógicas con inmediato (`ANDI`, `ANDIH`, `ORI`, `XORI`) y la
carga de constante alta `LUI`. El manual advierte de un bug conocido en `ANDI`, así que
puse especial atención ahí.

## Instrucciones
`ANDI $t1,$t0,0x00FF` ; `ANDIH $t2,$t0,0x00FF` ; `ORI $t3,$t0,0x1234` ;
`XORI $t4,$t0,0x00FF` ; `LUI $t5,0xABCD`

## Precondiciones
- Para `ANDI`/`ANDIH`: `$t0 = 0xFFFFFFFF`
- Para `ORI`: `$t0 = 0`
- Para `XORI`: `$t0 = 0x0000FFFF`
- `LUI` no necesita precondición (constante pura)

## Code
Formato L: `opcode rs rt h imm(16)`.
- `ANDI` (h=0) opcode=4 → `0x229600FF`
- `ANDIH` (h=1) opcode=4 → `0x229900FF`
- `ORI` opcode=5 → `0x2A9A1234`
- `XORI` opcode=6 → `0x329C00FF`
- `LUI` opcode=7 rt=15 imm=0xABCD → `0x381EABCD`
```
set pc 0x0
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
step        <- este es LUI, se ejecutó en proceso aislado aparte para no contaminar
registers
```
(La ejecución de `LUI` se repitió también sola, en un proceso nuevo, para confirmar sin
ninguna duda que el fallo es de `LUI` y no arrastre de los pasos anteriores.)

## Postcondiciones
```
R[11]: 0x000000FF   (ANDI:  0xFFFFFFFF & 0x00FF        = 0xFF)          ✓ correcto
R[12]: 0x00FF0000   (ANDIH: 0xFFFFFFFF & (0x00FF<<16)   = 0x00FF0000)    ✓ correcto
R[13]: 0x00001234   (ORI:   0 | 0x1234                  = 0x1234)       ✓ correcto
R[14]: 0x0000FF00   (XORI:  0x0000FFFF ^ 0x000000FF      = 0x0000FF00)   ✓ correcto

R[15]: 0x00000000   (LUI: debería ser 0xABCD0000)   <-- NO se actualizó
PC tras el step de LUI: 0x00000004 (avanzó normal)
CAUSE   : 0x00000003   VBR     : 0x00000002   <-- mismo patrón que ADDI
```

## Conclusiones
**ANDI, ANDIH, ORI y XORI: anduvieron correctamente** — con los valores que probé, el
bug histórico de `ANDI` mencionado en el manual **no se manifestó** (puede que ya esté
corregido en esta versión v0.4, o que dependa de un caso borde distinto al que probé;
lo dejo anotado por si algún compañero lo reproduce con otros valores).

**LUI no anduvo.** Mismo patrón de falla que `ADDI` (Caso 7): el registro destino no se
actualiza, y `CAUSE`/`VBR` cambian a los mismos valores espurios (`0x3`/`0x2`). Es
altamente probable que `ADDI` y `LUI` compartan el mismo bug de fondo en el
decodificador/ruteo de escritura de resultado (ambas son de los formatos que no pasan
por la ALU de registro-registro), aunque no tengo acceso al código fuente para
confirmarlo con certeza.

---

# Caso 9
## Descripción
Testeo `SW` (store word) y `LW` (load word) verificando el round-trip completo:
guardar un valor de 32 bits en memoria y volver a leerlo.

## Instrucciones
`SW $t1,0($t0)` ; `LW $t2,0($t0)`

## Precondiciones
- `$t0 = 0x00000100` (dirección base, dentro de la RAM de usuario)
- `$t1 = 0xDEADBEEF` (valor a guardar)
- Memoria en `0x100` inicialmente en `0x00000000`

## Code
- `SW` opcode=9 rs=10,rt=11,imm=0 → `0x4A960000`
- `LW` opcode=8 rs=10,rt=12,imm=0 → `0x42980000`
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
```
examine 0x100 (antes):  0x00000100: 0x00000000
[step SW]
examine 0x100 (después): 0x00000100: 0xDEADBEEF     <- confirmado en memoria
Last Memory Operation: Address 0x100 | Size 4 | Type WRITE

[step LW]
R[12]: 0xDEADBEEF
Last Memory Operation: Address 0x100 | Size 4 | Type READ
```

## Conclusiones
**Anduvo.** `SW` escribe correctamente la palabra completa en la dirección efectiva
`$rs+imm`, y `LW` la recupera intacta. El log de "Last Memory Operation" del debugger
confirma tipo y tamaño de acceso correctos.

---

# Caso 10
## Descripción
Testeo `SB` (store byte) junto con `LB` y `LBU` (load byte con y sin extensión de
signo), para exponer la diferencia entre ambas.

## Instrucciones
`SB $t1,0($t0)` ; `LB $t2,0($t0)` ; `LBU $t3,0($t0)`

## Precondiciones
- `$t0 = 0x00000200`
- `$t1 = 0xFFFFFF80` (byte bajo = `0x80`, bit de signo del byte activo)

## Code
- `SB` opcode=11 → `0x5A960000`
- `LB` opcode=14 rt=12 → `0x72980000`
- `LBU` opcode=15 rt=13 → `0x7A9A0000`
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
```
examine 0x200 tras SB: 0x00000200: 0x00000080   (solo se guardó el byte bajo)
R[12]: 0xFFFFFF80   (LB:  0x80 con signo → se extiende con 1s)
R[13]: 0x00000080   (LBU: 0x80 sin signo → se extiende con 0s)
```

## Conclusiones
**Anduvo.** `SB` guarda únicamente el byte menos significativo, y la diferencia entre
`LB`/`LBU` (extensión de signo vs. cero) se confirma exactamente según la fórmula
`SBE`/`ZBE` de la tabla A.1.

---

# Resumen final

| # | Instrucción(es) | Resultado |
|---|---|---|
| 1 | ADD, SUB | ✅ OK |
| 2 | AND, OR, XOR, NOR | ✅ OK |
| 3 | SLT, SLTU | ✅ OK |
| 4 | SLL, SRL, SRA | ✅ OK |
| 5 | SLLR, SRLR, SRAR | ✅ OK |
| 6 | MUL, DIV, REST | ✅ OK |
| 7 | **ADDI** | ❌ **BUG** — no actualiza destino, corrompe CAUSE/VBR |
| 8 | ANDI, ANDIH, ORI, XORI | ✅ OK |
| 8 | **LUI** | ❌ **BUG** — mismo patrón de falla que ADDI |
| 9 | SW, LW | ✅ OK |
| 10 | SB, LB, LBU | ✅ OK |

**Total: 28 de ~55-58 variantes probadas (~50%). 26 correctas, 2 bugs confirmados.**

**Pendientes:** el resto de loads/stores (`SH`,`LH`,`LHU`), comparaciones e inmediatos
restantes (`SLTI`,`SLTIU`), todos los branches (`BEQ`,`BNE`,`BLT`,`BGT`,`BLE`,`BGE`),
saltos (`J`,`JAL`,`JR`,`JALR`), variantes de multiplicación/división sin signo
(`MULH`,`MULHU`,`DIVU`,`RESTU`), la familia indexada (`LWX`,`LHX`,`LHUX`,`LBX`,`LBUX`),
y — a propósito para el final, según sugerencia del profesor — `TRAP`, `RFT`, `CFS`,
`CTS`.

**Bugs a reportar al profesor con urgencia:**
1. **`ADDI`**: no actualiza el registro destino y corrompe `CAUSE`/`VBR` con los valores
   `0x00000003`/`0x00000002`. Ese estado de fallo contamina cualquier instrucción
   ejecutada después en la misma sesión del emulador.
2. **`LUI`**: mismo patrón de falla exacto que `ADDI` (probablemente comparten causa
   raíz en el decodificador).
