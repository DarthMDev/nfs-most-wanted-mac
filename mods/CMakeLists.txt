# The plugins this game ships: its bundled core mods.
add_custom_target(core_mods
  COMMAND ${Python3_EXECUTABLE} ${POP_ROOT}/tools/recomp/build_core.py
          --source ${CMAKE_CURRENT_SOURCE_DIR}/core
          --dest ${POP_OUT}/mods/core --cc ${CMAKE_C_COMPILER}
  WORKING_DIRECTORY ${POP_ROOT}
  COMMENT "Installing core mods into build/recomp/mods/core"
  VERBATIM)

add_custom_target(plugins DEPENDS core_mods)

# Read-only retail vehicle admission observer: synthetic branch/ABI checks.
if(NOT IOS AND NOT ANDROID)
  add_executable(vehicle_admission_tests ${RECOMP_GAME_DIR}/native/tests/vehicle_admission_tests.cpp)
  target_include_directories(vehicle_admission_tests PRIVATE ${POP_ROOT}/mods)
  target_compile_options(vehicle_admission_tests PRIVATE ${POP_WARN_HOST})
  pop_optimize(vehicle_admission_tests 2)
  pop_test_binary(vehicle_admission_tests)
  add_test(NAME vehicle_admission_tests COMMAND vehicle_admission_tests)
  set_tests_properties(vehicle_admission_tests PROPERTIES LABELS nogame)
endif()

# Frame-arena cadence/high-water diagnostics are opt-in and read-only.
if(NOT IOS AND NOT ANDROID)
  add_executable(frame_arena_tests ${RECOMP_GAME_DIR}/native/tests/frame_arena_tests.cpp)
  target_compile_options(frame_arena_tests PRIVATE ${POP_WARN_HOST})
  pop_optimize(frame_arena_tests 2)
  pop_test_binary(frame_arena_tests)
  add_test(NAME frame_arena_tests COMMAND frame_arena_tests)
  set_tests_properties(frame_arena_tests PROPERTIES LABELS nogame)
endif()

if(NOT IOS AND NOT ANDROID)
  # New-career cash ledger and the helicopter takedown fix (pure).
  add_executable(career_fixes_tests ${RECOMP_GAME_DIR}/native/tests/career_fixes_tests.cpp)
  target_compile_options(career_fixes_tests PRIVATE ${POP_WARN_HOST})
  pop_optimize(career_fixes_tests 1)
  pop_test_binary(career_fixes_tests)
  add_test(NAME career_fixes_tests COMMAND career_fixes_tests)
  set_tests_properties(career_fixes_tests PROPERTIES LABELS nogame)
  add_executable(xenon_particle_tests ${RECOMP_GAME_DIR}/native/tests/xenon_particle_tests.cpp)
  target_compile_options(xenon_particle_tests PRIVATE ${POP_WARN_HOST})
  pop_optimize(xenon_particle_tests 1)
  pop_test_binary(xenon_particle_tests)
  add_test(NAME xenon_particle_tests COMMAND xenon_particle_tests)
  set_tests_properties(xenon_particle_tests PROPERTIES LABELS nogame)
endif()
