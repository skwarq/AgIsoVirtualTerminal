if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/skwarq/AgIsoStack-plus-plus.git
    GIT_TAG be6834c)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
