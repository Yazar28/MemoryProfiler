set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

qt_add_executable(MemoryProfilerGUI
    main.cpp
    MainWindow.cpp
    MainWindow.h
    MainWindow.ui
)

target_link_libraries(MemoryProfilerGUI PRIVATE Qt6::Widgets memorytracker)
