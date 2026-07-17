# media-controller.pro
# Defines the modules and sources for the qmake build system

QT += core websockets
QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = media-controller

SOURCES += \
    main.cpp \
    MediaController.cpp

HEADERS += \
    MediaController.h
