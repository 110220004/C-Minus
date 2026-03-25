# Reglas del lenguaje C-- (C Minus Minus)

## 1. Objetivo de esta entrega
C-- es un lenguaje educativo para practicar compiladores e interpretes.
Esta segunda evaluacion incluye funciones basicas:
- Tipos de datos
- Identificadores de variables
- Impresion en pantalla
- Comentarios

## 2. Tipos de datos soportados
- `int`: enteros (ejemplo: `10`, `-3`)
- `float`: decimales (ejemplo: `3.14`, `0.5`)
- `string`: texto entre comillas dobles (ejemplo: `"hola"`)
- `bool`: booleanos (`true`, `false`)

## 3. Identificadores de variables
Reglas:
- Deben iniciar con letra o guion bajo (`_`).
- Pueden contener letras, numeros y guion bajo.
- No pueden usar palabras reservadas (`int`, `float`, `string`, `bool`, `print`, `true`, `false`).

Ejemplos validos:
- `edad`
- `_contador`
- `promedio1`

## 4. Declaracion y asignacion
### Declaracion
```cmm
int edad;
float nota;
string nombre;
bool activo;
```

### Declaracion con inicializacion
```cmm
int edad = 20;
float nota = 90.5;
string nombre = "Ana";
bool activo = true;
```

### Asignacion posterior
```cmm
edad = 21;
nota = nota + 1.0;
```

## 5. Impresion en pantalla
Sintaxis:
```cmm
print(expresion);
```

Ejemplos:
```cmm
print("Hola");
print(edad);
print("Nombre: " + nombre);
```

## 6. Comentarios
- Comentario de una linea:
```cmm
// Este es un comentario
```

- Comentario de multiples lineas:
```cmm
/*
   Comentario
   multilinea
*/
```

## 7. Operaciones permitidas en esta etapa
- Aritmetica: `+`, `-`, `*`, `/` para `int` y `float`.
- Concatenacion con `+` cuando participa un `string`.
- Parentesis para agrupar expresiones.

## 8. Reglas semanticas basicas
- No se puede usar una variable sin declararla.
- No se puede usar una variable sin inicializar.
- No se puede redeclarar una variable con el mismo nombre dentro del mismo ambito.
- Se permite shadowing: una variable interna puede usar el mismo nombre de una externa.
- La asignacion debe respetar el tipo declarado.
- Se permite convertir `int` a `float` al asignar a una variable `float`.

## 9. Estructura general del programa
Un programa es una secuencia de sentencias terminadas en `;`.

Sentencias soportadas:
- Declaracion
- Asignacion
- Impresion (`print`)
- Bloque (`{ ... }`) con ambito local

## 10. EBNF simplificada
```ebnf
programa      = { sentencia } EOF ;

sentencia     = declaracion ";"
              | asignacion ";"
              | impresion ";"
              | bloque ;

bloque        = "{" { sentencia } "}" ;

declaracion   = tipo identificador [ "=" expresion ] ;
asignacion    = identificador "=" expresion ;
impresion     = "print" "(" expresion ")" ;

tipo          = "int" | "float" | "string" | "bool" ;

expresion     = termino { ("+" | "-") termino } ;
termino       = factor { ("*" | "/") factor } ;
factor        = numero
              | cadena
              | booleano
              | identificador
              | "(" expresion ")"
              | "-" factor ;

numero        = entero | decimal ;
booleano      = "true" | "false" ;
identificador = (letra | "_") { letra | digito | "_" } ;
```

## 11. Alcance actual y mejora futura
Esta version no incluye estructuras de control (`if`, `while`), funciones ni comparaciones logicas complejas.
El diseno del lexer, parser y tabla de simbolos permite extender facilmente el lenguaje en futuras entregas.
