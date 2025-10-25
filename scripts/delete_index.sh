#!/bin/sh
# remove_index.sh
# Elimina ./data/dataset/index de forma segura.
# Uso:
#   ./remove_index.sh           # pide confirmación antes de borrar ./data/dataset/index
#   ./remove_index.sh -f        # fuerza borrado sin preguntar
#   ./remove_index.sh -n        # dry-run (simula)
#   ./remove_index.sh /ruta/a/otro/index  # elimina la ruta indicada (con las mismas comprobaciones)

# Defaults
FORCE=0
DRYRUN=0

# parse flags (solo -f y -n soportados). El primer argumento no-flag es la ruta.
while [ $# -gt 0 ]; do
  case "$1" in
    -f) FORCE=1; shift ;;
    -n) DRYRUN=1; shift ;;
    --) shift; break ;;
    -*) echo "Opción desconocida: $1"; exit 2 ;;
    *) break ;;
  esac
done

TARGET="${1:-./data/index}"

# resolver ruta absoluta de TARGET sin usar readlink -f (más portable)
TARGET_DIR=$(dirname -- "$TARGET" 2>/dev/null) || { echo "Error al obtener dirname"; exit 1; }
TARGET_BASE=$(basename -- "$TARGET" 2>/dev/null) || { echo "Error al obtener basename"; exit 1; }

if cd -- "$TARGET_DIR" 2>/dev/null; then
  ABS_TARGET="$(pwd -P)/$TARGET_BASE"
  # volver al directorio original
  cd - >/dev/null 2>&1 || true
else
  # no existe el directorio padre: igual resolvemos ruta relativa
  CUR="$(pwd -P)"
  ABS_TARGET="$CUR/$TARGET"
fi

# comprobaciones básicas de seguridad
if [ -z "$ABS_TARGET" ]; then
  echo "Ruta vacía — abortando."
  exit 1
fi

# evitar borrar la raíz o el cwd por accidente
if [ "$ABS_TARGET" = "/" ] || [ "$ABS_TARGET" = "" ] || [ "$ABS_TARGET" = "$(pwd -P)" ]; then
  echo "Ruta peligrosa detectada ($ABS_TARGET). Abortando."
  exit 1
fi

# comprobar si existe y es directorio
if [ ! -e "$ABS_TARGET" ]; then
  echo "No existe: $ABS_TARGET"
  exit 0
fi

if [ ! -d "$ABS_TARGET" ]; then
  echo "La ruta existe pero no es un directorio: $ABS_TARGET"
  exit 1
fi

# comprobación adicional: asegurar que el directorio sea .../data/dataset/index
PARENT1=$(basename -- "$(dirname -- "$ABS_TARGET")")
PARENT2=$(basename -- "$(dirname -- "$(dirname -- "$ABS_TARGET")")")

if [ "$PARENT1" != "dataset" ] || [ "$PARENT2" != "data" ]; then
  echo "ADVERTENCIA: el directorio objetivo no parece ser ./data/dataset/index"
  echo "Directorio objetivo: $ABS_TARGET"
  echo "Padre: $PARENT1  / Padre del padre: $PARENT2"
  echo "Para proceder debe usar -f para forzar (si realmente sabe lo que hace), o lanzar el script con la ruta exacta deseada."
  if [ "$FORCE" -ne 1 ]; then
    exit 2
  fi
fi

echo "Objetivo a eliminar: $ABS_TARGET"

if [ "$DRYRUN" -eq 1 ]; then
  echo "[DRY-RUN] No se realizará ninguna eliminación. Comando que se ejecutaría:"
  echo "rm -rf -- \"$ABS_TARGET\""
  exit 0
fi

if [ "$FORCE" -ne 1 ]; then
  printf "Confirmar eliminación recursiva de '%s' ? [y/N]: " "$ABS_TARGET"
  read -r answer
  case "$answer" in
    y|Y|sí|Si|SI) ;;
    *) echo "Cancelado por usuario."; exit 0 ;;
  esac
fi

# comando de borrado
rm -rf -- "$ABS_TARGET"
RET=$?

if [ $RET -eq 0 ]; then
  echo "Eliminado correctamente: $ABS_TARGET"
  exit 0
else
  echo "Error al eliminar $ABS_TARGET (código $RET)"
  exit $RET
fi
