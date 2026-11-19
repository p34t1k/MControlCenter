#!/bin/bash
# Exercises scripts/mcenter-battery against a SIMULATED battery (plain files + a small physics loop), no hardware, no root.
# Covers: full cycle, cancel mid-discharge, stalled battery, hard kill + boot-time recovery, soft-unplug bit preservation.
set -u
HERE="$(cd "$(dirname "$0")/.." && pwd)"; SCRIPT="${MCC_SCRIPT:-$HERE/scripts/mcenter-battery}"
fail=0; pass=0
ok() { if eval "$2"; then pass=$((pass + 1)); echo "  ok   $1"; else fail=$((fail + 1)); echo "  FAIL $1   [$2]"; fi; }

# real sysfs reads are atomic; plain "echo > file" is not (a reader can see it empty), which made the script see a phantom unplug on slow CI machines
put() { echo "$2" > "$1.tmp" && mv -f "$1.tmp" "$1"; }

setup() {   # fresh simulated machine: 60 % battery, charger in, limit 50-60, register 0xBF = 0x38 (bits 3-5 set: they must survive)
    T="$(mktemp -d)"; export MCC_PS="$T/ps" MCC_RUN="$T/run" MCC_LIB="$T/lib" MCC_FAKE_REGS="$T/regs" MCC_POLL=0.2 MCC_STALL=4 MCC_REASSERT=1
    mkdir -p "$T/ps/BAT1" "$T/ps/ADP1" "$T/run" "$T/lib" "$T/regs"
    echo Battery > "$T/ps/BAT1/type"; echo Mains > "$T/ps/ADP1/type"; echo 1 > "$T/ps/ADP1/online"
    echo 60 > "$T/ps/BAT1/capacity"; echo "Not charging" > "$T/ps/BAT1/status"; echo 1800000 > "$T/ps/BAT1/charge_now"; echo 3000000 > "$T/ps/BAT1/charge_full"; echo 0 > "$T/ps/BAT1/current_now"
    echo 50 > "$T/ps/BAT1/charge_control_start_threshold"; echo 60 > "$T/ps/BAT1/charge_control_end_threshold"; echo 56 > "$T/regs/191"
    ( cap=60
      while :; do
          if [ -f "$T/freeze" ]; then sleep 0.1; continue; fi
          bf=$(cat "$T/regs/191" 2>/dev/null || echo 56); end=$(cat "$T/ps/BAT1/charge_control_end_threshold" 2>/dev/null || echo 60)
          if [ $((bf & 1)) = 1 ]; then put "$T/ps/ADP1/online" 0; st=Discharging; [ $cap -gt 0 ] && cap=$((cap - 1))
          else put "$T/ps/ADP1/online" 1
              if [ $cap -lt "$end" ]; then st=Charging; cap=$((cap + 1)); else st="Not charging"; [ $cap -ge 100 ] && st=Full; fi
          fi
          put "$T/ps/BAT1/capacity" $cap; put "$T/ps/BAT1/status" "$st"; put "$T/ps/BAT1/charge_now" $((cap * 30000))
          sleep 0.1
      done ) & SIM=$!
}
teardown() { kill "$SIM" 2>/dev/null; wait "$SIM" 2>/dev/null; rm -rf "$T"; }
st_of() { sed -n "s/^$1=//p" "$T/run/calibration" 2>/dev/null; }
wait_cond() { local i; for i in $(seq 1 400); do eval "$1" && return 0; sleep 0.1; done; return 1; }
unplugged() { [ $(( $(cat "$T/regs/191") & 1 )) = 1 ] && [ "$(cat "$T/ps/ADP1/online")" = 0 ]; }
wait_state() { local i; for i in $(seq 1 600); do [ "$(st_of state)" = "$1" ] && return 0; sleep 0.1; done; return 1; }
restored() { [ "$(cat "$T/regs/191")" = 56 ] && [ "$(cat "$T/ps/BAT1/charge_control_end_threshold")" = 60 ] && [ "$(cat "$T/ps/BAT1/charge_control_start_threshold")" = 50 ] && [ ! -f "$T/lib/calibration.orig" ]; }

echo "1) full cycle"; setup
timeout 120 bash "$SCRIPT" run --floor=20 >/dev/null 2>&1; rc=$?
ok "exits 0" "[ $rc = 0 ]"; ok "state is done" '[ "$(st_of state)" = done ]'
ok "ended at or below the floor" '[ "$(st_of endpct)" -le 20 ]'
ok "register restored with bits 3-5 intact (0x38)" 'restored'
teardown

echo "2) cancelled (SIGTERM) in the middle of the discharge"; setup
bash "$SCRIPT" run --floor=20 >/dev/null 2>&1 & P=$!
wait_state discharging; wait_cond unplugged; ok "reached discharging" '[ "$(st_of state)" = discharging ]'
ok "simulated unplug is really on (bit 0 set, AC offline)" '[ $(( $(cat "$T/regs/191") & 1 )) = 1 ] && [ "$(cat "$T/ps/ADP1/online")" = 0 ]'
kill -TERM $P; wait $P 2>/dev/null
ok "register + limits restored after cancel" 'restored'; ok "state is aborted" '[ "$(st_of state)" = aborted ]'
teardown

echo "3) battery stops changing (stall watchdog)"; setup
bash "$SCRIPT" run --floor=20 >/dev/null 2>&1 & P=$!
wait_state discharging; wait_cond unplugged; touch "$T/freeze"; wait $P 2>/dev/null; rc=$?
ok "run ended with an error" "[ $rc != 0 ]"; ok "state is aborted" '[ "$(st_of state)" = aborted ]'
echo "  [diag] state=$(st_of state) msg=$(st_of msg)"
ok "everything restored" 'restored'; ok "reason mentions no change" 'st_of msg | grep -qi "did not change\|no progress"'
teardown

echo "4) hard kill (SIGKILL) then recovery at boot"; setup
bash "$SCRIPT" run --floor=20 >/dev/null 2>&1 & P=$!
wait_state discharging; wait_cond unplugged; kill -KILL $P; wait $P 2>/dev/null
echo "  [diag] state=$(st_of state) reg=$(cat "$T/regs/191") ac=$(cat "$T/ps/ADP1/online") cap=$(cat "$T/ps/BAT1/capacity") save=$([ -f "$T/lib/calibration.orig" ] && echo yes || echo no)"
ok "kill left the simulation on and the saved originals behind" '[ $(( $(cat "$T/regs/191") & 1 )) = 1 ] && [ -f "$T/lib/calibration.orig" ]'
bash "$SCRIPT" boot >/dev/null 2>&1
ok "boot recovery restored everything" 'restored'
teardown

echo "5) the explicit 'restore' command"; setup
echo 57 > "$T/regs/191"; printf 'time=1\nbf=56\nlimits=50 60\n' > "$T/lib/calibration.orig"; echo 100 > "$T/ps/BAT1/charge_control_end_threshold"
bash "$SCRIPT" restore >/dev/null 2>&1
ok "restore puts register and limits back" 'restored'
teardown

echo; echo "passed $pass, failed $fail"; [ "$fail" = 0 ]
