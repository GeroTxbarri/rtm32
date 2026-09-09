Implementación de la Instrucción ORIL en Hardware (CircuitVerse)
Implementada de la instruccion ORIL a nivel de compuertas lógicas utilizando el simulador CircuitVerse.

1. Concepto General de la Instrucción oril
La instrucción oril (comúnmente OR Immediate Lower u OR Immediate Long dependiendo de la arquitectura) es una operación fundamental en el lenguaje ensamblador y en la arquitectura de procesadores (CPU).

Su función principal es tomar un valor existente en un registro, aplicarle una compuerta lógica OR bit a bit contra una constante numérica proporcionada en la misma instrucción (el valor inmediato), y sobreescribir el registro original con el nuevo resultado.

La Lógica OR
La operación OR evalúa pares de bits bajo la siguiente regla matemática: Si al menos uno de los bits es 1, el resultado es 1. Si ambos son 0, el resultado es 0.
Por lo tanto, la instrucción oril se utiliza estratégicamente para encender (setear a 1) bits específicos de un registro sin alterar (apagar) los bits que ya estaban encendidos.

2. Arquitectura del Circuito en CircuitVerse
Para replicar el comportamiento interno de una Unidad Aritmético-Lógica (ALU) ejecutando un oril, el circuito se diseñó utilizando una topología de retroalimentación secuencial.

Componentes Utilizados:
Registro (16-bit): El componente de memoria principal. Almacena el estado actual de los datos.

Compuerta OR (16-bit, 2 entradas): El núcleo de procesamiento lógico de la instrucción.

Input Inmediato (16-bit): Representa el bus de datos por donde ingresa la constante declarada en la instrucción de ensamblador.

Input Habilitador / Enable (1-bit): Señal de control que autoriza al registro a aceptar nuevos datos.

Input Reset Asíncrono (1-bit): Señal de seguridad para vaciar el registro (0000 0000 0000 0000) independientemente del estado del reloj.

Reloj / Clock: Generador de pulsos que sincroniza la escritura en la memoria.

Output Visual (16-bit): Conectado a la salida de la compuerta OR para previsualizar el resultado antes de la escritura.

Topología y Conexiones (Routing):
Entrada A de la compuerta OR: Conectada a la salida de datos (Data Out) del Registro de 16-bit.

Entrada B de la compuerta OR: Conectada directamente al Input Inmediato de 16-bit.

Bucle de Retroalimentación: La salida de la compuerta OR se conecta de vuelta al puerto de entrada (Data In) del Registro. Esto cierra el ciclo, permitiendo que el resultado sobreescriba el operando original.

3. Mecanismo de Funcionamiento (Flujo de Ejecución)
El circuito ejecuta la instrucción en un ciclo definido por la señal de reloj, emulando las etapas de un procesador real:

Lectura y Cálculo Combinacional (Inmediato): En cuanto se establece un valor en el Input Inmediato, la compuerta OR recibe este valor por una vía y el estado actual del registro por la otra. La compuerta resuelve la operación en tiempo real. El resultado queda "esperando" en la entrada del registro.

Ejecución (Flanco de Reloj): Al emitir un pulso en el Clock (transición de 0 a 1), el registro captura el valor calculado por la compuerta OR que estaba esperando en su entrada y lo almacena de forma permanente.

Cierre del Ciclo: El pin Enable vuelve a 0 para proteger el dato almacenado contra sobrescrituras accidentales en ciclos de reloj posteriores.

4. Aplicaciones en el Mundo Real
El diseño en hardware de esta instrucción y su uso en software de bajo nivel son críticos para el funcionamiento de cualquier sistema operativo y microcontrolador moderno.

Enmascaramiento de Bits (Bit Masking): Es la técnica de modificar bits individuales dentro de un bloque de memoria más grande. Con oril, un programador puede enviar una máscara (ej. 0000 0000 0000 0001) para encender luces LED específicas, activar motores o habilitar puertos en un microcontrolador (como un Arduino o Raspberry Pi) sin afectar los componentes conectados a los otros pines.

Gestión de Registros de Estado (Flags / Status Registers): Las CPUs tienen registros donde cada bit representa una configuración crítica (como el Modo Núcleo/Usuario, interrupciones habilitadas, o banderas matemáticas de la ALU). Usar oril permite al sistema operativo activar una función (por ejemplo, habilitar interrupciones de hardware) enviando un 1 a esa posición específica, garantizando por la propia naturaleza de la compuerta OR que las configuraciones previas (los 1 que ya existían) no se borren accidentalmente.

Construcción de Constantes Largas: En arquitecturas RISC (como ARM o PowerPC), las instrucciones tienen un tamaño fijo (generalmente 32 bits). Esto hace imposible cargar una variable de 32 bits directamente en un solo paso, porque no hay espacio físico en la instrucción. El sistema lo soluciona cargando primero los 16 bits superiores (con otras instrucciones) y luego usando oril para "inyectar" los 16 bits inferiores sin destruir la mitad superior.
