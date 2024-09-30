# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Espressif/frameworks/esp-idf-v4.4.8/components/bootloader/subproject"
  "C:/Users/Ducne/Documents/GitHub/IOT_Hoshiden/IOT_Hoshiden_V100/build/bootloader"
  "C:/Users/Ducne/Documents/GitHub/IOT_Hoshiden/IOT_Hoshiden_V100/build/bootloader-prefix"
  "C:/Users/Ducne/Documents/GitHub/IOT_Hoshiden/IOT_Hoshiden_V100/build/bootloader-prefix/tmp"
  "C:/Users/Ducne/Documents/GitHub/IOT_Hoshiden/IOT_Hoshiden_V100/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/Ducne/Documents/GitHub/IOT_Hoshiden/IOT_Hoshiden_V100/build/bootloader-prefix/src"
  "C:/Users/Ducne/Documents/GitHub/IOT_Hoshiden/IOT_Hoshiden_V100/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Ducne/Documents/GitHub/IOT_Hoshiden/IOT_Hoshiden_V100/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
