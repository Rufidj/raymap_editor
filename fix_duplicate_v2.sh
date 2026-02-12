#!/bin/bash
# Script para limpiar código duplicado en main.prg (versión mejorada)

FILE="/home/ruben/Documentos/TEST/src/main.prg"

# Encontrar la línea donde termina process main()
MAIN_END=$(grep -n "^process main()" "$FILE" | head -1 | cut -d: -f1)

if [ -z "$MAIN_END" ]; then
    echo "No se encontró process main()"
    exit 1
fi

# Buscar el 'end' que cierra main() (aproximadamente)
# Asumimos que main() termina antes de la línea 250
MAIN_END_LINE=$(awk "NR>$MAIN_END && NR<250 && /^end$/ {print NR; exit}" "$FILE")

if [ -z "$MAIN_END_LINE" ]; then
    echo "No se encontró el end de main()"
    exit 1
fi

# Encontrar la línea donde empieza [[ED_PROCESSES_START]]
PROCESSES_START=$(grep -n "// \[\[ED_PROCESSES_START\]\]" "$FILE" | head -1 | cut -d: -f1)

if [ -z "$PROCESSES_START" ]; then
    echo "No se encontró [[ED_PROCESSES_START]]"
    exit 1
fi

echo "main() termina en línea: $MAIN_END_LINE"
echo "[[ED_PROCESSES_START]] en línea: $PROCESSES_START"

# Calcular líneas a eliminar
START_DELETE=$((MAIN_END_LINE + 1))
END_DELETE=$((PROCESSES_START - 1))

if [ $START_DELETE -lt $END_DELETE ]; then
    echo "Eliminando líneas $START_DELETE a $END_DELETE"
    sed -i "${START_DELETE},${END_DELETE}d" "$FILE"
    # Agregar líneas en blanco
    sed -i "${START_DELETE}i\\\n\\\n" "$FILE"
    echo "Código duplicado eliminado de $FILE"
else
    echo "No hay código duplicado para eliminar"
fi
