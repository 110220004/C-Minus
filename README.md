# C-- (C Minus Minus)


## Requisitos cubiertos
- Tipos de datos (`int`, `float`, `string`, `bool`)
- Identificadores de variables
- Impresion en pantalla (`print(...)`)
- Comentarios (`//` y `/* ... */`)

## Estructura del proyecto
- `src/main.c`: lexer + parser + ejecucion de C--
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

## Sintaxis base de C--
```cmm
int edad = 20;
float promedio = 90.5;
string nombre = "Wendell";
bool activo = true;

print("Nombre: " + nombre);
print(edad);
```

## Notas
- Cada sentencia termina con `;`.
- Se validan errores semanticos basicos (redeclaracion, variable no declarada, tipo incompatible, etc.).
- Esta base esta pensada para crecer en futuras entregas (if, while, funciones, comparaciones, etc.).

## Entrega en GitHub
Para subir el proyecto a tu repositorio:
```bash
git init
git add .
git commit -m "Evaluacion 2: base de lenguaje C--"
git branch -M main
git remote add origin <URL_DE_TU_REPO>
git push -u origin main
```
