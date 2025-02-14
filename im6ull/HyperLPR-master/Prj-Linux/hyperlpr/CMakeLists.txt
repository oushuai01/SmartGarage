cmake_minimum_required(VERSION 3.6)
project(SwiftPR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#find_package(OpenCV REQUIRED)

include_directories( ${OpenCV_INCLUDE_DIRS})

include_directories(/usr/local/arm/.4.0/usr/include)
include_directories(/usr/local/arm/.4.0/usr/arm-none-linux-gnueabi/include)
include_directories(/home/gec/Downloads/packages/opencv-3.4.15/build/install/include)
include_directories(include)

set(CMAKE_C_COMPILER /usr/local/arm/5.4.0/usr/bin/arm-linux-gcc)
set(CMAKE_CXX_COMPILER /usr/local/arm/5.4.0/usr/bin/arm-linux-g++)

set(OpenCV_LIBS opencv_core
	        opencv_dnn
	        opencv_features2d
	        opencv_flann
	        opencv_highgui
	        opencv_imgcodecs
	        opencv_imgproc
	        opencv_ml
	        opencv_objdetect
	        opencv_photo
	        opencv_shape
	        opencv_stitching
	        opencv_superres
	        opencv_videoio
	        opencv_video
	        opencv_videostab
		)

LINK_DIRECTORIES(/home/gec/Downloads/packages/opencv-3.4.15/build/install/lib)


set(SRC_DETECTION src/PlateDetection.cpp  src/util.h include/PlateDetection.h)

set(SRC_FINEMAPPING src/FineMapping.cpp )

set(SRC_FASTDESKEW src/FastDeskew.cpp )

set(SRC_SEGMENTATION src/PlateSegmentation.cpp )

set(SRC_RECOGNIZE src/Recognizer.cpp src/CNNRecognizer.cpp)

set(SRC_PIPLINE src/Pipeline.cpp)

set(SRC_SEGMENTATIONFREE src/SegmentationFreeRecognizer.cpp )

# #TEST_DETECTION
# add_executable(TEST_Detection ${SRC_DETECTION} demos/test_detection.cpp)
# target_link_libraries(TEST_Detection ${OpenCV_LIBS})

# #TEST_FINEMAPPING
# add_executable(TEST_FINEMAPPING ${SRC_FINEMAPPING} demos/test_finemapping.cpp)
# target_link_libraries(TEST_FINEMAPPING ${OpenCV_LIBS})

# #TEST_DESKEW

# add_executable(TEST_FASTDESKEW ${SRC_FASTDESKEW} demos/test_fastdeskew.cpp)
# target_link_libraries(TEST_FASTDESKEW ${OpenCV_LIBS})

# #TEST_SEGMENTATION

# add_executable(TEST_SEGMENTATION ${SRC_SEGMENTATION} ${SRC_RECOGNIZE} demos/test_segmentation.cpp)
# target_link_libraries(TEST_SEGMENTATION ${OpenCV_LIBS})

# #TEST_RECOGNIZATION

# add_executable(TEST_RECOGNIZATION  ${SRC_RECOGNIZE} demos/test_recognization.cpp)
# target_link_libraries(TEST_RECOGNIZATION ${OpenCV_LIBS})

# #TEST_SEGMENTATIONFREE
# add_executable(TEST_SEGMENTATIONFREE ${SRC_SEGMENTATIONFREE} demos/test_segmentationFree.cpp)
# target_link_libraries(TEST_SEGMENTATIONFREE ${OpenCV_LIBS})

# #TEST_PIPELINE

# add_executable(TEST_PIPLINE ${SRC_DETECTION} ${SRC_FINEMAPPING} ${SRC_FASTDESKEW} ${SRC_SEGMENTATION} ${SRC_RECOGNIZE} ${SRC_PIPLINE} ${SRC_SEGMENTATIONFREE} demos/test_pipeline.cpp)
# target_link_libraries(TEST_PIPLINE ${OpenCV_LIBS})


add_executable(alpr main.cpp ${SRC_DETECTION} ${SRC_FINEMAPPING}
                             ${SRC_FASTDESKEW} ${SRC_SEGMENTATION}
							 ${SRC_RECOGNIZE} ${SRC_PIPLINE}
							 ${SRC_SEGMENTATIONFREE})
target_link_libraries(alpr ${OpenCV_LIBS})
