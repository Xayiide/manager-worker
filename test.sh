#! /usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "Usage: $(basename "$0") <path/to/file.csv>"
  exit 1
fi
test -f "$1" || (echo "\"$1\": No such file or directory" && exit 1)

# Llama al manager
src/manager/manager "file://$(dirname "$(realpath "$1")")/filelist.csv" 6000 &

# Llama a unos cuantos workers
#for _ in {1..4}; do
#    src/worker/worker "localhost" "6000" &
#done

# Espera a que terminen
time wait
