#!/bin/bash

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

    mkdir -p "./build/assets/shaders"

    if [ -e "$output_file" ]; then
        src_mtime=$(stat -c %Y "$input_file")
        dst_mtime=$(stat -c %Y "$output_file")

        if [ "$src_mtime" -le "$dst_mtime" ]; then
            verbose "$output_file not modified, skipping."
            return
        fi
    fi

    tr -d '\r' < "$input_file" > "$output_file"
    echo "Successfully compiled $input_file to $output_file"
}

main() {
    input_path='./assets/shaders'
    output_path='./build/assets/shaders'

    fs_files=($(find "$input_path" -type f -name '*.fs'))

    for fs_file in "${fs_files[@]}"; do
	filename="${fs_file%.*}"

	output_filename=$(basename "$fs_file")
        output_file="$output_path/$output_filename"

        compile_shader "$fs_file" "$output_file"
    done

    vs_files=($(find "$input_path" -type f -name '*.vs'))

    for vs_file in "${vs_files[@]}"; do
	filename="${vs_file%.*}"

	output_filename=$(basename "$vs_file")
        output_file="$output_path/$output_filename"

        compile_shader "$vs_file" "$output_file"
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
