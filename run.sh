#!/bin/bash

# Script para lanzar AGIStudio
# Uso: ./run.sh [opciones]
#   -d, --debug       Lanzar con información de depuración
#   -h, --help        Mostrar esta ayuda

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # Sin color

# Configuración
BUILD_DIR="build"
DEBUG_MODE=false

# Función de ayuda
show_help() {
    echo "Uso: $0 [opciones]"
    echo ""
    echo "Opciones:"
    echo "  -d, --debug       Lanzar con información de depuración"
    echo "  -h, --help        Mostrar esta ayuda"
    echo ""
    echo "Ejemplos:"
    echo "  $0                # Lanzar la aplicación"
    echo "  $0 -d             # Lanzar con modo debug"
}

# Parsear argumentos
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            DEBUG_MODE=true
            shift
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

# Verificar que el proyecto esté compilado
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    APP_PATH="$BUILD_DIR/src/AGIStudio.app"
    EXEC_PATH="$APP_PATH/Contents/MacOS/AGIStudio"

    if [ ! -d "$APP_PATH" ]; then
        echo -e "${RED}Error: La aplicación no está compilada.${NC}"
        echo -e "Ejecuta ${YELLOW}./build.sh${NC} primero para compilar el proyecto."
        exit 1
    fi

    if [ ! -f "$EXEC_PATH" ]; then
        echo -e "${RED}Error: El ejecutable no existe en $EXEC_PATH${NC}"
        echo -e "Ejecuta ${YELLOW}./build.sh --clean${NC} para recompilar el proyecto."
        exit 1
    fi

    echo -e "${GREEN}Lanzando AGIStudio...${NC}"

    if [ "$DEBUG_MODE" = true ]; then
        echo -e "${YELLOW}Modo debug activado${NC}"
        # Ejecutar directamente el binario para ver la salida en la terminal
        "$EXEC_PATH"
    else
        # Usar 'open' para lanzar como aplicación nativa de macOS
        open "$APP_PATH"
    fi
else
    # Linux
    EXEC_PATH="$BUILD_DIR/src/AGIStudio"

    if [ ! -f "$EXEC_PATH" ]; then
        echo -e "${RED}Error: La aplicación no está compilada.${NC}"
        echo -e "Ejecuta ${YELLOW}./build.sh${NC} primero para compilar el proyecto."
        exit 1
    fi

    echo -e "${GREEN}Lanzando AGIStudio...${NC}"

    if [ "$DEBUG_MODE" = true ]; then
        echo -e "${YELLOW}Modo debug activado${NC}"
    fi

    # En Linux, ejecutar directamente
    "$EXEC_PATH"
fi
