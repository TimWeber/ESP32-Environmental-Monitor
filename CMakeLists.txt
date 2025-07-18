cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Copy template config to data directory if it doesn't exist
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/data/settings.json")
    # Create a script to strip comments from JSONC to JSON
    set(STRIP_COMMENTS_SCRIPT "${CMAKE_SOURCE_DIR}/config/strip_comments.py")
    file(WRITE ${STRIP_COMMENTS_SCRIPT}
"#!/usr/bin/env python3
import re
import sys

def strip_jsonc_comments(content):
    # Remove single-line comments (// ...)
    content = re.sub(r'//.*$', '', content, flags=re.MULTILINE)
    
    # Remove multi-line comments (/* ... */)
    content = re.sub(r'/\\*.*?\\*/', '', content, flags=re.DOTALL)
    
    # Remove empty lines and clean up whitespace
    lines = content.split('\\n')
    cleaned_lines = []
    for line in lines:
        stripped = line.strip()
        if stripped:
            cleaned_lines.append(line)
    
    return '\\n'.join(cleaned_lines)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: python3 strip_comments.py <input.jsonc> <output.json>')
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        cleaned_content = strip_jsonc_comments(content)
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(cleaned_content)
        
        print(f'Successfully converted {input_file} to {output_file}')
    except Exception as e:
        print(f'Error: {e}')
        sys.exit(1)
")
    
    # Make the script executable
    execute_process(COMMAND chmod +x ${STRIP_COMMENTS_SCRIPT})
    
    # Convert JSONC to JSON by stripping comments
    execute_process(
        COMMAND python3 ${STRIP_COMMENTS_SCRIPT} 
        "${CMAKE_SOURCE_DIR}/config/settings.template.jsonc" 
        "${CMAKE_SOURCE_DIR}/data/settings.json"
        RESULT_VARIABLE STRIP_RESULT
    )
    
    if(STRIP_RESULT EQUAL 0)
        message(STATUS "Converted JSONC template to data/settings.json - please edit with your settings")
    else()
        message(WARNING "Failed to convert JSONC template, copying as-is")
        file(COPY "${CMAKE_SOURCE_DIR}/config/settings.template.jsonc" 
             DESTINATION "${CMAKE_SOURCE_DIR}/data/"
             FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
        file(RENAME "${CMAKE_SOURCE_DIR}/data/settings.template.jsonc" 
                    "${CMAKE_SOURCE_DIR}/data/settings.json")
    endif()
endif()

project(SQLiteTest)
