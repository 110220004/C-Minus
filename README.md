# C-- (C Minus Minus)


## Requisitos cubiertos
- Tipos de datos (`int`, `float`, `string`, `bool`)
- Identificadores de variables
- Impresion en pantalla (`print(...)`)
- Comentarios (`//` y `/* ... */`)
- Bloques con ambitos anidados (`{ ... }`)
- Shadowing entre ambitos (variable interna tiene prioridad)

## Estructura del proyecto
- `src/main.c`: lexer + parser + ejecucion de C--
- `docs/reglas_c_minus_minus.md`: documento con reglas del lenguaje
- `docs/desafio_ambitos_shadowing.md`: reporte del desafio de investigacion
- `examples/`: archivos de ejemplo y pruebas
- `Makefile`: compilacion y ejecucion

## Compilar
```bash
make
```

## Ejecutar un ejemplo
```bash
./cmm examples/programa_basico.cmm
```

Tambien puedes usar:
```bash
make run
```

## Sintaxis base de C--
```cmm
int edad = 20;
float promedio = 90.5;
string nombre = "Wendell";
bool activo = true;

print("Nombre: " + nombre);
print(edad);

{
	int edad = 30;
	print("Edad local: " + edad);
}

print("Edad global: " + edad);
```

## Notas
- Cada sentencia termina con `;`.
- Tambien existen sentencias de bloque con `{` y `}`.
- Se validan errores semanticos basicos (redeclaracion en el mismo ambito, variable no declarada, tipo incompatible, etc.).
- Esta base esta pensada para crecer en futuras entregas (if, while, funciones, comparaciones, etc.).

## Planes para siguiente version de C-Minus
- Comparaciones y operadores logicos: permitir expresiones condicionales reales para toma de decisiones.
- Sentencias if/else: ejecutar bloques en funcion de condiciones booleanas.
- Ciclos while y for: agregar repeticion controlada para programas mas completos, entre otras cosas.
