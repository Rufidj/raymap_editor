#!/bin/bash
# Script para eliminar código duplicado en main.prg

FILE="/home/ruben/Documentos/TEST/src/main.prg"

# Eliminar líneas 296 a 662 (código duplicado de entidades)
sed -i '296,662d' "$FILE"

echo "Código duplicado eliminado de $FILE"
