cmake_minimum_required(VERSION 3.10)

project(ProbeMaestro.QtInspector)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick Widgets REQUIRED)

add_library(QtInspector SHARED
    qml.qrc
    main.cpp
    Exporters/IExporter.h
    Exporters/IExporter.cpp
    Exporters/QObjectToCppExporter.h
    Exporters/QObjectToCppExporter_p.h
    Exporters/QObjectToCppExporter.cpp
    ExecutableLoader/ExecutableLoader.h
    ExecutableLoader/ExecutableLoader_p.h
    ExecutableLoader/ExecutableLoader.cpp
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
    QtInspector.cpp
    QtInspector.h
    compat_Qt.h
    ObjectTreeModel.h
    ObjectTreeModel.cpp
    ShortcutManager.h
    ShortcutManager.cpp
)

target_link_libraries(QtInspector
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Quick
        Qt${QT_VERSION_MAJOR}::Widgets

        Kernel32
        Version
)
