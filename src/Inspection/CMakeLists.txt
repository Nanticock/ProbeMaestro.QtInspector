# to stop the aplication from running in the qt creator terminal,
# goto Project>Run>Run in terminal
# Refer to: https://sl.bing.net/c1K2xlXP1Hg

project(ProbeMaestro.QtInspector)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick Widgets REQUIRED)

# Add an executable target
add_executable(RunExeAsDll # we can add WIN32 to make it a GUI application
    Exporters/IExporter.h
    Exporters/IExporter.cpp
    Exporters/QObjectToCppExporter.h
    Exporters/QObjectToCppExporter_p.h
    Exporters/QObjectToCppExporter.cpp
    ExecutableLoader/ExecutableLoader.h
    ExecutableLoader/ExecutableLoader_p.h
    ExecutableLoader/ExecutableLoader.cpp
    MainThreadHijacker.h
    MainThreadHijacker.cpp
    MainWindow.h
    MainWindow.cpp
    MainWindow.ui
    QObjectViewer/QObjectViewer.h
    QObjectViewer/QObjectViewer.cpp
    UndoRedoStack/UndoRedoStack.hpp
    QObjectViewer/RTTIHelper/RTTIHelper.h
    QObjectViewer/RTTIHelper/rttiTraversalExample_msvc.cpp
    QObjectViewer/PropertyEditors/IPropertyEditor.h
    QObjectViewer/PropertyEditors/IPropertyEditor_p.h
    QObjectViewer/PropertyEditors/IPropertyEditor.cpp
    QObjectViewer/PropertyEditors/PointerPropertyEditor.h
    QObjectViewer/PropertyEditors/PointerPropertyEditor.cpp
    QObjectViewer/PropertyEditors/VariantPropertyEditor.h
    QObjectViewer/PropertyEditors/VariantPropertyEditor.cpp
    QObjectViewer/PropertyEditors/BoolPropertyEditor.h
    QObjectViewer/PropertyEditors/BoolPropertyEditor.cpp
    QObjectViewer/PropertyEditors/ColorPropertyEditor.h
    QObjectViewer/PropertyEditors/ColorPropertyEditor.cpp
    QObjectViewer/PropertyEditors/VariantListPropertyEditor.h
    QObjectViewer/PropertyEditors/VariantListPropertyEditor.cpp
    QObjectViewer/PropertyEditors/VariantMapPropertyEditor.h
    QObjectViewer/PropertyEditors/VariantMapPropertyEditor.cpp
    QObjectViewer/PropertyEditors/QmlListReferencePropertyEditor.h
    QObjectViewer/PropertyEditors/QmlListReferencePropertyEditor.cpp
    QObjectViewer/PropertyEditors/JSValuePropertyEditor.h
    QObjectViewer/PropertyEditors/JSValuePropertyEditor.cpp
    QObjectViewer/PropertyEditors/VariantPropertyEditor_p.h
    QObjectViewer/PropertyEditors/StringPropertyEditor.h
    QObjectViewer/PropertyEditors/StringPropertyEditor.cpp
    QObjectViewer/PropertyEditors/QQmlListPropertyEditor.h
    QObjectViewer/PropertyEditors/QQmlListPropertyEditor_p.h
    QObjectViewer/PropertyEditors/QQmlListPropertyEditor.cpp
    QObjectViewer/ConnectionInspector/ConnectionInspector.h
    QObjectViewer/ConnectionInspector/ConnectionInspector.cpp
    QObjectViewer/ConnectionInspector/QtCorePrivate/5/qobject_p.h
    WindowChildrenTreeModel/WindowChildrenTreeItem.h
    WindowChildrenTreeModel/WindowChildrenTreeItem.cpp
    WindowChildrenTreeModel/WindowChildrenTreeModel.h
    WindowChildrenTreeModel/WindowChildrenTreeModel.cpp
    MemoryMaps/MemoryMap.h
    MemoryMaps/MemoryMap_p.h
    MemoryMaps/MemoryMap.cpp
    QObjectViewer/PointerChecker/PointerChecker.h
    QObjectViewer/PointerChecker/PointerChecker.cpp
    QObjectViewer/PointerChecker/QObjectPointerChecker.h
    QObjectViewer/PointerChecker/QObjectPointerChecker.cpp
    QObjectViewer/PointerChecker/QMetaObjectPointerChecker.h
    QObjectViewer/PointerChecker/QMetaObjectPointerChecker.cpp
    QObjectViewer/ObjectLocator/ObjectLocator.h
    QObjectViewer/ObjectLocator/ObjectLocator.cpp
    QObjectViewer/ObjectLocator/CacheRepo.h
    QObjectViewer/ObjectLocator/CacheRepo.cpp
    Core/SemVer.h
    Core/CompilerInfo.h
    Compat/MemoryMaps/MemoryMap.h
    Compat/MemoryMaps/MemoryMap.cpp
    Compat/MemoryMaps/ObjectsMemoryMap/QtObjectsMemoryMap.h
    Compat/MemoryMaps/ObjectsMemoryMap/QtObjectsMemoryMap.cpp
    Compat/MemoryMaps/FunctionsMemoryMaps/FunctionsMemoryMap.h
    Compat/MemoryMaps/FunctionsMemoryMaps/FunctionsMemoryMap.cpp
    main.cpp
    qml.qrc
)

target_link_libraries(RunExeAsDll
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Widgets
)

# system libraries
target_link_libraries(RunExeAsDll
    PRIVATE
        Kernel32
        Version
)

