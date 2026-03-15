# Main component CMakeLists.txt

set(COMPONENT_SRCS
    "hsm_esp32_example.c"
    "../../../core/osa_hsm.c"
    "../../../core/osa_worker.c"
    "../../../platform/esp32/osa_thread.c"
    "../../../platform/esp32/osa_sync.c"
    "../../../platform/esp32/osa_queue.c"
    "../../../platform/esp32/osa_timer.c"
    "../../../platform/esp32/osa_error.c"
)

set(COMPONENT_ADD_INCLUDEDIRS
    "."
    "../../../core"
    "../../../platform"
    "../../../platform/esp32"
)

register_component()
