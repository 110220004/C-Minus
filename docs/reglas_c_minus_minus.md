# Reglas del lenguaje C-- (C Minus Minus)

## 1. Objetivo de esta entrega
C-- es un lenguaje educativo para practicar compiladores e interpretes.
Esta version incluye:
- Tipos de datos
- Identificadores de variables
- Impresion en pantalla
- Comentarios
- Bloques y manejo de scope con shadowing
- Funciones con parametros tipados y `return`
- Construccion de AST y ejecucion por interprete

## 2. Tipos de datos soportados
- `int`: enteros (ejemplo: `10`, `-3`)
- `float`: decimales (ejemplo: `3.14`, `0.5`)
- `string`: texto entre comillas dobles (ejemplo: `"hola"`)
- `bool`: booleanos (`true`, `false`)

## 3. Identificadores de variables
Reglas:
- Deben iniciar con letra o guion bajo (`_`).
- Pueden contener letras, numeros y guion bajo.
- No pueden usar palabras reservadas (`int`, `float`, `string`, `bool`, `print`, `true`, `false`, `func`, `return`).

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

## 8. Funciones

### Declaracion de funcion
Sintaxis:
```cmm
func nombre(param1:tipo, param2:tipo, ...) -> tipo_retorno {
   // sentencias
   return expresion;
}
```

Ejemplo:
```cmm
func add(a:int, b:int) -> int {
   return a + b;
}
```

### Llamada de funcion
Sintaxis:
```cmm
nombre(expr1, expr2, ...)
```

Ejemplo:
```cmm
int x = add(2, 3);
```

### Return
- `return expr;` retorna un valor.
- `return;` puede parsearse, pero la funcion igual debe cumplir su tipo de retorno declarado (`int`, `float`, `string`, `bool`).
- El tipo retornado debe ser compatible con el tipo declarado de la funcion.

## 9. Reglas semanticas basicas
- No se puede usar una variable sin declararla.
- No se puede usar una variable sin inicializar.
- No se puede redeclarar una variable con el mismo nombre dentro del mismo ambito.
- Se permite shadowing: una variable interna puede usar el mismo nombre de una externa y la interna tiene prioridad en ese scope.
- La asignacion debe respetar el tipo declarado.
- Se permite convertir `int` a `float` al asignar a una variable `float`.
- No se puede redeclarar una funcion con el mismo nombre.
- En una llamada, la cantidad de argumentos debe coincidir con la cantidad de parametros.

## 10. Estructura general del programa
Un programa es una secuencia de sentencias terminadas en `;`.

Sentencias soportadas:
- Declaracion
- Asignacion
- Impresion (`print`)
- Bloque (`{ ... }`) con ambito local
- Declaracion de funcion (`func`)
- Return (`return`)

## 11. EBNF simplificada
```ebnf
programa      = { sentencia } EOF ;

sentencia     = declaracion ";"
              | asignacion ";"
              | impresion ";"
              | retorno
              | funcion
              | bloque ;

bloque        = "{" { sentencia } "}" ;

declaracion   = tipo identificador [ "=" expresion ] ;
asignacion    = identificador "=" expresion ;
impresion     = "print" "(" expresion ")" ;
retorno       = "return" [ expresion ] ;

funcion       = "func" identificador "(" [ parametros ] ")" "->" tipo_retorno bloque ;
parametros    = parametro { "," parametro } ;
parametro     = identificador ":" tipo ;

tipo          = "int" | "float" | "string" | "bool" ;
tipo_retorno  = tipo ;

expresion     = termino { ("+" | "-") termino } ;
termino       = factor { ("*" | "/") factor } ;
factor        = numero
              | cadena
              | booleano
              | identificador
              | llamada_funcion
              | "(" expresion ")"
              | "-" factor ;

llamada_funcion = identificador "(" [ argumentos ] ")" ;
argumentos      = expresion { "," expresion } ;

numero        = entero | decimal ;
booleano      = "true" | "false" ;
identificador = (letra | "_") { letra | digito | "_" } ;
```

## 12. Modelo de ejecucion (resumen)
- El parser construye un AST completo (`AST_PROGRAM`) en lugar de ejecutar directamente.
- El interprete realiza dos pases:
1. Registro de funciones (`AST_FUNC_DECL`).
2. Ejecucion de sentencias y expresiones.
- El `ScopeManager` maneja un stack de scopes (`push`/`pop`) y resuelve nombres desde el scope actual hacia los padres.
- Para shadowing, las variables locales se definen en el scope actual y tapan a las externas mientras el scope este activo.

## 13. Alcance actual y mejora futura
Esta version aun no incluye estructuras de control (`if`, `while`) ni operadores relacionales/logicos.
La arquitectura modular (AST + interpreter + scope + symbols) deja el proyecto listo para agregar esas caracteristicas.
