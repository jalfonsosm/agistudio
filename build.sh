#!/bin/bash

# Script de compilación para AGIStudio
# Uso: ./build.sh [opciones]
#   -c, --clean       Limpiar build antes de compilar
#   -r, --release     Compilar en modo Release (por defecto: Debug)
#   -j, --jobs N      Número de trabajos paralelos (por defecto: número de cores)
#   -h, --help        Mostrar esta ayuda

set -e  # Salir si hay error

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # Sin color

# Configuración por defecto
BUILD_TYPE="Debug"
CLEAN_BUILD=false
JOBS=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
BUILD_DIR="build"

# Función de ayuda
show_help() {
    echo "Uso: $0 [opciones]"
    echo ""
    echo "Opciones:"
    echo "  -c, --clean       Limpiar directorio build antes de compilar"
    echo "  -r, --release     Compilar en modo Release (por defecto: Debug)"
    echo "  -j, --jobs N      Número de trabajos paralelos (por defecto: $JOBS)"
    echo "  -h, --help        Mostrar esta ayuda"
    echo ""
    echo "Ejemplos:"
    echo "  $0                  # Compilación Debug incremental"
    echo "  $0 -r               # Compilación Release incremental"
    echo "  $0 -c -r            # Compilación Release desde cero"
    echo "  $0 -j 8             # Compilación con 8 trabajos paralelos"
}

# Parsear argumentos
while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}Opción desconocida: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

echo -e "${GREEN}=== Compilando AGIStudio ===${NC}"
echo -e "Tipo de compilación: ${YELLOW}$BUILD_TYPE${NC}"
echo -e "Trabajos paralelos: ${YELLOW}$JOBS${NC}"

# Limpiar si se solicitó
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Limpiando directorio build...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Crear directorio build si no existe
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Creando directorio build...${NC}"
    mkdir -p "$BUILD_DIR"
fi

# Entrar al directorio build
cd "$BUILD_DIR"

# Ejecutar CMake
echo -e "${YELLOW}Configurando proyecto con CMake...${NC}"
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..

# Compilar
echo -e "${YELLOW}Compilando proyecto...${NC}"
make -j"$JOBS"

echo -e "${GREEN}=== Compilación exitosa ===${NC}"

# Detectar sistema operativo y mostrar la ruta correcta del ejecutable
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo -e "Bundle de aplicación: ${YELLOW}$BUILD_DIR/src/AGIStudio.app${NC}"
    echo -e "Ejecutable: ${YELLOW}$BUILD_DIR/src/AGIStudio.app/Contents/MacOS/AGIStudio${NC}"
    echo ""
    echo "Para ejecutar el programa:"
    echo "  open $BUILD_DIR/src/AGIStudio.app"
    echo "O desde la línea de comandos:"
    echo "  $BUILD_DIR/src/AGIStudio.app/Contents/MacOS/AGIStudio"
else
    echo -e "Ejecutable: ${YELLOW}$BUILD_DIR/src/AGIStudio${NC}"
    echo ""
    echo "Para ejecutar el programa:"
    echo "  $BUILD_DIR/src/AGIStudio"
fi
