cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME Sound_modulation_group_demonstration_project)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# TEAM CODE (integration: Brendon): keep the CubeMX FatFs DMA template out of the build (Sep 2026)
# The CubeMX FatFs "DMA template" (FATFS/Target/sd_diskio.c) stays on disk for
# CubeMX but is NOT compiled: Core/Src/sd_diskio_rtos.c provides SD_Driver
# instead (see the header comment there for why — its completion-flag
# handling loses DMA completions under FreeRTOS and then spins 30 s).
# HEADER_FILE_ONLY drops it from the build without touching the generated
# cmake/stm32cubemx/CMakeLists.txt, so a CubeMX regeneration stays harmless.
set_source_files_properties(${CMAKE_SOURCE_DIR}/FATFS/Target/sd_diskio.c
    TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
    PROPERTIES HEADER_FILE_ONLY TRUE)

# TEAM CODE (integration: Brendon): 1 s SDIO data timeout (Sep 2026)
# SDIO hardware data timeout (DTIMER, counted in SDIO_CK cycles; the HAL's
# stm32f4xx_ll_sdmmc.h only defines it #ifndef). The HAL default 0xFFFFFFFF is
# ~9 minutes at the 8 MHz transfer clock, which is how long a card pulled
# mid-transfer would wedge the polled USB-MSC read inside the OTG_FS interrupt
# (HAL_GetTick does not advance there). 8 000 000 cycles = 1 s at 8 MHz, well
# above the SD spec's 250 ms per-block programming limit. Applied through the
# CubeMX interface target so the HAL library and the application agree.
target_compile_definitions(stm32cubemx INTERFACE SDMMC_DATATIMEOUT=8000000U)

# TEAM CODE (integration: Brendon): two HAL SD files always built at -O2 (Sep 2026)
# USB-MSC moves the card's blocks with the POLLED HAL_SD_ReadBlocks/
# WriteBlocks inside the OTG_FS interrupt (brendon_ports.c), feeding the SDIO
# FIFO word by word with hardware flow control off (F4 errata). On the 4-bit
# bus at 8 MHz a word must move every ~72 CPU cycles; at -O0 the HAL's FIFO
# loop takes ~90-105, so host writes hit TXUNDERR and reads RXOVERR (checked in
# the Debug disassembly, 26 Sep 2026). These two HAL files are therefore always
# built with -O2; the source-level option comes after the build type's -O0 on
# the command line, so it wins. FatFs recording/playback uses DMA and is not
# affected either way.
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sd.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_sdmmc.c
    TARGET_DIRECTORY STM32_Drivers
    PROPERTIES COMPILE_OPTIONS "-O2")

# TEAM CODE (integration: Brendon): Tom's DSP and TomTask always built at -O2 (TOM package, 29-30 Oct 2026, approved by the user)
# FxRenderTask makes the effect files on spare CPU time, and TomTask runs the
# speaker crossover on every 4 ms half. At the Debug build's -O0 the Autotune
# file of a 10 s recording takes about 65 s ("Preparing" for a minute) and the
# crossover alone takes 29 % of the CPU; at -O2 about 11 s and 6-7 %. So these
# eight files are always built with -O2, like the two HAL files above: the
# per-file option comes after the build type's -O0 on the command line, so it
# wins. The rest of the firmware stays at -O0 for the debugger. A Tom file left
# out of this list (for example after a rename) still works, only slower:
# check once that compile_commands.json shows -O2 on each of them.
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/PD5b_Tom/Application/AudioEffects/audio_effects.c
    ${CMAKE_SOURCE_DIR}/PD5b_Tom/Application/AudioEffects/pitch_detector.c
    ${CMAKE_SOURCE_DIR}/PD5b_Tom/Application/AudioEffects/pitch_shifter.c
    ${CMAKE_SOURCE_DIR}/PD5b_Tom/Application/RateAdapter/rate_adapter.c
    ${CMAKE_SOURCE_DIR}/PD5b_Tom/Application/FxRender/fx_render.c
    ${CMAKE_SOURCE_DIR}/PD5b_Tom/Application/Crossover/crossover.c
    ${CMAKE_SOURCE_DIR}/PD5b_Tom/Application/TunePlayer/tune_player.c
    ${CMAKE_SOURCE_DIR}/Core/Src/tom_dac_output.c
    PROPERTIES COMPILE_OPTIONS "-O2")

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # TEAM CODE (integration: team): PDM2PCM library folder (first import, May 2026; author not recorded)
    # PDM2PCM ships as precompiled static libraries (one per filter
    # arithmetic width). Add the folder so the linker can find them.
    ${CMAKE_SOURCE_DIR}/PDM2PCM
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # TEAM CODE (integration: Brendon): firmware source list (the third-party display files are marked below)
    # ------------------------------------------------------------------
    # Merged-build subsystem modules
    # ------------------------------------------------------------------
    Core/Src/app_rtos.c                   # Shared RTOS objects (queues/pool/semaphore)
    Core/Src/brendon_battery_usb.c        # Brendon: BrendonTask glue (PD6)
    Core/Src/brendon_ports.c              # Brendon: HAL implementations of PD5b_Brendon/Interfaces
    Core/Src/jack_audio_sd.c              # Jack: JackAudioTask + JackStorageTask glue (PD6)
    Core/Src/jack_ports.c                 # Jack: FATFS implementation of PD5b_Jack/Interfaces/storage_port.h
    Core/Src/jack_fx.c                    # Jack: effect-file engine glue (chunks to/from FxRenderTask; JACK package, 30 Oct 2026)
    Core/Src/tom_dac_output.c             # Tom: TomTask — DAC double buffer: 32/48 kHz playback, crossover, trim (PD6)
    Core/Src/tom_fx_render.c              # Tom: FxRenderTask — makes the effect files (started by App_RTOS_Init)
    Core/Src/tony_ui.c                    # Tony: TonyTask glue around the PD5b UI state machine (PD6)
    Core/Src/tony_debug.c                 # Tony: hidden TFT debug screen (page actions + small-text renderer)
    Core/Src/tony_ports.c                 # Tony: SPI1 / ADC1 wrappers (IT + flag or blocking fallback)
    Core/Src/sd_detect_override.c         # SD BSP overrides: detect polarity, card bring-up (4-bit bus with 1-bit fallback), synchronous DMA transfers with a yielding wait
    Core/Src/sd_diskio_rtos.c             # FatFs disk I/O (replaces the CubeMX DMA template, see above)

    # THIRD-PARTY CODE: the three files below are the ILI9341 display library (Matej Artnak, MIT) and fonts.c (font tables, compiled but unused; the display uses 5x5_font.h) - see the notice in each file.
    # ------------------------------------------------------------------
    # Subsystem driver code (already dropped into Core/Src by the team)
    # ------------------------------------------------------------------
    Core/Src/fonts.c                      # Tony's font bitmaps
    Core/Src/ILI9341_GFX.c                # Tony's ILI9341 graphics primitives
    Core/Src/ILI9341_STM32_Driver.c       # Tony's ILI9341 low-level driver

    # ------------------------------------------------------------------
    # PD5b_Brendon host-tested application logic (PD6). Tests/ and Tools/
    # are deliberately NOT compiled into the firmware.
    # ------------------------------------------------------------------
    PD5b_Brendon/Application/battery_monitor.c
    PD5b_Brendon/Application/battery_time.c # time to empty / to full (2-minute average of the instant current, shown in steps)
    PD5b_Brendon/Application/bq34z100.c   # the only bq34z100 driver (Core copy deleted)
    PD5b_Brendon/Application/charger_status.c
    PD5b_Brendon/Application/msc_store.c

    # ------------------------------------------------------------------
    # PD5b_Jack host-tested application logic (PD6). Tests/ is NOT built.
    # ------------------------------------------------------------------
    PD5b_Jack/Application/file_index.c
    PD5b_Jack/Application/playback_session.c
    PD5b_Jack/Application/recording_session.c
    PD5b_Jack/Application/storage_status.c
    PD5b_Jack/Application/wav_storage.c
    # TEAM CODE (integration: Brendon): JACK package, effect files and delete (29-30 Oct 2026)
    PD5b_Jack/Application/fx_store.c      # effect-file paths, DELETE's name check, the header tag
    PD5b_Jack/Application/fx_pipe.c       # effect-file job: sample counts (read, sent, back, written)
    PD5b_Jack/Application/fx_queue.c      # effect-file job: which file to make next
    PD5b_Jack/Application/fx_job.c        # effect-file job engine (one step per JackStorageTask pass)
    PD5b_Jack/Application/play_plan.c     # what PLAY does: the effect file, Preparing, or the original

    # ------------------------------------------------------------------
    # PD5b_Tom host-tested DSP (PD6). Tests/, Tools/ and the offline
    # ResearchAutotune prototype (td_psola_autotune.c: whole-buffer,
    # research-only) are NOT compiled into the firmware.
    # ------------------------------------------------------------------
    PD5b_Tom/Application/AudioEffects/audio_effects.c
    PD5b_Tom/Application/AudioEffects/pitch_detector.c
    PD5b_Tom/Application/AudioEffects/pitch_shifter.c
    PD5b_Tom/Application/TunePlayer/tune_player.c   # startup sound + warning tunes (rendered by TomTask)
    # TOM package (effect files and the crossover, 29-30 Oct 2026):
    PD5b_Tom/Application/RateAdapter/rate_adapter.c # 2:3 upsampler, 32 -> 48 kHz (TomTask and FxRenderTask)
    PD5b_Tom/Application/FxRender/fx_render.c       # the old live effect chain, run offline (FxRenderTask)
    PD5b_Tom/Application/Crossover/crossover.c      # 1600 Hz LR4 speaker crossover (TomTask)

    # ------------------------------------------------------------------
    # PD5b_Tony host-tested UI logic (PD6). Tests/ is NOT built.
    # ------------------------------------------------------------------
    PD5b_Tony/Application/debug_menu.c    # hidden debug screen: pages, cursor, BACK-hold detector
    PD5b_Tony/Application/display_logic.c
    PD5b_Tony/Application/encoder_sm.c
    PD5b_Tony/Application/sd_popup.c      # no-SD pop-up: REC / PLAY pre-check, words, 3 s timing
    PD5b_Tony/Application/status_view.c   # power header + SD footer: text, colours, SD chip
    PD5b_Tony/Application/touch_calibration.c
    PD5b_Tony/Application/touch_filter.c  # one press = one tap: release debounce, HELD for hold-repeat
    PD5b_Tony/Application/touch_region.c
    PD5b_Tony/Application/ui_state_machine.c
    # TONY package (written by Brendon 29-30 Oct 2026): the new UI modules
    PD5b_Tony/Application/file_window.c   # 16 file names around the selection, out of every WAV on the card
    PD5b_Tony/Application/delete_confirm.c # the Delete row's "tap again" rule (0.5 to 3 s)
    PD5b_Tony/Application/screen_saver.c  # the frog-tongue screen saver after 30 s without input

    # NOTE: Core/Src/ramdisk.c is intentionally NOT compiled. It was used
    # by Brendon's standalone USB-C demo as the MSC backing store. In the
    # merged build, USB MSC is backed by the SD card via the rewritten
    # USB_DEVICE/App/usbd_storage_if.c — ramdisk.c is dead code now.
    # Leave the file on disk for reference but keep it out of the build.
)

# TEAM CODE (integration: Brendon): build-stamp dependencies for the debug screen (28 Oct 2026)
# Build stamp: App_RTOS_BuildString() (the debug screen's System page) is
# __DATE__ " " __TIME__ of app_rtos.c, so it only moves when that file is
# compiled. Make app_rtos.c depend on every other source and header in the
# folders the team edits: Core/, the PD5b_* modules (their Application and
# Interfaces folders, not the Tests), USB_DEVICE/ and FATFS/. An incremental
# build that changes any of them recompiles it too, so the stamp says when this
# image was built. Nothing else is watched (Drivers/, Middlewares/, the linker
# script): after a change only there, a clean build moves the stamp.
# CONFIGURE_DEPENDS re-checks the lists on every build, so an added or deleted
# file needs no manual re-configure.
file(GLOB_RECURSE APP_BUILD_STAMP_DEPENDS CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/Core/Src/*.c
    ${CMAKE_SOURCE_DIR}/Core/Inc/*.h
    ${CMAKE_SOURCE_DIR}/PD5b_*/Application/*.c
    ${CMAKE_SOURCE_DIR}/PD5b_*/Application/*.h
    ${CMAKE_SOURCE_DIR}/PD5b_*/Interfaces/*.h
    ${CMAKE_SOURCE_DIR}/USB_DEVICE/*.c
    ${CMAKE_SOURCE_DIR}/USB_DEVICE/*.h
    ${CMAKE_SOURCE_DIR}/FATFS/*.c
    ${CMAKE_SOURCE_DIR}/FATFS/*.h
)
list(REMOVE_ITEM APP_BUILD_STAMP_DEPENDS ${CMAKE_SOURCE_DIR}/Core/Src/app_rtos.c)
set_source_files_properties(${CMAKE_SOURCE_DIR}/Core/Src/app_rtos.c
    PROPERTIES OBJECT_DEPENDS "${APP_BUILD_STAMP_DEPENDS}")

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # TEAM CODE (integration: team): PDM2PCM header folder (first import, May 2026; author not recorded)
    # PDM2PCM headers (pdm2pcm.h and pdm2pcm_glo.h). The CubeMX-managed
    # CMakeLists already includes Core/Inc so our subsystem headers
    # there are findable without an extra entry.
    ${CMAKE_SOURCE_DIR}/PDM2PCM
    # TEAM CODE (integration: Brendon): the PD5b module folders (Sep-Oct 2026)
    ${CMAKE_SOURCE_DIR}/PD5b_Brendon/Application
    ${CMAKE_SOURCE_DIR}/PD5b_Brendon/Interfaces
    ${CMAKE_SOURCE_DIR}/PD5b_Jack/Application
    ${CMAKE_SOURCE_DIR}/PD5b_Jack/Interfaces
    ${CMAKE_SOURCE_DIR}/PD5b_Tom/Application/AudioEffects
    ${CMAKE_SOURCE_DIR}/PD5b_Tom/Application/TunePlayer
    # TOM package (effect files and the crossover, 29-30 Oct 2026):
    ${CMAKE_SOURCE_DIR}/PD5b_Tom/Application/RateAdapter
    ${CMAKE_SOURCE_DIR}/PD5b_Tom/Application/FxRender
    ${CMAKE_SOURCE_DIR}/PD5b_Tom/Application/Crossover
    ${CMAKE_SOURCE_DIR}/PD5b_Tony/Application
    ${CMAKE_SOURCE_DIR}/PD5b_Tony/Interfaces
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # TEAM CODE (integration: team): PDM2PCM filter library and libm (first import, May 2026; author not recorded)
    # ------------------------------------------------------------------
    # PDM2PCM filter — precompiled by ST for Cortex-M4.
    #
    # Two variants ship with the X-CUBE-MEMSMIC1 software pack:
    #   libPDMFilter_CM4_GCC_wc16.a   16-bit internal arithmetic (smaller,
    #                                  faster — sufficient for this mic)
    #   libPDMFilter_CM4_GCC_wc32.a   32-bit internal arithmetic (higher
    #                                  precision filtering, ~30 % more cycles)
    #
    # We default to wc16. To swap, change "wc16" to "wc32" on the line
    # below. Both libraries expose the same PDM_Filter_* API, so no code
    # changes are required. The linker resolves "PDMFilter_CM4_GCC_wc16"
    # to libPDMFilter_CM4_GCC_wc16.a in the PDM2PCM/ directory we added
    # to target_link_directories above.
    # ------------------------------------------------------------------
    PDMFilter_CM4_GCC_wc16

    # libm (newlib's math library) — needed because PDM2PCM internally
    # calls powf() and roundf(), and Tom's DAC sine generation uses
    # sinf(). Without -lm the linker reports:
    #   undefined reference to `powf'
    #   undefined reference to `roundf'
    # plus "dangerous relocation / unknown destination type (ARM/Thumb)"
    # noise that's just a consequence of those symbols being unresolved.
    m
)
