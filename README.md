# RISCV_Simulator-CMPE_110_Project
Reuploaded for portfolio purposes. This was the final version of this project, which was the result of a quarter-long collaboration between myself and two other teammates (listed below). Some starter code WAS provided by the professor.

----------------------------------------------------------------------------------------------------------------------------------------------------------------------

# Programming Assignment 4: Handling Virtual Memory
Maxwell Bruce, Samuel Woolledge, Jiapei Kuang

## Program Purpose
The purpose of this program is to simulate a five stage pipeline RISC-V processor via C, with full forwarding, branch prediction, caching, and virtual memory translation. The user must supply their own RISC-V assembly code, which can be converted into a readable format via  commands supplied by the RISC-V toolchain, and can then be loaded into and ran by the simulator through its own commands.

## Files
- README.md
- designdoc.pdf
- designdoc_assign1.pdf
- designdoc_assign2.pdf
- designdoc_assign3.pdf
- CMakeLists.txt
- Makefile
- src
  - branch_predictor.c
  - branch_predictor.h
  - cache.c
  - cache.h
  - mem.c
  - mem.h
  - riscv.h
  - riscv_pipeline_registers.h
  - riscv_sim_pipeline_framework.c
  - riscv_sim_pipeline_framework.h
  - riscv_virtualizer.c
  - riscv_virtualizer.h
  - TLB.c
  - TLB.h
  - unit_tests.c
- tests
  - build_test.sh
  - build_tests_dir.sh
  - run_test.sh
  - run_tests_dir.sh
  - asm_tests
    - auipc_test.asm, bin, reg
    - branch_test.asm, bin, reg
    - branches_forwarding_stalls.asm, bin, reg
    - cache_test.asm, bin, reg
    - cache_test2.asm, bin, reg
    - cache_test3.asm, bin, reg
    - cache_test4.asm, bin, reg
    - case4.asm, bin, reg
    - case5.asm, bin, reg
    - case6.asm, bin, reg
    - case7.asm, bin, reg
    - forwarding_add_test.asm, bin, reg
    - load_test.asm, bin, reg
    - loads_branches_stalling_forwarding.asm, bin, reg
    - page_test.asm, bin, reg
    - stall_test.asm, bin, reg

## Generating a Readable Input
Given a RISC-V assembly program "sample.s", we can convert create an output file in ASCII that is loadable by our simulator via the following commands on any machine that has the RISC-V toolchain installed:

riscv-as sample.s -o sample

rsicv-objcopy -Obinary sample.o sample.obj

od -t x1 sample.obj > sample.bin

## Compiling and Running the Simulator
The makefile supplied will be able to compile the program by running the "make" command, assuming that all required files are in the "src" directory. From here, once the executable is called, there are a number of commands that can be used to interact with the simulator.

"load /x offset sample" - Loads the file "sample" into the simulator, assuming
it is a properly generated and readable input, starting from 0+offset.

"dump /x offset num_bytes [sample]" - Prints the contents of the simulator
memory (to sample, if provided) at [offset, offset + num_bytes].

"readreg reg_num" - Prints the value of register "reg_num".

"writereg reg_num value" - Sets the value of register "reg_num" to value.

"setpc pc" - Sets the value of the program counter to "pc".

"getpc" - Prints the program counter.

"run num_steps" - Runs for "num_steps".

"getptbr" - Prints the valuse of Page Table Base Register (ptbr).

"exit" - Exits the simulator.

## Internal Design
In order to process the instructions supplied, the simulator calls five different functions, each of which represent a stage in the RISC-V pipeline - Fetch, Decode, Execute, Memory and Writeback (called in reverse order internally). Each function passes information along to the next stage by storing it into stage registers, which are in turn used by the next stage, and so on until Writeback. Fetch does not have a stage register - instead, it gets its information directly from the instruction at the current program counter. Branch prediction occurs in the Fetch stage, by comparing the address against a simulated BTB. I-TLB will be accessed in Fetch stage and in parallel with I-Cache. Necessary memory reads are performed in the Fetch stage, and necessary register reads are performed in the Decode stage, via the memory_read and register_read functions supplied by Professor Miller respectively. In the Execute stage, ALU operations for the various instructions are performed and branch mispredictions are caught (flushing the Fetch and Decode stages), and the BTB is updated as needed. In the Memory stage, stalls due to loads and dependencies are caught, and memory writes occur via the supplied memory_write function. D-TLB will be accessed and in parallel with D-Cache. Register writes occur in the Writeback stage via the supplied register_write function. Stalls caused by cache misses for reads and writes and TLB misses are caught in Fetch and Memory respectively.
