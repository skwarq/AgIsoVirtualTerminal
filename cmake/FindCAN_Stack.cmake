if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 0d5fd0b53e7d2b42ee54949896072300d5c0359a)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
