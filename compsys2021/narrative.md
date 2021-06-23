# CompSys 2021 Narrative

## 1. What is Computational Storage

- Its only recently that definitions for computational storage have truly been
  defined.
- Computational Storage, in essence, involves bringing computations closer to
  the storage layer. Might be explicit such as with Programmable Storage, can be
  transparent as with real time data compression.
- These types of Computational Storage Services can be separated into Fixed and
  Programmable types. Such as... database computers from the 1970
- This presentation will focus on Programmable Computational
  Storage Services.
  
## 2. When Computational Storage

- Researching or applying computational storage does not make sense in all
  situations.
- Key considerations are:
  - lack of sufficient bandwidth on interconnects or
    networks.
  - Processing or data movement bottlenecks such as those by marginal
    generational bandwidth improvements in DRAM.
  - Presence of data reduction operations in intended applications.
- PCI-SIG expects to ratify PCIe gen6 by the end of 2021 with 256 GB/s transfer
  rates. 
- So far PCIe data rates have doubled every 3 years, however, it is unclear
  how long this trend will last.
- Generational improvements in these standards come at a cost such as reduced
  trace / cable length.
- HDMI 1.4 supported passive cables of up to 25 meters
  while 2.1 is limited to 5 meters. This is a reduction of 5x the total length.
- Similarly, tighter requirements imposed by DDR4 and PCIe gen 4 have driven up
  cost of average consumer motherboards from around 75-150 to 200-250 euro.
- Are we nearing the limitations of copper differential signaling?

## 2. Support Technologies

- Recently the development and experimentation using CSD has become easier
  thanks to BPF and ZNS.
- With Zoned Namespaces the host controls data placement.
- By exposing the internal flash structure to the host.
- Majority of the Flash Translation Layer now implemented on host.
  (host-managed)
- Number of advantages, assert safety and security properties
  before submitting user program to device.
- Traditionally combining general programmability with convential
  SSD FTL resulted in very convaluted FTL implementations.
- Primary reasons, Computational Flash Storage has not taken off.
- BPF is often used to perform event based tasks within Linux but
  effectively is an Instruction Set Architecture that can be easily
  implemented by basic VMs, such as a stack machine.
- The ISA also defines calls allowing for an ABI detached
  from the actual vendor implementation.

## 3. ZCSD

- We present ZCSD, an emulated programmable...
- Emulated because it runs inside QEMU
- Not necessary but done because of the general availability of ZNS SSDs
  currently.
- With ZCSD a user writes and compiles a program in BPF adhering to our
  ABI. Subsequently this program is submitted to ZCSD using hypothetical
  extensions to the NVMe command set. After which the program starts
  executing performing any I/O operations through SPDK. The program
  declares the data to return which is fetched by the user afterwards.
- The actual execution of BPF code is performed by an uBPF VM.
- Both of these tools are readily available and open source.
- The ABI and command extension are intentionally simple so that this
  work can be extended to fit many future research questions as opposed
  to already steering in a strong direction.
  
## 4. An example

- Here we demonstrate a basic filter example written in C. This file
  is compiled into BPF by Clang and submitted to ZCSD.
- All current calls available from the ABI are shown on the left hand side.
- The basic filter examples iterates over all integers in the first zone
  of the device reading a single 4k sector at a time. Subsequently, it
  counts all occurrences of integers above a predefined threshold and
  reports this count to the host at the end.
- This type of application is an excellent example of a simple
  operation that can be trivially parallelized and only returns a
  tiny amount of data in comparison to the total data read.
- From the performance graph we can see that the overhead
  incurred by BPF execution is minimal in the case of Just in Time
  compilation.
- It should be noted that using JIT does do away with any memory bounds
  checking that can be performed upon each memory access in uBPF

## 5. Whats next

- ZCSD functions as a basic prototype so anyone can explore further
  research questions.
- These could include investigation the required additions to NVMe command set.
- Handling abstractions such as those for databases and file systems.
- More importantly, is designing a robust BPF ABI that handles all these
  different challenges.
  - Safety properties such as required for concurrent read / write operations,
    validating memory accesses and statically verifying user programs.
  - Security properties such as ACL.
  - Designing the ABI in such a way that it can be easily be implemented by
    vendors in a high performance low energy manner. By designing the ABI around
    FPGAs and DSPs.
- Certain applications could auto generate BPF programs on the fly, either by
  in place modification of variables or by complete recompilation. BPF supports
  changes variables of already compiled programs through BPF relocations
  although this currently is not supported by uBPF.
- Finally, ease of use such as logging, debugging, profiling and error
  reporting.
  - How do you single step a program executing on a flash storage device that
    depending on the ABI call might actually trigger operations in a DSP?