#!/usr/bin/env bash
source $(dirname $0)/opcode.sh
killall $opcode

# Check arguments
if [ "$#" -ne 1 ]; then
  echo "Illegal args. Usage: client.sh <size in KB>"
	exit
fi

size=`expr 1024 \* $1`

# We need tx-depth=1 to have only one WRITE in flight
$opcode --gid-index=0 --port=3185 --ib-dev=mlx5_0 --ib-port=1 \
  --tx-depth=1 --report_gbits --run_infinitely --duration=1 \
  --size=$size fawn-pluto0
