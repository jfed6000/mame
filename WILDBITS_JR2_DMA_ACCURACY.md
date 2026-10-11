# Wildbits Jr2 DMA Engine Emulation Accuracy Assessment & Improvement Roadmap

This document provides a comprehensive technical assessment of the TinyVicky II Direct Memory Access (DMA) engine emulation in MAME (`wildbits_jr2.cpp`) compared against the FPGA RTL implementation (`fpga-6809-cores-staging`), the Nitrobotics documentation, and [`WILDBITS_JR2_ARCHITECTURE.md`](file:///Users/richardlucente/development/git/mame/WILDBITS_JR2_ARCHITECTURE.md). It incorporates the authoritative firmware baseline **`wildbits_jr2_6809_Longview_rc24`** (built 2026-10-09), detailing hardware data integrity hardening, physical bus architecture, timing nuances, and modular implementation options.

---

## 1. Executive Summary

The MAME `wbjr2` emulator provides a functional, register-level emulation of the TinyVicky II DMA controller ($FEC0–$FED7) that currently supports:
- 1D linear transfers with non-contiguous 24-bit length registers (`{$FECF, $FECC, $FECD}`).
- 2D rectangular block transfers (width, height, source stride, destination stride).
- 1D linear fills and 2D block fills (including 16-bit alternating byte fills).
- Complete DMA logic operations (ALU ops 0–4: COPY, OR, AND, XOR, MASK nibble transparency, plus NOT inversion).
- Rising-edge transfer triggers on bit 7 (`Start_Trf: 0 → 1`), interrupt generation on `INT_DMA0` (Group 0, bit 6), and even/odd byte-lane masking.
- Hardware register readback permutation signature across $FEC4–$FED3 and logic operation probe bit 7 at $FED4.

However, several architectural, spatial, and timing discrepancies exist between the emulator and the Longview_rc24 FPGA RTL:

1. **Synchronous Execution vs. Asynchronous Hardware Stalls**: MAME executes transfers synchronously in a single C++ call within `dma_w()`, meaning software cannot observe in-flight transfer status via $FEC1 bit 7 (`TRF_IP`), and transfers never yield to the CPU across frames.
2. **Missing Vertical Blanking (VBLANK) Gating & Multi-Frame Resumption**: Hardware restricts DMA execution strictly to the vertical blanking window (scanlines 0 through ~42, occupying ~1.4 ms / ~8.4% of each 60 Hz frame). Large transfers in hardware yield across multiple display frames; MAME completes them instantaneously.
3. **Physical Memory Routing & Sectored I/O Discrepancies**:
   - In Longview_rc24, the external physical SRAM is **2,016 KB contiguous** (Blocks `$00–$FB`, physical `0x000000–0x1F7FFF`).
   - Former blocks `$C0–$C6` were **reclaimed as ordinary system SRAM**; internal FPGA dual-port Block RAMs now reside exclusively in Sectored I/O Blocks **`$FC–$FF`** (`$FC` VICKY, `$FD` Font/CLUT, `$FE` Text RAM, `$FF` Color RAM).
   - In hardware, the DMA engine connects directly to the external 2 MB physical SRAM bus and **bypasses the 6809 MMU entirely**. Accesses to Blocks `$40–$7F` (Flash) and `$80–$9F` (Cartridge) always hit the underlying physical SRAM, never Flash or Cartridge, regardless of `FLASHDIS`.
   - In MAME, `dma_read_byte` and `dma_write_byte` evaluate MMU `FLASHDIS` status, erroneously routing DMA into `m_flash` and `m_cart` when `FLASHDIS = 0`, and treating `$C0–$CF` and `$F0–$FF` as dead unmapped holes (`0xFF`).
4. **Double-Speed (16-bit) DMA Copies**: MAME treats bit 6 of $FEC0 as 16-bit fill only. In hardware, bit 6 (`Double_Speed_DMA`) enables double-speed 16-bit word transfers for copies as well, advancing pointers by 2 bytes per clock and asserting both `MSBn` and `LSBn`.
5. **Longview_rc24 Data Integrity Hardening (RC20–RC24)**:
   - `DMA_CanStart` gated with `VDMA_Channel_Owned_i` to eliminate masked writes and dropped entire frames when display fetches overrun Start of Frame (SOF) under `LONGVIEW_MEMFAST`.
   - `VRAM_1D_ST00B` boundary pause-resume wait-state to eliminate single-byte corruptions at VBLANK slice boundaries.
   - `Op_Fill_Done` boundary termination check preventing dropped final bytes in logic-op pattern fills.
   - 16-bit physical SRAM byte-lane alignment (`Rd_Lane`, `Op_S_Lane`, `Wr_Lane`) and `{2{Rd_Byte}}` / `{2{Op_Res8}}` bus replication.
   - `OP_WR0` ALU pipeline settling wait-state before write strobe assertion.
   - 8-clock post-transfer pipeline drain (`DMA_Drain == 7`).
   - Hardware fill truncation limits (~130 KB accumulator limit).

---

## 2. FPGA Hardware RTL vs. Current Emulator Comparison

### A. Memory Bus Architecture, Physical Addressing & Longview Contiguous SRAM
* **Hardware RTL (`TinyVKY_DMA_Controller.v`, `TinyVicky_MemoryManagementBlock.v`, `TyVKy2K2x1_MMU_Register.v`)**:
  - The DMA engine connects directly to the external 2 MB physical SRAM bus via `VGE_VidMem_Data_i` / `VGE_VidMem_Addy_o`.
  - The DMA controller outputs a 24-bit address, but only bits 20:1 connect to the physical 16-bit SRAM chips (two 8-bit chips: even byte on low lane [7:0], odd byte on high lane [15:8]).
  - **Complete MMU Bypass**: DMA does not consult the 6809 MLUTs, DAT banking, or the fixed I/O inhibit terms. It has zero physical wiring to the SST39 Flash chip select (`FLASH_CSn`), the external expansion/cartridge select (`CART_CSn`), or internal FPGA Block RAMs.
  - **Contiguous 2,016 KB SRAM Space**: In Longview_rc24, Blocks `$00–$FB` (physical `0x000000–0x1F7FFF`) represent 2,016 KB of contiguous physical SRAM. Former blocks `$C0–$C6` are ordinary system SRAM.
  - **Sectored I/O Isolation (`$FC–$FF`)**: Internal FPGA dual-port Block RAMs (TinyVicky control registers, CLUTs, fonts, text matrix, and color attribute RAM) reside in Blocks `$FC–$FF` (`0x1F8000–0x1FFFFF`). The DMA controller cannot access these internal BRAMs; DMA transfers targeting `$1F8000–$1FFFFF` hit external SRAM space (which is unpopulated above 2,016 KB).
  - **Flash/Cartridge Window Independence**: In hardware, DMA reads/writes targeting physical blocks `$40–$7F` (`0x080000–0x0FFFFF`) and `$80–$9F` (`0x100000–0x13FFFF`) always access physical SRAM, even when `FLASHDIS = 0` (where the 6809 CPU sees Flash ROM and Cartridges).
* **Current MAME (`wildbits_jr2.cpp` lines 2384–2445)**:
  - `dma_read_byte` and `dma_write_byte` check `m_mmu_io_ctrl & 0x04` (`FLASHDIS`):
    - When `FLASHDIS = 0`, addresses in `$40–$7F` access `m_flash` and `$80–$9F` access `m_cart`.
    - Blocks `$C0–$CF` and `$F0–$FF` return `0xFF` on read and discard writes under an obsolete assumption that they are internal BRAMs or unmapped holes.
  - *Discrepancy*:
    1. DMA accesses to `$40–$9F` in hardware always access physical SRAM, never Flash or Cartridge, regardless of `FLASHDIS`.
    2. Blocks `$C0–$CF` and `$F0–$FB` are ordinary physical SRAM in Longview_rc24 and must be readable and writable by DMA.
    3. Blocks `$FC–$FF` are the actual Sectored I/O blocks that bypass external SRAM.

---

### B. Timing, Execution Model, VBLANK Window & Bus Handshake
* **Hardware RTL (`TinyVKY_DMA_Controller.v`, `TyVKy2K2turbo_MMU_FNX6809.v`)**:
  - The DMA engine operates on the 100 MHz clock domain (`EngineClk100Mhz_i`).
  - **VBLANK Window**: DMA execution is strictly confined to the vertical blanking period:
    ```verilog
    wire DMA_CanStart = VDMA_Transfer_Time_Available_RESYNC[2]
                     && VDMA_Trf_Time_Before2Late_RESYNC[2]
                     && VDMA_Channel_Owned_i;
    ```
    This window opens at scanline 0 and closes at scanline ~42 (~1.4 ms / ~8.4% of the 16.67 ms 60 Hz frame).
  - **Handshake & 6809 Bus Halt**: When triggered, the controller raises `DMA_Request` (`Bus_RDY_o`), signalling the MMU scheduler to assert 6809 HALT. It waits for acknowledgement (`DMA_Grant_Sync[1] && DMA_Stopped_Sync[1]`) before transferring data.
  - **Multi-Frame Transfer Yielding**: If a transfer does not complete before scanline ~42, the engine enters `WAIT_NEXT_SOF`, drains its pipeline for 8 clocks (`DMA_Drain == 7`), and releases `DMA_Request`. The 6809 CPU resumes running during active video display (scanlines 43–524). At the next Start-of-Frame (SOF), DMA re-arbitrates for the bus and resumes.
  - **In-Flight Status Register ($FEC1)**: Bit 7 (`TRF_IP`) is a live hardware level that remains `1` for the entire duration of the transfer (including while parked in `WAIT_NEXT_SOF` across frame intervals). It self-clears to `0` only upon final completion (`END_GEN_INT`).
* **Current MAME (`wildbits_jr2.cpp` lines 2510–2642)**:
  - `dma_execute()` executes synchronously in a C++ `for` loop inside `dma_w()`.
  - It sets `m_dma_status = 0x80`, completes all byte transfers in host CPU time, deducts estimated cycles via `m_maincpu->eat_cycles(cycles)`, immediately sets `m_dma_status = 0x00`, and fires `INT_DMA0`.
  - *Discrepancy*: Software polling $FEC1 bit 7 immediately after triggering DMA will never observe bit 7 set to `1`. Transfers never yield across 60 Hz video frames, violating hardware timing expectations of drivers and benchmarks.

---

### C. Double-Speed (16-Bit Word) DMA Transfers
* **Hardware RTL (`TinyVKY_DMA_Controller.v`, lines 280–328)**:
  - Bit 6 of $FEC0 (`SixteenBit_Enable` / `Double_Speed_DMA`) enables 16-bit word operations for **both copy and fill**:
    ```verilog
    wire Double_Speed_DMA = VDMA_CTRL_Reg[6];
    ```
  - In double-speed mode:
    - Source and destination address pointers increment by `+2` per transfer cycle.
    - Both SRAM byte-select lines (`VDMA_Transaction_LSBn_o` and `MSBn_o`) are asserted active-low simultaneously.
    - End-of-transfer comparisons mask bit 0:
      ```verilog
      wire Transfer_Done = Double_Speed_DMA ?
          ( {Write_Data_Address_Pointer[23:1], 1'b0} == {VDMA_Dst_Addy_Stop[23:1], 1'b0} ) :
          ( Write_Data_Address_Pointer == VDMA_Dst_Addy_Stop );
      ```
* **Current MAME (`wildbits_jr2.cpp` lines 2530, 2590)**:
  - Bit 6 of $FEC0 is evaluated only in fill loops (`is_16bit_fill`) to alternate between `$FEC2` and `$FEC3`.
  - In copy loops, MAME always transfers 1 byte per iteration (`dst + i`, `src + i`).
  - *Discrepancy*: Double-speed block and linear copies are not emulated as 16-bit word operations; addresses increment by 1 instead of 2.

---

### D. Logic Operations (ALU), Byte-Lane Alignment & Pipeline Settling
* **Hardware RTL (`TinyVKY_DMA_Controller.v`, `TinyVicky_MemoryManagementBlock.v`)**:
  - **Clock Cycle Requirements (100 MHz Domain)**:
    - **1D Linear Fill**: 1 clock per byte/word.
    - **1D Linear Copy**: 4 clocks per byte (`VRAM_1D_ST00B` + `VRAM_1D_ST01` + `VRAM_1D_ST02` + `VRAM_1D_ST03`).
    - **Logic Operations**: 8 clocks per byte (`OP_RD_S0`, `OP_RD_S0B`, `OP_RD_S1`, `OP_RD_D0`, `OP_RD_D0B`, `OP_RD_D1`, `OP_WR0`, `OP_WR`). Each byte requires reading source into `Op_S`, reading destination into `Op_D`, allowing ALU to settle, and writing back.
  - **RC20 16-bit SRAM Byte-Lane Alignment (`Rd_Lane`, `Op_S_Lane`, `Wr_Lane`)**:
    - Registered `Rd_Lane <= VDMA_Transaction_LSBn_i` (address bit 0 of the read address).
    - Latches `Op_S <= VGE_VidMem_Data_i` and `Op_S_Lane <= Rd_Lane`.
    - For plain copies, extracts `Rd_Byte = Rd_Lane ? Data[15:8] : Data[7:0]` and replicates across both lanes: `{2{Rd_Byte}}`.
    - For logic ops, extracts 8-bit operands:
      ```verilog
      wire [7:0] Op_S8 = VDMA_Copy_Fill_Strobe_i ? VDMA_Data_2_Fill_i : ( Op_S_Lane ? Op_S[15:8] : Op_S[7:0] );
      wire [7:0] Op_D8 = Wr_Lane ? Op_D[15:8] : Op_D[7:0];
      ```
      Computes `Op_Res8` (COPY, OR, AND, XOR, MASK, NOT), and replicates `{2{Op_Res8}}` to both lanes.
  - **ALU Pipeline Settling (`OP_WR0`)**:
    - Dedicated settling state `OP_WR0` (`6'b10_0011`) ensures `Op_D` holds destination data and the ALU settles before `Write_Strobe` asserts, guaranteeing stable data hold before and after SRAM `WEn`.
* **Current MAME (`wildbits_jr2.cpp`)**:
  - MAME functionally implements all logic operations (ALU ops 0–4 + NOT).
  - It estimates cycle consumption using coarse cycle dividers (`count / 16`, `count / 6`, `count / 5`, `count / 2`).

---

### E. RC20–RC24 Data Integrity Hardening & Multi-Frame Pause-Resume Latency
* **Hardware RTL (`TinyVKY_DMA_Controller.v` commits `8e67004` and `2026-10-06`, Longview_rc24)**:
  1. **Channel Ownership Gating (`DMA_CanStart`)**:
     Under `LONGVIEW_MEMFAST`, if the video display engine's final line fetch overruns past Start of Frame (SOF), the memory block keeps the display channel assigned. In Longview_rc24, `DMA_CanStart` strictly requires `VDMA_Channel_Owned_i`, preventing writes while the display engine owns the bus and eliminating masked writes.
  2. **Boundary Pause-Resume Wait-State (`VRAM_1D_ST00B`)**:
     When a multi-frame transfer yields at scanline ~42 and resumes at the next SOF, resuming directly into `ST01` provided only 1 clock of read margin before latching data, corrupting the first byte of every resumed frame slice. In Longview_rc24, `CPU_STOPPED_ST1` routes through `VRAM_1D_ST00B`, guaranteeing 2 full clocks of read margin on resume.
  3. **Op-Fill Completion Boundary (`Op_Fill_Done`)**:
     Evaluates completion after the write with the pointer incremented:
     ```verilog
     wire Op_Fill_Done = Double_Speed_DMA ? 
         ( {Write_Data_Address_Pointer[23:1], 1'b0} > {VDMA_Dst_Addy_Stop[23:1], 1'b0} ) : 
         ( Write_Data_Address_Pointer > VDMA_Dst_Addy_Stop );
     ```
     Guarantees 1D and 2D pattern fills reach their terminating byte without truncation.
  4. **Post-Transfer Pipeline Drain (`DMA_Drain == 7`)**:
     Holds the CPU bus halt for **8 additional 100 MHz clock cycles** after the final byte transfer so internal output pipelines drain completely before the CPU resumes.
* **Current MAME (`wildbits_jr2.cpp`)**:
  - Because transfers run synchronously, MAME does not model channel ownership arbitration, multi-frame boundary resumption states, or pipeline drain latency.

---

### F. Hardware Register Readback Permutation Signature ($FEC0–$FED7)
* **Hardware RTL (`TinyVKY_DMA_Reg_Block.v`)**:
  - While writes to `$FEC0–$FED7` land directly into `VDMA_REG[0..23]`, hardware reads are permuted through the address multiplexer:
    - `$FEC0`: `DMA_CTRL` (`[7:Start (reads 0), 6:DoubleSpeed, 5:MaskMSB, 4:MaskLSB, 3:IntEn, 2:Fill, 1:2D, 0:Enable]`).
    - `$FEC1`: `DMA_STATUS` (returns `{1'b1, VDMA_REG[20][6:0]}` / Bit 7 `TRF_IP`, bit 6:0 hardwired 0).
    - `$FEC2–$FEC3`: `DMA_FILL_WORD_H/L`.
    - `$FEC4–$FEC7`: Returns quads `$C7, $C6, $C5, $C4` (source address reversed).
    - `$FEC8–$FECB`: Returns quads `$CB, $CA, $C9, $C8` (destination address reversed).
    - `$FECC–$FECF`: Returns quads `$CD, $CC, $CF, $CE` (size pairs swapped).
    - `$FED0–$FED3`: Returns quads `$D1, $D0, $D3, $D2` (stride pairs swapped).
    - `$FED4`: `DMA_OP_REG` (returns `{1'b1, 3'b0, op_not, op[2:0]}`; bit 7 reads 1 on rc18/rc20+).
    - `$FED5–$FED7`: Returns registers `$D5, $D6, $D7`.
    - `$FED8–$FEDF`: Unmapped; reads return constant `$FF`.
* **Current MAME (`wildbits_jr2.cpp` lines 2644–2675)**:
  - MAME accurately models this exact readback permutation signature in `dma_r()`!

---

### G. Hardware Counter Boundaries & 130 KB Truncation Limits
* **Hardware RTL & Nitrobotics Architecture Notes**:
  - Due to internal 17-bit accumulator/counter bounds in the address arithmetic units, linear fills exceeding ~130 KB stop early while still asserting completion status. Software must chunk large fills under 100 KB.
* **Current MAME (`wildbits_jr2.cpp`)**:
  - Unbounded loop iterating up to 16 MB (`count <= 0xFFFFFF`).

---

## 3. Options for Improving DMA Emulation Accuracy

### Option 1: Physical SRAM Bus Isolation & Longview Contiguous Addressing (Low Effort, High Architectural Fidelity)
* **Objective**: Remove unphysical MMU/Flash/Cartridge routing from `dma_read_byte` and `dma_write_byte`, and support the full 2,016 KB contiguous SRAM space.
* **Implementation Details**:
  - In `dma_read_byte(uint32_t phys_addr)`:
    - Mask address to 2 MB physical space: `phys_addr &= 0x1FFFFF`.
    - If `phys_addr < 0x1F8000` (Blocks `$00–$FB`, 2,016 KB contiguous SRAM): return `m_ram[phys_addr]`.
    - If `phys_addr >= 0x1F8000` (Blocks `$FC–$FF`, Sectored I/O / internal BRAMs): return `0xFF` (unmapped from external SRAM DMA perspective).
  - In `dma_write_byte(uint32_t phys_addr, uint8_t data)`:
    - Mask address: `phys_addr &= 0x1FFFFF`.
    - If `phys_addr < 0x1F8000`: `m_ram[phys_addr] = data`.
    - If `phys_addr >= 0x1F8000`: ignore write.
  - Remove all checks against `m_mmu_io_ctrl & 0x04`, `m_flash`, and `m_cart`.
* **Pros**: Eliminates unphysical Flash/Cartridge corruption during DMA; correctly models Longview_rc24's 2,016 KB contiguous SRAM; strictly mirrors external SRAM bus pinouts.

---

### Option 2: Full Double-Speed 16-Bit Word Support for Copies and Fills (Low Effort, Spec Compliance)
* **Objective**: Fully implement hardware double-speed DMA operations for both copy and fill paths.
* **Implementation Details**:
  - In `dma_execute()`, evaluate `bool double_speed = (m_dma_reg[0] & 0x40) != 0`.
  - When `double_speed` is active:
    - Step loop counters by 2: `i += 2` (1D) or `x += 2` (2D).
    - Align transfers to 16-bit words: read 16 bits from `{s_addr & ~1, (s_addr & ~1) + 1}`, apply op (or 16-bit fill word), and write 16 bits to `{d_addr & ~1, (d_addr & ~1) + 1}` subject to lane masks.
    - End condition masks bit 0 matching RTL `{addr[23:1], 1'b0}`.
* **Pros**: Enables high-throughput 16-bit graphics blits; matches hardware double-speed specification.

---

### Option 3: Timer/VBLANK-Gated Asynchronous DMA Engine & Multi-Frame Resumption (Medium Effort, High Timing Fidelity)
* **Objective**: Eliminate instantaneous synchronous DMA; emulate VBLANK window gating, multi-frame transfer yielding, and asynchronous status polling.
* **Implementation Details**:
  - Add DMA state machine variables to `wildbits_jr2_state`:
    - `bool m_dma_active`: Transfer currently queued or in-flight.
    - `uint32_t m_dma_src_cur, m_dma_dst_cur`: Active 24-bit physical address pointers.
    - `uint32_t m_dma_rem_bytes`: Remaining transfer byte count.
    - `uint16_t m_dma_cur_x, m_dma_cur_y`: Active 2D coordinates.
    - `bool m_dma_resumed_slice`: Tracks resumption at frame boundaries to model `VRAM_1D_ST00B` 2-clock latency.
  - In `dma_w(0, data)`:
    - On rising edge of bit 7 (`Start_Trf`), latch parameters, initialize working pointers, set `m_dma_status = 0x80`, and mark `m_dma_active = true`. Do **not** execute the transfer loop immediately.
  - In `vblank_w(1)` or scanline callback:
    - At Start of Frame (scanline 0), verify channel ownership (`VDMA_Channel_Owned_i`).
    - During scanlines 0 through ~42 (vertical blanking window, ~1.4 ms), run DMA transfer slices at 100 MHz engine clock equivalent throughput.
    - At scanline ~42, suspend transfer, drain pipeline for 8 clocks, release CPU bus halt, and enter `WAIT_NEXT_SOF`.
    - When `m_dma_rem_bytes == 0` (or 2D rows complete):
      - Clear `m_dma_status = 0x00`.
      - Clear `m_dma_active = false`.
      - Assert `set_irq(0, 0x40)` if interrupt enable ($FEC0 bit 3) is active.
* **Pros**:
  - Software polling $FEC1 bit 7 observes genuine in-flight hardware status.
  - Large multi-frame blits yield CPU execution during active display lines (scanlines 43–524), eliminating audio stutter and input stalls during background DMA.

---

### Option 4: Cycle-Accurate 100 MHz Bus Arbitration & `LONGVIEW_MEMFAST` Integration (Medium/High Effort)
* **Objective**: Replicate exact 6809 bus handshake, CPU HALT suspension, and 100 MHz clock domain timings.
* **Implementation Details**:
  - On DMA start, assert `m_maincpu->set_input_line(M6809_HALT_LINE, ASSERT_LINE)`.
  - Calculate cycle consumption based on 100 MHz hardware states:
    - 1D linear fill: 1 clock (10 ns) / byte (or word in double-speed).
    - 1D linear copy: 4 clocks (40 ns) / byte (or word in double-speed).
    - Logic op transfer: 8 clocks (80 ns) / byte.
    - First-byte and resume wait-state: 2 clocks (`VRAM_1D_ST00B`).
    - Post-transfer pipeline drain: 8 clocks (80 ns).
  - Scale engine clocks to the active `LONGVIEW_MEMFAST` CPU clock domain (Turbo mode: 20 MHz / 14.28 MHz / 25 MHz; Stock mode: 16.67 MHz / 12.5 MHz / 25 MHz).
* **Pros**: Perfect cycle accounting and true CPU bus suspension matching hardware RTL.

---

### Option 5: Modeling Hardware Address Bounds & Corner-Case Limits (Low Effort)
* **Objective**: Replicate ~130 KB fill truncation behavior and address wrap.
* **Implementation Details**:
  - Mask linear counter increments to 17 bits ($1FFFF), replicating the internal accumulator overflow that occurs on oversized transfers in hardware.
* **Pros**: Accurately flags software bugs that attempt unchunked oversized DMA transfers.

---

## 4. Recommended Implementation Strategy

For a structured enhancement of the MAME `wbjr2` emulator aligned with `wildbits_jr2_6809_Longview_rc24`:

1. **Phase 1 (Immediate / Core Architecture & Memory Isolation)**:
   - Implement **Option 1 (Physical SRAM Isolation)**: Simplify `dma_read_byte` and `dma_write_byte` to access `m_ram[phys_addr & 0x1FFFFF]` for all blocks `$00–$FB` (2,016 KB contiguous SRAM), returning `0xFF` / ignoring writes for Blocks `$FC–$FF`. Eliminate erroneous Flash/Cartridge routing.
   - Implement **Option 2 (Double-Speed 16-Bit DMA Copies & Fills)**: Add 16-bit word-stepping and word address masking to 1D and 2D copies when bit 6 of $FEC0 is active.
2. **Phase 2 (Timing, Asynchrony & VBLANK Gating)**:
   - Implement **Option 3 (VBLANK-Gated Asynchronous Engine)**: Drive DMA bursts during scanlines 0–42 to support true in-flight polling of $FEC1 bit 7, multi-frame transfer yielding, and `VRAM_1D_ST00B` resume margins.
3. **Phase 3 (Hardware Quirks & Validation)**:
   - Add **Option 4** and **Option 5** as required to pass cycle-exact benchmarks (`dmaxfer` ed. 4) and enforce the ~130 KB fill truncation limit.
