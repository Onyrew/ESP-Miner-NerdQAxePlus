# NerdQAxe++ performance handoff

The firmware changes reduce ESP32 work when receiving Stratum V1 messages, preparing ASIC jobs, and processing results. Their expected benefit is less host-side overhead and faster dispatch of new pool work. The bench comparison below does not establish a sustained hashrate increase.

## Changes in Git

- Stratum V1 parses JSON-RPC lines from its receive buffer, compacts the buffer only when it needs space, and logs wire traffic at debug level. Callers consume each line before the next receive or buffer reset.
- V1 job creation caches a binary coinbase template and updates Extranonce2 in place. Merkle roots stay binary through ASIC job construction. Pool notifications remain pending if they arrive while the job task is busy.
- Local ASIC and CAN result handlers take a single-allocation job snapshot. The local result handler formats the best difficulty only when it logs a result.
- Stratum V2 uses the shared hex decoder and checks the extended share's Extranonce2 length before decoding. The AxeOS System page allows one status request in flight and resumes polling after timeouts or request errors.

## Bench status on 2026-11-24

Two four-chip NerdQAxe++ units with BM1370 ASICs booted `v1.1.3-alpha.1.perf4.pwth4` and submitted accepted Stratum V1 shares. At 600 MHz, 1180 mV and a 500 ms job interval, one unit's reported one-hour hashrate moved from 4,661.2 to 4,684.7 GH/s between `.perf4` and `.perf4.pwth4` (+0.50%). This was a single before-and-after reading. A longer comparison with matched settings and pool-side accepted work would be needed to quantify any gain. Bench coverage was two units on Stratum V1.

The `.pwth4` build used a local BM1370 experiment. It changed the final data byte from `0x0C` to `0x10` in both the broadcast and per-chip writes to core register `0x80` during `BM1370::init`. The archived source is `build/nerdqaxe-plusplus-v1-timing-source/components/bm1397/bm1370.cpp`. The checked-in driver retains `0x0C`.

## Local build and recovery artifacts

These paths are relative to the original checkout. The Git-ignored `build/` directory contains the images, measurement JSON, and device-specific backups.

- `build/plus2-perf4-pwth4/esp-miner.bin` holds the tested OTA app. Its SHA-256 is `0B324F23BC960433CEA9DFA590EA5C405B25D9CD33FFB0853EB13218F602D327`.
- `build/plus2-perf4/esp-miner.bin` holds the earlier `.perf4` OTA app. Its matching web image is `build/plus2-perf4/www.bin`.
- `build/nerdqaxe-device-252/` and `build/nerdqaxe-device-68/` contain the before-update boot-selection backups, partition tables, and smoke-test telemetry. Before restoring a boot-selection backup over USB, verify that the selected slot still contains the expected image on that device.
- `build/nerdqaxe-recovery-v1.1.3-alpha.1/` contains the factory recovery image for NerdQAxe++.
