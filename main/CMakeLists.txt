# The MIT License (MIT)
#
# Copyright (c) 2020 Scott Aron Bloom
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

project( CompareCSV ) 

include( include.cmake )
include( ${CMAKE_SOURCE_DIR}/SABUtils/Project.cmake )

add_executable( CompareCSV WIN32
                 ${project_SRCS} 
                 ${project_H} 
                 ${qtproject_SRCS} 
                 ${qtproject_QRC} 
                 ${qtproject_QRC_SRCS} 
                 ${qtproject_UIS_H} 
                 ${qtproject_MOC_SRCS} 
                 ${qtproject_H} 
                 ${qtproject_UIS}
                 ${qtproject_QRC_SOURCES}
                 ${_CMAKE_FILES}
                 ${_CMAKE_MODULE_FILES}
                application.rc 
          )
get_filename_component( QTDIR ${DEPLOYQT_EXECUTABLE} DIRECTORY )

set ( DEBUG_PATH 
        "%PATH%"
        "$<TARGET_FILE_DIR:SABUtils>"
        "${QTDIR}"
        )
set_target_properties( CompareCSV PROPERTIES FOLDER Apps 
                                    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:CompareCSV>" 
                                    VS_DEBUGGER_COMMAND "$<TARGET_FILE:CompareCSV>" 
                                    VS_DEBUGGER_ENVIRONMENT "PATH=${DEBUG_PATH}" 
                     )
                     
                     
set_target_properties( CompareCSV PROPERTIES FOLDER Apps )

target_link_libraries( CompareCSV 
                 Qt5::Widgets
                 Qt5::Core
                 SABUtils
                 MainWindow
          )
DeployQt( CompareCSV . INSTALL_ONLY 1 )
DeploySystem( CompareCSV . INSTALL_ONLY 1 )

INSTALL( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . )
INSTALL( FILES ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${PROJECT_NAME}.pdb DESTINATION . CONFIGURATIONS Debug RelWithDebInfo )

