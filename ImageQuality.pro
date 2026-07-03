QT += core gui widgets network

CONFIG += c++11
CONFIG -= no_exceptions

RC_ICONS = app.ico

SOURCES += main.cpp \
    ImageSharpness.cpp \
    mainwindow.cpp \
    ImageLoader.cpp

HEADERS += mainwindow.h \
    ImageLoader.h \
    ImageSharpness.h

FORMS += mainwindow.ui

win32 {
    INCLUDEPATH += D:/zl/OpenCV454/include
    LIBS += -LD:/zl/OpenCV454 \
        -llibopencv_world454
}

android {
    OPENCV_ANDROID_SDK = D:/zl/opencv-4.5.4-android-sdk/OpenCV-android-sdk/sdk/native
    INCLUDEPATH += $$OPENCV_ANDROID_SDK/jni/include
    LIBS += -L$$OPENCV_ANDROID_SDK/libs/arm64-v8a \
        -lopencv_java4 \
        -lz

    ANDROID_EXTRA_LIBS += \
        $$OPENCV_ANDROID_SDK/libs/arm64-v8a/libopencv_java4.so

    ANDROID_GRADLE_EXTRA_ARGS += "packagingOptions { pickFirst 'lib/arm64-v8a/libopencv_java4.so' }"

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
    ANDROID_VERSION_NAME = "1.0"
    ANDROID_VERSION_CODE = 1
    ANDROID_TARGET_SDK_VERSION = 33

    ANDROID_PERMISSIONS += \
        android.permission.READ_MEDIA_IMAGES \
        android.permission.READ_EXTERNAL_STORAGE \
        android.permission.WRITE_EXTERNAL_STORAGE

    DEFINES += QT_AUTO_SCREEN_SCALE_FACTOR=1
}

DEFINES += QT_DEPRECATED_WARNINGS

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml \

RESOURCES += \
    resources.qrc
