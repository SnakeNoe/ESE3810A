# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/noe_9/esp/v5.2/esp-idf/components/bootloader/subproject"
  "C:/Espressif/workspace/example_espressif_coap_4.3.4_1_coap_server/build/bootloader"
  "C:/Espressif/workspace/example_espressif_coap_4.3.4_1_coap_server/build/bootloader-prefix"
  "C:/Espressif/workspace/example_espressif_coap_4.3.4_1_coap_server/build/bootloader-prefix/tmp"
  "C:/Espressif/workspace/example_espressif_coap_4.3.4_1_coap_server/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Espressif/workspace/example_espressif_coap_4.3.4_1_coap_server/build/bootloader-prefix/src"
  "C:/Espressif/workspace/example_espressif_coap_4.3.4_1_coap_server/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Espressif/workspace/example_espressif_coap_4.3.4_1_coap_server/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Espressif/workspace/example_espressif_coap_4.3.4_1_coap_server/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
