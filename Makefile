CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2
TARGET = cmm
SRC = src/main.c src/ast.c src/interpreter.c src/scope.c src/symbols.c
TEST_OK = \
	examples/programa_basico.cmm \
	examples/ejemplo_aritmetica.cmm \
	examples/ejemplo_tipos.cmm \
	examples/ejemplo_conversion.cmm \
	examples/ejemplo_shadowing_simple.cmm \
	examples/ejemplo_shadowing_3_niveles.cmm \
	examples/funcion_return_basico.cmm \
	examples/funcion_shadowing.cmm \
	examples/funcion_param_shadowing.cmm
TEST_ERR = \
	examples/error_tipo.cmm \
	examples/error_no_inicializada.cmm \
	examples/error_division_cero.cmm \
	examples/error_redeclaracion_mismo_ambito.cmm

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET) examples/programa_basico.cmm

test: $(TARGET)
	@echo "== Pruebas que deben funcionar =="
	@status=0; \
	for f in $(TEST_OK); do \
		echo "[OK] $$f"; \
		if ./$(TARGET) $$f; then \
			echo "Resultado: PASO"; \
		else \
			echo "Resultado: FALLO (debia ejecutarse sin error)"; \
			status=1; \
		fi; \
		echo; \
	done; \
	echo "== Pruebas que deben fallar =="; \
	for f in $(TEST_ERR); do \
		echo "[ERR] $$f"; \
		if ./$(TARGET) $$f; then \
			echo "Resultado: FALLO (debia reportar error)"; \
			status=1; \
		else \
			echo "Resultado: PASO (error detectado correctamente)"; \
		fi; \
		echo; \
	done; \
	if [ $$status -ne 0 ]; then \
		echo "Resumen: algunas pruebas fallaron."; \
		exit 1; \
	fi; \
	echo "Resumen: todas las pruebas pasaron."

clean:
	rm -f $(TARGET)
