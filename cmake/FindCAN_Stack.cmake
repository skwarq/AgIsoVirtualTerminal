if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/skwarq/AgIsoStack-plus-plus.git
    GIT_TAG 36c60cd94b35fb01aec5b13f2bd6dbafaada52af)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
