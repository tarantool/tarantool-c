find_package(Sphinx)

set(SPHINX_BUILD_HTML_DIR "${PROJECT_BINARY_DIR}/doc/_build/html")
set(SPHINX_HTML_DIR       "${PROJECT_BINARY_DIR}/output/")

if    (NOT Sphinx-NOTFOUND)
    add_custom_target(sphinx-html
        COMMAND "${SPHINX_EXECUTABLE}"
            -b html
            -d "${SPHINX_BUILD_HTML_DIR}"
            "${PROJECT_SOURCE_DIR}/doc/source"
            "${SPHINX_HTML_DIR}"
        COMMENT "Building HTML documentation with Sphinx"
    )
endif ()
