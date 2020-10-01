# FATE-OS
FATE-OS, the "FAke Time Executable Operating System", (a perhaps not so humorous joke on "Real Time") is probably the worst OS you have ever seen. However, it is probably also the smallest kernel you have ever seen.
FATE-OS is intended for education: specifically, for students that have never seen an RTOS before and are being introduced to scheduling and event-driven concepts. Hence, its simplicity and shortcomings (for example, stack manipulation for context-switching is done through a hack, to avoid assembly language as much as possible).
FATE-OS is hardware-specific, namely for the MSP432 Launchpad board. The current implementation supports priority-based periodic tasks (v1.0) and priority-based periodic and aperiodic tasks (v1.1) 
