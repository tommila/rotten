#!/bin/bash

BGFX_DIR=$PWD/third_party/bgfx
SHADERC_BIN=$BGFX_DIR/.build/linux64_gcc/bin/shadercDebug
SILENT="false"

verbose() {
    if [ "$SILENT" == "true" ]; then
        shift
        # Silencing the echo command
        return
    fi
    echo "$@"
}

compile_shader() {
    input_file="$1"
    output_file="$2"
    var_file="$3"
    shader_type="$4"

    mkdir -p "./build/assets/shaders"

    if [ -e "${SHADERC_BIN}" ]; then
        if [ -e "$output_file" ]; then
            src_mtime=$(stat -c %Y "$input_file")
            dst_mtime=$(stat -c %Y "$output_file")

            if [ "$src_mtime" -le "$dst_mtime" ]; then
                verbose "$output_file not modified, skipping."
                return
            fi
        fi

        command=(
            "${SHADERC_BIN}"
            '-f' "$input_file"
            '-o' "$output_file"
            '-i' "${BGFX_DIR}/src"
            '--type' "$shader_type"
            '--varyingdef' "$var_file"
            '--platform' 'linux'
	    '--profile' '320_es'
        )

        if "${command[@]}"; then
            echo "Successfully compiled $input_file to $output_file"
            src_mtime=$(stat -c %Y "$input_file")
            touch -d "@$src_mtime" "$output_file"
        else
            echo "Error compiling $input_file"
        fi
    else
        echo "shaderc not compiled"
	break
    fi
}

main() {
    input_path='./assets/shaders'
    output_path='./build/assets/shaders'

    shader_files=($(find "$input_path" -type f -name '*.vs.sc' -o -name '*.fs.sc'))

    for shader_file in "${shader_files[@]}"; do
	extension="${shader_file##*.}"
	filename="${shader_file%.*}"

        varying_def_file="$(basename "$shader_file" | sed 's/\.fs\.sc\|\.vs\.sc//').def.sc"
        shader_type="vertex"

        if [[ $shader_file == *.fs.sc ]]; then
            shader_type="fragment"
        fi

	output_filename=$(basename "$shader_file")
        output_file="$output_path/$output_filename.bin"
        compile_shader "$shader_file" "$output_file" "$input_path/$varying_def_file" "$shader_type"
    done
}

if [ "$1" == "--silent" ] || [ "$1" == "-s" ]; then
    SILENT="true"
fi

if [ "$0" = "$BASH_SOURCE" ]; then
    if [ "$1" == "--daemon" ] || [ "$1" == "-d" ]; then
	SILENT="true"
	while true; do
	    main
	    sleep 1
	done
    else
	main
    fi
fi
