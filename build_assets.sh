#!/bin/bash

# Define the source and destination directories
source_dir="./assets"
destination_dir="./build/assets"

# Create a blacklist of folders to be disregarded
blacklist_folders=()
blacklist_formats=("blend" "modfile")

# Create the destination directory if it doesn't exist
mkdir -p "$destination_dir"

SILENT="false"

verbose() {
    if [ "$SILENT" == "true" ]; then
        return
    fi
    echo "$@"
}

# Function to copy files from source to destination based on timestamps
copy_files() {
    local src="$1"
    local dst="$2"
    while read -r src_file _; do
	relative_path="${src_file#$src/}"
	dst_file="$dst/$relative_path"
	local extension="${src_file##*.}"
	local skip_file=0

        for pattern in "${blacklist_formats[@]}"; do
            if [[ $extension =~ $pattern ]]; then
                skip_file=1
                break
            fi
        done

	if [ $skip_file -eq 0 ]; then
	    # Check if the file already exists in the destination
	    verbose "$dst_file"

	    if [ -e "$dst_file" ]; then
		src_mtime=$(stat -c %Y "$src_file")
		dst_mtime=$(stat -c %Y "$dst_file")

		# Only copy if the source file is newer than the destination file
		if [ "$src_mtime" -gt "$dst_mtime" ]; then
		    echo "Copying $src_file to $dst_file"
		    cp -p "$src_file" "$dst_file"
		    ((MODIFIED++))
		fi
	    else
		# Create the destination directory for the file if it doesn't exist
		mkdir -p "$(dirname "$dst_file")"

		echo "Copying $src_file to $dst_file"
		cp -p "$src_file" "$dst_file"
		((MODIFIED++))
	    fi
	fi
    done < <(find "$src" -type f)
}

# Copy files from the source to the destination, checking timestamps
# Check if the user passed the --daemon or -d argument
if [ "$1" == "--daemon" ] || [ "$1" == "-d" ]; then
    # Run the function in a loop
    SILENT="true"
    while true; do
	MODIFIED=0
	copy_files "$source_dir" "$destination_dir"
	if [[ $MODIFIED > 0 ]]; then
	    echo "Touch modfile"
	    echo "modfile" > "./build/assets/modfile"
	fi

	verbose "File copy completed."
	# Run the compile_shaders.sh Bash script
	# if [ -e "./build_shaders.sh" ]; then
	#     bash "./build_shaders.sh" --silent
	# else
	#     echo "Error: compile_shaders.sh script not found."
	#     break
	# fi
	sleep 1
    done
else
    copy_files "$source_dir" "$destination_dir"
    # if [ -e "./build_shaders.sh" ]; then
    # 	echo "Running compile_shaders.sh script..."
    # 	bash "./build_shaders.sh"
    # else
    # 	echo "Error: compile_shaders.sh script not found."
    # fi
    echo "Touch modfile"
    echo "modfile" > "./build/assets/modfile"
fi
