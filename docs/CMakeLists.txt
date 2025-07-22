#=========================================
# Documentation
#=========================================

#=========================================
# Python
#=========================================

find_package (Python3 COMPONENTS Interpreter)

execute_process (COMMAND "${Python3_EXECUTABLE}" -m venv "${CMAKE_SOURCE_DIR}/.venv")


## update the environment with VIRTUAL_ENV variable (mimic the activate script)
set (ENV{VIRTUAL_ENV} "${CMAKE_SOURCE_DIR}/.venv")
## change the context of the search
set (Python3_FIND_VIRTUALENV FIRST)
## unset Python3_EXECUTABLE because it is also an input variable (see documentation, Artifacts Specification section)
unset (Python3_EXECUTABLE)
## Launch a new search
find_package (Python3 COMPONENTS Interpreter Development)

execute_process (COMMAND "${Python3_EXECUTABLE}" -m pip install -r "${CMAKE_SOURCE_DIR}/docs/python_requirements.txt")


# Set variable for hint for finding correct sphinx-build in .venv
# Will be used in cmake/FindSphinx.cmake
if(WIN32)
set(PYTHON_ENVIRONMENT ${CMAKE_SOURCE_DIR}/.venv/Scripts/)
message("Python Environment: ${PYTHON_ENVIRONMENT}")
endif()

if(UNIX)
set(PYTHON_ENVIRONMENT ${CMAKE_SOURCE_DIR}/.venv/bin/)
message("Python Environment: ${PYTHON_ENVIRONMENT}")
endif()


#=========================================
# Doxygen
#=========================================

find_package(Doxygen REQUIRED)


if(Doxygen_FOUND)


    # Doxyfile Configurations
    
    # Location of source code for doxygen to parse
    set(DOXYGEN_INPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/src)

    # Location to place the output of doxygen
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
    
    # Project Name that appears in doxygen documentation header
    set(DOXYGEN_PROJECT_NAME "Truchas" )
    
    
    set(DOXYGEN_GENERATE_XML "YES")
    
    set(DOXYGEN_GENERATE_LATEX "YES")
    
    # These 2 need to be set to the following to display LaTex correctly
    set(DOXYGEN_LATEX_CMD_NAME "latex")
    set(DOXYGEN_USE_MATHJAX "YES")
    
    
    # Other custom settings for Doxygen
    set(DOXYGEN_FULL_PATH_NAMES "NO")
    set(DOXYGEN_EXTRACT_ALL "YES")
    set(DOXYGEN_EXTRACT_PRIVATE "YES")
    set(DOXYGEN_EXTRACT_PRIV_VIRTUAL "YES")
    set(DOXYGEN_EXTRACT_STATIC "YES")
    set(DOXYGEN_HTML_COLORSTYLE "TOGGLE")
    set(DOXYGEN_HTML_DYNAMIC_MENUS "NO")

    # For future implementation of Sphinx and Breathe
    set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIRECTORY}/html/index.html)


    
    # Create the output directory, doxygen/, if not already there
    file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIRECTORY}) 

    # Create doxygen target for generating doxygen docs
    # Target name is doxygen.  This can be changed if desired.
    doxygen_add_docs(
                        doxygen 
                        ${DOXYGEN_INPUT_DIRECTORY}
                    )



    message("DOXYGEN_INPUT_DIRECTORY: ${DOXYGEN_INPUT_DIRECTORY}")
    message("DOXYGEN_OUTPUT_DIRECTORY: ${DOXYGEN_OUTPUT_DIRECTORY}")
    message("DOXYGEN_INDEX_FILE: ${DOXYGEN_INDEX_FILE}")


endif()


# #=========================================
# # Sphinx / Breathe
# #=========================================

find_package(Sphinx REQUIRED)


set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

add_custom_target(Sphinx ALL
                  COMMAND
                  ${SPHINX_EXECUTABLE} -b html
                  ${SPHINX_SOURCE} ${SPHINX_BUILD}
                  -Dbreathe_projects.truchas=${DOXYGEN_OUTPUT_DIRECTORY}/xml
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  DEPENDS
                   # Other docs files you want to track should go here (or in some variable)
                   ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
                   ${DOXYGEN_INDEX_FILE}
                   MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
                  COMMENT "Generating documentation with Sphinx")