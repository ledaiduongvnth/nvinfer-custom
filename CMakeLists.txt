cmake_minimum_required(VERSION 3.15)
project(debugInfer)
enable_language(CXX)
include(FindCUDA)

set(CMAKE_CXX_STANDARD 14)
find_package(CUDA REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(
        debugInfer
        gstnvinfer.h
        gstnvinfer.cpp
        gstnvinfer_allocator.cpp
        gstnvinfer_allocator.h
        gstnvinfer_impl.cpp
        gstnvinfer_impl.h
        gstnvinfer_meta_utils.cpp
        gstnvinfer_meta_utils.h
        gstnvinfer_property_parser.cpp
        gstnvinfer_property_parser.h
)

target_link_libraries(
        debugInfer
        ${CUDA_LIBRARIES}
        nvinfer
        nvinfer_plugin
        ${OpenCV_LIBS}
        -L/usr/local/cuda-10.2/lib64/ -lcudart
        -L/opt/nvidia/deepstream/deepstream-5.0/lib -lnvdsgst_meta -lnvds_meta -lnvds_inferutils -lnvds_utils -lm -lnvds_infercustomparser
        -lgstreamer-1.0
        -lgobject-2.0
        -lglib-2.0
        -L/mnt/hdd/CLionProjects/debugInfer/libs/nvdsinfer
)
include_directories(
        ${CUDA_INCLUDE_DIRS}
        /opt/nvidia/deepstream/deepstream-5.0/sources/includes
        /usr/include/glib-2.0
        /usr/include/gstreamer-1.0
        /mnt/hdd/CLionProjects/debugInfer/includes
        /mnt/hdd/CLionProjects/debugInfer/libs/nvdsinfer
)
