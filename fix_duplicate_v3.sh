#!/bin/bash
# Script simple para limpiar código duplicado entre bloques marcados

FILE="/home/ruben/Documentos/TEST/src/main.prg"

# Encontrar líneas de los marcadores
DECL_END=$(grep -n "// \[\[ED_DECLARATIONS_END\]\]" "$FILE" | head -1 | cut -d: -f1)
PROC_START=$(grep -n "// \[\[ED_PROCESSES_START\]\]" "$FILE" | head -1 | cut -d: -f1)

if [ -z "$DECL_END" ] || [ -z "$PROC_START" ]; then
    echo "No se encontraron los marcadores"
    exit 1
fi

echo "[[ED_DECLARATIONS_END]] en línea: $DECL_END"
echo "[[ED_PROCESSES_START]] en línea: $PROC_START"

# Calcular líneas a eliminar (todo lo que está entre los dos marcadores)
START_DELETE=$((DECL_END + 1))
END_DELETE=$((PROC_START - 1))

if [ $START_DELETE -lt $END_DELETE ]; then
    echo "Eliminando líneas $START_DELETE a $END_DELETE"
    sed -i "${START_DELETE},${END_DELETE}d" "$FILE"
    # Agregar líneas en blanco
    sed -i "${START_DELETE}i\\\n" "$FILE"
    echo "Código duplicado eliminado de $FILE"
else
    echo "No hay código duplicado para eliminar"
fi
