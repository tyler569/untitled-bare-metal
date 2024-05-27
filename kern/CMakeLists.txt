add_executable(untitled_bare_metal
  main.c
  alloc.c
  debug.c
  elf.c
  ipc.c
  syscall.c
  obj/tcb.c
  obj/untyped.c
  ../lib/hexdump.c
  ../lib/print.c
  ../lib/sort.c
  ../lib/spin_lock.c
  ../lib/string.c
)

add_compile_definitions(
  __KERNEL__=1
)

if ("${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "Generic x86_64")
  target_sources(untitled_bare_metal PRIVATE
    ../arch/x86_64/debug.c
    ../arch/x86_64/entry.c
    ../arch/x86_64/isrs.S
    ../arch/x86_64/frame.c
    ../arch/x86_64/gdt.c
    ../arch/x86_64/idt.c
    ../arch/x86_64/initrd.c
    ../arch/x86_64/intrin.c
    ../arch/x86_64/ints.c
    ../arch/x86_64/lapic.c
    ../arch/x86_64/phy_mem.c
    ../arch/x86_64/mmu.c
    ../arch/x86_64/smp.c
    ../arch/x86_64/syscall.c
    ../arch/x86_64/syscall.S
  )

  target_compile_options(untitled_bare_metal PRIVATE -mcmodel=kernel)
  target_link_options(untitled_bare_metal PRIVATE
    -T ${CMAKE_SOURCE_DIR}/arch/x86_64/link.ld
    -z max-page-size=0x1000
  )
  set_target_properties(untitled_bare_metal
    PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/arch/x86_64/link.ld
  )
else ()
  message(FATAL_ERROR "Unsupported target: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
endif ()

define_file_basename_for_sources(untitled_bare_metal)
