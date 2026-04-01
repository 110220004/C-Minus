# C-- (C Minus Minus)


## Requisitos cubiertos
- Tipos de datos (`int`, `float`, `string`, `bool`)
- Identificadores de variables
- Impresion en pantalla (`print(...)`)
- Comentarios (`//` y `/* ... */`)
- Bloques con ambitos anidados (`{ ... }`)
- Shadowing entre ambitos (variable interna tiene prioridad)
- Funciones (`func`), parametros tipados, llamada y `return`
- Construccion de AST + ejecucion con interprete

## Estructura del proyecto
- `src/main.c`: lexer + parser que construye AST
- `src/ast.h`, `src/ast.c`: nodos del AST y liberacion de memoria
- `src/interpreter.h`, `src/interpreter.c`: ejecutor del AST (2 pases)
- `src/scope.h`, `src/scope.c`: manejo de scopes con shadowing
- `src/symbols.h`, `src/symbols.c`: tipos y valores en runtime
- `docs/reglas_c_minus_minus.md`: documento con reglas del lenguaje
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

## Ejecutar pruebas
```bash
make test
```

## Sintaxis base de C-- (variables, bloques y print)
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

## Sintaxis de funciones
```cmm
func add(a:int, b:int) -> int {
	return a + b;
}

int x = add(2, 3);
print(x);
```

## Shadowing esperado
```cmm
int x = 10;

func f() -> int {
	int x = 20;
	return x;
}

print(x);   // 10
print(f()); // 20
print(x);   // 10
```

## Notas
- Cada sentencia termina con `;`.
- Se soportan bloques con `{` y `}` y scopes de funcion.
- Las funciones se registran en un primer pase para permitir llamadas aunque la declaracion aparezca despues.
- Reglas semanticas validadas: redeclaracion en el mismo ambito, variable no declarada/no inicializada, tipo incompatible y division por cero.
